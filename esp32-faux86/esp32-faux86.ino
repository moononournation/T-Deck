/***
 * Required libraries:
 * Arduino_GFX: https://github.com/moononournation/Arduino_GFX.git
 * Faux86-remake: https://github.com/moononournation/Faux86-remake.git
 */

#include <FFat.h>
#include <VM.h>

#include "asciivga_dat.h"
#include "pcxtbios_bin.h"
#include "rombasic_bin.h"
#include "videorom_bin.h"

#include "ArduinoInterface.h"

/*******************************************************************************
 * Start of Arduino_GFX setting
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
// #define GFX_BL DF_GFX_BL // default backlight pin, you may replace DF_GFX_BL to actual backlight pin
// Arduino_DataBus *bus = create_default_Arduino_DataBus();
// Arduino_TFT *gfx = new Arduino_ILI9341(bus, DF_GFX_RST, 3 /* rotation */, false /* IPS */);
#define GFX_DEV_DEVICE LILYGO_T_DECK
#define GFX_EXTRA_PRE_INIT()                         \
	{                                                  \
		pinMode(39 /* TDECK_SDCARD_CS */, OUTPUT);       \
		digitalWrite(39 /* TDECK_SDCARD_CS */, HIGH);    \
		pinMode(9 /* TDECK_RADIO_CS */, OUTPUT);         \
		digitalWrite(9 /* TDECK_RADIO_CS */, HIGH);      \
		pinMode(10 /* TDECK_PERI_POWERON */, OUTPUT);    \
		digitalWrite(10 /* TDECK_PERI_POWERON */, HIGH); \
		delay(500);                                      \
	}
#define GFX_BL 42
Arduino_DataBus *bus = new Arduino_ESP32SPI(11 /* DC */, 12 /* CS */, 40 /* SCK */, 41 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_TFT *gfx = new Arduino_ST7789(bus, GFX_NOT_DEFINED /* RST */, 1 /* rotation */, false /* IPS */);
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

Faux86::VM *vm86;
Faux86::ArduinoHostSystemInterface hostInterface(gfx);

void setup()
{
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	// while(!Serial);
	Serial.println("esp32-faux86");

#ifdef GFX_EXTRA_PRE_INIT
	GFX_EXTRA_PRE_INIT();
#endif

	Serial.println("Init display");
	if (!gfx->begin(80000000))
	{
		Serial.println("Init display failed!");
	}
	gfx->fillScreen(BLACK);

#ifdef GFX_BL
	pinMode(GFX_BL, OUTPUT);
	digitalWrite(GFX_BL, HIGH);
#endif

	if (!FFat.begin(false))
	{
		Serial.println("ERROR: File system mount failed!");
	}

	Faux86::Config vmConfig(&hostInterface);

	vmConfig.singleThreaded = true;
	vmConfig.enableAudio = false;
	vmConfig.useDisneySoundSource = false;
	vmConfig.useSoundBlaster = false;
	vmConfig.useAdlib = false;
	vmConfig.usePCSpeaker = true;
	vmConfig.slowSystem = true;
	vmConfig.frameDelay = 250;				 // 250ms 4fps
	vmConfig.audio.sampleRate = 22050; // 32000 //44100 //48000;
	vmConfig.audio.latency = 200;
	vmConfig.framebuffer.width = 720;
	vmConfig.framebuffer.height = 480;
	vmConfig.cpuSpeed = 0; // no limit

	// vmConfig.biosFile = hostInterface.openFile("/ffat/pcxtbios.bin");
	// vmConfig.romBasicFile = hostInterface.openFile("/ffat/rombasic.bin");
	// vmConfig.videoRomFile = hostInterface.openFile("/ffat/videorom.bin");
	// vmConfig.asciiFile = hostInterface.openFile("/ffat/asciivga.dat");
	vmConfig.biosFile = new Faux86::EmbeddedDisk(pcxtbios_bin, pcxtbios_bin_len);
	vmConfig.romBasicFile = new Faux86::EmbeddedDisk(rombasic_bin, rombasic_bin_len);
	vmConfig.videoRomFile = new Faux86::EmbeddedDisk(videorom_bin, videorom_bin_len);
	vmConfig.asciiFile = new Faux86::EmbeddedDisk(asciivga_dat, asciivga_dat_len);

	// vmConfig.diskDriveA = hostInterface.openFile("/ffat/fd0.img");
	// vmConfig.bootDrive = 0; // DRIVE_A;
	vmConfig.diskDriveC = hostInterface.openFile("/ffat/hd0_12m.img");
	vmConfig.bootDrive = 128U; // DRIVE_C;

	vm86 = new Faux86::VM(vmConfig);
	if (vm86->init())
	{
		// hostInterface.init(vmConfig.resw, vmConfig.resh, vmConfig.enableMenu);
		hostInterface.init(vm86);
	}
}

void loop()
{
	vm86->simulate();
	hostInterface.tick();
}
