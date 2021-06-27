/*
 Name:		Pet_Bit_Mk2.ino
 Created:	6/25/2021 7:19:18 PM
 Author:	Chris
*/

// Libraries.

#include <vfs_api.h>                // File system library.
#include <FSImpl.h>                 // File system library.
#include <FS.h>                     // File system library.
#include <SPI.h>					// SPI bus for TFT using software SPI.
#include <TFT_eSPI.h>               // TFT SPI library.
#include <EEPROM.h>					// EEPROM library.

#include "Free_Fonts.h"				// Free fonts for use with eTFT.
#include "colours.h"                // Colours.
#include "screenLayout.h"				// Screen layout
#include "drawBitmap.h"		// Draw battery level bitmaps.
#include "xphDial.h"				// XPH Odometre
#include "graphDistance.h"			// Daily distance chart
#include "graphTime.h"				// Daily time chart
#include "format_function.h"		// Special formatting function from Kris Kasprzak
#include "buttonIcons.h"			// bUTTON icons.
#include "startScreen.h"			// Start screen bitmap.
#include "icons.h"					// Battery level bitmaps.
#include "touchCalibrate.h"			// Calibrate touch screen.

// TFT SPI Interface for ESP32 using TFT-eSPI.

#define TFT_CS   5
#define TFT_DC   4
#define TFT_LED  15
#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_RST  22
#define TFT_MISO 19
#define TOUCH_CS 14

// Pin out Configuration.

byte sDCS = 33;						// SD Card chip select pin.
const int interruptWheelSensor = 26;		// Reed swtich sensor.

byte bPlus = 1;						// Button +
byte bMinus = 3;					// Button -

// Configure ILI9341 display.

TFT_eSPI tft = TFT_eSPI();			// Invoke custom library.
boolean screenRedraw = 0;

// Data variables.

byte configurationFlag = 0;					// Configuration menu flag.

volatile unsigned int  distanceCounter = 0;	// Counting rotations for distance travelled.

volatile unsigned long passedTime;			// Setting time to calculate speed.
volatile unsigned long startTime;			// Setting time to calculate speed.

volatile unsigned long lastRotation1;		// Checking if wheel is still turning.
volatile unsigned long lastRotation2;		// Checking if wheel is still turning.

float circumference;						// Wheel circumference.
float circImperial;							// Conversion into MPH.

float distanceTravelled = 0.00;				// Total distance travelled.
float speedKph = 0.00;
float speedMph = 0.00;
float rpm = 0.00;

float maxKphSpeed = 0;						// Recording max speed.

char rpmArray[7];							// Holding data in character arrays for formatting reasons.
char kphArray[7];
char mphArray[7];
char maxKphArray[7];
char averageKphSpeedArray[7];
char sessionDistanceArray[7];
char currentSessionTimeArray[7];

// Configure EEEPROM. 

int eeMenuAddress = 0;						// EEPROM address for start menu position.
int eeMenuSetting;							// Actual commit for writing, 4 bytes.
boolean eeMenuSettingChange = false;		// Used for menu scrolling before committing to save EEPROM writes.

int eeCircAddress = 4;						// EEPROM address for circumference.
float eeCircSetting;						// Actual commit for writing, 4 bytes.
boolean eeCircSettingChange = false;		// Used for circumference setting before committing to save EEPROM writes.

int eeTotalDistanceAddress = 8;				// EEPROM address for total distance.
unsigned long eeTotalDistance;				// Actual commit for writing, 4 bytes.
boolean eeTotalDistanceChange = false;		// Used for total distance before committing to save EEPROM writes.

int eeSessionTimeArray1Address = 16;		// EEPROM address for session time 1
unsigned long eeSessionTime1;				// Actual commit for writing, 4 bytes.

int eeSessionTimeArray2Address = 20;		// EEPROM address for session time 2
unsigned long eeSessionTime2;				// Actual commit for writing, 4 bytes.

int eeSessionTimeArray3Address = 24;		// EEPROM address for session time 3
unsigned long eeSessionTime3;				// Actual commit for writing, 4 bytes.

int eeSessionTimeArray4Address = 28;		// EEPROM address for session time 4
unsigned long eeSessionTime4;				// Actual commit for writing, 4 bytes.

int eeSessionTimeArray5Address = 32;		// EEPROM address for session time 5
unsigned long eeSessionTime5;				// Actual commit for writing, 4 bytes.

int eeSessionTimeArray6Address = 36;		// EEPROM address for session time 6
unsigned long eeSessionTime6;				// Actual commit for writing, 4 bytes.

int eeSessionTimeArray7Address = 40;		// EEPROM address for session time 7
unsigned long eeSessionTime7;				// Actual commit for writing, 4 bytes.

int eeSessionDistanceArray1Address = 44;	// EEPROM address for session distance 1
unsigned int eeSessionDistance1;			// Actual commit for writing, 4 bytes.

int eeSessionDistanceArray2Address = 48;	// EEPROM address for session distance 2
unsigned int eeSessionDistance2;			// Actual commit for writing, 4 bytes.

int eeSessionDistanceArray3Address = 52;	// EEPROM address for session distance 3
unsigned int eeSessionDistance3;			// Actual commit for writing, 4 bytes.

int eeSessionDistanceArray4Address = 56;	// EEPROM address for session distance 4
unsigned int eeSessionDistance4;			// Actual commit for writing, 4 bytes.

int eeSessionDistanceArray5Address = 60;	// EEPROM address for session distance 5
unsigned int eeSessionDistance5;			// Actual commit for writing, 4 bytes.

int eeSessionDistanceArray6Address = 64;	// EEPROM address for session distance 6
unsigned int eeSessionDistance6;			// Actual commit for writing, 4 bytes.

int eeSessionDistanceArray7Address = 68;	// EEPROM address for session distance 7
unsigned int eeSessionDistance7;			// Actual commit for writing, 4 bytes.

boolean eeSessionChange = false;			// Used for session time before committing to save EEPROM writes.

int eeResetSettingAddress = 72;				// EEPROM address for master reset
unsigned int eeResetSetting;				// Actual commit for writing, 1 byte.
boolean eeResetSettingChange = false;		// Used for reset setting change before committing to save EEPROM writes.

int eeSessionArrayPositionAddress = 76;		// EEPROM address for array position.
unsigned int eeSessionArrayPosition;		// Actual commit for writing, 4 bytes.

// Average speed calculation variables.

const int numReadings = 10;

float	readings[numReadings];				// Latest Kph readings.
int		readIndex = 0;						// The index of the current reading.
float	total = 0.00;						// The running total of the readings.
float	averageKphSpeed = 0.00;				// The average speed.

// Session time variables.

unsigned long sessionTimeCap = 60;			// Set cap for graph if statements.
boolean recordSessions = 0;					// Flag to trigger the recording of each session.
volatile boolean sessionTimeFlag = 0;		// Flag to trigger the recording of each session.
volatile unsigned long sessionStartTime;	// Time each pt session starts.
unsigned long sessionTimeArray[7];			// Array for storing 7 sessions.
byte sessionArrayPosition = 0;				// Array position, this is also used for the distance array position.
volatile unsigned long sessionTimeMillis;	// Time each pt session in millis.
volatile unsigned int sessionTime;			// Time each pt session in minutes.

float sessionStartDistance = 0.00;
float sessionDistance;						// Session distance.

unsigned int distanceGraphCap = 999;		// Set cap for graph if statements.

unsigned int sessionTimeArray1;				// These variables are needed for the Kris Kasprzak charts.
unsigned int sessionTimeArray2;
unsigned int sessionTimeArray3;
unsigned int sessionTimeArray4;
unsigned int sessionTimeArray5;
unsigned int sessionTimeArray6;
unsigned int sessionTimeArray7;

unsigned int distanceTravelledArray[7];		// Array for storing 7 sessions.

unsigned int distanceTravelledArray1;		// These variables are needed for the Kris Kasprzak charts.
unsigned int distanceTravelledArray2;
unsigned int distanceTravelledArray3;
unsigned int distanceTravelledArray4;
unsigned int distanceTravelledArray5;
unsigned int distanceTravelledArray6;
unsigned int distanceTravelledArray7;

// Battery measurement variables.

#define sensitivity (4 / 1024.0)				// Battery sensitivity setting.
unsigned long batteryMeasureInterval = 600000;	// Battery measuring interval.
unsigned long batteryMeasureNow;
float sensorValue;
float sensorValueVolts;
float sensorValuePerc;

// Menu positions and refresh.

byte screenMenu = 4;				// Screen menu selection.
boolean menuChange = 1;

// Function for odemeter testing only.

unsigned long speedMeasureInterval = 50;	// Demo speed dial data function.
unsigned long speedMeasureNow;				// Demo speed dial data function.
boolean speedDirection = 1;					// Demo speed dial data function.

// Dial and chart function parametres.

boolean dial_1 = true;				// Odometer dial.

boolean graph_1 = true;				// Bar graph.
boolean graph_2 = true;
boolean graph_3 = true;
boolean graph_4 = true;
boolean graph_5 = true;
boolean graph_6 = true;
boolean graph_7 = true;

boolean graph_8 = true;				// Bar graph.
boolean graph_9 = true;
boolean graph_10 = true;
boolean graph_11 = true;
boolean graph_12 = true;
boolean graph_13 = true;
boolean graph_14 = true;

/*-----------------------------------------------------------------*/

void setup() {

	//Begin serial mode.

	Serial.begin(115200);

	// Set pin modes.

	pinMode(TFT_LED, OUTPUT);				// Output for LCD back light.
	pinMode(sDCS, OUTPUT);					// Output for chip select for SD SPI bus.
	pinMode(interruptWheelSensor, INPUT_PULLUP);	// Wheel sensor (REED switch).
	digitalWrite(TFT_LED, HIGH);			// Outout for LCD back light.

	// Set all SPI chip selects to HIGH to stablise SPI bus.

	digitalWrite(TOUCH_CS, HIGH);			// Touch controller chip select.
	digitalWrite(TFT_CS, HIGH);				// TFT screen chip select.
	digitalWrite(sDCS, HIGH);				// SD card chips select.

	// Configure interupt.

	attachInterrupt(digitalPinToInterrupt(interruptWheelSensor), rotationInterruptISR, FALLING);

	// Configure EEPROM.

	EEPROM.begin(512);
	EEPROM.get(eeResetSettingAddress, eeResetSetting);

	/*
	*
	* This line is used for initial ESP8266 power on from manufacture to set EEPROM, comment out once uploaded and re-upload.
	*
	* resetSystemDemoData();
	*
	*/

	if (eeResetSetting == 1) {

		resetSystemData();
	}

	else if (eeResetSetting == 2) {

		resetSystemDemoData();
	}

	// Load previous settings and last recorded data.

	EEPROM.get(eeMenuAddress, eeMenuSetting);
	EEPROM.get(eeCircAddress, eeCircSetting);
	EEPROM.get(eeMenuAddress, screenMenu);
	EEPROM.get(eeCircAddress, circumference);
	EEPROM.get(eeTotalDistanceAddress, distanceCounter);
	EEPROM.get(eeSessionArrayPositionAddress, sessionArrayPosition);

	EEPROM.commit();

	EEPROM.get(eeSessionTimeArray1Address, sessionTimeArray[0]);
	EEPROM.get(eeSessionTimeArray2Address, sessionTimeArray[1]);
	EEPROM.get(eeSessionTimeArray3Address, sessionTimeArray[2]);
	EEPROM.get(eeSessionTimeArray4Address, sessionTimeArray[3]);
	EEPROM.get(eeSessionTimeArray5Address, sessionTimeArray[4]);
	EEPROM.get(eeSessionTimeArray6Address, sessionTimeArray[5]);
	EEPROM.get(eeSessionTimeArray7Address, sessionTimeArray[6]);

	EEPROM.commit();

	sessionTimeArray1 = sessionTimeArray[0] / 100 / 60;
	sessionTimeArray2 = sessionTimeArray[1] / 100 / 60;
	sessionTimeArray3 = sessionTimeArray[2] / 100 / 60;
	sessionTimeArray4 = sessionTimeArray[3] / 100 / 60;
	sessionTimeArray5 = sessionTimeArray[4] / 100 / 60;
	sessionTimeArray6 = sessionTimeArray[5] / 100 / 60;
	sessionTimeArray7 = sessionTimeArray[6] / 100 / 60;

	EEPROM.get(eeSessionDistanceArray1Address, distanceTravelledArray[0]);
	EEPROM.get(eeSessionDistanceArray2Address, distanceTravelledArray[1]);
	EEPROM.get(eeSessionDistanceArray3Address, distanceTravelledArray[2]);
	EEPROM.get(eeSessionDistanceArray4Address, distanceTravelledArray[3]);
	EEPROM.get(eeSessionDistanceArray5Address, distanceTravelledArray[4]);
	EEPROM.get(eeSessionDistanceArray6Address, distanceTravelledArray[5]);
	EEPROM.get(eeSessionDistanceArray7Address, distanceTravelledArray[6]);

	EEPROM.commit();

	distanceTravelledArray1 = distanceTravelledArray[0];
	distanceTravelledArray2 = distanceTravelledArray[1];
	distanceTravelledArray3 = distanceTravelledArray[2];
	distanceTravelledArray4 = distanceTravelledArray[3];
	distanceTravelledArray5 = distanceTravelledArray[4];
	distanceTravelledArray6 = distanceTravelledArray[5];
	distanceTravelledArray7 = distanceTravelledArray[6];

	// Ensure menu position, circumference & array position are within parametres.

	if (screenMenu < 0 || screenMenu > 4) {

		screenMenu = 4;

	} // Close if.

	if (circumference < 0 || circumference > 2) {

		circumference = 1.0;

	} // Close if.

	if (sessionArrayPosition < 0 || sessionArrayPosition > 6) {

		sessionArrayPosition = 0;

	} // Close if.

	// Initialise TFT display.

	tft.begin();
	tft.setRotation(1);
	tft.setCursor(0, 0);
	tft.fillScreen(ILI9341_BLACK);

	digitalWrite(TFT_LED, LOW);				// LOW to turn backlight on.
	delay(500);

	// Start up screen.

	tft.fillScreen(ILI9341_WHITE);

	int h = 80, w = 112, row, col, buffidx = 0;

	for (row = 16; row < h; row++) {		// For each scanline.

		for (col = 48; col < w; col++) {	// For each pixel, read from Flash Memory, since image is stored as uint16_t, pgm_read_word is used as it uses 16bit address.
			tft.drawPixel(col, row, pgm_read_word(startScreen + buffidx));
			buffidx++;

		} // End for loop.

	} // End for loop.

	tft.setTextSize(1);
	tft.setTextColor(BLACK);
	tft.setCursor(120, 120);
	tft.println("PetBit");

	delay(1500);

	// Calibrate touch screen.

	// touch_calibrate(tft); // Build future meny option in settings

	uint16_t calData[5] = { 365, 3511, 243, 3610, 7 };
	tft.setTouch(calData);

	// Clear screen.

	tft.fillScreen(ILI9341_BLACK);

	// Draw border and buttons at start.

	drawBorder();
	startUp();

	screenMenu = 1;

	speedMeasureNow = millis() - speedMeasureInterval;	// Set initial level so first measurement is made.
	speedMeasureNow = millis();							// Reset measurement to match interval.

} // Close setup.

/*-----------------------------------------------------------------*/

void loop() {

	menu_Change();		// Reset menu change at each pass after touch is pressed.
	mainData();			// Calculates main data.
	averageSpeed();		// Calculate average speed.
	//demoSpeedData();	// Demo KPH data for testing, comment when finished.

	// Draw screen icons and button images.

	if (screenMenu == 5) {

		drawBitmap(tft, SETTINGS_COG_Y, SETTINGS_COG_X, settingsRed, SETTINGS_COG_W, SETTINGS_COG_H);
		drawBitmap(tft, BUTTON4_Y + 1, BUTTON4_X + 1, configSet, BUTTON4_W - 2, BUTTON4_H - 2);
		tft.drawRect(BUTTON4_X, BUTTON4_Y, BUTTON4_W, BUTTON4_H, TFT_BLACK);
		drawBitmap(tft, BUTTON3_Y + 1, BUTTON3_X + 1, configPlus, BUTTON3_W - 2, BUTTON3_H - 2);
		tft.drawRect(BUTTON3_X, BUTTON3_Y, BUTTON3_W, BUTTON3_H, TFT_BLACK);
		drawBitmap(tft, BUTTON2_Y + 1, BUTTON2_X + 1, configMinus, BUTTON2_W - 2, BUTTON2_H - 2);
		tft.drawRect(BUTTON2_X, BUTTON2_Y, BUTTON2_W, BUTTON2_H, TFT_BLACK);
		drawBitmap(tft, BUTTON1_Y + 1, BUTTON1_X + 1, configExit, BUTTON1_W - 2, BUTTON1_H - 2);
		tft.drawRect(BUTTON1_X, BUTTON1_Y, BUTTON1_W, BUTTON1_H, TFT_BLACK);

	}

	else drawBitmap(tft, SETTINGS_COG_Y, SETTINGS_COG_X, settingsWhite, SETTINGS_COG_W, SETTINGS_COG_H);

	if (screenMenu == 4) {

		drawBitmap(tft, BUTTON4_Y + 1, BUTTON4_X + 1, distanceIconRed, BUTTON4_W - 2, BUTTON4_H - 2);
		tft.drawRect(BUTTON4_X, BUTTON4_Y, BUTTON4_W, BUTTON4_H, TFT_RED);

	}

	else if (screenMenu != 5) {

		drawBitmap(tft, BUTTON4_Y + 1, BUTTON4_X + 1, distanceIconWhite, BUTTON4_W - 2, BUTTON4_H - 2);
		tft.drawRect(BUTTON4_X, BUTTON4_Y, BUTTON4_W, BUTTON4_H, TFT_BLACK);
	}

	if (screenMenu == 3) {

		drawBitmap(tft, BUTTON3_Y + 1, BUTTON3_X + 1, timeIconRed, BUTTON3_W - 2, BUTTON4_H - 2);
		tft.drawRect(BUTTON3_X, BUTTON3_Y, BUTTON3_W, BUTTON3_H, TFT_RED);
	}

	else if (screenMenu != 5) {

		drawBitmap(tft, BUTTON3_Y + 1, BUTTON3_X + 1, timeIconWhite, BUTTON3_W - 2, BUTTON4_H - 2);
		tft.drawRect(BUTTON3_X, BUTTON3_Y, BUTTON3_W, BUTTON3_H, TFT_BLACK);
	}

	if (screenMenu == 2) {

		drawBitmap(tft, BUTTON2_Y + 1, BUTTON2_X + 1, speedIconRed, BUTTON2_W - 2, BUTTON2_H - 2);
		tft.drawRect(BUTTON2_X, BUTTON2_Y, BUTTON2_W, BUTTON2_H, TFT_RED);
	}

	else if (screenMenu != 5) { 
		
		drawBitmap(tft, BUTTON2_Y + 1, BUTTON2_X + 1, speedIconWhite, BUTTON2_W - 2, BUTTON2_H - 2); 
		tft.drawRect(BUTTON2_X, BUTTON2_Y, BUTTON2_W, BUTTON2_H, TFT_BLACK);
	}

	if (screenMenu == 1) {

		drawBitmap(tft, BUTTON1_Y + 1, BUTTON1_X + 1, sessionIconRed, BUTTON1_W - 2, BUTTON1_H - 2);
		tft.drawRect(BUTTON1_X, BUTTON1_Y, BUTTON1_W, BUTTON1_H, TFT_RED);
	}

	else if (screenMenu != 5) { 
		
		drawBitmap(tft, BUTTON1_Y + 1, BUTTON1_X + 1, sessionIconWhite, BUTTON1_W - 2, BUTTON1_H - 2); 
		tft.drawRect(BUTTON1_X, BUTTON1_Y, BUTTON1_W, BUTTON1_H, TFT_BLACK);
	}

	drawBitmap(tft, BATTERY_ICON_Y, BATTERY_ICON_X, ccBatt100, BATTERY_ICON_W, BATTERY_ICON_H);

	drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiWhite, WIFI_ICON_W, WIFI_ICON_H);


	uint16_t x, y;

	// See if there's any touch data for us
	if (tft.getTouch(&x, &y)) {
		// Draw a block spot to show where touch was calculated to be
#ifdef BLACK_SPOT
		tft.fillCircle(x, y, 2, TFT_BLACK);
#endif

		// Button one.

		if ((x > BUTTON1_X) && (x < (BUTTON1_X + BUTTON1_W))) {
			if ((y > BUTTON1_Y) && (y <= (BUTTON1_Y + BUTTON1_H))) {

				if (screenMenu != 1) {		// To stop screen flicker when pressing the same menu button again.

					screenMenu = 1;
					menuChange = 1;
					screenRedraw = 1;
					graph_8 = true;
					graph_9 = true;
					graph_10 = true;
					graph_11 = true;
					graph_12 = true;
					graph_13 = true;
					graph_14 = true;
					Serial.print("Button 1 hit ");
					Serial.print("Screen Menu: ");
					Serial.print(screenMenu);
					Serial.println(" ");

				} // Close if.

			} // Close if.

		} // Close if.

		// Button two.

		if ((x > BUTTON2_X) && (x < (BUTTON2_X + BUTTON2_W))) {
			if ((y > BUTTON2_Y) && (y <= (BUTTON2_Y + BUTTON2_H))) {

				if (screenMenu != 2) {		// To stop screen flicker when pressing the same menu button again.

					screenMenu = 2;
					menuChange = 1;
					screenRedraw = 1;
					graph_1 = true;
					graph_2 = true;
					graph_3 = true;
					graph_4 = true;
					graph_5 = true;
					graph_6 = true;
					graph_7 = true;;
					Serial.print("Button 2 hit ");
					Serial.print("Screen Menu: ");
					Serial.print(screenMenu);
					Serial.println(" ");

				} // Close if.

			} // Close if.

		} // Close if.

		// Button three.

		if ((x > BUTTON3_X) && (x < (BUTTON3_X + BUTTON3_W))) {
			if ((y > BUTTON3_Y) && (y <= (BUTTON3_Y + BUTTON3_H))) {

				if (screenMenu != 3) {		// To stop screen flicker when pressing the same menu button again.

					screenMenu = 3;
					menuChange = 1;
					screenRedraw = 1;
					dial_1 = true;
					Serial.print("Button 3 hit ");
					Serial.print("Screen Menu: ");
					Serial.print(screenMenu);
					Serial.println(" ");

				} // Close if.

			} // Close if.

		} // Close if.

		// Button four.

		if ((x > BUTTON4_X) && (x < (BUTTON4_X + BUTTON4_W))) {
			if ((y > BUTTON4_Y) && (y <= (BUTTON4_Y + BUTTON4_H))) {

				if (screenMenu != 4) {		// To stop screen flicker when pressing the same menu button again.

					screenMenu = 4;
					menuChange = 1;
					screenRedraw = 1;
					Serial.print("Button 4 hit ");
					Serial.print("Screen Menu: ");
					Serial.print(screenMenu);
					Serial.println(" ");

				} // Close if.

			} // Close if.

		} // Close if.

		if ((x > SETTINGS_COG_X) && (x < (SETTINGS_COG_X + SETTINGS_COG_W))) {
			if ((y > SETTINGS_COG_Y) && (y <= (SETTINGS_COG_Y + SETTINGS_COG_H))) {

				if (screenMenu != 5) {		// To stop screen flicker when pressing the same menu button again.

					screenMenu = 5;
					menuChange = 1;
					screenRedraw = 1;
					Serial.print("Button 5 hit ");
					Serial.print("Screen Menu: ");
					Serial.print(screenMenu);
					Serial.println(" ");

				} // Close if.

			} // Close if.

		} // Close if.

	} // Close if.

	// Trigger screen choice.

	if (screenMenu == 1) {

		if (screenRedraw == 1) {

			drawBlackBox();
			screenRedraw = 0;

		} // Close if.

		CurrentExerciseScreen();

	} // Close if.

	if (screenMenu == 2) {

		if (screenRedraw == 1) {

			drawBlackBox();
			screenRedraw = 0;

		} // Close if.

		XphDialScreen(tft, dialX, dialY, 80, 0, 20, 2, 170, speedKph, 2, 0, RED, WHITE, BLACK, "Xph", dial_1); // XPH dial screen function.

	} // Close if.

	if (screenMenu == 3) {

		if (screenRedraw == 1) {

			drawBlackBox();
			screenRedraw = 0;

		} // Close if.

		// Session time bar graphs.

		if (sessionTimeArray1 <= 60) {

			ptSessionTimeV1(tft, graphX1, graphY, graphW, graphH, 0, 60, 10, sessionTimeArray1, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "1", graph_1);

		} // Close if.

		else ((ptSessionTimeV1(tft, graphX1, graphY, graphW, graphH, 0, 60, 10, sessionTimeCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "1", graph_1)));

		if (sessionTimeArray2 <= 60) {

			ptSessionTimeV2(tft, graphX2, graphY, graphW, graphH, 0, 60, 10, sessionTimeArray2, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "2", graph_2);

		} // Close if.

		else ((ptSessionTimeV2(tft, graphX2, graphY, graphW, graphH, 0, 60, 10, sessionTimeCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "2", graph_2)));

		if (sessionTimeArray3 <= 60) {

			ptSessionTimeV2(tft, graphX3, graphY, graphW, graphH, 0, 60, 10, sessionTimeArray3, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "3", graph_3);

		} // Close if.

		else ((ptSessionTimeV2(tft, graphX3, graphY, graphW, graphH, 0, 60, 10, sessionTimeCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "3", graph_3)));

		if (sessionTimeArray4 <= 60) {

			ptSessionTimeV2(tft, graphX4, graphY, graphW, graphH, 0, 60, 10, sessionTimeArray4, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "4", graph_4);

		} // Close if.

		else ((ptSessionTimeV2(tft, graphX4, graphY, graphW, graphH, 0, 60, 10, sessionTimeCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "4", graph_4)));

		if (sessionTimeArray5 <= 60) {

			ptSessionTimeV2(tft, graphX5, graphY, graphW, graphH, 0, 60, 10, sessionTimeArray5, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "5", graph_5);

		} // Close if.

		else ((ptSessionTimeV2(tft, graphX5, graphY, graphW, graphH, 0, 60, 10, sessionTimeCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "5", graph_5)));

		if (sessionTimeArray6 <= 60) {

			ptSessionTimeV2(tft, graphX6, graphY, graphW, graphH, 0, 60, 10, sessionTimeArray6, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "6", graph_6);

		} // Close if.

		else ((ptSessionTimeV2(tft, graphX6, graphY, graphW, graphH, 0, 60, 10, sessionTimeCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "6", graph_6)));

		if (sessionTimeArray7 <= 60) {

			ptSessionTimeV3(tft, graphX7, graphY, graphW, graphH, 0, 60, 10, sessionTimeArray7, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "7", graph_7);

		} // Close if.

		else ((ptSessionTimeV3(tft, graphX7, graphY, graphW, graphH, 0, 60, 10, sessionTimeCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "7", graph_7)));

	} // Close if.

	if (screenMenu == 4) {

		if (screenRedraw == 1) {

			drawBlackBox();
			screenRedraw = 0;

		} // Close if.

		// Distance bar graphs.

		if (distanceTravelledArray1 <= 750) {

			ptSessionDistanceV1(tft, graphX1, graphY, graphW, graphH, 0, 1000, 200, distanceTravelledArray1, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "1", graph_8);

		} // Close if.

		else if (distanceTravelledArray1 >= 999) {

			ptSessionDistanceV1(tft, graphX1, graphY, graphW, graphH, 0, 1000, 200, distanceGraphCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "1", graph_8);

		} // Close else if.

		else ((ptSessionDistanceV1(tft, graphX1, graphY, graphW, graphH, 0, 1000, 200, distanceTravelledArray1, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "1", graph_8)));

		if (distanceTravelledArray2 <= 750) {

			ptSessionDistanceV2(tft, graphX2, graphY, graphW, graphH, 0, 1000, 200, distanceTravelledArray2, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "2", graph_9);

		} // Close if.

		else if (distanceTravelledArray2 >= 999) {

			ptSessionDistanceV2(tft, graphX2, graphY, graphW, graphH, 0, 1000, 200, distanceGraphCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "2", graph_9);

		} // Close else if.

		else ((ptSessionDistanceV2(tft, graphX2, graphY, graphW, graphH, 0, 1000, 200, distanceTravelledArray2, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "2", graph_9)));

		if (distanceTravelledArray3 <= 750) {

			ptSessionDistanceV2(tft, graphX3, graphY, graphW, graphH, 0, 1000, 200, distanceTravelledArray3, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "3", graph_10);

		} // Close if.

		else if (distanceTravelledArray3 >= 999) {

			ptSessionDistanceV2(tft, graphX3, graphY, graphW, graphH, 0, 1000, 200, distanceGraphCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "3", graph_10);

		} // Close else if.

		else ((ptSessionDistanceV2(tft, graphX3, graphY, graphW, graphH, 0, 1000, 200, distanceTravelledArray3, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "3", graph_10)));

		if (distanceTravelledArray4 <= 750) {

			ptSessionDistanceV2(tft, graphX4, graphY, graphW, graphH, 0, 1000, 200, distanceTravelledArray4, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "4", graph_11);

		} // Close if.

		else if (distanceTravelledArray4 >= 999) {

			ptSessionDistanceV2(tft, graphX4, graphY, graphW, graphH, 0, 1000, 200, distanceGraphCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "4", graph_11);

		} // Close else if.

		else ((ptSessionDistanceV2(tft, graphX4, 110, graphW, graphH, 0, 1000, 200, distanceTravelledArray4, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "4", graph_11)));

		if (distanceTravelledArray5 <= 750) {

			ptSessionDistanceV2(tft, graphX5, graphY, graphW, graphH, 0, 1000, 200, distanceTravelledArray5, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "5", graph_12);

		} // Close if.

		else if (distanceTravelledArray5 >= 999) {

			ptSessionDistanceV2(tft, graphX5, graphY, graphW, graphH, 0, 1000, 200, distanceGraphCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "5", graph_12);

		} // Close else if.

		else ((ptSessionDistanceV2(tft, graphX5, graphY, graphW, graphH, 0, 1000, 200, distanceTravelledArray5, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "5", graph_12)));

		if (distanceTravelledArray6 <= 750) {

			ptSessionDistanceV2(tft, graphX6, graphY, graphW, graphH, 0, 1000, 200, distanceTravelledArray6, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "6", graph_13);

		} // Close if.

		else if (distanceTravelledArray6 >= 999) {

			ptSessionDistanceV2(tft, graphX6, graphY, graphW, graphH, 0, 1000, 200, distanceGraphCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "6", graph_13);

		} // Close else if.

		else ((ptSessionDistanceV2(tft, graphX6, graphY, graphW, graphH, 0, 1000, 200, distanceTravelledArray6, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "6", graph_13)));

		if (distanceTravelledArray7 <= 750) {

			ptSessionDistanceV3(tft, graphX7, graphY, graphW, graphH, 0, 1000, 200, distanceTravelledArray7, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "7", graph_14);

		} // Close if.

		else if (distanceTravelledArray7 >= 999) {

			ptSessionDistanceV3(tft, graphX7, graphY, graphW, graphH, 0, 1000, 200, distanceGraphCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "7", graph_14);

		} // Close else if.

		else ((ptSessionDistanceV3(tft, graphX7, graphY, graphW, graphH, 0, 1000, 200, distanceTravelledArray7, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "7", graph_14)));

	} // Close if.

	if (screenMenu == 5) {

		if (screenRedraw == 1) {

			drawBlackBox();
			screenRedraw = 0;

		} // Close if.

		configurationDisplay();

	} // Close if.

} // Close loop.

/*-----------------------------------------------------------------*/

void mainData() {

	// Set speed variables to "0.00" if the wheel doesnt turn for X period (currently 4000ms).

	if (lastRotation2 < millis()) {

		rpm = 0.00;
		speedKph = 0.00;
		speedMph = 0.00;
		recordSessions = 1;
		eeSessionChange = true;

	} // Close if.

	else {

		// Calculate current session time.

		sessionTimeMillis = millis() - sessionStartTime;
		sessionTime = sessionTimeMillis / 10 / 60;

	} // Close else.


	// Ensure data is always "0" or greater, if not set to "0"

	if ((rpm >= 0) || (speedKph >= 0) || (speedMph >= 0)) {

	} // Close if.

	else {
		rpm = 0.00;
		speedKph = 0.00;
		speedMph = 0.00;

	} // Close else.

	if ((sessionTimeFlag == 1) && (recordSessions == 1)) {							// Calculate session duration.

		sessionTimeArray[sessionArrayPosition] = millis() - sessionStartTime;		// Calculate last session time in millis & store into array.
		distanceTravelledArray[sessionArrayPosition] = distanceTravelled - sessionStartDistance; // Store distance travelled into array.

		if (eeSessionChange == true) {

			switch (sessionArrayPosition) {

			case 0:

				sessionTimeArray1 = sessionTimeArray[0] / 1000 / 60;					// Divided by 1000 for millis to seconds, then divided by 60 for minutes.
				distanceTravelledArray1 = distanceTravelledArray[0];					// Update chart data.
				EEPROM.put(eeSessionTimeArray1Address, sessionTimeArray[0]);			// Record the chart data in EEPROM.
				EEPROM.put(eeSessionDistanceArray1Address, distanceTravelledArray[0]);	// Record the chart data in EEPROM.
				EEPROM.commit();
				eeSessionChange = false;

			case 1:

				sessionTimeArray2 = sessionTimeArray[1] / 1000 / 60;
				distanceTravelledArray2 = distanceTravelledArray[1];
				EEPROM.put(eeSessionTimeArray2Address, sessionTimeArray[1]);
				EEPROM.put(eeSessionDistanceArray2Address, distanceTravelledArray[1]);
				EEPROM.commit();
				eeSessionChange = false;

			case 2:

				sessionTimeArray3 = sessionTimeArray[2] / 1000 / 60;
				distanceTravelledArray3 = distanceTravelledArray[2];
				EEPROM.put(eeSessionTimeArray3Address, sessionTimeArray[2]);
				EEPROM.put(eeSessionDistanceArray3Address, distanceTravelledArray[2]);
				EEPROM.commit();
				eeSessionChange = false;

			case 3:

				sessionTimeArray4 = sessionTimeArray[3] / 1000 / 60;
				distanceTravelledArray4 = distanceTravelledArray[3];
				EEPROM.put(eeSessionTimeArray4Address, sessionTimeArray[3]);
				EEPROM.put(eeSessionDistanceArray4Address, distanceTravelledArray[3]);
				EEPROM.commit();
				eeSessionChange = false;

			case 4:

				sessionTimeArray5 = sessionTimeArray[4] / 1000 / 60;
				distanceTravelledArray5 = distanceTravelledArray[4];
				EEPROM.put(eeSessionTimeArray5Address, sessionTimeArray[4]);
				EEPROM.put(eeSessionDistanceArray5Address, distanceTravelledArray[4]);
				EEPROM.commit();
				eeSessionChange = false;

			case 5:

				sessionTimeArray6 = sessionTimeArray[5] / 1000 / 60;
				distanceTravelledArray6 = distanceTravelledArray[5];
				EEPROM.put(eeSessionTimeArray6Address, sessionTimeArray[5]);
				EEPROM.put(eeSessionDistanceArray6Address, distanceTravelledArray[5]);
				EEPROM.commit();
				eeSessionChange = false;

			case 6:

				sessionTimeArray7 = sessionTimeArray[6] / 1000 / 60;
				distanceTravelledArray7 = distanceTravelledArray[6];
				EEPROM.put(eeSessionTimeArray7Address, sessionTimeArray[6]);
				EEPROM.put(eeSessionDistanceArray7Address, distanceTravelledArray[6]);
				EEPROM.commit();
				eeSessionChange = false;

			} // Close switch case.

		} // Close if.

		sessionArrayPosition++;													// Increment sessionTimeArrayPosition.

		if (sessionArrayPosition >= 7) {										// Check array position is within parametres.

			sessionArrayPosition = 0;

		} // Close if.

		EEPROM.put(eeSessionArrayPositionAddress, sessionArrayPosition);		// Record the next array position in EEPROM.
		EEPROM.commit();

		sessionTimeFlag = 0;
		recordSessions = 0;

		if (eeTotalDistanceChange == true) {

			EEPROM.put(eeTotalDistanceAddress, distanceCounter);
			EEPROM.commit();
			eeTotalDistanceChange = false;

		} // Close if.

	} // Close if.

	// Calculate current session distance travelled.

	sessionDistance = distanceTravelled - sessionStartDistance;

	if (maxKphSpeed == 0) {				// Reset start up session distance displayed to zero, otherwise it displays current total distance.

		sessionDistance = 0;

	} // Close if.

	if (sessionTime == 0) {

		// Reset max speed.

		maxKphSpeed = 0;

	} // Close if.

	/*
	Configure speed varibales to the same format for screen layout using dtostrf.

	dtostrf(floatvar, StringLengthIncDecimalPoint, numVarsAfterDecimal, charbuf);

	where

		floatvar					float variable.
		StringLengthIncDecimalPoint	This is the length of the string that will be created.
		numVarsAfterDecimal			The number of digits after the deimal point to print.
		charbuf						The array to store the results*/

	dtostrf(rpm, 6, 2, rpmArray);
	dtostrf(speedKph, 6, 2, kphArray);
	dtostrf(speedMph, 6, 2, mphArray);
	dtostrf(maxKphSpeed, 6, 2, maxKphArray);
	dtostrf(averageKphSpeed, 6, 2, averageKphSpeedArray);
	dtostrf(sessionDistance, 6, 0, sessionDistanceArray);
	dtostrf(sessionTime, 6, 0, currentSessionTimeArray);

} // Close function.

/*-----------------------------------------------------------------*/

void averageSpeed() {

	// Subtract the last reading.

	total = total - readings[readIndex];

	// Get latest Kph.

	readings[readIndex] = speedKph;

	// Add the reading to the total.

	total = total + readings[readIndex];

	// Advance to the next position in the average array.

	readIndex = readIndex + 1;

	// If we're at the end of the array.

	if (readIndex >= numReadings) {

		readIndex = 0;

	} // Close if.

}  // Close function.

/*-----------------------------------------------------------------*/

void CurrentExerciseScreen() {

	tft.setFreeFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(WHITE);
	tft.setCursor(13, 26);
	tft.print("Current Session");

	tft.setTextSize(1);
	tft.setTextColor(WHITE);

	tft.setCursor(23, 70);
	tft.print("Xph: ");
	tft.setCursor(23, 90);
	tft.print("Ave: ");
	tft.setCursor(23, 110);
	tft.print("Max: ");
	tft.setCursor(23, 130);
	tft.print("Dis: ");
	tft.setCursor(23, 150);
	tft.print("Time: ");

	tft.setFreeFont();
	tft.setTextSize(2);
	tft.setTextColor(WHITE, BLACK);

	tft.setCursor(100, 64);
	tft.println(kphArray);

	tft.setCursor(100, 84);
	tft.println(averageKphSpeedArray);

	tft.setCursor(100, 104);
	tft.println(maxKphArray);

	tft.setCursor(100, 124);
	tft.println(sessionDistanceArray);

	tft.setCursor(100, 144);
	tft.println(currentSessionTimeArray);

} // Close function.

/*-----------------------------------------------------------------*/

void configurationDisplay() {

	// Drawing thicker rectangle with for loop, function, X, Y, W, H, colour.

	EEPROM.get(eeMenuAddress, eeMenuSetting);
	EEPROM.get(eeCircAddress, eeCircSetting);
	EEPROM.get(eeResetSettingAddress, eeResetSetting);

	tft.setFreeFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(WHITE);
	tft.setCursor(13, 26);
	tft.println("System Setup");
	tft.setFreeFont();

	//tft.setFreeFont(&FreeSans9pt7b);
	//tft.setTextSize(1);
	//tft.setTextColor(WHITE);
	//tft.setCursor(27, 70);
	//tft.println("Battery Level");

	//tft.setFreeFont();
	//tft.setTextSize(1);
	//tft.setTextColor(WHITE, BLACK);
	//tft.setCursor(35, 90);
	//tft.print("V: ");
	//tft.println(sensorValueVolts);
	//tft.setCursor(35, 110);
	//tft.print("%: ");
	//tft.println(int(sensorValuePerc));
	//tft.println();

	//if (sensorValuePerc > 95) {

	//	drawBitmap(tft, 27, 90, ccBatt100, 32, 18);

	//}

	//else if (sensorValuePerc > 79 && sensorValuePerc < 95) {

	//	drawBitmap(tft, 27, 90, ccBatt80, 32, 18);

	//}

	//else if (sensorValuePerc > 59 && sensorValuePerc < 80) {

	//	drawBitmap(tft, 27, 90, ccBatt60, 32, 18);

	//}

	//else if (sensorValuePerc > 39 && sensorValuePerc < 60) {

	//	drawBitmap(tft, 27, 90, ccBatt40, 32, 18);

	//}

	//else if (sensorValuePerc > 19 && sensorValuePerc < 40) {

	//	drawBitmap(tft, 27, 90, ccBatt20, 32, 18);

	//}

	//else if (sensorValuePerc < 20) {

	//	drawBitmap(tft, 27, 90, ccBatt00, 32, 18);

	//}

	tft.setTextColor(WHITE, BLACK);

	if (configurationFlag == 3) {

		tft.setTextColor(TFT_RED, BLACK);

	} // Close if.

	else tft.setTextColor(WHITE, BLACK);

	tft.setCursor(23, 70);
	tft.print("System Reset     : ");
	tft.println(eeResetSetting);

	if (configurationFlag == 2) {

		tft.setTextColor(TFT_RED, BLACK);

	} // Close if.

	else tft.setTextColor(WHITE, BLACK);

	tft.setCursor(23, 90);
	tft.print("Circumference    : ");
	tft.println(eeCircSetting);
	tft.println();
	tft.setCursor(23, 110);

	if (configurationFlag == 1) {

		tft.setTextColor(TFT_RED, BLACK);

	} // Close if.

	else tft.setTextColor(WHITE, BLACK);

	tft.print("Screen Menu Strt : ");
	tft.println(eeMenuSetting);
	tft.setCursor(23, 130);

	tft.setTextColor(WHITE, BLACK);
	tft.print("Total Distance To Date");
	tft.setCursor(23, 150);
	tft.println(distanceTravelled = distanceCounter * circumference);

	if (digitalRead(bPlus) == LOW && (digitalRead(bMinus) == LOW)) {

		if (configurationFlag == 4) {

			configurationFlag = 0;

		}

		configurationFlag++;

	} // Close if.

	if (digitalRead(bPlus) == LOW && configurationFlag == 1)
	{
		if (eeMenuSetting == 4)
		{
			eeMenuSetting = 0;
			eeMenuSettingChange = true;

		}
		else
		{
			eeMenuSetting++;
			eeMenuSettingChange = true;
		}

	} // Close if.

	if (digitalRead(bMinus) == LOW && configurationFlag == 1)
	{
		if (eeMenuSetting == 0)
		{
			eeMenuSetting = 4;
			eeMenuSettingChange = true;
		}
		else
		{
			eeMenuSetting--;
			eeMenuSettingChange = true;
		}

	} // Close if.

		// Write menu setting to EEPROM.

	if (eeMenuSettingChange == true) {

		EEPROM.put(eeMenuAddress, eeMenuSetting);
		EEPROM.commit();
		eeMenuSettingChange = false;

	} // Close if.

	if (digitalRead(bPlus) == LOW && configurationFlag == 2)
	{
		if (eeCircSetting > 2.00)
		{
			eeCircSetting = 0.00;
			eeCircSettingChange = true;

		}
		else
		{
			eeCircSetting = eeCircSetting + 0.01;
			eeCircSettingChange = true;
		}

	} // Close if.

	if (digitalRead(bMinus) == LOW && configurationFlag == 2)
	{
		if (eeCircSetting < 0.00)
		{
			eeCircSetting = 2.00;
			eeCircSettingChange = true;
		}
		else
		{
			eeCircSetting = eeCircSetting - 0.01;
			eeCircSettingChange = true;
		}

	} // Close if.

	// Write debug setting to EEPROM.

	if (eeCircSettingChange == true) {

		EEPROM.put(eeCircAddress, eeCircSetting);
		EEPROM.commit();
		eeCircSettingChange = false;

	} // Close if.

	if (digitalRead(bPlus) == LOW && configurationFlag == 3)
	{
		if (eeResetSetting == 2)
		{
			eeResetSetting = 0;
			eeResetSettingChange = true;

		}
		else
		{
			eeResetSetting++;
			eeResetSettingChange = true;
		}

	} // Close if.

	if (digitalRead(bMinus) == LOW && configurationFlag == 3)
	{
		if (eeResetSetting == 0)
		{
			eeResetSetting = 2;
			eeResetSettingChange = true;
		}
		else
		{
			eeResetSetting--;
			eeResetSettingChange = true;
		}

	} // Close if.

	// Write menu setting to EEPROM.

	if (eeResetSettingChange == true) {

		EEPROM.put(eeResetSettingAddress, eeResetSetting);
		EEPROM.commit();
		eeResetSettingChange = false;

	} // Close if.

} // Close function.

/*-----------------------------------------------------------------*/

void drawBorder()
{
	// Draw layout borders.

	tft.drawRect(FRAME1_X, FRAME1_Y, FRAME1_W, FRAME1_H, TFT_WHITE);
	tft.drawRect(FRAME2_X, FRAME2_Y, FRAME2_W, FRAME2_H, TFT_WHITE);

} // Close function.

/*-----------------------------------------------------------------*/

void drawBlackBox()
{
	// Clear screen by using a black box.

	tft.fillRect(FRAME2_X + 1, FRAME2_Y + 22, FRAME2_W - 2, FRAME2_H - 23, TFT_BLACK);		// This covers only the graphs and charts, not the system icons to save refresh flicker.
	tft.fillRect(FRAME2_X + 1, FRAME2_Y + 1, FRAME2_W - 90, FRAME2_H - 200, TFT_BLACK);		// Ths covers the title text per page

} // Close function.

/*-----------------------------------------------------------------*/

void menu_Change() {

	menuChange = 0;

} // Close function.

/*-----------------------------------------------------------------*/

void startUp() {

	// Draw buttons at startup.

	//tft.fillRect(BUTTON1_X, BUTTON1_Y, BUTTON1_W, BUTTON1_H, TFT_RED);
	//tft.drawRect(BUTTON1_X, BUTTON1_Y, BUTTON1_W, BUTTON1_H, TFT_WHITE);

	//tft.fillRect(BUTTON2_X, BUTTON2_Y, BUTTON2_W, BUTTON2_H, LTBLUE);
	//tft.drawRect(BUTTON2_X, BUTTON2_Y, BUTTON2_W, BUTTON2_H, TFT_WHITE);

	//tft.fillRect(BUTTON3_X, BUTTON3_Y, BUTTON3_W, BUTTON3_H, LTBLUE);
	//tft.drawRect(BUTTON3_X, BUTTON3_Y, BUTTON3_W, BUTTON3_H, TFT_WHITE);

	//tft.fillRect(BUTTON4_X, BUTTON4_Y, BUTTON4_W, BUTTON4_H, LTBLUE);
	//tft.drawRect(BUTTON4_X, BUTTON4_Y, BUTTON4_W, BUTTON4_H, TFT_WHITE);

} // Close function.

/*-----------------------------------------------------------------*/

void rotationInterruptISR() {

	static unsigned long  last_interrupt_time = 0;                  // Function to solve debounce
	unsigned long         interrupt_time = millis();

	if (interrupt_time - last_interrupt_time > 100) {

		detachInterrupt(interruptWheelSensor);

		passedTime = millis() - startTime;
		startTime = millis();

		rpm = (60000 * circumference) / passedTime;			// Revs per minute.
		speedKph = (3600 * circumference) / passedTime;		// km/h.
		speedMph = (3600 * circImperial) / passedTime;		// Miles per hour.

		distanceCounter++;
		eeTotalDistanceChange = true;
		distanceTravelled = distanceCounter * circumference;

		if (sessionTimeFlag == 0) {		// Set session timer to start.

			sessionTimeFlag = 1;
			sessionStartTime = millis();
			sessionStartDistance = distanceTravelled;
			recordSessions = 0;

		} // Close if.

		lastRotation1 = millis();
		lastRotation2 = lastRotation1 + 4000;

		attachInterrupt(digitalPinToInterrupt(interruptWheelSensor), rotationInterruptISR, FALLING);

	} // Close if.

	last_interrupt_time = interrupt_time;

}  // Close function.

/*-----------------------------------------------------------------*/

void demoSpeedData() {

	if (millis() >= speedMeasureNow + speedMeasureInterval) {

		if (speedKph == 20) {

			speedDirection = 0;
		}

		if (speedDirection == 0) {

			speedKph--;
		}

		if (speedKph == -1) {

			speedDirection = 1;
		}

		if (speedDirection == 1) {

			speedKph++;
		}

		speedMeasureNow = millis();

	} // Close if.

} // Close function.

/*-----------------------------------------------------------------*/

void resetSystemData() {

	eeMenuSetting = 0;
	EEPROM.put(eeMenuAddress, eeMenuSetting);
	EEPROM.commit();

	eeCircSetting = 1.00;
	EEPROM.put(eeCircAddress, eeCircSetting);
	EEPROM.commit();

	eeTotalDistance = 0.00;
	EEPROM.put(eeTotalDistanceAddress, eeTotalDistance);
	EEPROM.commit();

	eeSessionArrayPosition = 0;
	EEPROM.put(eeSessionArrayPositionAddress, eeSessionArrayPosition);

	EEPROM.put(eeSessionTimeArray1Address, 0);									// Populate arrays with zero data.
	EEPROM.put(eeSessionTimeArray2Address, 0);
	EEPROM.put(eeSessionTimeArray3Address, 0);
	EEPROM.put(eeSessionTimeArray4Address, 0);
	EEPROM.put(eeSessionTimeArray5Address, 0);
	EEPROM.put(eeSessionTimeArray6Address, 0);
	EEPROM.put(eeSessionTimeArray7Address, 0);
	EEPROM.commit();

	EEPROM.put(eeSessionDistanceArray1Address, 0);
	EEPROM.put(eeSessionDistanceArray2Address, 0);
	EEPROM.put(eeSessionDistanceArray3Address, 0);
	EEPROM.put(eeSessionDistanceArray4Address, 0);
	EEPROM.put(eeSessionDistanceArray5Address, 0);
	EEPROM.put(eeSessionDistanceArray6Address, 0);
	EEPROM.put(eeSessionDistanceArray7Address, 0);
	EEPROM.commit();

	eeResetSetting = 0;
	EEPROM.put(eeResetSettingAddress, 0);
	EEPROM.commit();

	EEPROM.get(eeMenuAddress, eeMenuSetting);
	EEPROM.get(eeCircAddress, eeCircSetting);
	EEPROM.get(eeMenuAddress, screenMenu);
	EEPROM.get(eeCircAddress, circumference);
	EEPROM.get(eeTotalDistanceAddress, distanceCounter);

	EEPROM.commit();

	EEPROM.get(eeSessionTimeArray1Address, sessionTimeArray[0]);
	EEPROM.get(eeSessionTimeArray2Address, sessionTimeArray[1]);
	EEPROM.get(eeSessionTimeArray3Address, sessionTimeArray[2]);
	EEPROM.get(eeSessionTimeArray4Address, sessionTimeArray[3]);
	EEPROM.get(eeSessionTimeArray5Address, sessionTimeArray[4]);
	EEPROM.get(eeSessionTimeArray6Address, sessionTimeArray[5]);
	EEPROM.get(eeSessionTimeArray7Address, sessionTimeArray[6]);

	EEPROM.commit();

	sessionTimeArray1 = sessionTimeArray[0] / 1000 / 60;							// Times to be updated, needs to be divided by 1000.
	sessionTimeArray2 = sessionTimeArray[1] / 1000 / 60;
	sessionTimeArray3 = sessionTimeArray[2] / 1000 / 60;
	sessionTimeArray4 = sessionTimeArray[3] / 1000 / 60;
	sessionTimeArray5 = sessionTimeArray[4] / 1000 / 60;
	sessionTimeArray6 = sessionTimeArray[5] / 1000 / 60;
	sessionTimeArray7 = sessionTimeArray[6] / 1000 / 60;

	EEPROM.get(eeSessionDistanceArray1Address, distanceTravelledArray[0]);
	EEPROM.get(eeSessionDistanceArray2Address, distanceTravelledArray[1]);
	EEPROM.get(eeSessionDistanceArray3Address, distanceTravelledArray[2]);
	EEPROM.get(eeSessionDistanceArray4Address, distanceTravelledArray[3]);
	EEPROM.get(eeSessionDistanceArray5Address, distanceTravelledArray[4]);
	EEPROM.get(eeSessionDistanceArray6Address, distanceTravelledArray[5]);
	EEPROM.get(eeSessionDistanceArray7Address, distanceTravelledArray[6]);

	EEPROM.commit();

	distanceTravelledArray1 = distanceTravelledArray[0];
	distanceTravelledArray2 = distanceTravelledArray[1];
	distanceTravelledArray3 = distanceTravelledArray[2];
	distanceTravelledArray4 = distanceTravelledArray[3];
	distanceTravelledArray5 = distanceTravelledArray[4];
	distanceTravelledArray6 = distanceTravelledArray[5];
	distanceTravelledArray7 = distanceTravelledArray[6];

} // Close function.

/*-----------------------------------------------------------------*/

void resetSystemDemoData() {

	eeMenuSetting = 0;
	EEPROM.put(eeMenuAddress, eeMenuSetting);
	EEPROM.commit();

	eeCircSetting = 2.00;
	EEPROM.put(eeCircAddress, eeCircSetting);
	EEPROM.commit();

	eeTotalDistance = 5000.00;
	EEPROM.put(eeTotalDistanceAddress, eeTotalDistance);
	EEPROM.commit();

	eeSessionArrayPosition = 0;
	EEPROM.put(eeSessionArrayPositionAddress, eeSessionArrayPosition);

	EEPROM.put(eeSessionTimeArray1Address, 0);									// Populate arrays with demo data.
	EEPROM.put(eeSessionTimeArray2Address, 100000);
	EEPROM.put(eeSessionTimeArray3Address, 200000);
	EEPROM.put(eeSessionTimeArray4Address, 300000);
	EEPROM.put(eeSessionTimeArray5Address, 400000);
	EEPROM.put(eeSessionTimeArray6Address, 500000);
	EEPROM.put(eeSessionTimeArray7Address, 600000);
	EEPROM.commit();

	EEPROM.put(eeSessionDistanceArray1Address, 0);
	EEPROM.put(eeSessionDistanceArray2Address, 200);
	EEPROM.put(eeSessionDistanceArray3Address, 400);
	EEPROM.put(eeSessionDistanceArray4Address, 600);
	EEPROM.put(eeSessionDistanceArray5Address, 800);
	EEPROM.put(eeSessionDistanceArray6Address, 990);
	EEPROM.put(eeSessionDistanceArray7Address, 1200);
	EEPROM.commit();

	eeResetSetting = 0;
	EEPROM.put(eeResetSettingAddress, 0);
	EEPROM.commit();

	EEPROM.get(eeMenuAddress, eeMenuSetting);
	EEPROM.get(eeCircAddress, eeCircSetting);
	EEPROM.get(eeMenuAddress, screenMenu);
	EEPROM.get(eeCircAddress, circumference);
	EEPROM.get(eeTotalDistanceAddress, distanceCounter);

	EEPROM.commit();

	EEPROM.get(eeSessionTimeArray1Address, sessionTimeArray[0]);
	EEPROM.get(eeSessionTimeArray2Address, sessionTimeArray[1]);
	EEPROM.get(eeSessionTimeArray3Address, sessionTimeArray[2]);
	EEPROM.get(eeSessionTimeArray4Address, sessionTimeArray[3]);
	EEPROM.get(eeSessionTimeArray5Address, sessionTimeArray[4]);
	EEPROM.get(eeSessionTimeArray6Address, sessionTimeArray[5]);
	EEPROM.get(eeSessionTimeArray7Address, sessionTimeArray[6]);

	EEPROM.commit();

	sessionTimeArray1 = sessionTimeArray[0] / 100 / 60;							// Times to be updated, needs to be divided by 1000.
	sessionTimeArray2 = sessionTimeArray[1] / 100 / 60;
	sessionTimeArray3 = sessionTimeArray[2] / 100 / 60;
	sessionTimeArray4 = sessionTimeArray[3] / 100 / 60;
	sessionTimeArray5 = sessionTimeArray[4] / 100 / 60;
	sessionTimeArray6 = sessionTimeArray[5] / 100 / 60;
	sessionTimeArray7 = sessionTimeArray[6] / 100 / 60;

	EEPROM.get(eeSessionDistanceArray1Address, distanceTravelledArray[0]);
	EEPROM.get(eeSessionDistanceArray2Address, distanceTravelledArray[1]);
	EEPROM.get(eeSessionDistanceArray3Address, distanceTravelledArray[2]);
	EEPROM.get(eeSessionDistanceArray4Address, distanceTravelledArray[3]);
	EEPROM.get(eeSessionDistanceArray5Address, distanceTravelledArray[4]);
	EEPROM.get(eeSessionDistanceArray6Address, distanceTravelledArray[5]);
	EEPROM.get(eeSessionDistanceArray7Address, distanceTravelledArray[6]);

	EEPROM.commit();

	distanceTravelledArray1 = distanceTravelledArray[0];
	distanceTravelledArray2 = distanceTravelledArray[1];
	distanceTravelledArray3 = distanceTravelledArray[2];
	distanceTravelledArray4 = distanceTravelledArray[3];
	distanceTravelledArray5 = distanceTravelledArray[4];
	distanceTravelledArray6 = distanceTravelledArray[5];
	distanceTravelledArray7 = distanceTravelledArray[6];

} // Close function.

/*-----------------------------------------------------------------*/