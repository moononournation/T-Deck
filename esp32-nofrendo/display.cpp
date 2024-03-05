extern "C"
{
#include <nes/nes.h>
}

#include "hw_config.h"

/*******************************************************************************
 * Start of Arduino_GFX setting
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#define GFX_DEV_DEVICE LILYGO_T_DECK
#define GFX_EXTRA_PRE_INIT()                    \
    {                                           \
        pinMode(TDECK_SDCARD_CS, OUTPUT);       \
        digitalWrite(TDECK_SDCARD_CS, HIGH);    \
        pinMode(TDECK_RADIO_CS, OUTPUT);        \
        digitalWrite(TDECK_RADIO_CS, HIGH);     \
        pinMode(TDECK_PERI_POWERON, OUTPUT);    \
        digitalWrite(TDECK_PERI_POWERON, HIGH); \
        delay(500);                             \
    }
#define GFX_BL TDECK_TFT_BACKLIGHT
Arduino_DataBus *bus = new Arduino_ESP32SPI(TDECK_TFT_DC, TDECK_TFT_CS, TDECK_SPI_SCK, TDECK_SPI_MOSI, TDECK_SPI_MISO);
Arduino_TFT *gfx = new Arduino_ST7789(bus, GFX_NOT_DEFINED /* RST */, 1 /* rotation */, false /* IPS */);
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

static int16_t w, h, frame_x, frame_y, frame_x_offset, frame_width, frame_height, frame_line_pixels;
extern int16_t bg_color;
extern uint16_t myPalette[];

extern void display_begin()
{
#ifdef GFX_EXTRA_PRE_INIT
    GFX_EXTRA_PRE_INIT();
#endif

    gfx->begin(80000000);
    bg_color = gfx->color565(24, 28, 24); // DARK DARK GREY
    gfx->fillScreen(bg_color);

#ifdef GFX_BL
    ledcSetup(0, 1000, 8);
    ledcAttachPin(GFX_BL, 0);
    ledcWrite(0, 191);
#endif
}

extern "C" void display_init()
{
    w = gfx->width();
    h = gfx->height();
    if (w < 480) // assume only 240x240 or 320x240
    {
        if (w > NES_SCREEN_WIDTH)
        {
            frame_x = (w - NES_SCREEN_WIDTH) / 2;
            frame_x_offset = 0;
            frame_width = NES_SCREEN_WIDTH;
            frame_height = NES_SCREEN_HEIGHT;
            frame_line_pixels = frame_width;
        }
        else
        {
            frame_x = 0;
            frame_x_offset = (NES_SCREEN_WIDTH - w) / 2;
            frame_width = w;
            frame_height = NES_SCREEN_HEIGHT;
            frame_line_pixels = frame_width;
        }
        frame_y = (gfx->height() - NES_SCREEN_HEIGHT) / 2;
    }
    else // assume 480x320
    {
        frame_x = 0;
        frame_y = 0;
        frame_x_offset = 8;
        frame_width = w;
        frame_height = h;
        frame_line_pixels = frame_width / 2;
    }
}

extern "C" void display_write_frame(const uint8_t *data[])
{
    gfx->startWrite();
    if (w < 480)
    {
        gfx->writeAddrWindow(frame_x, frame_y, frame_width, frame_height);
        for (int32_t i = 0; i < NES_SCREEN_HEIGHT; i++)
        {
            gfx->writeIndexedPixels((uint8_t *)(data[i] + frame_x_offset), myPalette, frame_line_pixels);
        }
    }
    else
    {
        /* ST7796 480 x 320 resolution */

        /* Option 1:
         * crop 256 x 240 to 240 x 214
         * then scale up width x 2 and scale up height x 1.5
         * repeat a line for every 2 lines
         */
        // gfx->writeAddrWindow(frame_x, frame_y, frame_width, frame_height);
        // for (int16_t i = 10; i < (10 + 214); i++)
        // {
        //     gfx->writeIndexedPixelsDouble((uint8_t *)(data[i] + 8), myPalette, frame_line_pixels);
        //     if ((i % 2) == 1)
        //     {
        //         gfx->writeIndexedPixelsDouble((uint8_t *)(data[i] + 8), myPalette, frame_line_pixels);
        //     }
        // }

        /* Option 2:
         * crop 256 x 240 to 240 x 214
         * then scale up width x 2 and scale up height x 1.5
         * simply blank a line for every 2 lines
         */
        int16_t y = 0;
        for (int16_t i = 10; i < (10 + 214); i++)
        {
            gfx->writeAddrWindow(frame_x, y++, frame_width, 1);
            gfx->writeIndexedPixelsDouble((uint8_t *)(data[i] + 8), myPalette, frame_line_pixels);
            if ((i % 2) == 1)
            {
                y++; // blank line
            }
        }

        /* Option 3:
         * crop 256 x 240 to 240 x 240
         * then scale up width x 2 and scale up height x 1.33
         * repeat a line for every 3 lines
         */
        // gfx->writeAddrWindow(frame_x, frame_y, frame_width, frame_height);
        // for (int16_t i = 0; i < 240; i++)
        // {
        //     gfx->writeIndexedPixelsDouble((uint8_t *)(data[i] + 8), myPalette, frame_line_pixels);
        //     if ((i % 3) == 1)
        //     {
        //         gfx->writeIndexedPixelsDouble((uint8_t *)(data[i] + 8), myPalette, frame_line_pixels);
        //     }
        // }

        /* Option 4:
         * crop 256 x 240 to 240 x 240
         * then scale up width x 2 and scale up height x 1.33
         * simply blank a line for every 3 lines
         */
        // int16_t y = 0;
        // for (int16_t i = 0; i < 240; i++)
        // {
        //     gfx->writeAddrWindow(frame_x, y++, frame_width, 1);
        //     gfx->writeIndexedPixelsDouble((uint8_t *)(data[i] + 8), myPalette, frame_line_pixels);
        //     if ((i % 3) == 1)
        //     {
        //         y++; // blank line
        //     }
        // }
    }
    gfx->endWrite();
}

extern "C" void display_clear()
{
    gfx->fillScreen(bg_color);
}
