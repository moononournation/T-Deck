/*******************************************************************************
 * Arduino ESP32 Faux86
 *
 * Required libraries:
 * Arduino_GFX: https://github.com/moononournation/Arduino_GFX.git
 * Faux86-remake: https://github.com/moononournation/Faux86-remake.git
 *
 * Arduino IDE Settings for T-Deck
 * Board:            "ESP32S3 Dev Module"
 * USB CDC On Boot:  "Enable"
 * Flash Mode:       "QIO 120MHz"
 * Flash Size:       "16MB(128Mb)"
 * Partition Scheme: "16M Flash(2M APP/12.5MB FATFS)"
 * PSRAM:            "OPI PSRAM"
 ******************************************************************************/

#include "TDECK_PINS.h"
#define TRACK_SPEED 2
#define KEY_SCAN_MS_INTERVAL 200

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
Arduino_DataBus *bus = new Arduino_ESP32SPIDMA(11 /* DC */, 12 /* CS */, 40 /* SCK */, 41 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_TFT *gfx = new Arduino_ST7789(bus, GFX_NOT_DEFINED /* RST */, 1 /* rotation */, false /* IPS */);
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

/*******************************************************************************
 * Please config the touch panel in touch.h
 ******************************************************************************/
#include "touch.h"

#include "keymap.h"

#include <WiFi.h>
#include <FFat.h>
#include <VM.h>

#include "asciivga_dat.h"
#include "pcxtbios_bin.h"
#include "rombasic_bin.h"
#include "videorom_bin.h"

bool keyboard_interrupted = false;
void IRAM_ATTR ISR_key()
{
	keyboard_interrupted = true;
}
unsigned long next_key_scan_ms = 0;
bool trackball_interrupted = false;
int16_t trackball_up_count = 1;
int16_t trackball_down_count = 1;
int16_t trackball_left_count = 1;
int16_t trackball_right_count = 1;
int16_t trackball_click_count = 0;
bool mouse_downed = false;
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

#include "ArduinoInterface.h"
Faux86::VM *vm86;
Faux86::ArduinoHostSystemInterface hostInterface(gfx);

void setup()
{
	WiFi.mode(WIFI_OFF);

	Serial.begin(115200);
	// Serial.setDebugOutput(true);
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

	Serial.println("Init touchscreen");
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

	if (!FFat.begin(false))
	{
		Serial.println("ERROR: File system mount failed!");
	}

	Faux86::Config vmConfig(&hostInterface);

	/* CPU settings */
	vmConfig.singleThreaded = true; // only WIN32 support multithreading
	vmConfig.slowSystem = true;			// slow system will reserve more time for audio, if enabled
	vmConfig.cpuSpeed = 0;					// no limit

	/* Video settings */
	vmConfig.frameDelay = 200; // 200ms 5fps
	vmConfig.framebuffer.width = 720;
	vmConfig.framebuffer.height = 480;

	/* Audio settings */
	vmConfig.enableAudio = false;
	vmConfig.useDisneySoundSource = false;
	vmConfig.useSoundBlaster = false;
	vmConfig.useAdlib = false;
	vmConfig.usePCSpeaker = true;
	vmConfig.audio.sampleRate = 22050; // 32000 //44100 //48000;
	vmConfig.audio.latency = 200;

	/* set BIOS ROM */
	// vmConfig.biosFile = hostInterface.openFile("/ffat/pcxtbios.bin");
	vmConfig.biosFile = new Faux86::EmbeddedDisk(pcxtbios_bin, pcxtbios_bin_len);

	/* set Basic ROM */
	// vmConfig.romBasicFile = hostInterface.openFile("/ffat/rombasic.bin");
	vmConfig.romBasicFile = new Faux86::EmbeddedDisk(rombasic_bin, rombasic_bin_len);

	/* set Video ROM */
	// vmConfig.videoRomFile = hostInterface.openFile("/ffat/videorom.bin");
	vmConfig.videoRomFile = new Faux86::EmbeddedDisk(videorom_bin, videorom_bin_len);

	/* set ASCII FONT Data */
	// vmConfig.asciiFile = hostInterface.openFile("/ffat/asciivga.dat");
	vmConfig.asciiFile = new Faux86::EmbeddedDisk(asciivga_dat, asciivga_dat_len);

	/* floppy drive image */
	// vmConfig.diskDriveA = hostInterface.openFile("/ffat/fd0.img");

	/* harddisk drive image */
	vmConfig.diskDriveC = hostInterface.openFile("/ffat/hd0_12m.img");

	/* set boot drive */
	// vmConfig.bootDrive = 0; // DRIVE_A;
	vmConfig.bootDrive = 128U; // DRIVE_C;

	vm86 = new Faux86::VM(vmConfig);
	if (vm86->init())
	{
		hostInterface.init(vm86);
	}
}

void loop()
{
	vm86->simulate();
	// hostInterface.tick();

	/* handle keyboard input */
	if (keyboard_interrupted || (millis() > next_key_scan_ms))
	{
		Wire.requestFrom(TDECK_KEYBOARD_ADDR, 1);
		while (Wire.available() > 0)
		{
			char key = Wire.read();
			if (key != 0)
			{
				uint16_t keyxt = ascii2xtMapping[key];
				Serial.printf("key: %c, keyxt: %0x\n", key, keyxt);
				vm86->input.handleKeyDown(keyxt);
				vm86->simulate();
				vm86->input.handleKeyUp(keyxt);
				next_key_scan_ms = millis() + KEY_SCAN_MS_INTERVAL;
			}
		}
		keyboard_interrupted = false;
	}

	/* handle trackball input */
	if (trackball_interrupted)
	{
		if (trackball_click_count > 0)
		{
			Serial.println("vm86->mouse.handleButtonDown(Faux86::SerialMouse::ButtonType::Left);");
			vm86->mouse.handleButtonDown(Faux86::SerialMouse::ButtonType::Left);
			mouse_downed = true;
		}
		int16_t x_delta = trackball_right_count - trackball_left_count;
		int16_t y_delta = trackball_down_count - trackball_up_count;
		if ((x_delta != 0) || (y_delta != 0))
		{
			Serial.printf("x_delta: %d, y_delta: %d\n", x_delta, y_delta);
			vm86->mouse.handleMove(x_delta, y_delta);
		}
		trackball_interrupted = false;
		trackball_up_count = 1;
		trackball_down_count = 1;
		trackball_left_count = 1;
		trackball_right_count = 1;
		trackball_click_count = 0;
	}
	else if (mouse_downed)
	{
		if (digitalRead(TDECK_TRACKBALL_CLICK) == HIGH) // released
		{
			Serial.println("vm86->mouse.handleButtonUp(Faux86::SerialMouse::ButtonType::Left);");
			vm86->mouse.handleButtonUp(Faux86::SerialMouse::ButtonType::Left);
			mouse_downed = false;
		}
	}
}
