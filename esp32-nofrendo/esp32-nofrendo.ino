/*******************************************************************************
 * Arduino ESP32 Nofrendo
 *
 * Required libraries:
 * Arduino_GFX: https://github.com/moononournation/Arduino_GFX.git
 * arduino-nofrendo: https://github.com/moononournation/arduino-nofrendo.git
 *
 * Please check hw_config.h and display.cpp for configuration details
 *
 * Arduino IDE Settings for T-Deck
 * Board:            "ESP32S3 Dev Module"
 * USB CDC On Boot:  "Enable"
 * Flash Mode:       "QIO 120MHz"
 * Flash Size:       "16MB(128Mb)"
 * Partition Scheme: "16M Flash(2M APP/12.5MB FATFS)"
 * PSRAM:            "OPI PSRAM"
 ******************************************************************************/

#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include <FFat.h>
#include <SPIFFS.h>
#include <SD.h>
#include <SD_MMC.h>

#include <Arduino_GFX_Library.h>

#include "hw_config.h"

extern "C"
{
#include <nofrendo.h>
}

int16_t bg_color;
extern Arduino_TFT *gfx;
extern void display_begin();

void setup()
{
    Serial.begin(115200);

    // If display and SD shared same interface, init SPI first
    SPI.begin(TDECK_SPI_SCK, TDECK_SPI_MISO, TDECK_SPI_MOSI);

    // turn off WiFi
    esp_wifi_deinit();

    // disable Core 0 WDT
    TaskHandle_t idle_0 = xTaskGetIdleTaskHandleForCPU(0);
    esp_task_wdt_delete(idle_0);

    // start display
    display_begin();

    // filesystem defined in hw_config.h
    FILESYSTEM_BEGIN

    // find first rom file (*.nes)
    File root = filesystem.open("/");
    char *argv[1];
    if (!root)
    {
        Serial.println("Filesystem mount failed! Please check hw_config.h settings.");
        gfx->println("Filesystem mount failed! Please check hw_config.h settings.");
    }
    else
    {
        bool foundRom = false;

        File file = root.openNextFile();
        while (file)
        {
            if (file.isDirectory())
            {
                // skip
            }
            else
            {
                char *filename = (char *)file.name();
                int8_t len = strlen(filename);
                if (strstr(strlwr(filename + (len - 4)), ".nes"))
                {
                    foundRom = true;
                    char fullFilename[256];
                    sprintf(fullFilename, "%s/%s", FSROOT, filename);
                    Serial.println(fullFilename);
                    argv[0] = fullFilename;
                    break;
                }
            }

            file = root.openNextFile();
        }

        if (!foundRom)
        {
            Serial.println("Failed to find rom file, please copy rom file to data folder and upload with \"ESP32 Sketch Data Upload\"");
            argv[0] = "/";
        }

        Serial.println("NoFrendo start!\n");
        nofrendo_main(1, argv);
        Serial.println("NoFrendo end!\n");
    }
}

void loop()
{
}
