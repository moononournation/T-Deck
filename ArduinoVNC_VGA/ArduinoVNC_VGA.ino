/*******************************************************************************
 * Arduino VNC
 * This is a T-Deck version VNC sample
 *
 * Required libraries:
 * Arduino_GFX: https://github.com/moononournation/Arduino_GFX.git
 * ArduinoVNC: https://github.com/moononournation/arduinoVNC.git
 *
 * Setup steps:
 * 1. Fill your own SSID_NAME, SSID_PASSWORD, VNC_IP, VNC_PORT and VNC_PASSWORD
 * 2. Change your LCD parameters in Arduino_GFX setting
 *
 * Arduino IDE Settings for T-Deck
 * Board:            "ESP32S3 Dev Module"
 * USB CDC On Boot:  "Enable"
 * Flash Mode:       "QIO 120MHz"
 * Flash Size:       "16MB(128Mb)"
 * Partition Scheme: "16M Flash(2M APP/12.5MB FATFS)"
 * PSRAM:            "OPI PSRAM"
 ******************************************************************************/

/* WiFi settings */
const char *SSID_NAME = "YourAP";
const char *SSID_PASSWORD = "PleaseInputYourPasswordHere";

const char *VNC_IP = "192.168.1.4";
const uint16_t VNC_PORT = 5901;
const char *VNC_PASSWORD = "vncpassword";

#define TRACK_SPEED 3

#include "TDECK_PINS.h"

/*******************************************************************************
 * Please config the touch panel in touch.h
 ******************************************************************************/
// #include "touch.h"

/*******************************************************************************
 * Please config the optional keyboard in keyboard.h
 ******************************************************************************/
#include "keyboard.h"

/*******************************************************************************
 * Start of Arduino_GFX setting
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#define GFX_DEV_DEVICE LILYGO_T_DECK
#define GFX_EXTRA_PRE_INIT()                \
  {                                         \
    pinMode(TDECK_PERI_POWERON, OUTPUT);    \
    digitalWrite(TDECK_PERI_POWERON, HIGH); \
    delay(500);                             \
  }
#define GFX_BL TDECK_TFT_BACKLIGHT
Arduino_DataBus *bus = new Arduino_ESP32SPI(TDECK_TFT_DC, TDECK_TFT_CS, TDECK_SPI_SCK, TDECK_SPI_MOSI, TDECK_SPI_MISO);
Arduino_GFX *gfx = new Arduino_ST7789(bus, GFX_NOT_DEFINED /* RST */, 1 /* rotation */, false /* IPS */);
Arduino_Canvas *canvasGfx = new Arduino_Canvas(640 /* width */, 480 /* height */, gfx);
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

#include <WiFi.h>
#include <Wire.h>
#include "VNC_GFX.h"
#include <VNC.h>

VNC_GFX *vnc_gfx = new VNC_GFX(canvasGfx);
arduinoVNC vnc = arduinoVNC(vnc_gfx);

void TFTnoWifi(void)
{
  gfx->fillScreen(BLACK);
  gfx->setCursor(0, ((gfx->height() / 2) - (5 * 8)));
  gfx->setTextColor(RED);
  gfx->setTextSize(5);
  gfx->println("NO WIFI!");
  gfx->setTextSize(2);
  gfx->println();
}

void TFTnoVNC(void)
{
  gfx->fillScreen(BLACK);
  gfx->setCursor(0, ((gfx->height() / 2) - (4 * 8)));
  gfx->setTextColor(GREEN);
  gfx->setTextSize(4);
  gfx->println("connect VNC");
  gfx->setTextSize(2);
  gfx->println();
  gfx->print(VNC_IP);
  gfx->print(":");
  gfx->println(VNC_PORT);
}

int16_t x = 0;
int16_t y = 0;
bool trackball_interrupted = false;
int16_t trackball_up_count = 1;
int16_t trackball_down_count = 1;
int16_t trackball_left_count = 1;
int16_t trackball_right_count = 1;
int16_t trackball_click_count = 0;
void IRAM_ATTR ISR_up()
{
  trackball_interrupted = true;
  trackball_up_count <<= TRACK_SPEED;
}
void IRAM_ATTR ISR_down()
{
  trackball_interrupted = true;
  trackball_down_count <<= TRACK_SPEED;
}
void IRAM_ATTR ISR_left()
{
  trackball_interrupted = true;
  trackball_left_count <<= TRACK_SPEED;
}
void IRAM_ATTR ISR_right()
{
  trackball_interrupted = true;
  trackball_right_count <<= TRACK_SPEED;
}
void IRAM_ATTR ISR_click()
{
  trackball_interrupted = true;
  ++trackball_click_count;
}
void handle_trackball()
{
  if (trackball_interrupted)
  {
    // Serial.printf("x: %d, y: %d, left: %d, right: %d, up: %d, down: %d, click: %d\n", x, y, trackball_left_count, trackball_right_count, trackball_up_count, trackball_down_count, trackball_click_count);
    x -= trackball_left_count;
    x += trackball_right_count;
    y -= trackball_up_count;
    y += trackball_down_count;

    if (x < 0)
    {
      x = 0;
    }
    else if (x > 639)
    {
      x = 639;
    }
    if (y < 0)
    {
      y = 0;
    }
    else if (y > 479)
    {
      y = 479;
    }

    if (trackball_click_count)
    {
      vnc.mouseEvent(x, y, 0b001);
      vnc.mouseEvent(x, y, 0b000);
    }
    else
    {
      vnc.mouseEvent(x, y, 0b000);
    }

    trackball_interrupted = false;
    trackball_up_count = 1;
    trackball_down_count = 1;
    trackball_left_count = 1;
    trackball_right_count = 1;
    trackball_click_count = 0;
  }
}

void handle_keyboard()
{
  int key = keyboard_get_key();
  if (key > 0)
  {
    Serial.println(key);
    switch (key)
    {
    case 8:
      key = 0xff08; // BackSpace
      break;
    case 9:
      key = 0xff09; // Tab
      break;
    case 10:
    case 13:
      key = 0xff0d; // Return or Enter
      break;
    case 27:
      key = 0xff1b; // Escape
      break;
    case 180:
      key = 0xff51; // Left
      break;
    case 181:
      key = 0xff52; // Up
      break;
    case 182:
      key = 0xff54; // Down
      break;
    case 183:
      key = 0xff53; // Right
      break;
    }
    vnc.keyEvent(key, 0b001);
    vnc.keyEvent(key, 0b000);
  }
}

void setup(void)
{
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("Arduino VNC");

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  Wire.begin(TDECK_I2C_SDA, TDECK_I2C_SCL);

  // Init keyboard device
  keyboard_init();

  Serial.println("Init display");
  if (!canvasGfx->begin(80000000))
  {
    Serial.println("Init display failed!");
  }
  gfx->fillScreen(BLACK);

#ifdef GFX_BL
  // pinMode(GFX_BL, OUTPUT);
  // digitalWrite(GFX_BL, HIGH);
  ledcSetup(0, 1000, 8);
  ledcAttachPin(GFX_BL, 0);
  ledcWrite(0, 191);
#endif

  // Init trackball
  pinMode(TDECK_TRACKBALL_UP, INPUT_PULLUP);
  attachInterrupt(TDECK_TRACKBALL_UP, ISR_up, FALLING);
  pinMode(TDECK_TRACKBALL_DOWN, INPUT_PULLUP);
  attachInterrupt(TDECK_TRACKBALL_DOWN, ISR_down, FALLING);
  pinMode(TDECK_TRACKBALL_LEFT, INPUT_PULLUP);
  attachInterrupt(TDECK_TRACKBALL_LEFT, ISR_left, FALLING);
  pinMode(TDECK_TRACKBALL_RIGHT, INPUT_PULLUP);
  attachInterrupt(TDECK_TRACKBALL_RIGHT, ISR_right, FALLING);
  pinMode(TDECK_TRACKBALL_CLICK, INPUT_PULLUP);
  attachInterrupt(TDECK_TRACKBALL_CLICK, ISR_click, FALLING);

  TFTnoWifi();

  Serial.println("Init WiFi");
  gfx->println("Init WiFi");
#if defined(ESP32)
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID_NAME, SSID_PASSWORD);
#elif defined(ESP8266)
  // disable sleep mode for better data rate
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID_NAME, SSID_PASSWORD);
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID_NAME, SSID_PASSWORD);
#elif defined(RTL8722DM)
  WiFi.begin((char *)SSID_NAME, (char *)SSID_PASSWORD);
#endif
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    gfx->print(".");
  }
  Serial.println(" CONNECTED");
  gfx->println(" CONNECTED");
  Serial.println("IP address: ");
  gfx->println("IP address: ");
  Serial.println(WiFi.localIP());
  gfx->println(WiFi.localIP());
  TFTnoVNC();

  Serial.println(F("[SETUP] VNC..."));

#ifdef SEPARATE_DRAW_TASK
  draw_task_setup();
#endif

  vnc.begin(VNC_IP, VNC_PORT);
  vnc.setPassword(VNC_PASSWORD); // optional
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    vnc.reconnect();
    TFTnoWifi();
    delay(100);
  }
  else
  {
    if (vnc.connected())
    {
      handle_trackball();
      handle_keyboard();
    }
    vnc.loop();
    canvasGfx->flushQuad();

    if (!vnc.connected())
    {
      TFTnoVNC();
      // some delay to not flood the server
      delay(5000);
    }
  }
}
