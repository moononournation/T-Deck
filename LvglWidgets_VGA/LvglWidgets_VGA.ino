/*******************************************************************************
 * LVGL Widgets
 * This is a widgets demo for LVGL - Light and Versatile Graphics Library
 * import from: https://github.com/lvgl/lv_demos.git
 *
 * Dependent libraries:
 * LVGL: https://github.com/lvgl/lvgl.git
 * 
 * Touch libraries:
 * XPT2046: https://github.com/PaulStoffregen/XPT2046_Touchscreen.git
 * TouchLib: https://github.com/mmMicky/TouchLib.git
 *
 * LVGL Configuration file:
 * Copy your_arduino_path/libraries/lvgl/lv_conf_template.h
 * to your_arduino_path/libraries/lv_conf.h
 *
 * In lv_conf.h around line 15, enable config file:
 * #if 1 // Set it to "1" to enable content
 *
 * Then find and set:
 * #define LV_COLOR_DEPTH     16
 * #define LV_TICK_CUSTOM     1
 *
 * #define LV_COLOR_16_SWAP   0 // for Canvas Class
 *
 * Enable LVGL Demo Widgets
 * #define LV_USE_DEMO_WIDGETS 1
 ******************************************************************************/
#include "lv_demo_widgets.h"

#include "TDECK_PINS.h"

/*******************************************************************************
 * Start of Arduino_GFX setting
 *
 * Arduino_GFX try to find the settings depends on selected board in Arduino IDE
 * Or you can define the display dev kit not in the board list
 * Defalult pin list for non display dev kit:
 * Arduino Nano, Micro and more: CS:  9, DC:  8, RST:  7, BL:  6, SCK: 13, MOSI: 11, MISO: 12
 * ESP32 various dev board     : CS:  5, DC: 27, RST: 33, BL: 22, SCK: 18, MOSI: 23, MISO: nil
 * ESP32-C3 various dev board  : CS:  7, DC:  2, RST:  1, BL:  3, SCK:  4, MOSI:  6, MISO: nil
 * ESP32-S2 various dev board  : CS: 34, DC: 38, RST: 33, BL: 21, SCK: 36, MOSI: 35, MISO: nil
 * ESP32-S3 various dev board  : CS: 40, DC: 41, RST: 42, BL: 48, SCK: 36, MOSI: 35, MISO: nil
 * ESP8266 various dev board   : CS: 15, DC:  4, RST:  2, BL:  5, SCK: 14, MOSI: 13, MISO: 12
 * Raspberry Pi Pico dev board : CS: 17, DC: 27, RST: 26, BL: 28, SCK: 18, MOSI: 19, MISO: 16
 * RTL8720 BW16 old patch core : CS: 18, DC: 17, RST:  2, BL: 23, SCK: 19, MOSI: 21, MISO: 20
 * RTL8720_BW16 Official core  : CS:  9, DC:  8, RST:  6, BL:  3, SCK: 10, MOSI: 12, MISO: 11
 * RTL8722 dev board           : CS: 18, DC: 17, RST: 22, BL: 23, SCK: 13, MOSI: 11, MISO: 12
 * RTL8722_mini dev board      : CS: 12, DC: 14, RST: 15, BL: 13, SCK: 11, MOSI:  9, MISO: 10
 * Seeeduino XIAO dev board    : CS:  3, DC:  2, RST:  1, BL:  0, SCK:  8, MOSI: 10, MISO:  9
 * Teensy 4.1 dev board        : CS: 39, DC: 41, RST: 40, BL: 22, SCK: 13, MOSI: 11, MISO: 12
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#define GFX_DEV_DEVICE LILYGO_T_DECK
#define GFX_BL TDECK_TFT_BACKLIGHT
Arduino_DataBus *bus = new Arduino_ESP32SPI(TDECK_TFT_DC, TDECK_TFT_CS, TDECK_SPI_SCK, TDECK_SPI_MOSI, TDECK_SPI_MISO);
Arduino_GFX *gfx = new Arduino_ST7789(bus, GFX_NOT_DEFINED /* RST */, 1 /* rotation */, false /* IPS */);
Arduino_Canvas *canvasGfx = new Arduino_Canvas(640 /* width */, 480 /* height */, gfx);
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

/*******************************************************************************
 * Please config the touch panel in touch.h
 ******************************************************************************/
#include "touch.h"

/*******************************************************************************
 * Please config the keyboard in keyboard.h
 ******************************************************************************/
#include "keyboard.h"

/* Change to your screen resolution */
static uint32_t screenWidth;
static uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;

/* Display flushing */
static bool canvasUpdated = false;
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  canvasGfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  canvasGfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  canvasUpdated = true;
  lv_disp_flush_ready(disp);
}

bool trackball_interrupted = false;
int16_t trackball_up_count = 0;
int16_t trackball_down_count = 0;
int16_t trackball_left_count = 0;
int16_t trackball_right_count = 0;
int16_t trackball_click_count = 0;
void IRAM_ATTR ISR_up()
{
  trackball_interrupted = true;
  ++trackball_up_count;
}
void IRAM_ATTR ISR_down()
{
  trackball_interrupted = true;
  ++trackball_down_count;
}
void IRAM_ATTR ISR_left()
{
  trackball_interrupted = true;
  ++trackball_left_count;
}
void IRAM_ATTR ISR_right()
{
  trackball_interrupted = true;
  ++trackball_right_count;
}
void IRAM_ATTR ISR_click()
{
  trackball_interrupted = true;
  ++trackball_click_count;
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  if (touch_has_signal())
  {
    if (touch_touched())
    {
      data->state = LV_INDEV_STATE_PR;

      /*Set the coordinates*/
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
    }
    else if (touch_released())
    {
      data->state = LV_INDEV_STATE_REL;
    }
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}

void my_keyboard_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
  static uint32_t last_key = 0;
  if (last_key)
  {
    data->key = last_key;
    data->state = LV_INDEV_STATE_REL;

    last_key = 0;
  }
  else
  {
    uint32_t key;

    if (trackball_up_count)
    {
      key = LV_KEY_UP;
      --trackball_up_count;
    }
    else if (trackball_down_count)
    {
      key = LV_KEY_DOWN;
      --trackball_down_count;
    }
    else if (trackball_left_count)
    {
      key = LV_KEY_LEFT;
      --trackball_left_count;
    }
    else if (trackball_right_count)
    {
      key = LV_KEY_RIGHT;
      --trackball_right_count;
    }
    else if (trackball_click_count)
    {
      key = LV_KEY_ENTER;
      --trackball_click_count;
    }
    else
    {
      key = keyboard_get_key();
    }

    if (key > 0)
    {
      // Serial.println(key);
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

      data->key = key;
      data->state = LV_INDEV_STATE_PR;

      last_key = key;
    }
  }
}

void setup()
{
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("LVGL Widgets Demo");

  //! ⚠️ The board peripheral power control pin needs to be set to HIGH when using the peripheral
  pinMode(TDECK_PERI_POWERON, OUTPUT);
  digitalWrite(TDECK_PERI_POWERON, HIGH);

  // There needs to be a delay after power on, give LILYGO-KEYBOARD some startup time
  delay(500);

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  if (!canvasGfx->begin(80000000))
  {
    Serial.println("canvasGfx->begin() failed!");
  }
  canvasGfx->fillScreen(BLACK);
  canvasGfx->flushQuad();

#ifdef GFX_BL
  // pinMode(GFX_BL, OUTPUT);
  // digitalWrite(GFX_BL, HIGH);
  ledcSetup(0, 1000, 8);
  ledcAttachPin(GFX_BL, 0);
  ledcWrite(0, 191);
#endif

  // Init touch device
  touch_init(canvasGfx->width(), canvasGfx->height(), canvasGfx->getRotation());

  // Init keyboard device
  keyboard_init();

  // Init touch device
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

  lv_init();

  screenWidth = canvasGfx->width();
  screenHeight = canvasGfx->height();
#ifdef ESP32
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * 40, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
  disp_draw_buf = (lv_color_t *)malloc(sizeof(lv_color_t) * screenWidth * 40);
#endif
  if (!disp_draw_buf)
  {
    Serial.println("LVGL disp_draw_buf allocate failed!");
  }
  else
  {
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * 40);

    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    /* Change the following line to your display resolution */
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* Initialize the input device driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    static lv_indev_drv_t indev_drv_key;
    lv_indev_drv_init(&indev_drv_key);
    indev_drv_key.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv_key.read_cb = my_keyboard_read;
    static lv_indev_t *kb_indev = lv_indev_drv_register(&indev_drv_key);

    static lv_group_t *g = lv_group_create();
    lv_group_set_default(g);
    lv_indev_set_group(kb_indev, g);

    lv_demo_widgets();

    Serial.println("Setup done");
  }
}

void loop()
{
  lv_timer_handler(); /* let the GUI do its work */
  if (canvasUpdated)
  {
    canvasGfx->flushQuad();
    canvasUpdated = false;
  }
  delay(5);
}
