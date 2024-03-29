#pragma once

#include "TDECK_PINS.h"

#define FSROOT "/fs"

#define FILESYSTEM_BEGIN FFat.begin(false, FSROOT); FS filesystem = FFat;
// #define FILESYSTEM_BEGIN SD.begin(TDECK_SDCARD_CS, SPI, 80000000); FS filesystem = SD;
// #define FILESYSTEM_BEGIN SPIClass spi = SPIClass(HSPI); spi.begin(TDECK_SPI_SCK, TDECK_SPI_MISO, TDECK_SPI_MOSI, TDECK_SDCARD_CS); FS filesystem = SD;

// enable audio
#define HW_AUDIO
#define HW_AUDIO_EXTDAC
#define HW_AUDIO_EXTDAC_WCLK TDECK_I2S_WS
#define HW_AUDIO_EXTDAC_BCLK TDECK_I2S_BCK
#define HW_AUDIO_EXTDAC_DOUT TDECK_I2S_DOUT
#define HW_AUDIO_SAMPLERATE 22050

/* controller is GPIO */
#define HW_CONTROLLER_I2C_TDECK_KEYBOARD
#define HW_CONTROLLER_I2C_SDA TDECK_I2C_SDA
#define HW_CONTROLLER_I2C_SCL TDECK_I2C_SCL
#define HW_CONTROLLER_I2C_FREQ 800000UL
