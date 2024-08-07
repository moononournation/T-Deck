#pragma once

//! The board peripheral power control pin needs to be set to HIGH when using the peripheral
#define TDECK_PERI_POWERON 10

#define TDECK_SPI_MOSI 41
#define TDECK_SPI_MISO 38
#define TDECK_SPI_SCK 40

#define TDECK_SDCARD_CS 39

#define TDECK_TFT_CS 12
#define TDECK_TFT_DC 11
#define TDECK_TFT_BACKLIGHT 42

#define TDECK_RADIO_CS 9
#define TDECK_RADIO_BUSY 13
#define TDECK_RADIO_RST 17
#define TDECK_RADIO_DIO1 45

#define TDECK_KEYBOARD_INT 46
#define TDECK_KEYBOARD_ADDR 0x55

#define TDECK_TRACKBALL_UP 3
#define TDECK_TRACKBALL_DOWN 15
#define TDECK_TRACKBALL_LEFT 1
#define TDECK_TRACKBALL_RIGHT 2
#define TDECK_TRACKBALL_CLICK 0

#define TDECK_ES7210_MCLK 48
#define TDECK_ES7210_LRCK 21
#define TDECK_ES7210_SCK 47
#define TDECK_ES7210_DIN 14

#define TDECK_BAT_ADC 4

// Display
#include <Arduino_GFX_Library.h>
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

// Button
// #define LEFT_BTN_PIN 0
// #define RIGHT_BTN_PIN 21

// I2C
#define I2C_SDA 18
#define I2C_SCL 8
#define I2C_FREQ 800000UL

// Touchscreen
#define TOUCH_MODULES_GT911 // GT911 / CST_SELF / CST_MUTUAL / ZTW622 / L58 / FT3267 / FT5x06
#define TOUCH_MODULE_ADDR GT911_SLAVE_ADDRESS1 // CTS328_SLAVE_ADDRESS / L58_SLAVE_ADDRESS / CTS826_SLAVE_ADDRESS / CTS820_SLAVE_ADDRESS / CTS816S_SLAVE_ADDRESS / FT3267_SLAVE_ADDRESS / FT5x06_ADDR / GT911_SLAVE_ADDRESS1 / GT911_SLAVE_ADDRESS2 / ZTW622_SLAVE1_ADDRESS / ZTW622_SLAVE2_ADDRESS
#define TOUCH_SCL I2C_SCL
#define TOUCH_SDA I2C_SDA
#define TOUCH_RES -1
#define TOUCH_INT TDECK_TOUCH_INT

// SD card
#define SD_SCK 3
#define SD_MOSI 4 // CMD
#define SD_MISO 2 // D0
#define SD_D1 1
#define SD_D2 6
#define SD_CS 5   // D3

// I2S
#define I2S_DEFAULT_GAIN_LEVEL 0.8
#define I2S_OUTPUT_NUM I2S_NUM_0
#define I2S_MCLK -1
#define I2S_BCLK 7
#define I2S_LRCK 5
#define I2S_DOUT 6
#define I2S_DIN -1

// #define AUDIO_MUTE_PIN 48   // LOW for mute
