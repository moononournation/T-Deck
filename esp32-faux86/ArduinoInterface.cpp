/*
	Faux86: A portable, open-source 8086 PC emulator.
	Copyright (C)2018 James Howard
	Based on Fake86
	Copyright (C)2010-2013 Mike Chambers

	Contributions and Updates (c)2023 Curtis aka ArnoldUK

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "ArduinoInterface.h"
#include <VM.h>

#include "esp32-hal.h"

using namespace Faux86;

bool sdlconsole_ctrl = 0;
bool sdlconsole_alt = 0;

ArduinoHostSystemInterface::ArduinoHostSystemInterface(Arduino_TFT *gfx)
		: _gfx(gfx)
{
	log_d("ArduinoHostSystemInterface::ArduinoHostSystemInterface()");
}

void ArduinoHostSystemInterface::init(VM *inVM)
{
	log_d("ArduinoHostSystemInterface::init(VM *inVM)");
	frameBufferInterface.setGfx(_gfx);
}

void ArduinoHostSystemInterface::resize(uint32_t desiredWidth, uint32_t desiredHeight)
{
	// log_d("ArduinoHostSystemInterface::resize(%d, %d)", desiredWidth, desiredHeight);
}

void ArduinoAudioInterface::init(VM &vm)
{
	log_d("ArduinoAudioInterface::init(VM &vm)");
}

void ArduinoAudioInterface::shutdown()
{
	log_d("ArduinoAudioInterface::shutdown()");
}

void ArduinoAudioInterface::fillAudioBuffer(void *udata, uint8_t *stream, int len)
{
	log_d("ArduinoAudioInterface::fillAudioBuffer(void *udata, uint8_t *stream, %d)", len);
}

ArduinoHostSystemInterface::~ArduinoHostSystemInterface()
{
	log_d("ArduinoHostSystemInterface::~ArduinoHostSystemInterface()");
}

DiskInterface *ArduinoHostSystemInterface::openFile(const char *filename)
{
	log_d("ArduinoHostSystemInterface::openFile(%s)", filename);
	return new StdioDiskInterface(filename);
}

void ArduinoFrameBufferInterface::setGfx(Arduino_TFT *gfx)
{
	log_d("ArduinoFrameBufferInterface::setGfx(Arduino_TFT *gfx)");
	_gfx = gfx;
}

RenderSurface *ArduinoFrameBufferInterface::getSurface()
{
	log_d("ArduinoFrameBufferInterface::getSurface()");
	return &renderSurface;
}

void ArduinoFrameBufferInterface::setPalette(Palette *palette)
{
	log_d("ArduinoFrameBufferInterface::setPalette(Palette *palette)");
}

void ArduinoFrameBufferInterface::blit(uint16_t *pixels, int w, int h, int stride)
{
	// log_d("ArduinoFrameBufferInterface::blit(uint32_t *pixels, %d, %d, %d)", w, h, stride);

#ifdef DEBUG_TIMING
	static uint8_t blit_fps = 0;
	static unsigned long next_10secound = 0;
	++blit_fps;
	if (millis() > next_10secound)
	{
		log_d("blit_fps: %.1f", blit_fps / 10.0);
		blit_fps = 0;
		next_10secound = ((millis() / 1000) + 10) * 1000;
	}
	return;
#endif

	int16_t y = 0;
	uint16_t *row1 = pixels;
	uint16_t *row2 = pixels + VGA_FRAMEBUFFER_WIDTH;
	int16_t hQuad = h / 2;
	int16_t wQuad = w / 2;
	uint32_t xSkip = ((VGA_FRAMEBUFFER_WIDTH - w) + VGA_FRAMEBUFFER_WIDTH);
	if (!_rowBuf)
	{
		_rowBuf = (uint16_t *)malloc(VGA_FRAMEBUFFER_WIDTH);
	}

	_gfx->startWrite();
	_gfx->writeAddrWindow(0, 0, wQuad, hQuad);
	uint16_t p;
	while (hQuad--)
	{
		for (int16_t i = 0; i < wQuad; ++i)
		{
			p = (*row1++ & 0b1110011110011100) >> 2;
			p += (*row1++ & 0b1110011110011100) >> 2;
			p += (*row2++ & 0b1110011110011100) >> 2;
			p += (*row2++ & 0b1110011110011100) >> 2;
			_rowBuf[i] = p;
		}
		_gfx->writePixels(_rowBuf, wQuad);
		row1 += xSkip;
		row2 += xSkip;
	}
	_gfx->endWrite();
}

uint64_t ArduinoTimerInterface::getHostFreq()
{
	// log_v("ArduinoTimerInterface::getHostFreq()");
	return CONFIG_FREERTOS_HZ;
}

uint64_t ArduinoTimerInterface::getTicks()
{
	// log_d("ArduinoTimerInterface::getTicks(): %d", xTaskGetTickCount());
	return xTaskGetTickCount();
}

void ArduinoHostSystemInterface::tick()
{
	// log_d("ArduinoHostSystemInterface::tick");
}

void Faux86::log(Faux86::LogChannel channel, const char *message, ...)
{
	if (ARDUHAL_LOG_LEVEL > channel)
	{
		va_list args;
		va_start(args, message);
		log_printf(message, args);
		log_printf("\n");
		va_end(args);
	}
}
