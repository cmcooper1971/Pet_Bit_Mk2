/*
 Name:		Pet_Bit_Mk2.ino
 Created:	6/25/2021 7:19:18 PM
 Author:	Chris
*/

// Libraries.

#include <WiFi.h>					// Arduino WiFi library.
#include <ESP32Time.h>				// F Biego ESp32 time library.
#include <AsyncTCP.h>				// Random Nerd library.
#include <ESPAsyncWebServer.h>		// Random Nerd library.
#include <SPIFFS.h>					// File store.
#include <Arduino_JSON.h>			// Arduino JSon Library.

#include <vfs_api.h>                // File system library.
#include <FSImpl.h>                 // File system library.
#include <FS.h>                     // File system library.
#include <SPI.h>					// SPI bus for TFT using software SPI.
#include <TFT_eSPI.h>               // TFT SPI library.
#include <EEPROM.h>					// EEPROM library.

#include "Free_Fonts.h"				// Free fonts for use with eTFT.
#include "colours.h"                // Colours.
#include "screenLayout.h"			// Screen layout.
#include "drawBitmap.h"				// Draw battery level bitmaps.
#include "xphDial.h"				// XPH Odometre.
#include "graphDistance.h"			// Daily distance chart.
#include "graphTime.h"				// Daily time chart.
#include "format_function.h"		// Special formatting function from Kris Kasprzak.
#include "buttonIcons.h"			// Button icons.
#include "startScreen.h"			// Start screen bitmap.
#include "startUpScreen.h"			// Start up screen.
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

unsigned long sleepT = 0;
unsigned long sleepTime = 60000;	// Reset to 300,000 when finished with design (5 minutes sleep time).

// Pin out Configuration.

const int sensorInt = 39;			// Reed switch sensor.
boolean sensorT = false;			// To update display when sensor passes.

// Configure ILI9341 display.

TFT_eSPI tft = TFT_eSPI();			// Invoke custom library.
boolean screenR = false;			// To limit screen flicker due to unneccesary screen draws.

// Configure sound.

const byte buzzerP = 21;			// Buzzer pin.
boolean buzzerYN;					// Buzzer enabled / disabled.
int buzzerF;						// Set frequency of the buzzer beep.
int buzzerD;						// Buzzer delay.

// Configure time settings.

ESP32Time rtc;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;
long		LocalTime;
int			localTimeInterval = 60000;

// Web Server configuration.

AsyncWebServer server(80);			// Create AsyncWebServer object on port 80
AsyncEventSource events("/events");	// Create an Event Source on /events

// WiFi Configuration.

boolean wiFiYN;						// WiFi reset.
boolean apMode = false;

// Search for parameter in HTTP POST request.

const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "subnet";
const char* PARAM_INPUT_5 = "gateway";
const char* PARAM_INPUT_6 = "dns";

//Variables to save values from HTML form.

String ssid;
String pass;
String ip;
String subnet;
String gateway;
String dns;

// File paths to save input values permanently.

const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* subnetPath = "/subnet.txt";
const char* gatewayPath = "/gateway.txt";
const char* dnsPath = "/dns.txt";

// Network variables.

IPAddress localIP;
IPAddress Gateway;
IPAddress Subnet;
IPAddress dns1;

// Json Variable to Hold Sensor Readings

JSONVar readings;

// Timer variables (check wifi).

unsigned long wiFiR = 0;					// WiFi retry (wiFiR) to attempt connecting to the last known WiFi if connection lost.
unsigned long wiFiRI = 60000;				// WiFi retry Interval (wiFiRI) used to retry connecting to the last known WiFi if connection lost.
volatile bool disWiFi = false;				// Used in the sensor interrupt function to disable WiFi.
volatile bool disWiFiF = false;				// Used in the sensor interrupt function to disable WiFi.
unsigned long previousMillis = 0;			// Used in the WiFI Init function.
const long interval = 10000;				// Interval to wait for Wi-Fi connection (milliseconds).

// Timer variables (to update web server).

unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

// Data variables.

byte configurationFlag = 1;					// Configuration menu flag.

volatile unsigned int distanceC = 0;		// Counting rotations for distance travelled.

volatile unsigned long passedTime;			// Setting time to calculate speed.
volatile unsigned long startTime;			// Setting time to calculate speed.

volatile unsigned long lastRotation1;		// Checking if wheel is still turning.
volatile unsigned long lastRotation2;		// Checking if wheel is still turning.

double circumference;						// Wheel circumference.
double circImperial;						// Conversion into MPH.

double distanceTravelled = 0.00;			// Total distance travelled.
double speedKph = 0.00;
double speedMph = 0.00;

float maxKphSpeed = 0;						// Recording max speed.

char kphArray[7];							// Holding data in character arrays for formatting reasons.
char mphArray[7];
char maxKphArray[7];
char maxMphArray[7];
char averageKphSpeedArray[7];
char averageMphSpeedArray[7];
char sessionDistanceArray[7];
char currentSessionTimeArray[7];

// Configure EEEPROM. 

int eeMenuAddress = 0;						// EEPROM address for start menu position.
int eeMenuSetting;							// Actual commit for writing, 4 bytes.
boolean eeMenuSettingChange = false;		// Change flag.

int eeCircAddress = 4;						// EEPROM address for circumference.
float eeCircSetting;						// Actual commit for writing, 4 bytes.
boolean eeCircSettingChange = false;		// Change flag.

int eeTotalDistanceAddress = 8;				// EEPROM address for total distance.
unsigned long eeTotalDistance;				// Actual commit for writing, 4 bytes.
boolean eeTotalDistanceChange = false;		// Change flag.

int eeSessionTimeArray0Address = 16;		// EEPROM address for session time 1
int eeSessionTimeArray1Address = 20;		// EEPROM address for session time 2
int eeSessionTimeArray2Address = 24;		// EEPROM address for session time 3
int eeSessionTimeArray3Address = 28;		// EEPROM address for session time 4
int eeSessionTimeArray4Address = 32;		// EEPROM address for session time 5
int eeSessionTimeArray5Address = 36;		// EEPROM address for session time 6
int eeSessionTimeArray6Address = 40;		// EEPROM address for session time 7

int eeSessionDistanceArray0Address = 44;	// EEPROM address for session distance 1
int eeSessionDistanceArray1Address = 48;	// EEPROM address for session distance 2
int eeSessionDistanceArray2Address = 52;	// EEPROM address for session distance 3
int eeSessionDistanceArray3Address = 56;	// EEPROM address for session distance 4
int eeSessionDistanceArray4Address = 60;	// EEPROM address for session distance 5
int eeSessionDistanceArray5Address = 64;	// EEPROM address for session distance 6
int eeSessionDistanceArray6Address = 68;	// EEPROM address for session distance 7

boolean eeSessionChange = false;			// Change flag.

int eeResetSettingAddress = 72;				// EEPROM address for master reset
unsigned int eeResetSetting;				// Actual commit for writing, 1 byte.
boolean eeResetSettingChange = false;		// Change flag.

int eegraphDMAddress = 80;					// EEPROM address for graph distance scale.
int eegraphDMIAddress = 88;					// EEPROM address for graph distance increment scale level.
int eegraphDAPAddress = 96;					// EEPROM address for graph distance array position.

int eegraphTMAddress = 104;					// EEPROM address for graph distance scale.
int eegraphTMIAddress = 112;				// EEPROM address for graph distance increment scale level.
int eegraphTAPAddress = 120;				// EEPROM address for graph distance array position.

boolean newMaxSpeedF = 0;					// New max speed flag.
double maxSessonDistance = 0;				// Max session distance.
unsigned long maxSessionTime = 0;			// Max session time.
boolean newBestSessionDistanceF = false;	// New best session distance flag.
boolean newBestSessionTimeF = false;		// New best session time flag.
boolean newBestDayF = false;				// New best day flag.

int eeBestMaxSpeed = 124;					// EEPRPOM address for best ever max speed recording.
int eeBestMaxSpeedMinute = 128;
int eeBestMaxSpeedHour = 132;
int eeBestMaxSpeedDoW = 136;
int eeBestMaxSpeedDay = 140;
int eeBestMaxSpeedMonth = 144;
int eeBestMaxSpeedYear = 148;

int eeBestDistanceS = 152;					// EEPRPOM address for best ever distance per session recording.
int eeBestDistanceSMinute = 156;
int eeBestDistanceSHour = 160;
int eeBestDistanceSDoW = 164;
int eeBestDistanceSDay = 168;
int eeBestDistanceSMonth = 172;
int eeBestDistanceSYear = 176;

int eeBestDistanceD = 180;					// EEPRPOM address for best ever distance per day recording.
int eeBestDistanceDMinute = 184;
int eeBestDistanceDHour = 188;
int eeBestDistanceDDoW = 192;
int eeBestDistanceDDay = 196;
int eeBestDistanceDMonth = 200;
int eeBestDistanceDYear = 204;

int eeBestTimeS = 208;						// EEPRPOM address for best ever time per session recording.
int eeBestTimeSMinute = 212;
int eeBestTimeSHour = 216;
int eeBestTimeSDoW = 220;
int eeBestTimeSDay = 224;
int eeBestTimeSMonth = 228;
int eeBestTimeSYear = 232;

int eeBestTimeD = 236;						// EEPRPOM address for best ever time per day recording.
int eeBestTimeDMinute = 240;
int eeBestTimeDHour = 244;
int eeBestTimeDDoW = 248;
int eeBestTimeDDay = 252;
int eeBestTimeDMonth = 256;
int eeBestTimeDYear = 260;

int eeBuzzerYNAddress = 300;				// EEPROM address for buzzer enabled / disabled.
boolean eeBuzzerYNChange;					// Change flag.

int eeCalYNAddress = 304;					// EEPROM address for touch screen calibration enabled disabled.
int eeCalDataAddress0 = 308;				// EEPROM address for touch screen calibration data.
int eeCalDataAddress1 = 312;				// EEPROM address for touch screen calibration data.
int eeCalDataAddress2 = 316;				// EEPROM address for touch screen calibration data.
int eeCalDataAddress3 = 320;				// EEPROM address for touch screen calibration data.
int eeCalDataAddress4 = 324;				// EEPROM address for touch screen calibration data.

int eeWiFiYNAddress = 328;					// EEPROM address for WiFi reset.
boolean eeWiFiYNChange;						// Change flag.

// Misc array and character spaces are to over write previous screen draw

char* menuArray[7] = { "","Current Session","Odemeter       ","Daily Times    ","Daily Distance ","Configuration  " };	// Default menu options.
char* resetArray[4] = { "None         ", "Full Reset   ", "Demo Data    ", "Records Reset" };							// Reset options.
char* dayArray[7] = { "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday" };							// Days of the week.
char* dayShortArray[7] = { "Su","Mo","Tu","We","Th","Fr","Sa" };														// Short days of the week.
char* ynArray[2] = { "No", "Yes" };																						// Yes / No options.

uint16_t calData[5];						// Touch screen calibration data.
boolean calTouchScreen = false;				// Change flag to trigger calibration function.

// Average speed calculation variables.

const int numReadings = 10;

double	readingsAV[numReadings];			// Latest Kph readings.
int		readIndex = 0;						// The index of the current reading.
double	total = 0.00;						// The running total of the readings.
double	averageKphSpeed = 0.00;				// The average speed in Kph.

// Session time variables.

unsigned long sessionTimeCap;				// Set cap for graph if statements.
unsigned int distanceGraphCap;				// Set cap for graph if statements.

boolean recordSessions = false;				// Flag to trigger the recording of each session.
volatile boolean sessionTimeFlag = false;	// Flag to trigger the recording of each session.
volatile unsigned long sessionStartTime;	// Time each pt session starts.
unsigned long sessionTimeArray[7];			// Array for storing 7 sessions.
byte sessionArrayPosition = 5;				// Array position, this is also used for the distance array position.
volatile unsigned long sessionTimeMillis;	// Time each pt session in millis.
volatile unsigned long sessionTime;			// Time each pt session in minutes.

double sessionStartDistance = 0.00;
double sessionDistance;						// Session distance.

unsigned int sessionTimeArray0;				// These variables are needed for the Kris Kasprzak charts.
unsigned int sessionTimeArray1;
unsigned int sessionTimeArray2;
unsigned int sessionTimeArray3;
unsigned int sessionTimeArray4;
unsigned int sessionTimeArray5;
unsigned int sessionTimeArray6;

unsigned int distanceTravelledArray[7];		// Array for storing 7 sessions.

unsigned int distanceTravelledArray0;		// These variables are needed for the Kris Kasprzak charts.
unsigned int distanceTravelledArray1;
unsigned int distanceTravelledArray2;
unsigned int distanceTravelledArray3;
unsigned int distanceTravelledArray4;
unsigned int distanceTravelledArray5;
unsigned int distanceTravelledArray6;

// Menu positions and refresh.

byte screenMenu = 4;				// Screen menu selection.
boolean menuChange = true;

// Dial and chart function parametres.

boolean dial_1 = true;				// Odometer dial.

boolean graph_1 = true;				// Bar graph.
boolean graph_2 = true;
boolean graph_3 = true;
boolean graph_4 = true;
boolean graph_5 = true;
boolean graph_6 = true;
boolean graph_7 = true;

int graphTM;						// Graph time measurement scale.
int graphTMI;						// Graph time measurement scale increment setting.
boolean graphTSC = false;			// Flag to trigger the recording of settings to EEPROM. 
int graphTAP;						// Graph time array position.

const int graphTAM[12] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120 };		// Graph array time scale options.
const int graphTAI[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };					// Graph array time scale increment options.

boolean graph_8 = true;				// Bar graph.
boolean graph_9 = true;
boolean graph_10 = true;
boolean graph_11 = true;
boolean graph_12 = true;
boolean graph_13 = true;
boolean graph_14 = true;

int graphDM;						// Graph distance measurement scale.
int graphDMI;						// Graph distance measurement scale increment setting.
boolean graphDSC = false;			// Flag to trigger the recording of settings to EEPROM. 
int graphDAP;						// Graph distance array position.

const int graphDAM[4] = { 500, 1000, 1500, 2000 };		// Graph array distance scale options.
const int graphDAI[4] = { 100, 200, 300, 400 };			// Graph array distance scale increment options.

/*-----------------------------------------------------------------*/

// Interrupt for detecting sensor activation.

void IRAM_ATTR rotationInterruptISR() {

	static unsigned long  last_interrupt_time = 0;                  // Function to solve debounce.
	unsigned long         interrupt_time = millis();

	if (interrupt_time - last_interrupt_time > 50) {

		detachInterrupt(sensorInt);									// Detach interrupt.

		if (sensorT == false) {										// Simple indicator flag for TFT.

			sensorT = true;
		}

		else if (sensorT == true) {

			sensorT = false;
		}

		disWiFiF = true;											// Disable WiFi flag to stop repeat attempts.
		disWiFi = true;												// Disable WiFi on wheel turn.

		passedTime = millis() - startTime;
		startTime = millis();

		sleepT = millis();											// Restart auto sleep timer.

		speedKph = (3600 * circumference) / passedTime;				// Km per hour.
		speedMph = (3600 * circImperial) / passedTime;				// Miles per hour.

		distanceC++;												// Count rotations for distance calculations.
		eeTotalDistanceChange = true;								// Rotation flag.
		distanceTravelled = distanceC * circumference;				// Distance calculation.

		if (sessionTimeFlag == false) {								// Set session timer to start.

			sessionTimeFlag = true;
			sessionStartTime = millis();
			sessionStartDistance = distanceTravelled;
			recordSessions = false;
		}

		lastRotation1 = millis();
		lastRotation2 = lastRotation1 + 4000;

		attachInterrupt(digitalPinToInterrupt(sensorInt), rotationInterruptISR, FALLING);	// Attach interrupt.

	}

	last_interrupt_time = interrupt_time;

}  // Close function.

/*-----------------------------------------------------------------*/

// Initialize WiFi

bool initWiFi() {

	wiFiTitle();

	// If ESP32 inits successfully in station mode, recolour WiFi to red.

	drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiRed, WIFI_ICON_W, WIFI_ICON_H);

	// Check if settings are available to connect to WiFi.

	if (ssid == "" || ip == "") {
		Serial.println("Undefined SSID or IP address.");
		return false;
	}

	WiFi.mode(WIFI_STA);
	localIP.fromString(ip.c_str());
	Gateway.fromString(gateway.c_str());
	Subnet.fromString(subnet.c_str());
	dns1.fromString(dns.c_str());

	if (!WiFi.config(localIP, Gateway, Subnet, dns1)) {
		Serial.println("STA Failed to configure");
		return false;
	}

	WiFi.begin(ssid.c_str(), pass.c_str());
	Serial.println("Connecting to WiFi...");

	// If ESP32 inits successfully in station mode, recolour WiFi to amber.

	drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiAmber, WIFI_ICON_W, WIFI_ICON_H);

	unsigned long currentMillis = millis();
	previousMillis = currentMillis;

	while (WiFi.status() != WL_CONNECTED) {
		currentMillis = millis();
		if (currentMillis - previousMillis >= interval) {
			Serial.println("Failed to connect.");
			// If ESP32 fails to connect, recolour WiFi to red.
			drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiRed, WIFI_ICON_W, WIFI_ICON_H);
			drawBlackBox();
			return false;
		}

	}

	Serial.println(WiFi.localIP());
	Serial.println("");
	Serial.print("RRSI: ");
	Serial.println(WiFi.RSSI());

	// Update TFT with settings.

	tft.setTextColor(WHITE);
	tft.setFreeFont();
	tft.setCursor(23, 50);
	tft.print("WiFi Status: ");
	tft.setTextColor(LTBLUE);
	tft.setCursor(150, 50);
	tft.print(WiFi.status());
	tft.println("");

	tft.setCursor(23, 65);
	tft.setTextColor(WHITE);
	tft.print("SSID: ");
	tft.setTextColor(LTBLUE);
	tft.setCursor(150, 65);
	tft.print(WiFi.SSID());
	tft.println("");

	tft.setCursor(23, 80);
	tft.setTextColor(WHITE);
	tft.print("IP Address: ");
	tft.setTextColor(LTBLUE);
	tft.setCursor(150, 80);
	tft.print(WiFi.localIP());
	tft.println("");

	tft.setCursor(23, 95);
	tft.setTextColor(WHITE);
	tft.print("DNS Address: ");
	tft.setTextColor(LTBLUE);
	tft.setCursor(150, 95);
	tft.print(WiFi.dnsIP());
	tft.println("");

	tft.setCursor(23, 110);
	tft.setTextColor(WHITE);
	tft.print("Gateway Address: ");
	tft.setTextColor(LTBLUE);
	tft.setCursor(150, 110);
	tft.print(WiFi.gatewayIP());
	tft.println("");

	tft.setCursor(23, 125);
	tft.setTextColor(WHITE);
	tft.print("Signal Strenght: ");
	tft.setTextColor(LTBLUE);
	tft.setCursor(150, 125);
	tft.print(WiFi.RSSI());
	tft.println("");

	tft.setCursor(23, 140);
	tft.setTextColor(WHITE);
	tft.print("Time Server: ");
	tft.setTextColor(LTBLUE);
	tft.setCursor(150, 140);
	tft.print(ntpServer);

	// If ESP32 inits successfully in station mode, recolour WiFi to green.

	drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiGreen, WIFI_ICON_W, WIFI_ICON_H);

	// Update message to advise unit is starting.

	tft.setFreeFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(WHITE);
	tft.println("");
	tft.setCursor(20, 175);
	tft.print("Unit is starting...");
	tft.setFreeFont();

	screenR = 1;

	delay(3000);	// Wait a moment.

	return true;

} // Close function.

/*-----------------------------------------------------------------*/

// Initialize SPIFFS.

void initSPIFFS() {

	if (!SPIFFS.begin(true)) {

		Serial.println("An error has occurred while mounting SPIFFS");
	}

	else
	{
		Serial.println("SPIFFS mounted successfully");
	}

} // Close function.

/*-----------------------------------------------------------------*/

// Read File from SPIFFS.

String readFile(fs::FS& fs, const char* path) {

	Serial.printf("Reading file: %s\r\n", path);

	File file = fs.open(path);

	if (!file || file.isDirectory()) {

		Serial.println("- failed to open file for reading");
		return String();
	}

	String fileContent;

	while (file.available()) {

		fileContent = file.readStringUntil('\n');
		break;
	}

	return fileContent;

} // Close function.

/*-----------------------------------------------------------------*/

// Write file to SPIFFS.

void writeFile(fs::FS& fs, const char* path, const char* message) {

	Serial.printf("Writing file: %s\r\n", path);

	File file = fs.open(path, FILE_WRITE);

	if (!file) {

		Serial.println("- failed to open file for writing");
		return;
	}

	if (file.print(message)) {

		Serial.println("- file written");
	}

	else
	{
		Serial.println("- write failed");
	}

} // Close function.

/*-----------------------------------------------------------------*/

// Get and print time.

void printLocalTime() {

	struct tm timeinfo;

	if (!getLocalTime(&timeinfo)) {

		Serial.println("Failed, time set to default.");

		// Set time manually.

		rtc.setTime(00, 00, 00, 01, 01, 2021);

		tft.setTextColor(WHITE, BLACK);
		tft.setFreeFont();
		tft.setTextSize(1);
		tft.setCursor(13, 220);
		tft.println("Failed, time set to default.");

		return;
	}

	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M");

	// Text block to over write characters from longer dates when date changes and unit has been running.

	tft.setTextColor(BLACK);
	tft.setFreeFont();
	tft.setTextSize(1);
	tft.setCursor(150, 220);
	tft.println("                ");

	// Actual date time to display.

	tft.setTextColor(WHITE, BLACK);
	tft.setFreeFont();
	tft.setTextSize(1);
	tft.setCursor(13, 220);
	tft.println(&timeinfo, "%A, %B %d %Y %H:%M");

	// Check current time to pause interrupt and update daily best records.

	if (rtc.getHour(true) == 23 && rtc.getMinute() == 59) {

		detachInterrupt(sensorInt);									// detach interrupt.
	}

	if (rtc.getHour(true) == 0 && rtc.getMinute() == 0) {

		blankDailyData();
	}

	if (rtc.getHour(true) == 0 && rtc.getMinute() == 1) {

		attachInterrupt(digitalPinToInterrupt(sensorInt), rotationInterruptISR, FALLING);	// Attach interrupt.
	}

} // Close function.

/*-----------------------------------------------------------------*/

// Buzzer tone for touch screen.

void tone(byte pin, int freq) {

	//ESP8266 and ESP32 specific functions.

	ledcSetup(0, freq, 8);		// Setup buzzer.
	ledcAttachPin(pin, 0);		// Attach buzzer.
	ledcWriteTone(0, freq);		// Play tone.
	delay(buzzerD);				// Wait a moment.
	ledcWriteTone(0, 0);		// Stop tone.

} // Close function.

/*-----------------------------------------------------------------*/

// Return JSON String from sensor Readings

String getJSONReadings() {

	// Retrieve best ever / high scores from EEPROM.

	float tempMaxSpeed;
	char tempMaxSpeedDate[40];
	int tempDoW;
	int tempDay;
	int tempMonth;
	int tempYear;
	int tempHour;
	int tempMinute;

	EEPROM.get(eeBestMaxSpeed, tempMaxSpeed);
	EEPROM.get(eeBestMaxSpeedDay, tempDay);
	EEPROM.get(eeBestMaxSpeedMonth, tempMonth);
	EEPROM.get(eeBestMaxSpeedYear, tempYear);
	EEPROM.get(eeBestMaxSpeedHour, tempHour);
	EEPROM.get(eeBestMaxSpeedMinute, tempMinute);
	EEPROM.commit();

	sprintf(tempMaxSpeedDate, "%u/%02u/%02u at %02u:%02u", tempDay, tempMonth, tempYear, tempHour, tempMinute);

	int tempSessionDistanceRecord;
	char tempDistanceSDate[40];

	EEPROM.get(eeBestDistanceS, tempSessionDistanceRecord);
	EEPROM.get(eeBestDistanceSDay, tempDay);
	EEPROM.get(eeBestDistanceSMonth, tempMonth);
	EEPROM.get(eeBestDistanceSYear, tempYear);
	EEPROM.get(eeBestDistanceSHour, tempHour);
	EEPROM.get(eeBestDistanceSMinute, tempMinute);
	EEPROM.commit();

	sprintf(tempDistanceSDate, "%u/%02u/%02u at %02u:%02u", tempDay, tempMonth, tempYear, tempHour, tempMinute);

	int tempSessionTimeRecord;
	long tempSessionTimeRecord1;
	char tempTimeSDate[40];

	EEPROM.get(eeBestTimeS, tempSessionTimeRecord1);
	EEPROM.get(eeBestTimeSDay, tempDay);
	EEPROM.get(eeBestTimeSMonth, tempMonth);
	EEPROM.get(eeBestTimeSYear, tempYear);
	EEPROM.get(eeBestTimeSHour, tempHour);
	EEPROM.get(eeBestTimeSMinute, tempMinute);
	EEPROM.commit();

	tempSessionTimeRecord = tempSessionTimeRecord1 / 1000 / 60;
	sprintf(tempTimeSDate, "%u/%02u/%02u at %02u:%02u", tempDay, tempMonth, tempYear, tempHour, tempMinute);

	int tempDailyDistanceRecord;
	char tempDistanceDDate[40];

	EEPROM.get(eeBestDistanceD, tempDailyDistanceRecord);
	EEPROM.get(eeBestDistanceDDay, tempDay);
	EEPROM.get(eeBestDistanceDMonth, tempMonth);
	EEPROM.get(eeBestDistanceDYear, tempYear);
	EEPROM.get(eeBestDistanceDHour, tempHour);
	EEPROM.get(eeBestDistanceDMinute, tempMinute);
	EEPROM.commit();

	sprintf(tempDistanceDDate, "%u/%02u/%02u", tempDay, tempMonth, tempYear);

	int tempDailyTimeRecord;
	long tempDailyTimeRecord1;
	char tempTimeDDate[40];

	EEPROM.get(eeBestTimeD, tempDailyTimeRecord1);
	EEPROM.get(eeBestTimeDDoW, tempDoW);
	EEPROM.get(eeBestTimeDDay, tempDay);
	EEPROM.get(eeBestTimeDMonth, tempMonth);
	EEPROM.get(eeBestTimeDYear, tempYear);
	EEPROM.get(eeBestTimeDHour, tempHour);
	EEPROM.get(eeBestTimeDMinute, tempMinute);
	EEPROM.commit();

	tempDailyTimeRecord = tempDailyTimeRecord1 / 1000 / 60;
	sprintf(tempTimeDDate, "%u/%02u/%02u", tempDay, tempMonth, tempYear);

	// Get daily time activity.

	readings["sessionTimeArray0"] = String(sessionTimeArray0);
	readings["sessionTimeArray1"] = String(sessionTimeArray1);
	readings["sessionTimeArray2"] = String(sessionTimeArray2);
	readings["sessionTimeArray3"] = String(sessionTimeArray3);
	readings["sessionTimeArray4"] = String(sessionTimeArray4);
	readings["sessionTimeArray5"] = String(sessionTimeArray5);
	readings["sessionTimeArray6"] = String(sessionTimeArray6);

	// Get daily distance activity.

	readings["distanceTravelledArray0"] = String(distanceTravelledArray0);
	readings["distanceTravelledArray1"] = String(distanceTravelledArray1);
	readings["distanceTravelledArray2"] = String(distanceTravelledArray2);
	readings["distanceTravelledArray3"] = String(distanceTravelledArray3);
	readings["distanceTravelledArray4"] = String(distanceTravelledArray4);
	readings["distanceTravelledArray5"] = String(distanceTravelledArray5);
	readings["distanceTravelledArray6"] = String(distanceTravelledArray6);

	// Get high scores.

	readings["tempMaxSpeed"] = String(tempMaxSpeed);
	readings["tempMaxSpeedDate"] = String(tempMaxSpeedDate);
	readings["tempSessionDistanceRecord"] = String(tempSessionDistanceRecord);
	readings["tempDistanceSDate"] = String(tempDistanceSDate);
	readings["tempSessionTimeRecord"] = String(tempSessionTimeRecord);
	readings["tempTimeSDate"] = String(tempTimeSDate);
	readings["tempDailyDistanceRecord"] = String(tempDailyDistanceRecord);
	readings["tempDistanceDDate"] = String(tempDistanceDDate);
	readings["tempDailyTimeRecord"] = String(tempDailyTimeRecord);
	readings["tempTimeDDate"] = String(tempTimeDDate);

	String jsonString = JSON.stringify(readings);
	return jsonString;

}  // Close function.

/*-----------------------------------------------------------------*/

// Clear daily totals on date change and update any new daily activity records.

void blankDailyData() {

	sessionArrayPosition = rtc.getDayofWeek();

	//Serial.print("Session Array Position = ");
	//Serial.println(sessionArrayPosition);
	//Serial.println();

	if (sessionArrayPosition == 0) {

		int tempDistance1;
		int tempDistance2;

		EEPROM.get(eeBestDistanceD, tempDistance1);
		EEPROM.get(eeSessionDistanceArray6Address, tempDistance2);
		EEPROM.commit();

		//Serial.print("Data from Best Distance Record = ");
		//Serial.println(tempDistance1);
		//Serial.print("Data from Session Array 6 (Saturday) = ");
		//Serial.println(tempDistance2);
		//Serial.println("");

		if (tempDistance1 < tempDistance2) {

			EEPROM.put(eeBestDistanceD, tempDistance2);
			EEPROM.put(eeBestDistanceDDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestDistanceDDay, rtc.getDay());
			EEPROM.put(eeBestDistanceDMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestDistanceDYear, rtc.getYear());
			EEPROM.put(eeBestDistanceDHour, rtc.getHour(true));
			EEPROM.put(eeBestDistanceDMinute, rtc.getMinute());
			EEPROM.commit();

			/*int tempDistance;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestDistanceD, tempDistance);
			EEPROM.get(eeBestDistanceDDoW, tempDoW);
			EEPROM.get(eeBestDistanceDDay, tempDay);
			EEPROM.get(eeBestDistanceDMonth, tempMonth);
			EEPROM.get(eeBestDistanceDYear, tempYear);
			EEPROM.get(eeBestDistanceDHour, tempHour);
			EEPROM.get(eeBestDistanceDMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New best daily distance: ");
			Serial.println(tempDistance);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}

		unsigned long tempTime1;
		unsigned long tempTime2;

		EEPROM.get(eeBestTimeD, tempTime1);
		EEPROM.get(eeSessionTimeArray6Address, tempTime2);
		EEPROM.commit();

		//Serial.print("Data from Best Time Record = ");
		//Serial.println(tempTime1);
		//Serial.print("Data from Time Array 6 (Saturday) = ");
		//Serial.println(tempTime2);
		//Serial.println("");

		if (tempTime1 < tempTime2) {

			EEPROM.put(eeBestTimeD, tempTime2);
			EEPROM.put(eeBestTimeDDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestTimeDDay, rtc.getDay());
			EEPROM.put(eeBestTimeDMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestTimeDYear, rtc.getYear());
			EEPROM.put(eeBestTimeDHour, rtc.getHour(true));
			EEPROM.put(eeBestTimeDMinute, rtc.getMinute());

			/*long tempTime;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestTimeD, tempTime);
			EEPROM.get(eeBestTimeDDoW, tempDoW);
			EEPROM.get(eeBestTimeDDay, tempDay);
			EEPROM.get(eeBestTimeDMonth, tempMonth);
			EEPROM.get(eeBestTimeDYear, tempYear);
			EEPROM.get(eeBestTimeDHour, tempHour);
			EEPROM.get(eeBestTimeDMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New best daily time: ");
			Serial.println(tempTime);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}

		sessionTimeArray0 = 0;
		distanceTravelledArray0 = 0;

		EEPROM.put(eeSessionTimeArray0Address, 0);				// Record the chart data in EEPROM.
		EEPROM.put(eeSessionDistanceArray0Address, 0);			// Record the chart data in EEPROM.
		EEPROM.commit();

		//Serial.print("Daily Data Successfully Deleted: ");
		//Serial.print(dayArray[0]);
		//Serial.println();
	}

	else if (sessionArrayPosition == 1) {

		int tempDistance1;
		int tempDistance2;

		EEPROM.get(eeBestDistanceD, tempDistance1);
		EEPROM.get(eeSessionDistanceArray0Address, tempDistance2);
		EEPROM.commit();

		//Serial.print("Data from Best Distance Record = ");
		//Serial.println(tempDistance1);
		//Serial.print("Data from Session Array 0 (Sunday) = ");
		//Serial.println(tempDistance2);
		//Serial.println("");

		if (tempDistance1 < tempDistance2) {

			EEPROM.put(eeBestDistanceD, tempDistance2);
			EEPROM.put(eeBestDistanceDDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestDistanceDDay, rtc.getDay());
			EEPROM.put(eeBestDistanceDMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestDistanceDYear, rtc.getYear());
			EEPROM.put(eeBestDistanceDHour, rtc.getHour(true));
			EEPROM.put(eeBestDistanceDMinute, rtc.getMinute());
			EEPROM.commit();

			/*int tempDistance;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestDistanceD, tempDistance);
			EEPROM.get(eeBestDistanceDDoW, tempDoW);
			EEPROM.get(eeBestDistanceDDay, tempDay);
			EEPROM.get(eeBestDistanceDMonth, tempMonth);
			EEPROM.get(eeBestDistanceDYear, tempYear);
			EEPROM.get(eeBestDistanceDHour, tempHour);
			EEPROM.get(eeBestDistanceDMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New best daily distance: ");
			Serial.println(tempDistance);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}

		unsigned long tempTime1;
		unsigned long tempTime2;

		EEPROM.get(eeBestTimeD, tempTime1);
		EEPROM.get(eeSessionTimeArray0Address, tempTime2);
		EEPROM.commit();

		//Serial.print("Data from Best Time Record = ");
		//Serial.println(tempTime1);
		//Serial.print("Data from Time Array 0 (Sunday) = ");
		//Serial.println(tempTime2);
		//Serial.println("");

		if (tempTime1 < tempTime2) {

			EEPROM.put(eeBestTimeD, tempTime2);
			EEPROM.put(eeBestTimeDDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestTimeDDay, rtc.getDay());
			EEPROM.put(eeBestTimeDMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestTimeDYear, rtc.getYear());
			EEPROM.put(eeBestTimeDHour, rtc.getHour(true));
			EEPROM.put(eeBestTimeDMinute, rtc.getMinute());

			/*long tempTime;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestTimeD, tempTime);
			EEPROM.get(eeBestTimeDDoW, tempDoW);
			EEPROM.get(eeBestTimeDDay, tempDay);
			EEPROM.get(eeBestTimeDMonth, tempMonth);
			EEPROM.get(eeBestTimeDYear, tempYear);
			EEPROM.get(eeBestTimeDHour, tempHour);
			EEPROM.get(eeBestTimeDMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New best daily time: ");
			Serial.println(tempTime);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}

		sessionTimeArray1 = 0;
		distanceTravelledArray1 = 0;

		EEPROM.put(eeSessionTimeArray1Address, 0);				// Record the chart data in EEPROM.
		EEPROM.put(eeSessionDistanceArray1Address, 0);			// Record the chart data in EEPROM.
		EEPROM.commit();

		//Serial.print("Daily Data Successfully Deleted: ");
		//Serial.print(dayArray[1]);
		//Serial.println();
	}

	else if (sessionArrayPosition == 2) {

		int tempDistance1;
		int tempDistance2;

		EEPROM.get(eeBestDistanceD, tempDistance1);
		EEPROM.get(eeSessionDistanceArray1Address, tempDistance2);
		EEPROM.commit();

		//Serial.print("Data from Best Distance Record = ");
		//Serial.println(tempDistance1);
		//Serial.print("Data from Session Array 1 (Monday) = ");
		//Serial.println(tempDistance2);
		//Serial.println("");

		if (tempDistance1 < tempDistance2) {

			EEPROM.put(eeBestDistanceD, tempDistance2);
			EEPROM.put(eeBestDistanceDDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestDistanceDDay, rtc.getDay());
			EEPROM.put(eeBestDistanceDMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestDistanceDYear, rtc.getYear());
			EEPROM.put(eeBestDistanceDHour, rtc.getHour(true));
			EEPROM.put(eeBestDistanceDMinute, rtc.getMinute());
			EEPROM.commit();

			/*int tempDistance;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestDistanceD, tempDistance);
			EEPROM.get(eeBestDistanceDDoW, tempDoW);
			EEPROM.get(eeBestDistanceDDay, tempDay);
			EEPROM.get(eeBestDistanceDMonth, tempMonth);
			EEPROM.get(eeBestDistanceDYear, tempYear);
			EEPROM.get(eeBestDistanceDHour, tempHour);
			EEPROM.get(eeBestDistanceDMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New best daily distance: ");
			Serial.println(tempDistance);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}

		unsigned long tempTime1;
		unsigned long tempTime2;

		EEPROM.get(eeBestTimeD, tempTime1);
		EEPROM.get(eeSessionTimeArray1Address, tempTime2);
		EEPROM.commit();

		//Serial.print("Data from Best Time Record = ");
		//Serial.println(tempTime1);
		//Serial.print("Data from Time Array 1 (Monday) = ");
		//Serial.println(tempTime2);
		//Serial.println("");

		if (tempTime1 < tempTime2) {

			EEPROM.put(eeBestTimeD, tempTime2);
			EEPROM.put(eeBestTimeDDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestTimeDDay, rtc.getDay());
			EEPROM.put(eeBestTimeDMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestTimeDYear, rtc.getYear());
			EEPROM.put(eeBestTimeDHour, rtc.getHour(true));
			EEPROM.put(eeBestTimeDMinute, rtc.getMinute());

			/*long tempTime;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestTimeD, tempTime);
			EEPROM.get(eeBestTimeDDoW, tempDoW);
			EEPROM.get(eeBestTimeDDay, tempDay);
			EEPROM.get(eeBestTimeDMonth, tempMonth);
			EEPROM.get(eeBestTimeDYear, tempYear);
			EEPROM.get(eeBestTimeDHour, tempHour);
			EEPROM.get(eeBestTimeDMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New best daily time: ");
			Serial.println(tempTime);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}

		sessionTimeArray2 = 0;
		distanceTravelledArray2 = 0;

		EEPROM.put(eeSessionTimeArray2Address, 0);				// Record the chart data in EEPROM.
		EEPROM.put(eeSessionDistanceArray2Address, 0);			// Record the chart data in EEPROM.
		EEPROM.commit();

		//Serial.print("Daily Data Successfully Deleted: ");
		//Serial.print(dayArray[2]);
		//Serial.println();
	}

	else if (sessionArrayPosition == 3) {

		int tempDistance1;
		int tempDistance2;

		EEPROM.get(eeBestDistanceD, tempDistance1);
		EEPROM.get(eeSessionDistanceArray2Address, tempDistance2);
		EEPROM.commit();

		//Serial.print("Data from Best Distance Record = ");
		//Serial.println(tempDistance1);
		//Serial.print("Data from Session Array 2 (Tuesday) = ");
		//Serial.println(tempDistance2);
		//Serial.println("");

		if (tempDistance1 < tempDistance2) {

			EEPROM.put(eeBestDistanceD, tempDistance2);
			EEPROM.put(eeBestDistanceDDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestDistanceDDay, rtc.getDay());
			EEPROM.put(eeBestDistanceDMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestDistanceDYear, rtc.getYear());
			EEPROM.put(eeBestDistanceDHour, rtc.getHour(true));
			EEPROM.put(eeBestDistanceDMinute, rtc.getMinute());
			EEPROM.commit();

			/*int tempDistance;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestDistanceD, tempDistance);
			EEPROM.get(eeBestDistanceDDoW, tempDoW);
			EEPROM.get(eeBestDistanceDDay, tempDay);
			EEPROM.get(eeBestDistanceDMonth, tempMonth);
			EEPROM.get(eeBestDistanceDYear, tempYear);
			EEPROM.get(eeBestDistanceDHour, tempHour);
			EEPROM.get(eeBestDistanceDMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New best daily distance: ");
			Serial.println(tempDistance);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}

		unsigned long tempTime1;
		unsigned long tempTime2;

		EEPROM.get(eeBestTimeD, tempTime1);
		EEPROM.get(eeSessionTimeArray2Address, tempTime2);
		EEPROM.commit();

		//Serial.print("Data from Best Time Record = ");
		//Serial.println(tempTime1);
		//Serial.print("Data from Time Array 2 (Tuesday) = ");
		//Serial.println(tempTime2);
		//Serial.println("");

		if (tempTime1 < tempTime2) {

			EEPROM.put(eeBestTimeD, tempTime2);
			EEPROM.put(eeBestTimeDDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestTimeDDay, rtc.getDay());
			EEPROM.put(eeBestTimeDMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestTimeDYear, rtc.getYear());
			EEPROM.put(eeBestTimeDHour, rtc.getHour(true));
			EEPROM.put(eeBestTimeDMinute, rtc.getMinute());

			/*long tempTime;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestTimeD, tempTime);
			EEPROM.get(eeBestTimeDDoW, tempDoW);
			EEPROM.get(eeBestTimeDDay, tempDay);
			EEPROM.get(eeBestTimeDMonth, tempMonth);
			EEPROM.get(eeBestTimeDYear, tempYear);
			EEPROM.get(eeBestTimeDHour, tempHour);
			EEPROM.get(eeBestTimeDMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New best daily time: ");
			Serial.println(tempTime);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}

		sessionTimeArray3 = 0;
		distanceTravelledArray3 = 0;

		EEPROM.put(eeSessionTimeArray3Address, 0);				// Record the chart data in EEPROM.
		EEPROM.put(eeSessionDistanceArray3Address, 0);			// Record the chart data in EEPROM.
		EEPROM.commit();

		//Serial.print("Daily Data Successfully Deleted: ");
		//Serial.print(dayArray[3]);
		//Serial.println();
	}

	else if (sessionArrayPosition == 4) {

		int  tempDistance1;
		int  tempDistance2;

		EEPROM.get(eeBestDistanceD, tempDistance1);
		EEPROM.get(eeSessionDistanceArray3Address, tempDistance2);
		EEPROM.commit();

		//Serial.print("Data from Best Distance Record = ");
		//Serial.println(tempDistance1);
		//Serial.print("Data from Session Array 3 (Wednesday) = ");
		//Serial.println(tempDistance2);
		//Serial.println("");

		if (tempDistance1 < tempDistance2) {

			EEPROM.put(eeBestDistanceD, tempDistance2);
			EEPROM.put(eeBestDistanceDDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestDistanceDDay, rtc.getDay());
			EEPROM.put(eeBestDistanceDMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestDistanceDYear, rtc.getYear());
			EEPROM.put(eeBestDistanceDHour, rtc.getHour(true));
			EEPROM.put(eeBestDistanceDMinute, rtc.getMinute());
			EEPROM.commit();

			/*int tempDistance;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestDistanceD, tempDistance);
			EEPROM.get(eeBestDistanceDDoW, tempDoW);
			EEPROM.get(eeBestDistanceDDay, tempDay);
			EEPROM.get(eeBestDistanceDMonth, tempMonth);
			EEPROM.get(eeBestDistanceDYear, tempYear);
			EEPROM.get(eeBestDistanceDHour, tempHour);
			EEPROM.get(eeBestDistanceDMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New best daily distance: ");
			Serial.println(tempDistance);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}

		unsigned long tempTime1;
		unsigned long tempTime2;

		EEPROM.get(eeBestTimeD, tempTime1);
		EEPROM.get(eeSessionTimeArray3Address, tempTime2);
		EEPROM.commit();

		//Serial.print("Data from Best Time Record = ");
		//Serial.println(tempTime1);
		//Serial.print("Data from Time Array 3 (Wednesday) = ");
		//Serial.println(tempTime2);
		//Serial.println("");

		if (tempTime1 < tempTime2) {

			EEPROM.put(eeBestTimeD, tempTime2);
			EEPROM.put(eeBestTimeDDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestTimeDDay, rtc.getDay());
			EEPROM.put(eeBestTimeDMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestTimeDYear, rtc.getYear());
			EEPROM.put(eeBestTimeDHour, rtc.getHour(true));
			EEPROM.put(eeBestTimeDMinute, rtc.getMinute());

			/*long tempTime;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestTimeD, tempTime);
			EEPROM.get(eeBestTimeDDoW, tempDoW);
			EEPROM.get(eeBestTimeDDay, tempDay);
			EEPROM.get(eeBestTimeDMonth, tempMonth);
			EEPROM.get(eeBestTimeDYear, tempYear);
			EEPROM.get(eeBestTimeDHour, tempHour);
			EEPROM.get(eeBestTimeDMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New best daily time: ");
			Serial.println(tempTime);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}

		sessionTimeArray4 = 0;
		distanceTravelledArray4 = 0;

		EEPROM.put(eeSessionTimeArray4Address, 0);				// Record the chart data in EEPROM.
		EEPROM.put(eeSessionDistanceArray4Address, 0);			// Record the chart data in EEPROM.
		EEPROM.commit();

		//Serial.print("Daily Data Successfully Deleted: ");
		//Serial.print(dayArray[4]);
		//Serial.println();
	}

	else if (sessionArrayPosition == 5) {

		int tempDistance1;
		int tempDistance2;

		EEPROM.get(eeBestDistanceD, tempDistance1);
		EEPROM.get(eeSessionDistanceArray4Address, tempDistance2);
		EEPROM.commit();

		//Serial.print("Data from Best Distance Record = ");
		//Serial.println(tempDistance1);
		//Serial.print("Data from Session Array 4 (Thursday) = ");
		//Serial.println(tempDistance2);
		//Serial.println("");

		if (tempDistance1 < tempDistance2) {

			EEPROM.put(eeBestDistanceD, tempDistance2);
			EEPROM.put(eeBestDistanceDDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestDistanceDDay, rtc.getDay());
			EEPROM.put(eeBestDistanceDMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestDistanceDYear, rtc.getYear());
			EEPROM.put(eeBestDistanceDHour, rtc.getHour(true));
			EEPROM.put(eeBestDistanceDMinute, rtc.getMinute());
			EEPROM.commit();

			/*int tempDistance;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestDistanceD, tempDistance);
			EEPROM.get(eeBestDistanceDDoW, tempDoW);
			EEPROM.get(eeBestDistanceDDay, tempDay);
			EEPROM.get(eeBestDistanceDMonth, tempMonth);
			EEPROM.get(eeBestDistanceDYear, tempYear);
			EEPROM.get(eeBestDistanceDHour, tempHour);
			EEPROM.get(eeBestDistanceDMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New best daily distance: ");
			Serial.println(tempDistance);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}

		unsigned long tempTime1;
		unsigned long tempTime2;

		EEPROM.get(eeBestTimeD, tempTime1);
		EEPROM.get(eeSessionTimeArray4Address, tempTime2);
		EEPROM.commit();

		//Serial.print("Data from Best Time Record = ");
		//Serial.println(tempTime1);
		//Serial.print("Data from Time Array 4 (Thursday) = ");
		//Serial.println(tempTime2);
		//Serial.println("");

		if (tempTime1 < tempTime2) {

			EEPROM.put(eeBestTimeD, tempTime2);
			EEPROM.put(eeBestTimeDDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestTimeDDay, rtc.getDay());
			EEPROM.put(eeBestTimeDMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestTimeDYear, rtc.getYear());
			EEPROM.put(eeBestTimeDHour, rtc.getHour(true));
			EEPROM.put(eeBestTimeDMinute, rtc.getMinute());

			/*long tempTime;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestTimeD, tempTime);
			EEPROM.get(eeBestTimeDDoW, tempDoW);
			EEPROM.get(eeBestTimeDDay, tempDay);
			EEPROM.get(eeBestTimeDMonth, tempMonth);
			EEPROM.get(eeBestTimeDYear, tempYear);
			EEPROM.get(eeBestTimeDHour, tempHour);
			EEPROM.get(eeBestTimeDMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New best daily time: ");
			Serial.println(tempTime);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}

		sessionTimeArray5 = 0;
		distanceTravelledArray5 = 0;

		EEPROM.put(eeSessionTimeArray5Address, 0);				// Record the chart data in EEPROM.
		EEPROM.put(eeSessionDistanceArray5Address, 0);			// Record the chart data in EEPROM.
		EEPROM.commit();

		//Serial.print("Daily Data Successfully Deleted: ");
		//Serial.print(dayArray[5]);
		//Serial.println();
	}

	else if (sessionArrayPosition == 6) {

		int tempDistance1;
		int tempDistance2;

		EEPROM.get(eeBestDistanceD, tempDistance1);
		EEPROM.get(eeSessionDistanceArray5Address, tempDistance2);
		EEPROM.commit();

		//Serial.print("Data from Best Distance Record = ");
		//Serial.println(tempDistance1);
		//Serial.print("Data from Session Array 5 (Friday) = ");
		//Serial.println(tempDistance2);
		//Serial.println("");

		if (tempDistance1 < tempDistance2) {

			EEPROM.put(eeBestDistanceD, tempDistance2);
			EEPROM.put(eeBestDistanceDDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestDistanceDDay, rtc.getDay());
			EEPROM.put(eeBestDistanceDMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestDistanceDYear, rtc.getYear());
			EEPROM.put(eeBestDistanceDHour, rtc.getHour(true));
			EEPROM.put(eeBestDistanceDMinute, rtc.getMinute());
			EEPROM.commit();

			/*int tempDistance;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestDistanceD, tempDistance);
			EEPROM.get(eeBestDistanceDDoW, tempDoW);
			EEPROM.get(eeBestDistanceDDay, tempDay);
			EEPROM.get(eeBestDistanceDMonth, tempMonth);
			EEPROM.get(eeBestDistanceDYear, tempYear);
			EEPROM.get(eeBestDistanceDHour, tempHour);
			EEPROM.get(eeBestDistanceDMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New best daily distance: ");
			Serial.println(tempDistance);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}

		unsigned long tempTime1;
		unsigned long tempTime2;

		EEPROM.get(eeBestTimeD, tempTime1);
		EEPROM.get(eeSessionTimeArray5Address, tempTime2);
		EEPROM.commit();

		//Serial.print("Data from Best Time Record = ");
		//Serial.println(tempTime1);
		//Serial.print("Data from Time Array 5 (Friday) = ");
		//Serial.println(tempTime2);
		//Serial.println("");

		if (tempTime1 < tempTime2) {

			EEPROM.put(eeBestTimeD, tempTime2);
			EEPROM.put(eeBestTimeDDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestTimeDDay, rtc.getDay());
			EEPROM.put(eeBestTimeDMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestTimeDYear, rtc.getYear());
			EEPROM.put(eeBestTimeDHour, rtc.getHour(true));
			EEPROM.put(eeBestTimeDMinute, rtc.getMinute());

			/*long tempTime;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestTimeD, tempTime);
			EEPROM.get(eeBestTimeDDoW, tempDoW);
			EEPROM.get(eeBestTimeDDay, tempDay);
			EEPROM.get(eeBestTimeDMonth, tempMonth);
			EEPROM.get(eeBestTimeDYear, tempYear);
			EEPROM.get(eeBestTimeDHour, tempHour);
			EEPROM.get(eeBestTimeDMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New best daily time: ");
			Serial.println(tempTime);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}

		sessionTimeArray6 = 0;
		distanceTravelledArray6 = 0;

		EEPROM.put(eeSessionTimeArray6Address, 0);				// Record the chart data in EEPROM.
		EEPROM.put(eeSessionDistanceArray6Address, 0);			// Record the chart data in EEPROM.
		EEPROM.commit();

		//Serial.print("Daily Data Successfully Deleted: ");
		//Serial.print(dayArray[6]);
		//Serial.println();
	}

} // Close function.

/*-----------------------------------------------------------------*/

void setup() {

	//Begin serial mode.

	Serial.begin(115200);
	delay(100);

	// Set pin modes.

	pinMode(TFT_LED, OUTPUT);				// Output for LCD back light.
	pinMode(sensorInt, INPUT);				// Wheel sensor (REED switch).

	// Switch on TFT LED back light.

	digitalWrite(TFT_LED, HIGH);			// Output for LCD back light (high is off).

	// Set all SPI chip selects to HIGH to stablise SPI bus.

	digitalWrite(TOUCH_CS, HIGH);			// Touch controller chip select.
	digitalWrite(TFT_CS, HIGH);				// TFT screen chip select.

	// Configure interupt.

	attachInterrupt(digitalPinToInterrupt(sensorInt), rotationInterruptISR, FALLING);

	// Configure EEPROM.

	EEPROM.begin(512);
	EEPROM.get(eeResetSettingAddress, eeResetSetting);

	/*
	*
	* This line is used for initial ESP power on from manufacture to set EEPROM, comment out once uploaded and re-upload.
	*
	* resetSystemDemoData();
	*
	*/

	// Check reset flag and either reset all settings back to new or load demo data.

	//eeResetSetting = 1;

	if (eeResetSetting == 1) {

		resetSystemData();
		Serial.println("Data loaded from full reset function");
		Serial.println();
	}

	else if (eeResetSetting == 2) {

		resetSystemDemoData();
		Serial.println("Data loaded from demo reset function");
		Serial.println();
	}

	else if (eeResetSetting == 3) {

		resetRecordsData();
		Serial.println("Data loaded to reset records");
		Serial.println();
	}

	else {
		Serial.println("No data loaded from reset function");
		Serial.println();
	}

	// Load previous settings and last recorded data.

	EEPROM.get(eeMenuAddress, eeMenuSetting);				// Load system settings.
	EEPROM.get(eeCircAddress, eeCircSetting);
	EEPROM.get(eeMenuAddress, screenMenu);
	EEPROM.get(eeCircAddress, circumference);
	EEPROM.get(eeTotalDistanceAddress, distanceC);

	EEPROM.commit();

	EEPROM.get(eegraphTMAddress, graphTM);					// Load graph time scale settings.
	EEPROM.get(eegraphTMIAddress, graphTMI);
	EEPROM.get(eegraphTAPAddress, graphTAP);

	// Output graph time scale data to serial for checking.

	//Serial.println("Time graph settings output from Setup");
	//Serial.print("Time Scale: ");
	//Serial.print(graphTM);
	//Serial.print(" & ");
	//Serial.print("Increments: ");
	//Serial.print(graphTMI);
	//Serial.print(" & ");
	//Serial.print("Position: ");
	//Serial.println(graphTAP);
	//Serial.println(" ");

	EEPROM.commit();

	EEPROM.get(eegraphDMAddress, graphDM);					// Load graph distance scale settings.
	EEPROM.get(eegraphDMIAddress, graphDMI);
	EEPROM.get(eegraphDAPAddress, graphDAP);

	// Output graph distance data to serial for checking.

	//Serial.println("Distance graph output from Setup");
	//Serial.print("Distance Scale: ");
	//Serial.print(graphDM);
	//Serial.print(" & ");
	//Serial.print("Increments: ");
	//Serial.print(graphDMI);
	//Serial.print(" & ");
	//Serial.print("Position: ");
	//Serial.print(graphDAP);
	//Serial.println(" ");

	EEPROM.commit();

	EEPROM.get(eeBuzzerYNAddress, buzzerYN);				// Load buzzer setting.
	EEPROM.get(eeWiFiYNAddress, wiFiYN);					// Load WiFi reset flag.

	EEPROM.commit();

	// Output variables data to serial for checking.

	//Serial.println();
	//Serial.print("Beep Flag: ");
	//Serial.println(buzzerYN);
	//Serial.print("WiFi Flag: ");
	//Serial.println(wiFiYN);
	//Serial.println();

	EEPROM.get(eeCalYNAddress, calTouchScreen);				// Load touch screen calibration data.
	EEPROM.get(eeCalDataAddress0, calData[0]);
	EEPROM.get(eeCalDataAddress1, calData[1]);
	EEPROM.get(eeCalDataAddress2, calData[2]);
	EEPROM.get(eeCalDataAddress3, calData[3]);
	EEPROM.get(eeCalDataAddress4, calData[4]);

	EEPROM.commit();

	// Output calibration data to serial for checking.

	Serial.print("Calibration Data: ");

	for (uint8_t i = 0; i < 5; i++) {

		Serial.print(calData[i]);
		if (i < 4) Serial.print(", ");
	}
	Serial.println(" ");

	EEPROM.get(eeSessionTimeArray0Address, sessionTimeArray[0]);				// Load previous session times into arrays.
	EEPROM.get(eeSessionTimeArray1Address, sessionTimeArray[1]);
	EEPROM.get(eeSessionTimeArray2Address, sessionTimeArray[2]);
	EEPROM.get(eeSessionTimeArray3Address, sessionTimeArray[3]);
	EEPROM.get(eeSessionTimeArray4Address, sessionTimeArray[4]);
	EEPROM.get(eeSessionTimeArray5Address, sessionTimeArray[5]);
	EEPROM.get(eeSessionTimeArray6Address, sessionTimeArray[6]);

	EEPROM.commit();

	sessionTimeCap = graphTM;

	//Serial.println();
	//Serial.print("Time Cap: ");
	//Serial.println(sessionTimeCap);
	//Serial.println(" ");

	sessionTimeArray0 = sessionTimeArray[0] / 1000 / 60;						// Update chart variables from arrays.
	sessionTimeArray1 = sessionTimeArray[1] / 1000 / 60;
	sessionTimeArray2 = sessionTimeArray[2] / 1000 / 60;
	sessionTimeArray3 = sessionTimeArray[3] / 1000 / 60;
	sessionTimeArray4 = sessionTimeArray[4] / 1000 / 60;
	sessionTimeArray5 = sessionTimeArray[5] / 1000 / 60;
	sessionTimeArray6 = sessionTimeArray[6] / 1000 / 60;

	EEPROM.get(eeSessionDistanceArray0Address, distanceTravelledArray[0]);		// Load previous session distance's into arrays.
	EEPROM.get(eeSessionDistanceArray1Address, distanceTravelledArray[1]);
	EEPROM.get(eeSessionDistanceArray2Address, distanceTravelledArray[2]);
	EEPROM.get(eeSessionDistanceArray3Address, distanceTravelledArray[3]);
	EEPROM.get(eeSessionDistanceArray4Address, distanceTravelledArray[4]);
	EEPROM.get(eeSessionDistanceArray5Address, distanceTravelledArray[5]);
	EEPROM.get(eeSessionDistanceArray6Address, distanceTravelledArray[6]);

	distanceGraphCap = graphDM;													// Update graph cap to stop value exceeding chart level.

	//Serial.print("Distance Cap: ");
	//Serial.println(distanceGraphCap);
	//Serial.println(" ");

	EEPROM.commit();

	distanceTravelledArray0 = distanceTravelledArray[0];						// Update chart variables from arrays.
	distanceTravelledArray1 = distanceTravelledArray[1];
	distanceTravelledArray2 = distanceTravelledArray[2];
	distanceTravelledArray3 = distanceTravelledArray[3];
	distanceTravelledArray4 = distanceTravelledArray[4];
	distanceTravelledArray5 = distanceTravelledArray[5];
	distanceTravelledArray6 = distanceTravelledArray[6];

	// Print best ever records data from EEPROM.
	/*
	float tempMaxSpeed;
	int tempDoW;
	int tempDay;
	int tempMonth;
	int tempYear;
	int tempHour;
	int tempMinute;

	EEPROM.get(eeBestMaxSpeed, tempMaxSpeed);
	EEPROM.get(eeBestMaxSpeedDoW, tempDoW);
	EEPROM.get(eeBestMaxSpeedDay, tempDay);
	EEPROM.get(eeBestMaxSpeedMonth, tempMonth);
	EEPROM.get(eeBestMaxSpeedYear, tempYear);
	EEPROM.get(eeBestMaxSpeedHour, tempHour);
	EEPROM.get(eeBestMaxSpeedMinute, tempMinute);
	EEPROM.commit();

	Serial.print("Best Max Speed Record: ");
	Serial.println(tempMaxSpeed);
	Serial.print("Date: ");
	Serial.print(dayArray[tempDoW]);
	Serial.print(", ");
	Serial.print(tempDay);
	Serial.print("/");
	Serial.print(tempMonth);
	Serial.print("/");
	Serial.print(tempYear);
	Serial.print(" at ");
	Serial.print(tempHour);
	Serial.print(":");
	Serial.println(tempMinute);
	Serial.println(" ");

	int tempDistanceS;

	EEPROM.get(eeBestDistanceS, tempDistanceS);
	EEPROM.get(eeBestDistanceSDoW, tempDoW);
	EEPROM.get(eeBestDistanceSDay, tempDay);
	EEPROM.get(eeBestDistanceSMonth, tempMonth);
	EEPROM.get(eeBestDistanceSYear, tempYear);
	EEPROM.get(eeBestDistanceSHour, tempHour);
	EEPROM.get(eeBestDistanceSMinute, tempMinute);
	EEPROM.commit();

	Serial.print("Best Session Distance Record: ");
	Serial.println(tempDistanceS);
	Serial.print("Date: ");
	Serial.print(dayArray[tempDoW]);
	Serial.print(", ");
	Serial.print(tempDay);
	Serial.print("/");
	Serial.print(tempMonth);
	Serial.print("/");
	Serial.print(tempYear);
	Serial.print(" at ");
	Serial.print(tempHour);
	Serial.print(":");
	Serial.println(tempMinute);
	Serial.println(" ");

	long tempTimeS;

	EEPROM.get(eeBestTimeS, tempTimeS);
	EEPROM.get(eeBestTimeSDoW, tempDoW);
	EEPROM.get(eeBestTimeSDay, tempDay);
	EEPROM.get(eeBestTimeSMonth, tempMonth);
	EEPROM.get(eeBestTimeSYear, tempYear);
	EEPROM.get(eeBestTimeSHour, tempHour);
	EEPROM.get(eeBestTimeSMinute, tempMinute);
	EEPROM.commit();

	Serial.print("Best Session Time Record: ");
	Serial.println(tempTimeS);
	Serial.print("Date: ");
	Serial.print(dayArray[tempDoW]);
	Serial.print(", ");
	Serial.print(tempDay);
	Serial.print("/");
	Serial.print(tempMonth);
	Serial.print("/");
	Serial.print(tempYear);
	Serial.print(" at ");
	Serial.print(tempHour);
	Serial.print(":");
	Serial.println(tempMinute);
	Serial.println(" ");

	int tempDistanceD;

	EEPROM.get(eeBestDistanceD, tempDistanceD);
	EEPROM.get(eeBestDistanceDDoW, tempDoW);
	EEPROM.get(eeBestDistanceDDay, tempDay);
	EEPROM.get(eeBestDistanceDMonth, tempMonth);
	EEPROM.get(eeBestDistanceDYear, tempYear);
	EEPROM.get(eeBestDistanceDHour, tempHour);
	EEPROM.get(eeBestDistanceDMinute, tempMinute);
	EEPROM.commit();

	Serial.print("Best Daily Distance Record: ");
	Serial.println(tempDistanceD);
	Serial.print("Date: ");
	Serial.print(dayArray[tempDoW]);
	Serial.print(", ");
	Serial.print(tempDay);
	Serial.print("/");
	Serial.print(tempMonth);
	Serial.print("/");
	Serial.print(tempYear);
	Serial.print(" at ");
	Serial.print(tempHour);
	Serial.print(":");
	Serial.println(tempMinute);
	Serial.println(" ");

	long tempTimeD;

	EEPROM.get(eeBestTimeD, tempTimeD);
	EEPROM.get(eeBestTimeDDoW, tempDoW);
	EEPROM.get(eeBestTimeDDay, tempDay);
	EEPROM.get(eeBestTimeDMonth, tempMonth);
	EEPROM.get(eeBestTimeDYear, tempYear);
	EEPROM.get(eeBestTimeDHour, tempHour);
	EEPROM.get(eeBestTimeDMinute, tempMinute);
	EEPROM.commit();

	Serial.print("Best Daily Time Record: ");
	Serial.println(tempTimeD);
	Serial.print("Date: ");
	Serial.print(dayArray[tempDoW]);
	Serial.print(", ");
	Serial.print(tempDay);
	Serial.print("/");
	Serial.print(tempMonth);
	Serial.print("/");
	Serial.print(tempYear);
	Serial.print(" at ");
	Serial.print(tempHour);
	Serial.print(":");
	Serial.println(tempMinute);
	Serial.println(" ");*/

	// Set menu position and circumference position from EEPROM data.

	screenMenu = eeMenuSetting;
	circumference = eeCircSetting;

	circImperial = circumference * 0.62137;		// for MPH calculation

	// Configure buzzer settings for touch screen.

	if (buzzerYN == false) {

		buzzerF = 0;	// Set frequency of the buzzer beep.
		buzzerD = 0;	// Set delay of the buzzer beep.
	}

	else
	{
		buzzerF = 1000;	// Set frequency of the buzzer beep.
		buzzerD = 75;	// Set deelay of the buzzer beep.
	}

	// Initialise TFT display.

	tft.begin();
	tft.setRotation(1);
	tft.setCursor(0, 0);
	tft.fillScreen(ILI9341_BLACK);

	digitalWrite(TFT_LED, LOW);				// LOW to turn backlight on.
	delay(500);

	// Start up screen image and title.

	tft.fillScreen(ILI9341_WHITE);
	drawBitmap(tft, 48, 96, startScreen, 128, 128);

	tft.setFreeFont(&FreeSans12pt7b);
	tft.setTextSize(1);
	tft.setTextColor(BLACK);
	tft.setCursor(130, 210);
	tft.println("PetBit");
	tft.setFreeFont();
	tft.setTextSize(1);
	tft.setCursor(150, 220);
	tft.println("Mk-2");

	delay(5000);

	// Clear screen.

	tft.fillScreen(ILI9341_BLACK);

	// Initialize SPIFFS.

	initSPIFFS();

	// Check if WiFi is to be reset.

	if (wiFiYN == true) {

		ssid = "blank";
		pass = "blank";
		ip = "blank";
		subnet = "blank";
		gateway = "blank";
		dns = "blank";

		writeFile(SPIFFS, ssidPath, ssid.c_str());
		writeFile(SPIFFS, passPath, pass.c_str());
		writeFile(SPIFFS, ipPath, ip.c_str());
		writeFile(SPIFFS, subnetPath, subnet.c_str());
		writeFile(SPIFFS, gatewayPath, gateway.c_str());
		writeFile(SPIFFS, dnsPath, dns.c_str());

		wiFiYN = false;
		EEPROM.put(eeWiFiYNAddress, wiFiYN);
		EEPROM.commit();
	}

	// Load values saved in SPIFFS.

	//ssid = "BT-7FA3K5";									// Remove these lines before final build.
	//pass = "iKD94Y3K4Qvkck";
	//ip = "192.168.1.200";
	//subnet = "255.255.255.0";
	//gateway = "192.168.1.254";
	//dns = "192.168.1.254";

	//writeFile(SPIFFS, ssidPath, ssid.c_str());
	//writeFile(SPIFFS, passPath, pass.c_str());
	//writeFile(SPIFFS, ipPath, ip.c_str());
	//writeFile(SPIFFS, subnetPath, subnet.c_str());
	//writeFile(SPIFFS, gatewayPath, gateway.c_str());
	//writeFile(SPIFFS, dnsPath, dns.c_str());

	Serial.println();
	ssid = readFile(SPIFFS, ssidPath);
	pass = readFile(SPIFFS, passPath);
	ip = readFile(SPIFFS, ipPath);
	subnet = readFile(SPIFFS, subnetPath);
	gateway = readFile(SPIFFS, gatewayPath);
	dns = readFile(SPIFFS, dnsPath);

	Serial.println();
	Serial.println(ssid);
	Serial.println(pass);
	Serial.println(ip);
	Serial.println(subnet);
	Serial.println(gateway);
	Serial.println(dns);
	Serial.println();

	// Check if screen calibration flag is set to yes, if not, load previous saved calibration data.

	if (calTouchScreen == 1) {

		touch_calibrate(tft);
	}

	else tft.setTouch(calData);

	// Draw border and buttons at start.

	drawBorder();																	// Screen border layouts.
	tft.fillCircle(SENSOR_ICON_X, SENSOR_ICON_Y, SENSOR_ICON_R, TFT_ORANGE);		// Draw initial sensor.

	// Initialize WiFi and web services.

	if (initWiFi()) {

		// Handle the Web Server in Station Mode and route for root / web page.

		server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {

			request->send(SPIFFS, "/index.html", "text/html");
			});

		server.serveStatic("/", SPIFFS, "/");

		// Request for the latest data readings.

		server.on("/readings", HTTP_GET, [](AsyncWebServerRequest* request) {

			String json = getJSONReadings();
			request->send(200, "application/json", json);
			json = String();
			});

		events.onConnect([](AsyncEventSourceClient* client) {

			if (client->lastId()) {
				Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
			}
			});

		server.addHandler(&events);

		server.begin();
	}

	else

	{
		apMode = true;	// Set variable to be true so void loop is by passed and doesnt run until false.

		// WiFi title page.

		wiFiTitle();

		// Initialize the ESP32 in Access Point mode, recolour to WiFI red.

		drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiRed, WIFI_ICON_W, WIFI_ICON_H);

		// Set Access Point.
		Serial.println("Setting AP (Access Point)");

		// NULL sets an open Access Point.

		WiFi.softAP("WIFI-MANAGER", NULL);

		IPAddress IP = WiFi.softAPIP();
		Serial.print("AP IP address: ");
		Serial.println(IP);

		// Web Server Root URL For WiFi Manager Web Page.

		server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
			request->send(SPIFFS, "/wifimanager.html", "text/html");
			});

		server.serveStatic("/", SPIFFS, "/");

		// Get the parameters submited on the form.

		server.on("/", HTTP_POST, [](AsyncWebServerRequest* request) {
			int params = request->params();
			for (int i = 0; i < params; i++) {
				AsyncWebParameter* p = request->getParam(i);
				if (p->isPost()) {
					// HTTP POST ssid value
					if (p->name() == PARAM_INPUT_1) {
						ssid = p->value().c_str();
						Serial.print("SSID set to: ");
						Serial.println(ssid);
						// Write file to save value
						writeFile(SPIFFS, ssidPath, ssid.c_str());
					}
					// HTTP POST pass value
					if (p->name() == PARAM_INPUT_2) {
						pass = p->value().c_str();
						Serial.print("Password set to: ");
						Serial.println(pass);
						// Write file to save value
						writeFile(SPIFFS, passPath, pass.c_str());
					}
					// HTTP POST ip value
					if (p->name() == PARAM_INPUT_3) {
						ip = p->value().c_str();
						Serial.print("IP Address set to: ");
						Serial.println(ip);
						// Write file to save value
						writeFile(SPIFFS, ipPath, ip.c_str());
					}
					// HTTP POST ip value
					if (p->name() == PARAM_INPUT_4) {
						subnet = p->value().c_str();
						Serial.print("Subnet Address: ");
						Serial.println(subnet);
						// Write file to save value
						writeFile(SPIFFS, subnetPath, subnet.c_str());
					}
					// HTTP POST ip value
					if (p->name() == PARAM_INPUT_5) {
						gateway = p->value().c_str();
						Serial.print("Gateway set to: ");
						Serial.println(gateway);
						// Write file to save value
						writeFile(SPIFFS, gatewayPath, gateway.c_str());
					}
					// HTTP POST ip value
					if (p->name() == PARAM_INPUT_6) {
						dns = p->value().c_str();
						Serial.print("DNS Address set to: ");
						Serial.println(dns);
						// Write file to save value
						writeFile(SPIFFS, dnsPath, dns.c_str());
					}
					//Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
				}
			}

			request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
			delay(3000);

			// After saving the parameters, restart the ESP32

			ESP.restart();
			});

		server.begin();

		tft.fillRect(39, 60, 183, 109, RED);
		tft.drawRect(38, 59, 185, 111, WHITE);
		tft.drawRect(37, 58, 187, 113, WHITE);
		tft.setFreeFont(&FreeSans9pt7b);
		tft.setTextSize(1);
		tft.setTextColor(WHITE); tft.setCursor(50, 78);
		tft.print("Access Point Mode");

		tft.setFreeFont();
		tft.setTextColor(WHITE);
		tft.setCursor(50, 90);
		tft.print("Could not connect to WiFi");
		tft.setCursor(50, 106);
		tft.print("1) Using your mobile phone");
		tft.setCursor(50, 118);
		tft.print("2) Connect to WiFI Manager");
		tft.setCursor(50, 130);
		tft.print("3) Browse to 192.168.4.1");
		tft.setCursor(50, 142);
		tft.print("4) Enter network settings");
		tft.setCursor(50, 154);
		tft.print("5) Unit will then restart");

		unsigned long previousMillis = millis();
		unsigned long interval = 120000;

		while (1) {

			// Hold from starting loop while in AP mode.

			unsigned long currentMillis = millis();

			// Restart after 2 minutes in case of failed reconnection with correc WiFi details.

			if (currentMillis - previousMillis >= interval) {

				ESP.restart();

			}

		}

	}

	// Initialize time and get the time.

	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	printLocalTime();
	LocalTime = millis();

	// Set array position to the day of the week.

	sessionArrayPosition = rtc.getDayofWeek();				// Get day of the week from RTC.

	Serial.print("Session Array Position: ");
	Serial.print(sessionArrayPosition);
	Serial.print(" Day: ");
	Serial.println(dayArray[sessionArrayPosition]);
	Serial.println();

	// Print WiFi configuration.

	Serial.println("");
	Serial.print("SSID set to: ");
	Serial.println(ssid);
	Serial.print("Password set to: ");
	Serial.println(pass);
	Serial.print("IP Address set to: ");
	Serial.println(ip);
	Serial.print("Subnet Address set to: ");
	Serial.println(subnet);
	Serial.print("Gateway Address set to: ");
	Serial.println(gateway);
	Serial.print("DNS Address set to: ");
	Serial.println(dns);
	Serial.println("");

	// Check if WiFi is disabled, technically it wont be unless the interupt sensor has triggered during start up.

	if (disWiFi == true) {

		//disconnect WiFi as it's no longer needed.

		WiFi.disconnect();
		drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiAmber, WIFI_ICON_W, WIFI_ICON_H);
	}

} // Close setup.

/*-----------------------------------------------------------------*/

void loop() {

	// Light sleep mode (LED back light off after x seconds).

	if (millis() >= sleepT + sleepTime) {

		digitalWrite(TFT_LED, HIGH);			// Output for LCD back light.
	}

	else 	digitalWrite(TFT_LED, LOW);			// Output for LCD back light.

	// Main functions, checking menu, calculations & average speed.

	menu_Change();		// Reset menu change at each pass after touch is pressed.
	mainData();			// Calculates main data.
	drawSensor();		// Change sensor indicator on main display.
	averageSpeed();		// Calculate average speed.

	// Enable / Disable WiFi when interrupt is in operation.

	if (disWiFi == true && disWiFiF == true) {

		if (WiFi.status() == WL_CONNECTED);
		WiFi.disconnect();
		drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiAmber, WIFI_ICON_W, WIFI_ICON_H);
	}

	else if (disWiFi == false && disWiFiF == true) {

		if (WiFi.status() != WL_CONNECTED);
		WiFi.begin(ssid.c_str(), pass.c_str());
		Serial.println("Connecting to WiFi...");
		Serial.print("IP Address: ");
		Serial.println(WiFi.localIP());
		Serial.println("");
		Serial.print("RRSI: ");
		Serial.println(WiFi.RSSI());
		drawBitmap(tft, WIFI_ICON_Y, WIFI_ICON_X, wiFiGreen, WIFI_ICON_W, WIFI_ICON_H);

		disWiFiF = false;
	}

	// Retry connecting to WiFi if connection is lost at all other times.

	unsigned long wiFiRC = millis();

	if ((WiFi.status() != WL_CONNECTED) && (disWiFi == false) && (wiFiRC - wiFiR >= wiFiRI)) {

		Serial.print(millis());
		Serial.println("Reconnecting to WiFi...");
		WiFi.disconnect();
		WiFi.reconnect();
		wiFiR = wiFiRC;
	}

	// Update time from Internet time server.

	if ((disWiFiF == false) && (millis() >= LocalTime + localTimeInterval)) {

		printLocalTime();			// Get time and update display.
		LocalTime = millis();
	}

	// Send Events to the client with sensor readins and update colors every 120 seconds.

	if ((disWiFiF == false) && (millis() - lastTime > timerDelay)) {

		String message = getJSONReadings();
		events.send(message.c_str(), "new_readings", millis());
		lastTime = millis();
	}

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

		tft.drawFastHLine(BUTTON1_X - 7, BUTTON1_Y, BUTTON1_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON1_X + 50, BUTTON1_Y, BUTTON1_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON1_X - 8, BUTTON1_Y + 1, BUTTON1_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON1_X - 7, BUTTON1_Y + 49, BUTTON1_H + 7, TFT_BLACK);

		tft.drawFastHLine(BUTTON2_X - 7, BUTTON2_Y, BUTTON2_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON2_X + 50, BUTTON2_Y, BUTTON2_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON2_X - 8, BUTTON2_Y + 1, BUTTON2_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON2_X - 7, BUTTON2_Y + 49, BUTTON2_H + 7, TFT_BLACK);

		tft.drawFastHLine(BUTTON3_X - 7, BUTTON3_Y, BUTTON3_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON3_X + 50, BUTTON3_Y, BUTTON3_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON3_X - 8, BUTTON3_Y + 1, BUTTON3_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON3_X - 7, BUTTON3_Y + 49, BUTTON3_H + 7, TFT_BLACK);

		tft.drawFastHLine(BUTTON4_X - 7, BUTTON4_Y, BUTTON4_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON4_X + 50, BUTTON4_Y, BUTTON4_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON4_X - 8, BUTTON4_Y + 1, BUTTON4_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON4_X - 7, BUTTON4_Y + 49, BUTTON4_H + 7, TFT_BLACK);
	}

	else drawBitmap(tft, SETTINGS_COG_Y, SETTINGS_COG_X, settingsWhite, SETTINGS_COG_W, SETTINGS_COG_H);

	if (screenMenu == 4) {

		drawBitmap(tft, BUTTON4_Y + 1, BUTTON4_X + 1, distanceIconWhite, BUTTON4_W - 2, BUTTON4_H - 2);

		tft.drawFastHLine(BUTTON1_X - 7, BUTTON1_Y, BUTTON1_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON1_X + 50, BUTTON1_Y, BUTTON1_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON1_X - 8, BUTTON1_Y + 1, BUTTON1_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON1_X - 7, BUTTON1_Y + 49, BUTTON1_H + 7, TFT_BLACK);

		tft.drawFastHLine(BUTTON2_X - 7, BUTTON2_Y, BUTTON2_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON2_X + 50, BUTTON2_Y, BUTTON2_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON2_X - 8, BUTTON2_Y + 1, BUTTON2_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON2_X - 7, BUTTON2_Y + 49, BUTTON2_H + 7, TFT_BLACK);

		tft.drawFastHLine(BUTTON3_X - 7, BUTTON3_Y, BUTTON3_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON3_X + 50, BUTTON3_Y, BUTTON3_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON3_X - 8, BUTTON3_Y + 1, BUTTON3_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON3_X - 7, BUTTON3_Y + 49, BUTTON3_H + 7, TFT_BLACK);

		tft.drawFastHLine(BUTTON4_X - 8, BUTTON4_Y, BUTTON4_H + 8, TFT_WHITE);
		tft.drawFastVLine(BUTTON4_X + 50, BUTTON4_Y, BUTTON4_H, TFT_WHITE);
		tft.drawFastVLine(BUTTON4_X - 8, BUTTON4_Y + 1, BUTTON4_H - 2, TFT_BLACK);
		tft.drawFastHLine(BUTTON4_X - 8, BUTTON4_Y + 49, BUTTON4_H + 8, TFT_WHITE);
	}

	else if (screenMenu != 5) {

		drawBitmap(tft, BUTTON4_Y + 1, BUTTON4_X + 1, distanceIconWhite, BUTTON4_W - 2, BUTTON4_H - 2);
		tft.drawRect(BUTTON4_X, BUTTON4_Y, BUTTON4_W, BUTTON4_H, TFT_BLACK);
	}

	if (screenMenu == 3) {

		drawBitmap(tft, BUTTON3_Y + 1, BUTTON3_X + 1, timeIconWhite, BUTTON3_W - 2, BUTTON4_H - 2);

		tft.drawFastHLine(BUTTON1_X - 7, BUTTON1_Y, BUTTON1_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON1_X + 50, BUTTON1_Y, BUTTON1_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON1_X - 8, BUTTON1_Y + 1, BUTTON1_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON1_X - 7, BUTTON1_Y + 49, BUTTON1_H + 7, TFT_BLACK);

		tft.drawFastHLine(BUTTON2_X - 7, BUTTON2_Y, BUTTON2_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON2_X + 50, BUTTON2_Y, BUTTON2_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON2_X - 8, BUTTON2_Y + 1, BUTTON2_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON2_X - 7, BUTTON2_Y + 49, BUTTON2_H + 7, TFT_BLACK);

		tft.drawFastHLine(BUTTON3_X - 8, BUTTON3_Y, BUTTON3_H + 8, TFT_WHITE);
		tft.drawFastVLine(BUTTON3_X + 50, BUTTON3_Y, BUTTON3_H, TFT_WHITE);
		tft.drawFastVLine(BUTTON3_X - 8, BUTTON3_Y + 1, BUTTON3_H - 2, TFT_BLACK);
		tft.drawFastHLine(BUTTON3_X - 8, BUTTON3_Y + 49, BUTTON3_H + 8, TFT_WHITE);

		tft.drawFastHLine(BUTTON4_X - 7, BUTTON4_Y, BUTTON4_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON4_X + 50, BUTTON4_Y, BUTTON4_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON4_X - 8, BUTTON4_Y + 1, BUTTON4_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON4_X - 7, BUTTON4_Y + 49, BUTTON4_H + 7, TFT_BLACK);
	}

	else if (screenMenu != 5) {

		drawBitmap(tft, BUTTON3_Y + 1, BUTTON3_X + 1, timeIconWhite, BUTTON3_W - 2, BUTTON4_H - 2);
		tft.drawRect(BUTTON3_X, BUTTON3_Y, BUTTON3_W, BUTTON3_H, TFT_BLACK);
	}

	if (screenMenu == 2) {

		drawBitmap(tft, BUTTON2_Y + 1, BUTTON2_X + 1, speedIconWhite, BUTTON2_W - 2, BUTTON2_H - 2);

		tft.drawFastHLine(BUTTON1_X - 7, BUTTON1_Y, BUTTON1_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON1_X + 50, BUTTON1_Y, BUTTON1_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON1_X - 8, BUTTON1_Y + 1, BUTTON1_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON1_X - 7, BUTTON1_Y + 49, BUTTON1_H + 7, TFT_BLACK);

		tft.drawFastHLine(BUTTON2_X - 8, BUTTON2_Y, BUTTON2_H + 8, TFT_WHITE);
		tft.drawFastVLine(BUTTON2_X + 50, BUTTON2_Y, BUTTON2_H, TFT_WHITE);
		tft.drawFastVLine(BUTTON2_X - 8, BUTTON2_Y + 1, BUTTON2_H - 2, TFT_BLACK);
		tft.drawFastHLine(BUTTON2_X - 8, BUTTON2_Y + 49, BUTTON2_H + 8, TFT_WHITE);

		tft.drawFastHLine(BUTTON3_X - 7, BUTTON3_Y, BUTTON3_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON3_X + 50, BUTTON3_Y, BUTTON3_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON3_X - 8, BUTTON3_Y + 1, BUTTON3_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON3_X - 7, BUTTON3_Y + 49, BUTTON3_H + 7, TFT_BLACK);

		tft.drawFastHLine(BUTTON4_X - 7, BUTTON4_Y, BUTTON4_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON4_X + 50, BUTTON4_Y, BUTTON4_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON4_X - 8, BUTTON4_Y + 1, BUTTON4_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON4_X - 7, BUTTON4_Y + 49, BUTTON4_H + 7, TFT_BLACK);
	}

	else if (screenMenu != 5) {

		drawBitmap(tft, BUTTON2_Y + 1, BUTTON2_X + 1, speedIconWhite, BUTTON2_W - 2, BUTTON2_H - 2);
		tft.drawRect(BUTTON2_X, BUTTON2_Y, BUTTON2_W, BUTTON2_H, TFT_BLACK);
	}

	if (screenMenu == 1) {

		drawBitmap(tft, BUTTON1_Y + 1, BUTTON1_X + 1, sessionIconWhite, BUTTON1_W - 2, BUTTON1_H - 2);

		tft.drawFastHLine(BUTTON1_X - 8, BUTTON1_Y, BUTTON1_H + 8, TFT_WHITE);
		tft.drawFastVLine(BUTTON1_X + 50, BUTTON1_Y, BUTTON1_H, TFT_WHITE);
		tft.drawFastVLine(BUTTON1_X - 8, BUTTON1_Y + 1, BUTTON1_H - 2, TFT_BLACK);
		tft.drawFastHLine(BUTTON1_X - 8, BUTTON1_Y + 49, BUTTON1_H + 8, TFT_WHITE);

		tft.drawFastHLine(BUTTON2_X - 7, BUTTON2_Y, BUTTON2_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON2_X + 50, BUTTON2_Y, BUTTON2_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON2_X - 8, BUTTON2_Y + 1, BUTTON2_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON2_X - 7, BUTTON2_Y + 49, BUTTON2_H + 7, TFT_BLACK);

		tft.drawFastHLine(BUTTON3_X - 7, BUTTON3_Y, BUTTON3_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON3_X + 50, BUTTON3_Y, BUTTON3_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON3_X - 8, BUTTON3_Y + 1, BUTTON3_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON3_X - 7, BUTTON3_Y + 49, BUTTON3_H + 7, TFT_BLACK);

		tft.drawFastHLine(BUTTON4_X - 7, BUTTON4_Y, BUTTON4_H + 7, TFT_BLACK);
		tft.drawFastVLine(BUTTON4_X + 50, BUTTON4_Y, BUTTON4_H, TFT_BLACK);
		tft.drawFastVLine(BUTTON4_X - 8, BUTTON4_Y + 1, BUTTON4_H - 2, TFT_WHITE);
		tft.drawFastHLine(BUTTON4_X - 7, BUTTON4_Y + 49, BUTTON4_H + 7, TFT_BLACK);
	}

	else if (screenMenu != 5) {

		drawBitmap(tft, BUTTON1_Y + 1, BUTTON1_X + 1, sessionIconWhite, BUTTON1_W - 2, BUTTON1_H - 2);
		tft.drawRect(BUTTON1_X, BUTTON1_Y, BUTTON1_W, BUTTON1_H, TFT_BLACK);
	}

	// Touch screen setup and triggers.

	uint16_t x, y;		// variables for touch data.

	// See if there's any touch data for us.

	if (tft.getTouch(&x, &y)) {

		// Restart sleep timer.

		sleepT = millis();

		// Button one.

		if ((x > BUTTON1_X) && (x < (BUTTON1_X + BUTTON1_W))) {
			if ((y > BUTTON1_Y) && (y <= (BUTTON1_Y + BUTTON1_H))) {

				if (screenMenu != 1) {		// To stop screen flicker when pressing the same menu button again.

					tone(buzzerP, buzzerF);
					screenMenu = 1;
					menuChange = 1;
					screenR = 1;
					//Serial.print("Button 1 hit ");
					//Serial.print(" : Screen Menu: ");
					//Serial.print(screenMenu);
					//Serial.println(" ");
				}

			}

		}

		// Button two.

		if ((x > BUTTON2_X) && (x < (BUTTON2_X + BUTTON2_W))) {
			if ((y > BUTTON2_Y) && (y <= (BUTTON2_Y + BUTTON2_H))) {

				if (screenMenu != 2 && screenMenu != 5) {		// To stop screen flicker when pressing the same menu button again.

					tone(buzzerP, buzzerF);
					screenMenu = 2;
					menuChange = 1;
					screenR = 1;
					//Serial.print("Button 2 hit ");
					//Serial.print(" : Screen Menu: ");
					//Serial.print(screenMenu);
					//Serial.println(" ");
				}

				else if (screenMenu == 5 && configurationFlag == 7) {

					tone(buzzerP, buzzerF);
					resetMenuSettingMinus();
				}

				else if (screenMenu == 5 && configurationFlag == 6) {

					tone(buzzerP, buzzerF);
					wiFiSettingMinus();
				}

				else if (screenMenu == 5 && configurationFlag == 5) {

					tone(buzzerP, buzzerF);
					buzzerSettingMinus();
				}

				else if (screenMenu == 5 && configurationFlag == 4) {

					tone(buzzerP, buzzerF);
					timeScaleSettingMinus();
				}

				else if (screenMenu == 5 && configurationFlag == 3) {

					tone(buzzerP, buzzerF);
					distanceScaleSettingMinus();
				}

				else if (screenMenu == 5 && configurationFlag == 2) {

					tone(buzzerP, buzzerF);
					circumferenceSettingMinus();
				}

				else if (screenMenu == 5 && configurationFlag == 1) {

					tone(buzzerP, buzzerF);
					menuSettingMinus();
				}

			}

		}

		// Button three.

		if ((x > BUTTON3_X) && (x < (BUTTON3_X + BUTTON3_W))) {
			if ((y > BUTTON3_Y) && (y <= (BUTTON3_Y + BUTTON3_H))) {

				if (screenMenu != 3 && screenMenu != 5) {		// To stop screen flicker when pressing the same menu button again.

					tone(buzzerP, buzzerF);
					screenMenu = 3;
					menuChange = 1;
					screenR = 1;
					//Serial.print("Button 3 hit ");
					//Serial.print(" : Screen Menu: ");
					//Serial.print(screenMenu);
					//Serial.println(" ");
				}

				else if (screenMenu == 5 && configurationFlag == 7) {

					tone(buzzerP, buzzerF);
					resetMenuSettingPlus();
				}

				else if (screenMenu == 5 && configurationFlag == 6) {

					tone(buzzerP, buzzerF);
					wiFiSettingPlus();
				}

				else if (screenMenu == 5 && configurationFlag == 5) {

					tone(buzzerP, buzzerF);
					buzzerSettingPlus();
				}

				else if (screenMenu == 5 && configurationFlag == 4) {

					tone(buzzerP, buzzerF);
					timeScaleSettingPlus();
				}

				else if (screenMenu == 5 && configurationFlag == 3) {

					tone(buzzerP, buzzerF);
					distanceScaleSettingPlus();
				}

				else if (screenMenu == 5 && configurationFlag == 2) {

					tone(buzzerP, buzzerF);
					circumferenceSettingPlus();
				}

				else if (screenMenu == 5 && configurationFlag == 1) {

					tone(buzzerP, buzzerF);
					menuSettingPlus();
				}

			}

		}

		// Button four.

		if ((x > BUTTON4_X) && (x < (BUTTON4_X + BUTTON4_W))) {
			if ((y > BUTTON4_Y) && (y <= (BUTTON4_Y + BUTTON4_H))) {

				if (screenMenu != 4 && screenMenu != 5) {		// To stop screen flicker when pressing the same menu button again.

					tone(buzzerP, buzzerF);
					screenMenu = 4;
					menuChange = 1;
					screenR = 1;
					//Serial.print("Button 4 hit ");
					//Serial.print(" : Screen Menu: ");
					//Serial.print(screenMenu);
					//Serial.println(" ");
				}

				else if (screenMenu == 5) {

					tone(buzzerP, buzzerF);
					configurationFlag++;

					if (configurationFlag == byte(9)) {

						configurationFlag = byte(1);
					}

					//Serial.print("Configuration Flag After If: ");
					//Serial.print(configurationFlag);
					//Serial.println(" ");
				}

			}

		}

		if ((x > SETTINGS_COG_X) && (x < (SETTINGS_COG_X + SETTINGS_COG_W))) {
			if ((y > SETTINGS_COG_Y) && (y <= (SETTINGS_COG_Y + SETTINGS_COG_H))) {

				if (screenMenu != 5) {		// To stop screen flicker when pressing the same menu button again.

					tone(buzzerP, buzzerF);
					screenMenu = 5;
					menuChange = 1;
					screenR = 1;
					//Serial.print("Button 5 hit ");
					//Serial.print(" : Screen Menu: ");
					//Serial.print(screenMenu);
					//Serial.println(" ");
				}

			}

		}

	}

	// Trigger screen choice - Current exercise screen.

	if (screenMenu == 1) {

		if (screenR == 1) {

			drawBlackBox();
			screenR = false;
		}

		currentExerciseScreen();
	}

	// Trigger screen choice - Odometer screen.

	if (screenMenu == 2) {

		if (screenR == 1) {


			drawBlackBox();
			dial_1 = true;
			screenR = false;
		}

		XphDialScreen(tft, dialX, dialY, 80, 0, 20, 2, 170, speedKph, 2, 0, RED, WHITE, BLACK, "Kph", dial_1); // XPH dial screen function.

	}

	// Trigger screen choice - Distance sessions screen.

	if (screenMenu == 3) {

		if (screenR == 1) {

			drawBlackBox();
			graph_1 = true;
			graph_2 = true;
			graph_3 = true;
			graph_4 = true;
			graph_5 = true;
			graph_6 = true;
			graph_7 = true;
			screenR = false;
		}

		// Session time bar graphs.

		if (sessionTimeArray1 <= (sessionTimeCap * 0.8)) {

			ptSessionTimeV1(tft, graphX1, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeArray1, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "Mo", graph_1);
		}

		else if (sessionTimeArray1 >= sessionTimeCap) {

			ptSessionTimeV1(tft, graphX1, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Mo", graph_1);
		}

		else ((ptSessionTimeV1(tft, graphX1, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeArray1, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Mo", graph_1)));

		if (sessionTimeArray2 <= (sessionTimeCap * 0.8)) {

			ptSessionTimeV2(tft, graphX2, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeArray2, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "Tu", graph_2);
		}

		else if (sessionTimeArray2 >= sessionTimeCap) {

			ptSessionTimeV2(tft, graphX2, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Tu", graph_2);
		}

		else ((ptSessionTimeV2(tft, graphX2, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeArray2, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Tu", graph_2)));

		if (sessionTimeArray3 <= (sessionTimeCap * 0.8)) {

			ptSessionTimeV2(tft, graphX3, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeArray3, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "We", graph_3);
		}

		else if (sessionTimeArray3 >= sessionTimeCap) {

			ptSessionTimeV2(tft, graphX3, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "We", graph_3);
		}

		else ((ptSessionTimeV2(tft, graphX3, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeArray3, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "We", graph_3)));

		if (sessionTimeArray4 <= (sessionTimeCap * 0.8)) {

			ptSessionTimeV2(tft, graphX4, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeArray4, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "Th", graph_4);
		}

		else if (sessionTimeArray4 >= sessionTimeCap) {

			ptSessionTimeV2(tft, graphX4, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Th", graph_4);
		}

		else ((ptSessionTimeV2(tft, graphX4, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeArray4, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Th", graph_4)));

		if (sessionTimeArray5 <= (sessionTimeCap * 0.8)) {

			ptSessionTimeV2(tft, graphX5, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeArray5, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "Fr", graph_5);
		}

		else if (sessionTimeArray5 >= sessionTimeCap) {

			ptSessionTimeV2(tft, graphX5, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Fr", graph_5);
		}

		else ((ptSessionTimeV2(tft, graphX5, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeArray5, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Fr", graph_5)));

		if (sessionTimeArray6 <= (sessionTimeCap * 0.8)) {

			ptSessionTimeV2(tft, graphX6, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeArray6, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "Sa", graph_6);
		}

		else if (sessionTimeArray6 >= sessionTimeCap) {

			ptSessionTimeV2(tft, graphX6, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Sa", graph_6);
		}

		else ((ptSessionTimeV2(tft, graphX6, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeArray6, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Sa", graph_6)));

		if (sessionTimeArray0 <= (sessionTimeCap * 0.8)) {

			ptSessionTimeV3(tft, graphX7, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeArray0, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "Su", graph_7);
		}

		else if (sessionTimeArray0 >= sessionTimeCap) {

			ptSessionTimeV3(tft, graphX7, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Su", graph_7);
		}

		else ((ptSessionTimeV3(tft, graphX7, graphY, graphW, graphH, 0, graphTM, graphTMI, sessionTimeArray0, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Su", graph_7)));

	}

	// Trigger screen choice - Time sessions screen.

	if (screenMenu == 4) {

		if (screenR == 1) {

			drawBlackBox();
			graph_8 = true;
			graph_9 = true;
			graph_10 = true;
			graph_11 = true;
			graph_12 = true;
			graph_13 = true;
			graph_14 = true;
			screenR = false;
		}

		// Distance bar graphs.

		if (distanceTravelledArray1 <= (distanceGraphCap * 0.8)) {

			ptSessionDistanceV1(tft, graphX1, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceTravelledArray1, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "Mo", graph_8);
		}

		else if (distanceTravelledArray1 >= distanceGraphCap) {

			ptSessionDistanceV1(tft, graphX1, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceGraphCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Mo", graph_8);
		}

		else ((ptSessionDistanceV1(tft, graphX1, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceTravelledArray1, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Mo", graph_8)));

		if (distanceTravelledArray2 <= (distanceGraphCap * 0.8)) {

			ptSessionDistanceV2(tft, graphX2, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceTravelledArray2, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "Tu", graph_9);
		}

		else if (distanceTravelledArray2 >= distanceGraphCap) {

			ptSessionDistanceV2(tft, graphX2, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceGraphCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Tu", graph_9);
		}

		else ((ptSessionDistanceV2(tft, graphX2, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceTravelledArray2, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Tu", graph_9)));

		if (distanceTravelledArray3 <= (distanceGraphCap * 0.8)) {

			ptSessionDistanceV2(tft, graphX3, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceTravelledArray3, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "We", graph_10);
		}

		else if (distanceTravelledArray3 >= distanceGraphCap) {

			ptSessionDistanceV2(tft, graphX3, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceGraphCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "We", graph_10);
		}

		else ((ptSessionDistanceV2(tft, graphX3, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceTravelledArray3, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "We", graph_10)));

		if (distanceTravelledArray4 <= (distanceGraphCap * 0.8)) {

			ptSessionDistanceV2(tft, graphX4, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceTravelledArray4, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "Th", graph_11);
		}

		else if (distanceTravelledArray4 >= distanceGraphCap) {

			ptSessionDistanceV2(tft, graphX4, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceGraphCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Th", graph_11);
		}

		else ((ptSessionDistanceV2(tft, graphX4, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceTravelledArray4, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Th", graph_11)));

		if (distanceTravelledArray5 <= (distanceGraphCap * 0.8)) {

			ptSessionDistanceV2(tft, graphX5, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceTravelledArray5, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "Fr", graph_12);
		}

		else if (distanceTravelledArray5 >= distanceGraphCap) {

			ptSessionDistanceV2(tft, graphX5, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceGraphCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Fr", graph_12);
		}

		else ((ptSessionDistanceV2(tft, graphX5, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceTravelledArray5, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Fr", graph_12)));

		if (distanceTravelledArray6 <= (distanceGraphCap * 0.8)) {

			ptSessionDistanceV2(tft, graphX6, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceTravelledArray6, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "Sa", graph_13);
		}

		else if (distanceTravelledArray6 >= distanceGraphCap) {

			ptSessionDistanceV2(tft, graphX6, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceGraphCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Sa", graph_13);
		}

		else ((ptSessionDistanceV2(tft, graphX6, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceTravelledArray6, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Sa", graph_13)));

		if (distanceTravelledArray0 <= (distanceGraphCap * 0.8)) {

			ptSessionDistanceV3(tft, graphX7, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceTravelledArray0, 3, 0, CYAN, DKGREY, WHITE, WHITE, BLACK, "Su", graph_14);
		}

		else if (distanceTravelledArray0 >= distanceGraphCap) {

			ptSessionDistanceV3(tft, graphX7, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceGraphCap, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Su", graph_14);
		}

		else ((ptSessionDistanceV3(tft, graphX7, graphY, graphW, graphH, 0, graphDM, graphDMI, distanceTravelledArray0, 3, 0, RED, DKGREY, WHITE, WHITE, BLACK, "Su", graph_14)));

	}

	// Trigger screen choice - Configuration screen.

	if (screenMenu == 5) {

		if (screenR == 1) {

			drawBlackBox();
			screenR = false;
		}

		configurationDisplay();
	}

	// Check for new max speed and save 

	if (speedKph > maxKphSpeed) {

		maxKphSpeed = speedKph;
		newMaxSpeedF = true;
	}

	// Calculate average speed & remove any minus calculations.

	averageKphSpeed = total / numReadings;

	if (averageKphSpeed < 0.99) {

		averageKphSpeed = 0.00;
	}

} // Close loop.

/*-----------------------------------------------------------------*/

// Process sensor data.

void mainData() {

	// Set speed variables to "0.00" if the wheel doesnt turn for X period (currently 4000ms).

	if (lastRotation2 < millis()) {

		speedKph = 0.00;
		speedMph = 0.00;
		recordSessions = true;
		eeSessionChange = true;
		disWiFi = false;

		if (sessionDistance > maxSessonDistance) {

			maxSessonDistance = sessionDistance;
			newBestSessionDistanceF = true;
		}

		if (sessionTimeMillis > maxSessionTime) {

			maxSessionTime = sessionTimeMillis;
			newBestSessionTimeF = true;
		}

		newMaxSpeedRecord();
		updateBestEverRecords();
	}

	else
	{
		// Calculate current session time.

		sessionTimeMillis = millis() - sessionStartTime;
		sessionTime = sessionTimeMillis / 1000;
	}

	// Ensure data is always "0" or greater, if not set to "0"

	if ((speedKph >= 0) || (speedMph >= 0)) {

		// Do nothing.
	}

	else
	{
		speedKph = 0.00;
		speedMph = 0.00;
	}

	// Calculate current session distance travelled.

	sessionDistance = distanceTravelled - sessionStartDistance;

	if (maxKphSpeed == 0) {				// Reset start up session distance displayed to zero, otherwise it displays current total distance.

		sessionDistance = 0;
	}

	if (sessionTime == 0) {				// Reset max speed.

		maxKphSpeed = 0;
	}

	// Update data arrays.

	if ((sessionTimeFlag == true) && (recordSessions == true)) {									// Calculate session duration.

		// Update array position for the day of the week.

		sessionArrayPosition = rtc.getDayofWeek();													// Get day of the week from RTC.

		//Serial.print("Session Array Position: ");
		//Serial.print(sessionArrayPosition);
		//Serial.print(" Day: ");
		//Serial.println(dayArray[sessionArrayPosition]);
		//Serial.println();

		sessionTimeArray[sessionArrayPosition] = millis() - sessionStartTime;						// Calculate last session time in millis & store into array.
		distanceTravelledArray[sessionArrayPosition] = distanceTravelled - sessionStartDistance;	// Store distance travelled into array.

		if (eeSessionChange == true) {

			unsigned long temporySession0;
			unsigned long temporySession1;
			unsigned int temporyDistance0;
			unsigned int temporyDistance1;

			switch (sessionArrayPosition) {

			case 0:

				EEPROM.get(eeSessionTimeArray0Address, temporySession0);
				temporySession1 = temporySession0 + sessionTimeArray[0];

				EEPROM.get(eeSessionDistanceArray0Address, temporyDistance0);
				temporyDistance1 = temporyDistance0 + distanceTravelledArray[0];

				sessionTimeArray0 = temporySession1 / 1000 / 60;						// Divided by 1000 for millis to seconds, then divided by 60 for minutes.
				distanceTravelledArray0 = temporyDistance1;								// Update chart data.
				EEPROM.put(eeSessionTimeArray0Address, temporySession1);				// Record the chart data in EEPROM.
				EEPROM.put(eeSessionDistanceArray0Address, temporyDistance1);			// Record the chart data in EEPROM.
				EEPROM.commit();
				eeSessionChange = false;

				//Serial.println("Case 0 Triggered: ");
				//Serial.println("");

				break;

			case 1:

				EEPROM.get(eeSessionTimeArray1Address, temporySession0);
				temporySession1 = temporySession0 + sessionTimeArray[1];

				EEPROM.get(eeSessionDistanceArray1Address, temporyDistance0);
				temporyDistance1 = temporyDistance0 + distanceTravelledArray[1];

				sessionTimeArray1 = temporySession1 / 1000 / 60;
				distanceTravelledArray1 = temporyDistance1;
				EEPROM.put(eeSessionTimeArray1Address, temporySession1);
				EEPROM.put(eeSessionDistanceArray1Address, temporyDistance1);
				EEPROM.commit();
				eeSessionChange = false;

				//Serial.println("Case 1 Triggered: ");
				//Serial.println("");

				break;

			case 2:

				EEPROM.get(eeSessionTimeArray2Address, temporySession0);
				temporySession1 = temporySession0 + sessionTimeArray[2];

				EEPROM.get(eeSessionDistanceArray2Address, temporyDistance0);
				temporyDistance1 = temporyDistance0 + distanceTravelledArray[2];

				sessionTimeArray2 = temporySession1 / 1000 / 60;
				distanceTravelledArray2 = temporyDistance1;
				EEPROM.put(eeSessionTimeArray2Address, temporySession1);
				EEPROM.put(eeSessionDistanceArray2Address, temporyDistance1);
				EEPROM.commit();
				eeSessionChange = false;

				//Serial.println("Case 2 Triggered: ");
				//Serial.println("");

				break;

			case 3:

				EEPROM.get(eeSessionTimeArray3Address, temporySession0);
				temporySession1 = temporySession0 + sessionTimeArray[3];

				EEPROM.get(eeSessionDistanceArray3Address, temporyDistance0);
				temporyDistance1 = temporyDistance0 + distanceTravelledArray[3];

				sessionTimeArray3 = temporySession1 / 1000 / 60;
				distanceTravelledArray3 = temporyDistance1;
				EEPROM.put(eeSessionTimeArray3Address, temporySession1);
				EEPROM.put(eeSessionDistanceArray3Address, temporyDistance1);
				EEPROM.commit();
				eeSessionChange = false;

				//Serial.println("Case 3 Triggered: ");
				//Serial.println("");

				break;

			case 4:

				EEPROM.get(eeSessionTimeArray4Address, temporySession0);
				temporySession1 = temporySession0 + sessionTimeArray[4];

				EEPROM.get(eeSessionDistanceArray4Address, temporyDistance0);
				temporyDistance1 = temporyDistance0 + distanceTravelledArray[4];

				sessionTimeArray4 = temporySession1 / 1000 / 60;
				distanceTravelledArray4 = temporyDistance1;
				EEPROM.put(eeSessionTimeArray4Address, temporySession1);
				EEPROM.put(eeSessionDistanceArray4Address, temporyDistance1);
				EEPROM.commit();
				eeSessionChange = false;

				//Serial.println("Case 4 Triggered: ");
				//Serial.println("");

				break;

			case 5:

				EEPROM.get(eeSessionTimeArray5Address, temporySession0);
				temporySession1 = temporySession0 + sessionTimeArray[5];

				EEPROM.get(eeSessionDistanceArray5Address, temporyDistance0);
				temporyDistance1 = temporyDistance0 + distanceTravelledArray[5];

				sessionTimeArray5 = temporySession1 / 1000 / 60;
				distanceTravelledArray5 = temporyDistance1;
				EEPROM.put(eeSessionTimeArray5Address, temporySession1);
				EEPROM.put(eeSessionDistanceArray5Address, temporyDistance1);
				EEPROM.commit();
				eeSessionChange = false;

				//Serial.println("Case 5 Triggered: ");
				//Serial.println("");

				break;

			case 6:

				EEPROM.get(eeSessionTimeArray6Address, temporySession0);
				temporySession1 = temporySession0 + sessionTimeArray[6];

				EEPROM.get(eeSessionDistanceArray6Address, temporyDistance0);
				temporyDistance1 = temporyDistance0 + distanceTravelledArray[6];

				sessionTimeArray6 = temporySession1 / 1000 / 60;
				distanceTravelledArray6 = temporyDistance1;
				EEPROM.put(eeSessionTimeArray6Address, temporySession1);
				EEPROM.put(eeSessionDistanceArray6Address, temporyDistance1);
				EEPROM.commit();
				eeSessionChange = false;

				//Serial.println("Case 6 Triggered: ");
				//Serial.println("");

				break;
			}

		}

		sessionTimeFlag = false;
		recordSessions = false;

		if (eeTotalDistanceChange == true) {

			EEPROM.put(eeTotalDistanceAddress, distanceC);
			EEPROM.commit();
			eeTotalDistanceChange = false;
		}

	}

	/* Configure speed varibales to the same format for screen layout using dtostrf.

	dtostrf(floatvar, StringLengthIncDecimalPoint, numVarsAfterDecimal, charbuf);

	where

		floatvar					float variable.
		StringLengthIncDecimalPoint	This is the length of the string that will be created.
		numVarsAfterDecimal			The number of digits after the deimal point to print.
		charbuf						The array to store the results */

	dtostrf(speedKph, 6, 2, kphArray);
	dtostrf(averageKphSpeed, 6, 2, averageKphSpeedArray);
	dtostrf(maxKphSpeed, 6, 2, maxKphArray);

	// float maxMphSpeed = maxKphSpeed * 0.621371;					// Commented out as no longer used.	// Max speed in Mph.
	float averageMphSpeed = averageKphSpeed * 0.621371;				// Average speed in Mph. 

	dtostrf(speedMph, 6, 2, mphArray);
	dtostrf(averageMphSpeed, 6, 2, averageMphSpeedArray);			// Commented out as no longer used.
	//dtostrf(maxMphSpeed, 6, 2, maxMphArray);						// Commented out as no longer used.

	dtostrf(sessionDistance, 6, 0, sessionDistanceArray);
	dtostrf(sessionTime, 6, 0, currentSessionTimeArray);

} // Close function.

/*-----------------------------------------------------------------*/

// Average speed calculation.

void averageSpeed() {

	// Subtract the last reading.

	total = total - readingsAV[readIndex];

	// Get latest Kph.

	readingsAV[readIndex] = speedKph;

	// Add the reading to the total.

	total = total + readingsAV[readIndex];

	// Advance to the next position in the average array.

	readIndex = readIndex + 1;

	// If we're at the end of the array.

	if (readIndex >= numReadings) {

		readIndex = 0;
	}

}  // Close function.

/*-----------------------------------------------------------------*/

// New Max Speed record update.

void newMaxSpeedRecord() {

	// Check and update new max speed record.

	if (disWiFi == false && newMaxSpeedF == true) {

		float tempMaxKphSpeed = 0.00;

		EEPROM.get(eeBestMaxSpeed, tempMaxKphSpeed);
		EEPROM.commit();

		if (tempMaxKphSpeed < maxKphSpeed) {

			EEPROM.put(eeBestMaxSpeed, maxKphSpeed);
			EEPROM.put(eeBestMaxSpeedDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestMaxSpeedDay, rtc.getDay());
			EEPROM.put(eeBestMaxSpeedMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestMaxSpeedYear, rtc.getYear());
			EEPROM.put(eeBestMaxSpeedHour, rtc.getHour(true));
			EEPROM.put(eeBestMaxSpeedMinute, rtc.getMinute());
			EEPROM.commit();

			/*float tempMaxSpeed;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestMaxSpeed, tempMaxSpeed);
			EEPROM.get(eeBestMaxSpeedDoW, tempDoW);
			EEPROM.get(eeBestMaxSpeedDay, tempDay);
			EEPROM.get(eeBestMaxSpeedMonth, tempMonth);
			EEPROM.get(eeBestMaxSpeedYear, tempYear);
			EEPROM.get(eeBestMaxSpeedHour, tempHour);
			EEPROM.get(eeBestMaxSpeedMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New max speed setting: ");
			Serial.println(tempMaxSpeed);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}

		newMaxSpeedF = false;
	}

} // Close function.

/*-----------------------------------------------------------------*/

// New best ever records update.

void updateBestEverRecords() {

	// Check and update new distance session record.

	if (disWiFi == false && newBestSessionDistanceF == true) {

		int tempDistanceSessionRecord;

		EEPROM.get(eeBestDistanceS, tempDistanceSessionRecord);
		EEPROM.commit();

		int tempSession = sessionDistance;								// SessionDistance variable must remain as a double for ESP32, hence conversion here.

		if (tempDistanceSessionRecord < tempSession) {

			EEPROM.put(eeBestDistanceS, tempSession);
			EEPROM.put(eeBestDistanceSDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestDistanceSDay, rtc.getDay());
			EEPROM.put(eeBestDistanceSMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestDistanceSYear, rtc.getYear());
			EEPROM.put(eeBestDistanceSHour, rtc.getHour(true));
			EEPROM.put(eeBestDistanceSMinute, rtc.getMinute());
			EEPROM.commit();

			/*int tempDistance;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestDistanceS, tempDistance);
			EEPROM.get(eeBestDistanceSDoW, tempDoW);
			EEPROM.get(eeBestDistanceSDay, tempDay);
			EEPROM.get(eeBestDistanceSMonth, tempMonth);
			EEPROM.get(eeBestDistanceSYear, tempYear);
			EEPROM.get(eeBestDistanceSHour, tempHour);
			EEPROM.get(eeBestDistanceSMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New max distance per session setting: ");
			Serial.println(tempDistance);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}
	}

	// Check and update new time session record.

	if (disWiFi == false && newBestSessionTimeF == true) {

		unsigned long tempTimeSessionRecord;

		EEPROM.get(eeBestTimeS, tempTimeSessionRecord);
		EEPROM.commit();

		if (tempTimeSessionRecord < sessionTimeMillis) {

			EEPROM.put(eeBestTimeS, sessionTimeMillis);
			EEPROM.put(eeBestTimeSDoW, rtc.getDayofWeek());
			EEPROM.put(eeBestTimeSDay, rtc.getDay());
			EEPROM.put(eeBestTimeSMonth, rtc.getMonth() + 1);			// Plus one as rtc.getMonth is ints 0 to 11.
			EEPROM.put(eeBestTimeSYear, rtc.getYear());
			EEPROM.put(eeBestTimeSHour, rtc.getHour(true));
			EEPROM.put(eeBestTimeSMinute, rtc.getMinute());
			EEPROM.commit();

			/*long tempTime;
			int tempDoW;
			int tempDay;
			int tempMonth;
			int tempYear;
			int tempHour;
			int tempMinute;

			EEPROM.get(eeBestTimeS, tempTime);
			EEPROM.get(eeBestTimeSDoW, tempDoW);
			EEPROM.get(eeBestTimeSDay, tempDay);
			EEPROM.get(eeBestTimeSMonth, tempMonth);
			EEPROM.get(eeBestTimeSYear, tempYear);
			EEPROM.get(eeBestTimeSHour, tempHour);
			EEPROM.get(eeBestTimeSMinute, tempMinute);
			EEPROM.commit();

			Serial.print("New max time per session setting: ");
			Serial.println(tempTime);
			Serial.print("Date: ");
			Serial.print(dayArray[tempDoW]);
			Serial.print(", ");
			Serial.print(tempDay);
			Serial.print("/");
			Serial.print(tempMonth);
			Serial.print("/");
			Serial.print(tempYear);
			Serial.print(" at ");
			Serial.print(tempHour);
			Serial.print(":");
			Serial.println(tempMinute);
			Serial.println(" ");*/
		}
	}

	newBestSessionDistanceF = false;
	newBestSessionTimeF = false;

} // Close function.

/*-----------------------------------------------------------------*/

// Current exercise display layout.

void currentExerciseScreen() {

	tft.setFreeFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(WHITE);
	tft.setCursor(13, 26);
	tft.print("Current Session");

	tft.setTextSize(1);
	tft.setTextColor(WHITE);

	tft.setCursor(23, 60);
	tft.print("Mph: ");
	tft.setCursor(23, 80);
	tft.print("Ave Mph: ");

	tft.setCursor(23, 110);
	tft.print("Kph: ");
	tft.setCursor(23, 130);
	tft.print("Ave Kph: ");

	tft.setCursor(23, 160);
	tft.print("Max Kph: ");
	tft.setCursor(23, 180);
	tft.print("Dis  (m):");
	tft.setCursor(23, 200);
	tft.print("Time (s): ");

	tft.setFreeFont();
	tft.setTextSize(2);
	tft.setTextColor(WHITE, BLACK);

	tft.setCursor(150, 47);
	tft.println(mphArray);

	tft.setCursor(150, 67);
	tft.println(averageMphSpeedArray);

	tft.setCursor(150, 97);
	tft.println(kphArray);

	tft.setCursor(150, 117);
	tft.println(averageKphSpeedArray);

	tft.setCursor(150, 147);
	tft.println(maxKphArray);

	tft.setCursor(150, 167);
	tft.println(sessionDistanceArray);

	tft.setCursor(150, 187);
	tft.println(currentSessionTimeArray);

	tft.setTextSize(1);

} // Close function.

/*-----------------------------------------------------------------*/

// Configuration display layout.

void configurationDisplay() {

	// Retrieve saved configuration data from EEPROM.

	EEPROM.get(eeMenuAddress, eeMenuSetting);
	EEPROM.get(eeCircAddress, eeCircSetting);
	EEPROM.get(eeResetSettingAddress, eeResetSetting);
	EEPROM.get(eeBuzzerYNAddress, buzzerYN);

	EEPROM.get(eegraphDMAddress, graphDM);										// Graph disrance scale.
	EEPROM.get(eegraphDMIAddress, graphDMI);
	EEPROM.get(eegraphDAPAddress, graphDAP);

	EEPROM.get(eegraphTMAddress, graphTM);										// Graph time scale.
	EEPROM.get(eegraphTMIAddress, graphTMI);
	EEPROM.get(eegraphTAPAddress, graphTAP);

	// Display configuration data and selection options.

	tft.setFreeFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(WHITE);
	tft.setCursor(13, 26);
	tft.println("System Setup");
	tft.setFreeFont();

	tft.setTextColor(WHITE, BLACK);

	// Menu option 1 is Start Screen menu.

	if (configurationFlag == 1) {

		tft.setTextColor(TFT_RED, BLACK);
	}

	else tft.setTextColor(WHITE, BLACK);

	tft.setCursor(23, 50);
	tft.print("Start Up Screen  : ");
	tft.setCursor(150, 50);
	tft.print(menuArray[eeMenuSetting]);

	// Menu option 2 is Circumference menu.

	if (configurationFlag == 2) {

		tft.setTextColor(TFT_RED, BLACK);
	}

	else tft.setTextColor(WHITE, BLACK);

	tft.setCursor(23, 65);
	tft.print("Circumference    : ");
	tft.setCursor(150, 65);
	tft.println(eeCircSetting);
	tft.println();

	// Menu option 3 is distance scale graph option menu.

	if (configurationFlag == 3) {

		tft.setTextColor(TFT_RED, BLACK);
	}

	else tft.setTextColor(WHITE, BLACK);

	if (graphDAP == 0) {					// Used to blank out figure from screen draw.

		tft.setCursor(167, 80);
		tft.print(" ");
	}

	tft.setCursor(23, 80);
	tft.print("Distance Scale   : ");
	tft.setCursor(150, 80);
	tft.print(graphDAM[graphDAP]);
	tft.println();

	// Menu option 4 is time scale graph option menu.

	if (configurationFlag == 4) {

		tft.setTextColor(TFT_RED, BLACK);
	}

	else tft.setTextColor(WHITE, BLACK);

	if (graphTAP == 0 || graphTAP == 8) {	// Used to blank out figure from screen draw.

		tft.setCursor(161, 95);
		tft.print(" ");
	}

	tft.setCursor(23, 95);
	tft.print("Time Scale       : ");
	tft.setCursor(150, 95);
	tft.print(graphTAM[graphTAP]);
	tft.println();

	// Menu option 5 is buzzer menu.

	if (configurationFlag == 5) {

		tft.setTextColor(TFT_RED, BLACK);
	}

	else tft.setTextColor(WHITE, BLACK);

	if (buzzerYN == false) {	// Used to blank out figure from screen draw.

		tft.setCursor(161, 110);
		tft.print(" ");
	}

	tft.setCursor(23, 110);
	tft.print("Touch Beep       : ");
	tft.setCursor(150, 110);
	tft.println(ynArray[buzzerYN]);
	tft.println();

	// Menu option 6 is WiFi reset menu.

	if (configurationFlag == 6) {

		tft.setTextColor(TFT_RED, BLACK);
	}

	else tft.setTextColor(WHITE, BLACK);

	if (wiFiYN == false) {	// Used to blank out figure from screen draw.

		tft.setCursor(161, 170);
		tft.print(" ");
	}

	tft.setCursor(23, 170);
	tft.print("WiFi Reset       : ");
	tft.setCursor(150, 170);
	tft.println(ynArray[wiFiYN]);
	tft.println();

	// Menu option 7 is System Reset menu.

	if (configurationFlag == 7) {

		tft.setTextColor(TFT_RED, BLACK);
	}

	else tft.setTextColor(WHITE, BLACK);

	tft.setCursor(23, 185);
	tft.print("System Reset     : ");
	tft.setCursor(150, 185);
	tft.println(resetArray[eeResetSetting]);

	// Menu option 8 is System Reset menu.

	if (configurationFlag == 8) {

		tft.fillRect(59, 81, 148, 74, RED);
		tft.drawRect(58, 80, 150, 76, TFT_WHITE);
		tft.drawRect(57, 79, 152, 78, TFT_WHITE);
		tft.setFreeFont(&FreeSans9pt7b);
		tft.setTextSize(1);
		tft.setTextColor(WHITE); tft.setCursor(72, 122);
		tft.print("Reset System?");

		delay(1000); // Give the screen a moment.

		while (configurationFlag == 8) {

			uint16_t x, y;		// variables for touch data.

			tft.getTouch(&x, &y);

			if ((x > BUTTON4_X) && (x < (BUTTON4_X + BUTTON4_W))) {
				if ((y > BUTTON4_Y) && (y <= (BUTTON4_Y + BUTTON4_H))) {

					tone(buzzerP, buzzerF);
					Serial.print("Yes Reset While Ran!");
					Serial.println(" ");
					ESP.restart();

				}

			}

			if ((x > BUTTON1_X) && (x < (BUTTON1_X + BUTTON1_W))) {
				if ((y > BUTTON1_Y) && (y <= (BUTTON1_Y + BUTTON1_H))) {

					tone(buzzerP, buzzerF);
					screenMenu = 5;
					menuChange = 1;
					screenR = 1;
					configurationFlag = 1;
					Serial.print("Button 5 hit ");
					Serial.print(" : Screen Menu: ");
					Serial.print(screenMenu);
					Serial.println(" ");
					tft.setFreeFont();
					delay(1000);

				}

			}

		}

	}

	// Display total distance travelled since initial start up.

	tft.setTextColor(WHITE, BLACK);
	tft.setCursor(23, 200);
	tft.print("Total Distance   : ");
	tft.setCursor(150, 200);
	tft.println(distanceTravelled = distanceC * circumference);

} // Close function.

/*-----------------------------------------------------------------*/

// Menu setting plus.

void menuSettingPlus() {

	// Incremental function to menu setting.

	if (eeMenuSetting == 5) {

		eeMenuSetting = 1;
		eeMenuSettingChange = true;
	}

	else
	{
		eeMenuSetting++;
		eeMenuSettingChange = true;
	}

	if (eeMenuSettingChange == true) {			// Write results to EEPROM to save.

		menuSettingSave();
	}

} // Close function.

	/*-----------------------------------------------------------------*/

// Menu setting minus.

void menuSettingMinus() {

	// Incremental function to menu setting.

	if (eeMenuSetting == 1) {

		eeMenuSetting = 5;
		eeMenuSettingChange = true;
	}

	else
	{
		eeMenuSetting--;
		eeMenuSettingChange = true;
	}

	if (eeMenuSettingChange == true) {			// Write results to EEPROM to save.

		menuSettingSave();
	}

}  // Close function. 

/*-----------------------------------------------------------------*/

// Save menu setting.

void menuSettingSave() {

	// Write menu setting to EEPROM.

	if (eeMenuSettingChange == true) {

		EEPROM.put(eeMenuAddress, eeMenuSetting);
		EEPROM.commit();
		eeMenuSettingChange = false;

		//Serial.print("Menu Setting: ");
		//Serial.print(eeMenuSetting);
		//Serial.println(" ");
	}

}  // Close function.

/*-----------------------------------------------------------------*/

// Graph distance scale menu plus.

void distanceScaleSettingPlus() {

	// Incremental function to menu setting.

	if (graphDAP == 3) {

		graphDAP = 0;
		graphDSC = true;
	}

	else
	{
		graphDAP++;
		graphDSC = true;
	}

	if (graphDSC == true) {			// Write results to EEPROM to save.

		distanceScaleSettingSave();
	}

} // Close function.

/*-----------------------------------------------------------------*/

// Graph distance scale menu plus.

void distanceScaleSettingMinus() {

	// Incremental function to menu setting.

	if (graphDAP == 0) {

		graphDAP = 3;
		graphDSC = true;
	}

	else
	{
		graphDAP--;
		graphDSC = true;
	}

	if (graphDSC == true) {			// Write results to EEPROM to save.

		distanceScaleSettingSave();
	}

} // Close function.

/*-----------------------------------------------------------------*/

// Save graph distance scale.

void distanceScaleSettingSave() {

	// Write menu setting to EEPROM.

	if (graphDSC == true) {

		graphDM = graphDAM[graphDAP];
		graphDMI = graphDAI[graphDAP];

		distanceGraphCap = graphDM;

		EEPROM.put(eegraphDMAddress, graphDM);
		EEPROM.put(eegraphDMIAddress, graphDMI);
		EEPROM.put(eegraphDAPAddress, graphDAP);
		EEPROM.commit();

		//Serial.print("Distance Scale: ");
		//Serial.print(graphDAM[graphDAP]);
		//Serial.print(" & ");
		//Serial.print("Increments: ");
		//Serial.print(graphDAI[graphDAP]);
		//Serial.print(" & ");
		//Serial.print("Position: ");
		//Serial.print(graphDAP);
		//Serial.println(" ");

		graphDSC = false;
	}

}  // Close function.

/*-----------------------------------------------------------------*/

// Graph time scale menu plus.

void timeScaleSettingPlus() {

	// Incremental function to menu setting.

	if (graphTAP == 11) {

		graphTAP = 0;
		graphTSC = true;
	}

	else
	{
		graphTAP++;
		graphTSC = true;
	}

	if (graphTSC == true) {			// Write results to EEPROM to save.

		timeScaleSettingSave();
	}

} // Close function.

/*-----------------------------------------------------------------*/

// Graph time scale menu minus.

void timeScaleSettingMinus() {

	// Decremental function to menu setting.

	if (graphTAP == 0) {

		graphTAP = 11;
		graphTSC = true;
	}

	else
	{
		graphTAP--;
		graphTSC = true;
	}

	if (graphTSC == true) {			// Write results to EEPROM to save.

		timeScaleSettingSave();
	}

} // Close function.

	/*-----------------------------------------------------------------*/

// Save graph time scale.

void timeScaleSettingSave() {

	// Write menu setting to EEPROM.

	if (graphTSC == true) {

		graphTM = graphTAM[graphTAP];
		graphTMI = graphTAI[graphTAP];

		sessionTimeCap = graphTM;

		EEPROM.put(eegraphTMAddress, graphTM);
		EEPROM.put(eegraphTMIAddress, graphTMI);
		EEPROM.put(eegraphTAPAddress, graphTAP);
		EEPROM.commit();

		//Serial.print("Time Scale: ");
		//Serial.print(graphTAM[graphTAP]);
		//Serial.print(" & ");
		//Serial.print("Increments: ");
		//Serial.print(graphTAI[graphTAP]);
		//Serial.print(" & ");
		//Serial.print("Position: ");
		//Serial.print(graphTAP);
		//Serial.println(" ");

		graphTSC = false;
	}

}  // Close function.

/*-----------------------------------------------------------------*/

// Buzzer setting plus changes.

void buzzerSettingPlus() {

	// Y/N function change to buzzer setting.

	if (buzzerYN == false)
	{
		buzzerYN = true;
		eeBuzzerYNChange = true;

	}

	if (eeBuzzerYNChange == true) {			// Write results to EEPROM to save.

		buzzerSettingSave();
	}

} // Close function.

/*-----------------------------------------------------------------*/

// Buzzer setting minus changes.

void buzzerSettingMinus() {

	// Y/N function change to buzzer setting.

	if (buzzerYN == true)
	{
		buzzerYN = false;
		eeBuzzerYNChange = true;

	}

	if (eeBuzzerYNChange == true) {			// Write results to EEPROM to save.

		buzzerSettingSave();
	}

}  // Close function. 

/*-----------------------------------------------------------------*/

// Buzzer setting save changes.

void buzzerSettingSave() {

	// Write buzzer setting to EEPROM.

	if (eeBuzzerYNChange == true) {

		EEPROM.put(eeBuzzerYNAddress, buzzerYN);
		EEPROM.commit();
		eeBuzzerYNChange = false;

		//Serial.print("Buzzer Enabled: ");
		//Serial.print(buzzerYN);
		//Serial.println(" ");
	}

}  // Close function.

/*-----------------------------------------------------------------*/

// WiFi setting plus changes.

void wiFiSettingPlus() {

	// Y/N function change to WiFi setting.

	if (wiFiYN == false) {

		wiFiYN = true;
		eeWiFiYNChange = true;
	}

	if (eeWiFiYNChange == true) {			// Write results to EEPROM to save.

		wiFiSettingSave();
	}

} // Close function.

/*-----------------------------------------------------------------*/

// WiFi setting minus changes.

void wiFiSettingMinus() {

	// Y/N function change to WiFi setting.

	if (wiFiYN == true) {

		wiFiYN = false;
		eeWiFiYNChange = true;
	}

	if (eeWiFiYNChange == true) {			// Write results to EEPROM to save.

		wiFiSettingSave();
	}

}  // Close function. 

/*-----------------------------------------------------------------*/

// WiFi setting save changes.

void wiFiSettingSave() {

	// Write WiFi setting to EEPROM.

	if (eeWiFiYNChange == true) {

		EEPROM.put(eeWiFiYNAddress, wiFiYN);
		EEPROM.commit();
		eeWiFiYNChange = false;

		//Serial.print("WiFi Reset: ");
		//Serial.print(wiFiYN);
		//Serial.println(" ");
	}

}  // Close function.

/*-----------------------------------------------------------------*/

// Reset setting plus changes.

void resetMenuSettingPlus() {

	// Incremental function to reset setting.

	if (eeResetSetting == 3) {

		eeResetSetting = 0;
		eeResetSettingChange = true;
	}

	else
	{
		eeResetSetting++;
		eeResetSettingChange = true;
	}

	if (eeResetSettingChange == true) {			// Write results to EEPROM to save.

		resetMenuSettingSave();
	}

} // Close function.

/*-----------------------------------------------------------------*/

// Reset setting plus changes.

void resetMenuSettingMinus() {

	// Decremental function to reset setting.

	if (eeResetSetting == 0) {

		eeResetSetting = 3;
		eeResetSettingChange = true;
	}

	else
	{
		eeResetSetting--;
		eeResetSettingChange = true;
	}

	if (eeResetSettingChange == true) {			// Write results to EEPROM to save.

		resetMenuSettingSave();
	}

} // Close function.
/*-----------------------------------------------------------------*/

// Reset setting save changes.

void resetMenuSettingSave() {

	// Write reset setting to EEPROM.

	if (eeResetSettingChange == true) {

		EEPROM.put(eeResetSettingAddress, eeResetSetting);
		EEPROM.commit();
		eeResetSettingChange = false;

		//Serial.print("Reset Setting: ");
		//Serial.print(eeResetSetting);
		//Serial.println(" ");
	}

} // Close function.

/*-----------------------------------------------------------------*/

// Circumference setting plus changes.

void circumferenceSettingPlus() {

	// Incremental function to circumference setting.

	if (eeCircSetting >= 2.00) {

		eeCircSetting = 0.01;
		eeCircSettingChange = true;
	}

	else
	{
		eeCircSetting = eeCircSetting + 0.01;
		eeCircSettingChange = true;
	}

	if (eeCircSettingChange == true) {			// Write results to EEPROM to save.

		circumferenceSettingSave();

	}

}  // Close function.

/*-----------------------------------------------------------------*/

// Circumference setting minus changes.

void circumferenceSettingMinus() {

	// Decremental function to circumference setting.

	if (eeCircSetting <= 0.01) {

		eeCircSetting = 2.00;
		eeCircSettingChange = true;
	}

	else
	{
		eeCircSetting = eeCircSetting - 0.01;
		eeCircSettingChange = true;
	}

	if (eeCircSettingChange == true) {			// Write results to EEPROM to save.

		circumferenceSettingSave();

	}

} // Close function.

/*-----------------------------------------------------------------*/

// Circumference setting save changes.

void circumferenceSettingSave() {

	// Write circumference setting to EEPROM.

	if (eeCircSettingChange == true) {

		EEPROM.put(eeCircAddress, eeCircSetting);
		EEPROM.commit();
		eeCircSettingChange = false;

		//Serial.print("Circumference Setting: ");
		//Serial.print(eeCircSetting);
		//Serial.println(" ");
	}

}  // Close function.

/*-----------------------------------------------------------------*/

// Draw borders.

void drawBorder() {

	// Draw layout borders.

	tft.drawRect(FRAME1_X, FRAME1_Y, FRAME1_W, FRAME1_H, TFT_WHITE);
	tft.drawRect(FRAME2_X, FRAME2_Y, FRAME2_W, FRAME2_H, TFT_WHITE);

} // Close function.

/*-----------------------------------------------------------------*/

// Draw black boxes when screens change.

void drawBlackBox() {

	// Clear screen by using a black box.

	tft.fillRect(FRAME2_X + 1, FRAME2_Y + 25, FRAME2_W - 2, FRAME2_H - 40, TFT_BLACK);		// This covers only the graphs and charts, not the system icons to save refresh flicker.
	tft.fillRect(FRAME2_X + 1, FRAME2_Y + 1, FRAME2_W - 90, FRAME2_H - 200, TFT_BLACK);		// Ths covers the title text per page

} // Close function.

/*-----------------------------------------------------------------*/

// Draw sensor circle.

void drawSensor() {

	// Draw sensor icon.

	tft.drawCircle(SENSOR_ICON_X, SENSOR_ICON_Y, SENSOR_ICON_R, TFT_WHITE);

	if (sensorT == true) {

		tft.fillCircle(SENSOR_ICON_X, SENSOR_ICON_Y, SENSOR_ICON_R - 1, TFT_GREEN);
	}

	else tft.fillCircle(SENSOR_ICON_X, SENSOR_ICON_Y, SENSOR_ICON_R - 1, TFT_BLACK);

} // Close function.

/*-----------------------------------------------------------------*/

// Update menu flag.

void menu_Change() {

	menuChange = 0;

} // Close function.

/*-----------------------------------------------------------------*/

// WiFi title page.

void wiFiTitle() {

	// WiFi title screen.

	tft.setFreeFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(WHITE);
	tft.setCursor(13, 26);
	tft.println("Setting up WiFi");
	tft.setFreeFont();

	tft.setTextColor(WHITE);
	tft.setFreeFont();
	tft.setCursor(23, 50);
	tft.print("WiFi Status: ");
	tft.setCursor(23, 65);
	tft.print("SSID: ");
	tft.setCursor(23, 80);
	tft.print("IP Address: ");
	tft.setCursor(23, 95);
	tft.print("DNS Address: ");
	tft.setCursor(23, 110);
	tft.print("Gateway Address: ");
	tft.setCursor(23, 125);
	tft.print("Signal Strenght: ");
	tft.setCursor(23, 140);
	tft.print("Time Server: ");

} // Close function.

/*-----------------------------------------------------------------*/

// Reset all data and parametres back to factory defaults (LOL).

void resetSystemData() {

	eeMenuSetting = 2;															// Default menu setting.
	EEPROM.put(eeMenuAddress, eeMenuSetting);
	EEPROM.commit();

	eeCircSetting = 1.00;														// Curcumference.
	EEPROM.put(eeCircAddress, eeCircSetting);
	EEPROM.commit();

	graphDM = 1000;																// Graph distance scale.
	graphDMI = 200;
	graphDAP = 1;
	EEPROM.put(eegraphDMAddress, graphDM);
	EEPROM.put(eegraphDMIAddress, graphDMI);
	EEPROM.put(eegraphDAPAddress, graphDAP);
	EEPROM.commit();

	graphTM = 60;																// Graph time scale.
	graphTMI = 6;
	graphTAP = 5;
	EEPROM.put(eegraphTMAddress, graphTM);
	EEPROM.put(eegraphTMIAddress, graphTMI);
	EEPROM.put(eegraphTAPAddress, graphTAP);
	EEPROM.commit();

	buzzerYN = 1;																// Buzzer flag.
	EEPROM.put(eeBuzzerYNAddress, buzzerYN);
	EEPROM.commit();

	wiFiYN = 0;																	// WiFi reset flag.
	EEPROM.put(eeWiFiYNAddress, wiFiYN);
	EEPROM.commit();

	calTouchScreen = 1;															// Calibration flag.
	EEPROM.put(eeCalYNAddress, calTouchScreen);
	EEPROM.commit();

	eeTotalDistance = 0.00;														// Total distance travelled.
	EEPROM.put(eeTotalDistanceAddress, eeTotalDistance);
	EEPROM.commit();

	EEPROM.put(eeSessionTimeArray0Address, 0);									// Populate arrays with zero data.
	EEPROM.put(eeSessionTimeArray1Address, 0);
	EEPROM.put(eeSessionTimeArray2Address, 0);
	EEPROM.put(eeSessionTimeArray3Address, 0);
	EEPROM.put(eeSessionTimeArray4Address, 0);
	EEPROM.put(eeSessionTimeArray5Address, 0);
	EEPROM.put(eeSessionTimeArray6Address, 0);
	EEPROM.commit();

	EEPROM.put(eeSessionDistanceArray0Address, 0);
	EEPROM.put(eeSessionDistanceArray1Address, 0);
	EEPROM.put(eeSessionDistanceArray2Address, 0);
	EEPROM.put(eeSessionDistanceArray3Address, 0);
	EEPROM.put(eeSessionDistanceArray4Address, 0);
	EEPROM.put(eeSessionDistanceArray5Address, 0);
	EEPROM.put(eeSessionDistanceArray6Address, 0);
	EEPROM.commit();

	EEPROM.put(eeBestMaxSpeed, 0);												// EEPRPOM address for best ever max speed recording.
	EEPROM.put(eeBestMaxSpeedMinute, 0);
	EEPROM.put(eeBestMaxSpeedHour, 0);
	EEPROM.put(eeBestMaxSpeedDoW, 5);
	EEPROM.put(eeBestMaxSpeedDay, 1);
	EEPROM.put(eeBestMaxSpeedMonth, 1);
	EEPROM.put(eeBestMaxSpeedYear, 2021);

	EEPROM.put(eeBestDistanceS, 0);												// EEPRPOM address for best ever distance per session recording.
	EEPROM.put(eeBestDistanceSMinute, 0);
	EEPROM.put(eeBestDistanceSHour, 0);
	EEPROM.put(eeBestDistanceSDoW, 5);
	EEPROM.put(eeBestDistanceSDay, 1);
	EEPROM.put(eeBestDistanceSMonth, 1);
	EEPROM.put(eeBestDistanceSYear, 2021);

	EEPROM.put(eeBestDistanceD, 0);												// EEPRPOM address for best ever distance per day recording.
	EEPROM.put(eeBestDistanceDMinute, 0);
	EEPROM.put(eeBestDistanceDHour, 0);
	EEPROM.put(eeBestDistanceDDoW, 5);
	EEPROM.put(eeBestDistanceDDay, 1);
	EEPROM.put(eeBestDistanceDMonth, 1);
	EEPROM.put(eeBestDistanceDYear, 2021);

	EEPROM.put(eeBestTimeS, 0);													// EEPRPOM address for best ever time per session recording.
	EEPROM.put(eeBestTimeSMinute, 0);
	EEPROM.put(eeBestTimeSHour, 0);
	EEPROM.put(eeBestTimeSDoW, 5);
	EEPROM.put(eeBestTimeSDay, 1);
	EEPROM.put(eeBestTimeSMonth, 1);
	EEPROM.put(eeBestTimeSYear, 2021);

	EEPROM.put(eeBestTimeD, 0);													// EEPRPOM address for best ever time per day recording.
	EEPROM.put(eeBestTimeDMinute, 0);
	EEPROM.put(eeBestTimeDHour, 0);
	EEPROM.put(eeBestTimeDDoW, 5);
	EEPROM.put(eeBestTimeDDay, 1);
	EEPROM.put(eeBestTimeDMonth, 1);
	EEPROM.put(eeBestTimeDYear, 2021);

	EEPROM.get(eeMenuAddress, eeMenuSetting);									// Load data from EEPROM.
	EEPROM.get(eeCircAddress, eeCircSetting);
	EEPROM.get(eeMenuAddress, screenMenu);
	EEPROM.get(eeCircAddress, circumference);
	EEPROM.get(eeTotalDistanceAddress, distanceC);

	EEPROM.get(eegraphDMAddress, graphDM);										// Graph disrance scale.
	EEPROM.get(eegraphDMIAddress, graphDMI);
	EEPROM.get(eegraphDAPAddress, graphDAP);

	EEPROM.get(eegraphTMAddress, graphTM);										// Graph time scale.
	EEPROM.get(eegraphTMIAddress, graphTMI);
	EEPROM.get(eegraphTAPAddress, graphTAP);

	EEPROM.commit();

	EEPROM.get(eeSessionTimeArray0Address, sessionTimeArray[0]);
	EEPROM.get(eeSessionTimeArray1Address, sessionTimeArray[1]);
	EEPROM.get(eeSessionTimeArray2Address, sessionTimeArray[2]);
	EEPROM.get(eeSessionTimeArray3Address, sessionTimeArray[3]);
	EEPROM.get(eeSessionTimeArray4Address, sessionTimeArray[4]);
	EEPROM.get(eeSessionTimeArray5Address, sessionTimeArray[5]);
	EEPROM.get(eeSessionTimeArray6Address, sessionTimeArray[6]);

	EEPROM.commit();

	sessionTimeArray0 = sessionTimeArray[0] / 1000 / 60;						// Times to be updated, needs to be divided by 1000.
	sessionTimeArray1 = sessionTimeArray[1] / 1000 / 60;
	sessionTimeArray2 = sessionTimeArray[2] / 1000 / 60;
	sessionTimeArray3 = sessionTimeArray[3] / 1000 / 60;
	sessionTimeArray4 = sessionTimeArray[4] / 1000 / 60;
	sessionTimeArray5 = sessionTimeArray[5] / 1000 / 60;
	sessionTimeArray6 = sessionTimeArray[6] / 1000 / 60;

	EEPROM.get(eeSessionDistanceArray0Address, distanceTravelledArray[0]);
	EEPROM.get(eeSessionDistanceArray1Address, distanceTravelledArray[1]);
	EEPROM.get(eeSessionDistanceArray2Address, distanceTravelledArray[2]);
	EEPROM.get(eeSessionDistanceArray3Address, distanceTravelledArray[3]);
	EEPROM.get(eeSessionDistanceArray4Address, distanceTravelledArray[4]);
	EEPROM.get(eeSessionDistanceArray5Address, distanceTravelledArray[5]);
	EEPROM.get(eeSessionDistanceArray6Address, distanceTravelledArray[6]);

	EEPROM.commit();

	distanceTravelledArray0 = distanceTravelledArray[0];
	distanceTravelledArray1 = distanceTravelledArray[1];
	distanceTravelledArray2 = distanceTravelledArray[2];
	distanceTravelledArray3 = distanceTravelledArray[3];
	distanceTravelledArray4 = distanceTravelledArray[4];
	distanceTravelledArray5 = distanceTravelledArray[5];
	distanceTravelledArray6 = distanceTravelledArray[6];

	Serial.println("Output from reset system data after EEPROM get");
	Serial.print("Distance Scale: ");
	Serial.print(graphDM);
	Serial.print(" & ");
	Serial.print("Increments: ");
	Serial.print(graphDMI);
	Serial.print(" & ");
	Serial.print("Position: ");
	Serial.print(graphDAP);
	Serial.println(" ");
	Serial.print("Time Scale: ");
	Serial.print(graphTM);
	Serial.print(" & ");
	Serial.print("Increments: ");
	Serial.print(graphTMI);
	Serial.print(" & ");
	Serial.print("Position: ");
	Serial.println(graphTAP);
	Serial.println(" ");

	eeResetSetting = 0;															// Reset EEPROM reset back to zero.
	EEPROM.put(eeResetSettingAddress, 0);
	EEPROM.commit();

} // Close function.

/*-----------------------------------------------------------------*/

// Load demo data.

void resetSystemDemoData() {

	eeMenuSetting = 2;															// Default menu setting.
	EEPROM.put(eeMenuAddress, eeMenuSetting);
	EEPROM.commit();

	eeCircSetting = 1.00;														// Curcumference.
	EEPROM.put(eeCircAddress, eeCircSetting);
	EEPROM.commit();

	graphDM = 1000;																// Graph distance scale.
	graphDMI = 200;
	graphDAP = 1;
	EEPROM.put(eegraphDMAddress, graphDM);
	EEPROM.put(eegraphDMIAddress, graphDMI);
	EEPROM.put(eegraphDAPAddress, graphDAP);
	EEPROM.commit();

	graphTM = 60;																// Graph time scale.
	graphTMI = 6;
	graphTAP = 5;
	EEPROM.put(eegraphTMAddress, graphTM);
	EEPROM.put(eegraphTMIAddress, graphTMI);
	EEPROM.put(eegraphTAPAddress, graphTAP);
	EEPROM.commit();

	buzzerYN = 1;																// Buzzer flag.
	EEPROM.put(eeBuzzerYNAddress, buzzerYN);
	EEPROM.commit();

	wiFiYN = 0;																	// WiFi reset flag.
	EEPROM.put(eeWiFiYNAddress, wiFiYN);
	EEPROM.commit();

	eeTotalDistance = 5000.00;													// Total distance travelled.
	EEPROM.put(eeTotalDistanceAddress, eeTotalDistance);
	EEPROM.commit();

	EEPROM.put(eeSessionTimeArray0Address, 0);									// Populate arrays with demo data.
	EEPROM.put(eeSessionTimeArray1Address, 100000);
	EEPROM.put(eeSessionTimeArray2Address, 200000);
	EEPROM.put(eeSessionTimeArray3Address, 300000);
	EEPROM.put(eeSessionTimeArray4Address, 400000);
	EEPROM.put(eeSessionTimeArray5Address, 500000);
	EEPROM.put(eeSessionTimeArray6Address, 600000);
	EEPROM.commit();

	EEPROM.put(eeSessionDistanceArray0Address, 0);
	EEPROM.put(eeSessionDistanceArray1Address, 200);
	EEPROM.put(eeSessionDistanceArray2Address, 400);
	EEPROM.put(eeSessionDistanceArray3Address, 600);
	EEPROM.put(eeSessionDistanceArray4Address, 800);
	EEPROM.put(eeSessionDistanceArray5Address, 900);
	EEPROM.put(eeSessionDistanceArray6Address, 1200);
	EEPROM.commit();

	EEPROM.put(eeBestMaxSpeed, 0);												// EEPRPOM address for best ever max speed recording.
	EEPROM.put(eeBestMaxSpeedMinute, 0);
	EEPROM.put(eeBestMaxSpeedHour, 0);
	EEPROM.put(eeBestMaxSpeedDoW, 0);
	EEPROM.put(eeBestMaxSpeedDay, 0);
	EEPROM.put(eeBestMaxSpeedMonth, 0);
	EEPROM.put(eeBestMaxSpeedYear, 0);

	EEPROM.put(eeBestDistanceS, 0);											// EEPRPOM address for best ever distance per session recording.
	EEPROM.put(eeBestDistanceSMinute, 0);
	EEPROM.put(eeBestDistanceSHour, 0);
	EEPROM.put(eeBestDistanceSDoW, 0);
	EEPROM.put(eeBestDistanceSDay, 0);
	EEPROM.put(eeBestDistanceSMonth, 0);
	EEPROM.put(eeBestDistanceSYear, 0);

	EEPROM.put(eeBestDistanceD, 0);											// EEPRPOM address for best ever distance per day recording.
	EEPROM.put(eeBestDistanceDMinute, 0);
	EEPROM.put(eeBestDistanceDHour, 0);
	EEPROM.put(eeBestDistanceDDoW, 0);
	EEPROM.put(eeBestDistanceDDay, 0);
	EEPROM.put(eeBestDistanceDMonth, 0);
	EEPROM.put(eeBestDistanceDYear, 0);

	EEPROM.put(eeBestTimeS, 0);												// EEPRPOM address for best ever time per session recording.
	EEPROM.put(eeBestTimeSMinute, 0);
	EEPROM.put(eeBestTimeSHour, 0);
	EEPROM.put(eeBestTimeSDoW, 0);
	EEPROM.put(eeBestTimeSDay, 0);
	EEPROM.put(eeBestTimeSMonth, 0);
	EEPROM.put(eeBestTimeSYear, 0);

	EEPROM.put(eeBestTimeD, 0);											// EEPRPOM address for best ever time per day recording.
	EEPROM.put(eeBestTimeDMinute, 0);
	EEPROM.put(eeBestTimeDHour, 0);
	EEPROM.put(eeBestTimeDDoW, 0);
	EEPROM.put(eeBestTimeDDay, 0);
	EEPROM.put(eeBestTimeDMonth, 0);
	EEPROM.put(eeBestTimeDYear, 0);

	EEPROM.get(eeMenuAddress, eeMenuSetting);									// Load data from EEPROM.
	EEPROM.get(eeCircAddress, eeCircSetting);
	EEPROM.get(eeMenuAddress, screenMenu);
	EEPROM.get(eeCircAddress, circumference);
	EEPROM.get(eeTotalDistanceAddress, distanceC);

	EEPROM.get(eegraphDMAddress, graphDM);										// Graph disrance scale.
	EEPROM.get(eegraphDMIAddress, graphDMI);
	EEPROM.get(eegraphDAPAddress, graphDAP);

	EEPROM.get(eegraphTMAddress, graphTM);										// Graph time scale.
	EEPROM.get(eegraphTMIAddress, graphTMI);
	EEPROM.get(eegraphTAPAddress, graphTAP);

	EEPROM.commit();

	EEPROM.get(eeSessionTimeArray0Address, sessionTimeArray[0]);
	EEPROM.get(eeSessionTimeArray1Address, sessionTimeArray[1]);
	EEPROM.get(eeSessionTimeArray2Address, sessionTimeArray[2]);
	EEPROM.get(eeSessionTimeArray3Address, sessionTimeArray[3]);
	EEPROM.get(eeSessionTimeArray4Address, sessionTimeArray[4]);
	EEPROM.get(eeSessionTimeArray5Address, sessionTimeArray[5]);
	EEPROM.get(eeSessionTimeArray6Address, sessionTimeArray[6]);

	EEPROM.commit();

	sessionTimeArray0 = sessionTimeArray[0] / 1000 / 60;						// Times to be updated, needs to be divided by 1000.
	sessionTimeArray1 = sessionTimeArray[1] / 1000 / 60;
	sessionTimeArray2 = sessionTimeArray[2] / 1000 / 60;
	sessionTimeArray3 = sessionTimeArray[3] / 1000 / 60;
	sessionTimeArray4 = sessionTimeArray[4] / 1000 / 60;
	sessionTimeArray5 = sessionTimeArray[5] / 1000 / 60;
	sessionTimeArray6 = sessionTimeArray[6] / 1000 / 60;

	EEPROM.get(eeSessionDistanceArray0Address, distanceTravelledArray[0]);
	EEPROM.get(eeSessionDistanceArray1Address, distanceTravelledArray[1]);
	EEPROM.get(eeSessionDistanceArray2Address, distanceTravelledArray[2]);
	EEPROM.get(eeSessionDistanceArray3Address, distanceTravelledArray[3]);
	EEPROM.get(eeSessionDistanceArray4Address, distanceTravelledArray[4]);
	EEPROM.get(eeSessionDistanceArray5Address, distanceTravelledArray[5]);
	EEPROM.get(eeSessionDistanceArray6Address, distanceTravelledArray[6]);

	EEPROM.commit();

	distanceTravelledArray0 = distanceTravelledArray[0];
	distanceTravelledArray1 = distanceTravelledArray[1];
	distanceTravelledArray2 = distanceTravelledArray[2];
	distanceTravelledArray3 = distanceTravelledArray[3];
	distanceTravelledArray4 = distanceTravelledArray[4];
	distanceTravelledArray5 = distanceTravelledArray[5];
	distanceTravelledArray6 = distanceTravelledArray[6];

	Serial.println("Output from reset system data after EEPROM get");
	Serial.print("Distance Scale: ");
	Serial.print(graphDM);
	Serial.print(" & ");
	Serial.print("Increments: ");
	Serial.print(graphDMI);
	Serial.print(" & ");
	Serial.print("Position: ");
	Serial.print(graphDAP);
	Serial.println(" ");
	Serial.print("Time Scale: ");
	Serial.print(graphTM);
	Serial.print(" & ");
	Serial.print("Increments: ");
	Serial.print(graphTMI);
	Serial.print(" & ");
	Serial.print("Position: ");
	Serial.println(graphTAP);
	Serial.println(" ");

	eeResetSetting = 0;															// Reset EEPROM reset back to zero.
	EEPROM.put(eeResetSettingAddress, 0);
	EEPROM.commit();

} // Close function.

/*-----------------------------------------------------------------*/

void resetRecordsData() {

	eeMenuSetting = 2;														// Default menu setting.
	EEPROM.put(eeMenuAddress, eeMenuSetting);
	EEPROM.commit();

	EEPROM.put(eeSessionTimeArray0Address, 0);								// Blank arrays.
	EEPROM.put(eeSessionTimeArray1Address, 0);
	EEPROM.put(eeSessionTimeArray2Address, 0);
	EEPROM.put(eeSessionTimeArray3Address, 0);
	EEPROM.put(eeSessionTimeArray4Address, 0);
	EEPROM.put(eeSessionTimeArray5Address, 0);
	EEPROM.put(eeSessionTimeArray6Address, 0);
	EEPROM.commit();

	EEPROM.put(eeSessionDistanceArray0Address, 0);
	EEPROM.put(eeSessionDistanceArray1Address, 0);
	EEPROM.put(eeSessionDistanceArray2Address, 0);
	EEPROM.put(eeSessionDistanceArray3Address, 0);
	EEPROM.put(eeSessionDistanceArray4Address, 0);
	EEPROM.put(eeSessionDistanceArray5Address, 0);
	EEPROM.put(eeSessionDistanceArray6Address, 0);
	EEPROM.commit();

	EEPROM.put(eeBestMaxSpeed, 0);											// EEPRPOM address for best ever max speed recording.
	EEPROM.put(eeBestMaxSpeedMinute, 0);
	EEPROM.put(eeBestMaxSpeedHour, 0);
	EEPROM.put(eeBestMaxSpeedDoW, 5);
	EEPROM.put(eeBestMaxSpeedDay, 1);
	EEPROM.put(eeBestMaxSpeedMonth, 1);
	EEPROM.put(eeBestMaxSpeedYear, 2021);

	EEPROM.put(eeBestDistanceS, 0);											// EEPRPOM address for best ever distance per session recording.
	EEPROM.put(eeBestDistanceSMinute, 0);
	EEPROM.put(eeBestDistanceSHour, 0);
	EEPROM.put(eeBestDistanceSDoW, 5);
	EEPROM.put(eeBestDistanceSDay, 1);
	EEPROM.put(eeBestDistanceSMonth, 1);
	EEPROM.put(eeBestDistanceSYear, 2021);

	EEPROM.put(eeBestDistanceD, 0);											// EEPRPOM address for best ever distance per day recording.
	EEPROM.put(eeBestDistanceDMinute, 0);
	EEPROM.put(eeBestDistanceDHour, 0);
	EEPROM.put(eeBestDistanceDDoW, 5);
	EEPROM.put(eeBestDistanceDDay, 1);
	EEPROM.put(eeBestDistanceDMonth, 1);
	EEPROM.put(eeBestDistanceDYear, 2021);

	EEPROM.put(eeBestTimeS, 0);												// EEPRPOM address for best ever time per session recording.
	EEPROM.put(eeBestTimeSMinute, 0);
	EEPROM.put(eeBestTimeSHour, 0);
	EEPROM.put(eeBestTimeSDoW, 5);
	EEPROM.put(eeBestTimeSDay, 1);
	EEPROM.put(eeBestTimeSMonth, 1);
	EEPROM.put(eeBestTimeSYear, 2021);

	EEPROM.put(eeBestTimeD, 0);												// EEPRPOM address for best ever time per day recording.
	EEPROM.put(eeBestTimeDMinute, 0);
	EEPROM.put(eeBestTimeDHour, 0);
	EEPROM.put(eeBestTimeDDoW, 5);
	EEPROM.put(eeBestTimeDDay, 1);
	EEPROM.put(eeBestTimeDMonth, 1);
	EEPROM.put(eeBestTimeDYear, 2021);

	eeResetSetting = 0;														// Reset EEPROM reset back to zero.
	EEPROM.put(eeResetSettingAddress, 0);
	EEPROM.commit();

} // Close function.

/*-----------------------------------------------------------------*/

