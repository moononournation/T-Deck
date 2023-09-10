/*******************************************************************************
 * LVGL WiFi Connect
 * This is a simple example for LVGL - Light and Versatile Graphics Library
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
 * For SPI/parallel 8 display set color swap can be faster, parallel 16/RGB screen don't swap!
 * #define LV_COLOR_16_SWAP   1 // for SPI and parallel 8
 * #define LV_COLOR_16_SWAP   0 // for parallel 16 and RGB
 ******************************************************************************/
#include <lvgl.h>
#include "ui.h"

#include "TDECK_PINS.h"
#include "WiFi.h"

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
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

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
        key = LV_KEY_BACKSPACE; // BackSpace
        break;
      case 9:
        key = LV_KEY_NEXT; // Tab
        break;
      case 10:
      case 13:
        key = LV_KEY_ENTER; // Return or Enter
        break;
      case 27:
        key = LV_KEY_ESC; // Escape
        break;
      case 180:
        key = LV_KEY_LEFT; // Left
        break;
      case 181:
        key = LV_KEY_UP; // Up
        break;
      case 182:
        key = LV_KEY_DOWN; // Down
        break;
      case 183:
        key = LV_KEY_RIGHT; // Right
        break;
      }

      data->key = key;
      data->state = LV_INDEV_STATE_PR;

      last_key = key;
    }
  }
}

void scanWiFi()
{
  // WiFi.scanNetworks will return the number of networks found.
  int n = WiFi.scanNetworks();
  Serial.println("Scan done");
  if (n == 0)
  {
    lv_roller_set_options(ui_RollerWiFiAP, "No networks found!", LV_ROLLER_MODE_NORMAL);
  }
  else
  {
    String stringWiFiList = "";
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      if (i > 0)
      {
        stringWiFiList += '\n';
      }
      stringWiFiList += WiFi.SSID(i);
    }
    lv_roller_set_options(ui_RollerWiFiAP, stringWiFiList.c_str(), LV_ROLLER_MODE_NORMAL);
  }

  // Delete the scan result to free memory for code below.
  WiFi.scanDelete();
}

void connectWiFi(lv_event_t *e)
{
  lv_scr_load_anim(ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);

  char strWiFiAP[256];
  lv_roller_get_selected_str(ui_RollerWiFiAP, strWiFiAP, sizeof(strWiFiAP));

  WiFi.mode(WIFI_STA);
  WiFi.begin(strWiFiAP, lv_textarea_get_text(ui_TextWiFiPassword));
}

void setup()
{
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("LVGL WiFi Connect Demo");

  // Set WiFi to station mode and disconnect from an AP if it was previously connected.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

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
  // pinMode(GFX_BL, OUTPUT);
  // digitalWrite(GFX_BL, HIGH);
  ledcSetup(0, 1000, 8);
  ledcAttachPin(GFX_BL, 0);
  ledcWrite(0, 191);
#endif

  // Init touch device
  touch_init(gfx->width(), gfx->height(), gfx->getRotation());

  // Init keyboard device
  keyboard_init();

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

  lv_init();

  screenWidth = gfx->width();
  screenHeight = gfx->height();
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

    /* Init SquareLine prepared UI */
    ui_init();

    lv_obj_add_event_cb(ui_ButtonWiFiConnect, connectWiFi, LV_EVENT_CLICKED, NULL);

    Serial.println("Setup done");

    lv_timer_handler(); /* let the GUI do its work */

    scanWiFi();
  }
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    lv_scr_load_anim(ui_Screen3, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
  }

  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
}
