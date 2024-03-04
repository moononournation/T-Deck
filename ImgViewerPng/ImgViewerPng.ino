/*******************************************************************************
 * PNG Image Viewer
 * This is a T-Deck version PNG Image Viewer sample
 *
 * Image Source: https://github.com/logos
 *
 * Required libraries:
 * Arduino_GFX: https://github.com/moononournation/Arduino_GFX.git
 * PNGdec: https://github.com/bitbank2/PNGdec.git
 *
 * Setup steps:
 * 1. Upload PNG file
 *   FatFS/LittleFS/SPIFFS:
 *     upload data with ESP32 Sketch Data Upload:
 *     ESP32: https://github.com/lorol/arduino-esp32fs-plugin
 *   SD:
 *     Most Arduino system built-in support SD file system.
 *
 * Arduino IDE Settings for T-Deck
 * Board:            "ESP32S3 Dev Module"
 * USB CDC On Boot:  "Enable"
 * Flash Mode:       "QIO 120MHz"
 * Flash Size:       "16MB(128Mb)"
 * Partition Scheme: "16M Flash(2M APP/12.5MB FATFS)"
 * PSRAM:            "OPI PSRAM"
 ******************************************************************************/
#define PNG_FILENAME "/octocat.png"
#define PNG_4BPP_FILENAME "/octocat-4bpp.png"

#include "TDECK_PINS.h"

/*******************************************************************************
 * Start of Arduino_GFX setting
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
Arduino_GFX *gfx = new Arduino_ST7789(bus, GFX_NOT_DEFINED /* RST */, 1 /* rotation */, false /* IPS */);
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

#include <FFat.h>
#include <LittleFS.h>
#include <SPIFFS.h>
#include <SD.h>

#include <PNGdec.h>
PNG png;

int16_t w, h, xOffset, yOffset;

// Functions to access a file on the SD card
File pngFile;

void *myOpen(const char *filename, int32_t *size)
{
  // pngFile = FFat.open(filename, "r");
  // pngFile = LittleFS.open(filename, "r");
  // pngFile = SPIFFS.open(filename, "r");
  pngFile = SD.open(filename, "r");

  if (!pngFile || pngFile.isDirectory())
  {
    Serial.println(F("ERROR: Failed to open " PNG_FILENAME " file for reading"));
    gfx->println(F("ERROR: Failed to open " PNG_FILENAME " file for reading"));
  }
  else
  {
    *size = pngFile.size();
    Serial.printf("Opened '%s', size: %d\n", filename, *size);
  }

  return &pngFile;
}

void myClose(void *handle)
{
  if (pngFile)
    pngFile.close();
}

int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length)
{
  if (!pngFile)
    return 0;
  return pngFile.read(buffer, length);
}

int32_t mySeek(PNGFILE *handle, int32_t position)
{
  if (!pngFile)
    return 0;
  return pngFile.seek(position);
}

// Function to draw pixels to the display
void PNGDraw(PNGDRAW *pDraw)
{
  uint16_t usPixels[320];
  uint8_t usMask[320];

  // Serial.printf("Draw pos = 0,%d. size = %d x 1\n", pDraw->y, pDraw->iWidth);
  png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0x00000000);
  png.getAlphaMask(pDraw, usMask, 1);
  gfx->draw16bitRGBBitmapWithMask(xOffset, yOffset + pDraw->y, usPixels, usMask, pDraw->iWidth, 1);
}

void setup()
{
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("Arduino_GFX PNG Image Viewer example");

  SPI.begin(TDECK_SPI_SCK, TDECK_SPI_MISO, TDECK_SPI_MOSI);

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

  w = gfx->width(), h = gfx->height();
  gfx->fillScreen(BLACK);
  for (int16_t x = 0; x < w; x += 5)
  {
    gfx->drawFastVLine(x, 0, h, PALERED);
  }

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  // if (!FFat.begin())
  // if (!LittleFS.begin())
  // if (!SPIFFS.begin())
  digitalWrite(TDECK_SDCARD_CS, HIGH);
  digitalWrite(TDECK_RADIO_CS, HIGH);
  digitalWrite(TDECK_TFT_CS, HIGH);
  if (!SD.begin(TDECK_SDCARD_CS, SPI, 800000U))
  {
    Serial.println(F("ERROR: File System Mount Failed!"));
    gfx->println(F("ERROR: File System Mount Failed!"));
  }
  else
  {
    unsigned long start = millis();
    int rc;
    rc = png.open(PNG_FILENAME, myOpen, myClose, myRead, mySeek, PNGDraw);
    if (rc == PNG_SUCCESS)
    {
      int16_t pw = png.getWidth();
      int16_t ph = png.getHeight();

      xOffset = (w - pw) / 2;
      yOffset = (h - ph) / 2;

      rc = png.decode(NULL, 0);

      Serial.printf("Draw offset: (%d, %d), time used: %lu\n", xOffset, yOffset, millis() - start);
      Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
      png.close();
    }
    else
    {
      Serial.println("png.open() failed!");
    }
  }

  delay(5000); // 5 seconds
}

void loop()
{
  unsigned long start = millis();
  int rc;
  rc = png.open(PNG_4BPP_FILENAME, myOpen, myClose, myRead, mySeek, PNGDraw);
  if (rc == PNG_SUCCESS)
  {
    // random draw position
    int16_t pw = png.getWidth();
    int16_t ph = png.getHeight();
    xOffset = random(w) - (pw / 2);
    yOffset = random(h) - (ph / 2);

    rc = png.decode(NULL, 0);

    Serial.printf("Draw offset: (%d, %d), time used: %lu\n", xOffset, yOffset, millis() - start);
    Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    png.close();
  }
  else
  {
    Serial.println("png.open() failed!");
  }

  delay(1000); // 1 second
}
