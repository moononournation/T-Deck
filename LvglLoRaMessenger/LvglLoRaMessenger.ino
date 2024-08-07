/*******************************************************************************
 * LVGL LoRa Messenger
 *
 * Required libraries:
 * Arduino_GFX: https://github.com/moononournation/Arduino_GFX.git
 * RadioLib: https://github.com/jgromes/RadioLib.git
 * LVGL: https://github.com/lvgl/lvgl.git
 *
 * Touch libraries:
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
 *
 * Arduino IDE Settings for T-Deck
 * Board:            "ESP32S3 Dev Module"
 * USB CDC On Boot:  "Enable"
 * Flash Mode:       "QIO 120MHz"
 * Flash Size:       "16MB(128Mb)"
 * Partition Scheme: "16M Flash(2M APP/12.5MB FATFS)"
 * PSRAM:            "OPI PSRAM"
 ******************************************************************************/

#define USERNAME "Nick"
// #define USERNAME "Carol"
#define KEY_SCAN_MS_INTERVAL 200

#include "T_DECK.h"
#include "touch.h"

#include <lvgl.h>
#include "ui.h"
#include <SPI.h>
#include <RadioLib.h>

void append_message(String msg)
{
  Serial.printf("append_message: %s\n", msg);
  msg += "\n";
  lv_textarea_set_cursor_pos(ui_TextMessages, -1);
  lv_textarea_add_text(ui_TextMessages, msg.c_str());
}

void resumeTextInput()
{
  lv_textarea_set_text(ui_TextInput, "");
  lv_obj_clear_state(ui_TextInput, LV_STATE_DISABLED);
  lv_obj_clear_state(ui_ButtonSend, LV_STATE_DISABLED);
  lv_group_focus_obj(ui_TextInput);
}

#include "LoRa.h"

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

bool keyboard_interrupted = false;
void IRAM_ATTR ISR_key()
{
  keyboard_interrupted = true;
}
unsigned long next_key_scan_ms = 0;
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
    if (keyboard_interrupted || (millis() > next_key_scan_ms))
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
        Wire.requestFrom(TDECK_KEYBOARD_ADDR, 1);
        while (Wire.available() > 0)
        {
          key = Wire.read();
        }
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
        next_key_scan_ms = millis() + KEY_SCAN_MS_INTERVAL;
        keyboard_interrupted = false;

        last_key = key;
      }
    }
  }
}

void sendTextInput(lv_event_t *e)
{
  transmissionFlag = false;

  strTextInput.clear();
  strTextInput += USERNAME;
  strTextInput += ": ";
  strTextInput += lv_textarea_get_text(ui_TextInput);

  lv_obj_add_state(ui_TextInput, LV_STATE_DISABLED);
  lv_obj_add_state(ui_ButtonSend, LV_STATE_DISABLED);

  hasMsgToSend = true;
}

void setup()
{
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("Arduino LoRa Messenger");

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
  // pinMode(GFX_BL, OUTPUT);
  // digitalWrite(GFX_BL, HIGH);
  // ledcSetup(0, 1000, 8);
  // ledcAttachPin(GFX_BL, 0);
  ledcAttach(GFX_BL, 1000, 8);
  ledcWrite(GFX_BL, 191);
#endif

  // Init touch device
  touch_init(gfx->width(), gfx->height(), gfx->getRotation());

  Serial.println("Init LILYGO Keyboard");
  pinMode(TDECK_KEYBOARD_INT, INPUT_PULLUP);
  attachInterrupt(TDECK_KEYBOARD_INT, ISR_key, FALLING);
  Wire.requestFrom(TDECK_KEYBOARD_ADDR, 1);
  if (Wire.read() == -1)
  {
    Serial.println("LILYGO Keyboad not online!");
  }

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

    lv_obj_add_event_cb(ui_ButtonSend, sendTextInput, LV_EVENT_CLICKED, NULL);

    Serial.println("Setup done");

    lv_timer_handler(); /* let the GUI do its work */
  }

  digitalWrite(TDECK_SDCARD_CS, HIGH);
  digitalWrite(TDECK_TFT_CS, HIGH);
  setupRadio();
}

void loop()
{
  lv_obj_invalidate(ui_TextMessages); // hack, it should be some sort of bug
  lv_timer_handler(); /* let the GUI do its work */

  digitalWrite(TDECK_SDCARD_CS, HIGH);
  digitalWrite(TDECK_TFT_CS, HIGH);
  loopRadio();

  delay(5);
}
