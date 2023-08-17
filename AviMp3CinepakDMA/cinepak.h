#pragma once

/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define BIG_ENDIAN_PIXEL
#define USE_DRAW_CALLBACK

#ifdef USE_DRAW_CALLBACK
typedef void(DRAW_CALLBACK)(uint16_t x, uint16_t y, uint16_t *p, uint16_t w, uint16_t h);
#endif

/**
 * Cinepak decoder.
 *
 * Used by BMP/AVI and PICT/QuickTime.
 *
 * Used in engines:
 *  - sherlock
 */
class CinepakDecoder
{
public:
	CinepakDecoder()
	{
		_y = 0;

		// Create a lookup for the clip function
		// This dramatically improves the performance of the color conversion
		_clipTableBuf = new uint8_t[1024];

		for (uint i = 0; i < 1024; i++)
		{
			if (i <= 512)
				_clipTableBuf[i] = 0;
			else if (i >= 768)
				_clipTableBuf[i] = 255;
			else
				_clipTableBuf[i] = i - 512;
		}

		_clipTable = _clipTableBuf + 512;
	}

	~CinepakDecoder()
	{
		delete[] _clipTableBuf;
	}

#ifdef USE_DRAW_CALLBACK
	void decodeFrame(uint8_t *data, size_t data_size, uint16_t *output_buf, size_t output_buf_size, DRAW_CALLBACK *draw, bool iskeyframe)
#else
	void decodeFrame(uint8_t *data, size_t data_size, uint16_t *output_buf, size_t output_buf_size)
#endif
	{
		_data = data;
		_data_size = data_size;
		_data_pos = 0;
		_output_buf = output_buf;
		_output_buf_size = output_buf_size;
#ifdef USE_DRAW_CALLBACK
		_draw = draw;
		_iskeyframe = iskeyframe;
#endif

		_flags = readUint8();
		_length = readUint24BE();
		_width = readUint16BE();
		_height = readUint16BE();
		_stripCount = readUint16BE();

		// log_i("Cinepak Frame: Width = %d, Height = %d, Strip Count = %d", _width, _height, _stripCount);

		// Borrowed from FFMPEG. This should cut out the extra data Cinepak for Sega has (which is useless).
		// The theory behind this is that this is here to confuse standard Cinepak decoders. But, we won't let that happen! ;)
		if (_length != (uint32_t)_data_size)
		{
			if (readUint16BE() == 0xFE00)
			{
				_data_pos += 4;
			}
			else if ((_data_size % _length) == 0)
			{
				_data_pos -= 2;
			}
		}

		_y = 0;

		for (uint16_t i = 0; i < _stripCount; i++)
		{
			_data_pos += 2;						 // Ignore, substitute with our own.
			_strip_length = readUint16BE() - 12; // Subtract the 12 uint8_t header
			_strip_top = _y;
			_data_pos += 2; // Ignore, substitute with our own.
			_data_pos += 2; // Ignore, substitute with our own.
			_strip_height = readUint16BE();
			_strip_bottom = _y + _strip_height;
			_data_pos += 2; // Ignore, substitute with our own.

			uint32_t pos = _data_pos;

			while ((uint32_t)_data_pos < (pos + _strip_length) && (_data_pos < (_data_size - 1)))
			{
				uint8_t chunkID = readUint8();

				if (_data_pos >= (_data_size - 1))
					break;

				// Chunk Size is 24-bit, ignore the first 4 bytes
				uint32_t chunkSize = readUint24BE() - 4;

				int32_t startPos = _data_pos;

				switch (chunkID)
				{
				case 0x20:
				case 0x21:
				case 0x24:
				case 0x25:
					loadCodebook(_v4_codebook, chunkID, chunkSize);
					break;
				case 0x22:
				case 0x23:
				case 0x26:
				case 0x27:
					loadCodebook(_v1_codebook, chunkID, chunkSize);
					break;
				case 0x30:
				case 0x31:
				case 0x32:
					decodeVectors(chunkID, chunkSize);
					break;
				default:
					log_i("Unknown Cinepak chunk ID %02x", chunkID);
					return;
				}

				if (_data_pos != startPos + (int32_t)chunkSize)
					_data_pos = startPos + chunkSize;
			}

			_y = _strip_bottom;
		}

		return;
	}

private:
	uint8_t *_data;
	size_t _data_size;
	size_t _data_pos;
	uint16_t *_output_buf;
	size_t _output_buf_size;
#ifdef USE_DRAW_CALLBACK
	DRAW_CALLBACK *_draw;
	bool _iskeyframe;
#endif

	uint8_t _flags;
	uint32_t _length;
	uint16_t _width;
	uint16_t _height;
	uint16_t _stripCount;

	int16_t _strip_top;
	int16_t _strip_bottom;
	int16_t _strip_height;
	uint16_t _strip_length;
	uint16_t _v1_codebook[1024], _v4_codebook[1024];

	int32_t _y;
	uint8_t *_clipTable, *_clipTableBuf;

	inline uint8_t readUint8()
	{
		return _data[_data_pos++];
	}

	inline uint16_t readUint16BE()
	{
		uint16_t a = _data[_data_pos++];
		uint16_t b = _data[_data_pos++];
		return (a << 8) | b;
	}

	inline uint16_t readUint24BE()
	{
		uint16_t a = _data[_data_pos++];
		uint16_t b = _data[_data_pos++];
		uint16_t c = _data[_data_pos++];
		return (a << 16) | (b << 8) | c;
	}

	inline uint32_t readUint32BE()
	{
		uint32_t a = _data[_data_pos++];
		uint32_t b = _data[_data_pos++];
		uint32_t c = _data[_data_pos++];
		uint32_t d = _data[_data_pos++];
		return (a << 24) | (b << 16) | (c << 8) | d;
	}

	void putPixelRaw(uint16_t *dst, uint8_t y)
	{
		// *dst = (((0xF8 & y) << 8) | ((0xFC & y) << 3) | ((y) >> 3));
		*dst = (((0xF8 & y)) | ((y) >> 5) | ((0x1C & y) << 11) | ((0xF8 & y) << 5));
	}

	void putPixelRaw(uint16_t *dst, uint8_t y, int8_t u, int8_t v)
	{
		uint8_t r = _clipTable[y + (v << 1)];
		uint8_t g = _clipTable[y - (u >> 1) - v];
		uint8_t b = _clipTable[y + (u << 1)];
#ifdef BIG_ENDIAN_PIXEL
		*dst = (((0xF8 & r)) | ((g) >> 5) | ((0x1C & g) << 11) | ((0xF8 & b) << 5));
#else
		*dst = (((0xF8 & r) << 8) | ((0xFC & g) << 3) | ((b) >> 3));
#endif
	}

	void loadCodebook(uint16_t *codeblock, uint8_t chunkID, uint32_t chunkSize)
	{
		// log_i("loadCodebook(%d, %d, %d)", chunkID, chunkSize);

		int32_t startPos = _data_pos;
		uint32_t flag = 0, mask = 0;

		for (uint16_t i = 0; i < 256; i++)
		{
			if ((chunkID & 0x01) && !(mask >>= 1))
			{
				if ((_data_pos - startPos + 4) > (int32_t)chunkSize)
					break;

				flag = readUint32BE();
				mask = 0x80000000;
			}

			if (!(chunkID & 0x01) || (flag & mask))
			{
				uint8_t n = (chunkID & 0x04) ? 4 : 6;
				if ((_data_pos - startPos + n) > (int32_t)chunkSize)
					break;

				uint8_t *y = _data + _data_pos;
				_data_pos += 4;
				uint16_t *p = codeblock + (i << 2);
				if (n == 6)
				{
					int8_t u = readUint8();
					int8_t v = readUint8();
					putPixelRaw(p++, y[0], u, v);
					putPixelRaw(p++, y[1], u, v);
					putPixelRaw(p++, y[2], u, v);
					putPixelRaw(p, y[3], u, v);
				}
				else
				{
					// This codebook type indicates either greyscale or
					// palettized video. For greyscale, default us to
					// 0 for both u and v.
					putPixelRaw(p++, y[0]);
					putPixelRaw(p++, y[1]);
					putPixelRaw(p++, y[2]);
					putPixelRaw(p, y[3]);
				}
			}
		}
	}

	void decodeVectors(uint8_t chunkID, uint32_t chunkSize)
	{
		uint32_t flag = 0, mask = 0;
		uint16_t *row0;
		uint16_t *row1;
		uint16_t *row2;
		uint16_t *row3;
		int32_t startPos = _data_pos;
		uint16_t *codeblock;

		for (uint16_t y = _strip_top; y < _strip_bottom; y += 4)
		{
#ifdef USE_DRAW_CALLBACK
			row0 = _output_buf;
			row1 = row0 + (_iskeyframe ? _width : 4);
			row2 = row1 + (_iskeyframe ? _width : 4);
			row3 = row2 + (_iskeyframe ? _width : 4);
#else
			row0 = _output_buf + (y * _width);
			row1 = row0 + _width;
			row2 = row1 + _width;
			row3 = row2 + _width;
#endif

			for (uint16_t x = 0; x < _width; x += 4)
			{
				if ((chunkID & 0x01) && !(mask >>= 1))
				{
					if ((_data_pos - startPos + 4) > (int32_t)chunkSize)
						return;

					flag = readUint32BE();
					mask = 0x80000000;
				}

				if (!(chunkID & 0x01) || (flag & mask))
				{
					if (!(chunkID & 0x02) && !(mask >>= 1))
					{
						if ((_data_pos - startPos + 4) > (int32_t)chunkSize)
							return;

						flag = readUint32BE();
						mask = 0x80000000;
					}

					if ((chunkID & 0x02) || (~flag & mask))
					{
						if ((_data_pos - startPos + 1) > (int32_t)chunkSize)
							return;

						// Get the codeblock
						codeblock = _v1_codebook + (readUint8() << 2);
						uint16_t codebit = *codeblock++;
						row0[0] = codebit;
						row0[1] = codebit;
						row1[0] = codebit;
						row1[1] = codebit;

						codebit = *codeblock++;
						row0[2] = codebit;
						row0[3] = codebit;
						row1[2] = codebit;
						row1[3] = codebit;

						codebit = *codeblock++;
						row2[0] = codebit;
						row2[1] = codebit;
						row3[0] = codebit;
						row3[1] = codebit;

						codebit = *codeblock;
						row2[2] = codebit;
						row2[3] = codebit;
						row3[2] = codebit;
						row3[3] = codebit;

#ifdef USE_DRAW_CALLBACK
						if (!_iskeyframe)
						{
							_draw(x, y, _output_buf, 4, 4);
						}
#endif
					}
					else if (flag & mask)
					{
						if ((_data_pos - startPos + 4) > (int32_t)chunkSize)
							return;

						codeblock = _v4_codebook + (readUint8() << 2);
						row0[0] = *codeblock++;
						row0[1] = *codeblock++;
						row1[0] = *codeblock++;
						row1[1] = *codeblock;

						codeblock = _v4_codebook + (readUint8() << 2);
						row0[2] = *codeblock++;
						row0[3] = *codeblock++;
						row1[2] = *codeblock++;
						row1[3] = *codeblock;

						codeblock = _v4_codebook + (readUint8() << 2);
						row2[0] = *codeblock++;
						row2[1] = *codeblock++;
						row3[0] = *codeblock++;
						row3[1] = *codeblock;

						codeblock = _v4_codebook + (readUint8() << 2);
						row2[2] = *codeblock++;
						row2[3] = *codeblock++;
						row3[2] = *codeblock++;
						row3[3] = *codeblock;

#ifdef USE_DRAW_CALLBACK
						if (!_iskeyframe)
						{
							_draw(x, y, _output_buf, 4, 4);
						}
#endif
					}
				}

#ifdef USE_DRAW_CALLBACK
				if (_iskeyframe)
#endif
				{
					row0 += 4;
					row1 += 4;
					row2 += 4;
					row3 += 4;
				}
			}

#ifdef USE_DRAW_CALLBACK
			if (_iskeyframe)
			{
				_draw(0, y, _output_buf, _width, 4);
			}
#endif
		}
	}
};
