const char *root = "/root";
const char *mpeg_file = "/root/224x128.mpg";

#include <WiFi.h>

#include <FFat.h>
#include <LittleFS.h>
#include <SD.h>
#include <SD_MMC.h>

#include "TDECK_PINS.h"

/*******************************************************************************
   Start of Arduino_GFX setting
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#define GFX_DEV_DEVICE LILYGO_T_DECK
#define GFX_EXTRA_PRE_INIT()                \
  {                                         \
    pinMode(TDECK_SDCARD_CS, OUTPUT);       \
    digitalWrite(TDECK_SDCARD_CS, HIGH);    \
    pinMode(TDECK_RADIO_CS, OUTPUT);        \
    digitalWrite(TDECK_RADIO_CS, HIGH);     \
    pinMode(TDECK_PERI_POWERON, OUTPUT);    \
    digitalWrite(TDECK_PERI_POWERON, HIGH); \
    delay(500);                             \
  }
#define GFX_BL TDECK_TFT_BACKLIGHT
Arduino_DataBus *bus = new Arduino_HWSPI(TDECK_TFT_DC, TDECK_TFT_CS, TDECK_SPI_SCK, TDECK_SPI_MOSI, TDECK_SPI_MISO);
Arduino_GFX *gfx = new Arduino_ST7789(bus, GFX_NOT_DEFINED /* RST */, 1 /* rotation */, true /* IPS */);
/*******************************************************************************
   End of Arduino_GFX setting
 ******************************************************************************/

// microSD card
#define SD_SCK TDECK_SPI_SCK
#define SD_MISO TDECK_SPI_MISO
#define SD_MOSI TDECK_SPI_MOSI
#define SD_CS TDECK_SDCARD_CS

#include "esp32_audio.h"
// I2S
#define I2S_DOUT TDECK_I2S_DOUT
#define I2S_BCLK TDECK_I2S_BCK
#define I2S_LRCK TDECK_I2S_WS

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"
plm_t *plm;
plm_frame_t *frame = NULL;
int plm_w;
int plm_h;
uint16_t *plm_buffer;
int decode_video_count = 0;
int decode_audio_count = 0;

void YCbCr2RGB565Be(uint8_t *yData, uint8_t *cbData, uint8_t *crData, uint16_t w, uint16_t h, uint16_t *dest)
{
  int cols = w >> 1;
  int rows = h >> 1;
  uint8_t *yData2 = yData + w;
  uint16_t *dest2 = dest + w;
  for (int row = 0; row < rows; ++row)
  {
    for (int col = 0; col < cols; ++col)
    {
      uint8_t cr = *crData++;
      uint8_t cb = *cbData++;
      int16_t r = CR2R16[cr];
      int16_t g = - CB2G16[cb] - CR2G16[cr];
      int16_t b = CB2B16[cb];
      int16_t y;

      y = Y2I16[*yData++];
      *dest++ = CLIPRBE[y + r] | CLIPGBE[y + g] | CLIPBBE[y + b];
      y = Y2I16[*yData++];
      *dest++ = CLIPRBE[y + r] | CLIPGBE[y + g] | CLIPBBE[y + b];
      y = Y2I16[*yData2++];
      *dest2++ = CLIPRBE[y + r] | CLIPGBE[y + g] | CLIPBBE[y + b];
      y = Y2I16[*yData2++];
      *dest2++ = CLIPRBE[y + r] | CLIPGBE[y + g] | CLIPBBE[y + b];
    }
    yData += w;
    yData2 += w;
    dest += w;
    dest2 += w;
  }
}

// This function gets called for each decoded video frame
void my_video_callback(plm_t *plm, plm_frame_t *frame, void *user)
{
  // Do something with frame->y.data, frame->cr.data, frame->cb.data
  YCbCr2RGB565Be(frame->y.data, frame->cb.data, frame->cr.data, frame->width, frame->height, plm_buffer);
  // explicit disable SD before use display
  digitalWrite(TDECK_SDCARD_CS, HIGH);
  gfx->draw16bitBeRGBBitmap(0, 0, plm_buffer, plm_w, plm_h);
  ++decode_video_count;
}

// This function gets called for each decoded audio frame
void my_audio_callback(plm_t *plm, plm_samples_t *frame, void *user)
{
  // Do something with samples->interleaved
  i2s_play_float(frame->interleaved, frame->count);
  ++decode_audio_count;
}

void setup(void)
{
  WiFi.mode(WIFI_OFF);

  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("MPEG Player");

  // If display and SD shared same interface, init SPI first
  SPI.begin(TDECK_SPI_SCK, TDECK_SPI_MISO, TDECK_SPI_MOSI);

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  if (!gfx->begin(80000000))
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  // if (!FFat.begin(false, root))
  // if (!LittleFS.begin(false, root))
  if (!SD.begin(SD_CS, SPI, 80000000, "/root"))
  // pinMode(SD_CS, OUTPUT);
  // digitalWrite(SD_CS, HIGH);
  // SD_MMC.setPins(SD_SCK, SD_MOSI, SD_MISO);
  // if (!SD_MMC.begin(root, true /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_DEFAULT))
  {
    Serial.println("ERROR: File system mount failed!");
  }
  else
  {
    i2s_init(
        I2S_NUM_0,
        44100 /* sample_rate */,
        -1 /* mck_io_num */, /*!< MCK in out pin. Note that ESP32 supports setting MCK on GPIO0/GPIO1/GPIO3 only*/
        I2S_BCLK,            /*!< BCK in out pin*/
        I2S_LRCK,            /*!< WS in out pin*/
        I2S_DOUT,            /*!< DATA out pin*/
        -1 /* data_in_num */ /*!< DATA in pin*/
    );
    i2s_zero_dma_buffer(I2S_NUM_0);

    plm = plm_create_with_filename(mpeg_file);
    if (!plm)
    {
      printf("Couldn't open file %s\n", mpeg_file);
    }

    // Probe the MPEG-PS data to find the actual number of video and audio streams
    plm_probe(plm, PLM_BUFFER_DEFAULT_SIZE);

    // Install the video & audio decode callbacks
    plm_set_video_decode_callback(plm, my_video_callback, NULL);
    plm_set_audio_decode_callback(plm, my_audio_callback, NULL);

    // plm_video_set_no_delay(plm->video_decoder, true);
    // plm_set_video_enabled(plm, false);
    // plm_set_audio_enabled(plm, false);

    plm_w = plm_get_width(plm);
    plm_h = plm_get_height(plm);
    plm_buffer = (uint16_t *)malloc(plm_w * plm_h * 2);
  }
}

void loop()
{
  unsigned long start_ms = millis();
  unsigned long next_frame_ms = start_ms;
  unsigned long cur_ms;
  unsigned long remain_ms = 0;
  unsigned long total_remain_ms = 0;
  do
  {
    cur_ms = millis();
    if (next_frame_ms > cur_ms)
    {
      remain_ms = next_frame_ms - cur_ms;
      delay(remain_ms >> 1);
      total_remain_ms += remain_ms;
    }
    else
    {
      // Serial.printf("Excess: %lu\n", cur_ms - next_frame_ms);
    }

    plm_decode(plm, 0.04);

    next_frame_ms += 40;
  } while (!plm_has_ended(plm));

  Serial.printf("Time used: %lu, decode_video_count: %d, decode_audio_count: %d, remain: %lu\n", millis() - start_ms, decode_video_count, decode_audio_count, total_remain_ms);
  delay(LONG_MAX);
}
