// 
// 
// 

#include <TFT_eSPI.h>
#include <EEPROM.h>					// EEPROM library.
#include "colours.h"
#include "Free_Fonts.h"				// Free fonts for use with eTFT.
#include "touchCalibrate.h"

/*-----------------------------------------------------------------*/

// Code to run a screen calibration, not needed when calibration values set in setup()

void touch_calibrate(TFT_eSPI& tft) {

    uint16_t calData[5];						// Touch screen calibration data.
    uint8_t calDataOK = 0;

    int eeCalYNAddress = 204;					// EEPROM address for touch screen calibration enabled disabled.
    int eeCalDataAddress0 = 208;				// EEPROM address for touch screen calibration data.
    int eeCalDataAddress1 = 212;				// EEPROM address for touch screen calibration data.
    int eeCalDataAddress2 = 216;				// EEPROM address for touch screen calibration data.
    int eeCalDataAddress3 = 220;				// EEPROM address for touch screen calibration data.
    int eeCalDataAddress4 = 224;				// EEPROM address for touch screen calibration data.

    // Calibrate.

    tft.fillScreen(TFT_BLACK);

    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(60, 105);
    tft.println("Touch Screen Calibration");
    tft.setFreeFont();
    
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(80, 115);
    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    tft.calibrateTouch(calData, TFT_WHITE, TFT_BLACK, 15);

    for (uint8_t i = 0; i < 5; i++)
    {
        Serial.print(calData[i]);
        if (i < 4) Serial.print(", ");
    }

    Serial.println(" };");
    Serial.print("  tft.setTouch(calData);");
    Serial.println(); Serial.println();

    tft.fillScreen(TFT_BLACK);

    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(70, 105);
    tft.println("Calibration Complete");
    //tft.setFreeFont();

    delay(500);
    
    EEPROM.put(eeCalDataAddress0, calData[0]);      // Update calibration data array into EEPROM.
    EEPROM.put(eeCalDataAddress1, calData[1]);
    EEPROM.put(eeCalDataAddress2, calData[2]);
    EEPROM.put(eeCalDataAddress3, calData[3]);
    EEPROM.put(eeCalDataAddress4, calData[4]);
    EEPROM.put(eeCalYNAddress, 0);                  // Set calibration flaf back to false.

    EEPROM.commit();

    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(70, 115);
    tft.println("Writing settings to EEPROM");   
    delay(1000);

    tft.setCursor(130, 135);
    tft.println("Success!");
    delay(3000);
    
    tft.fillScreen(ILI9341_BLACK);

} // Close function.

/*-----------------------------------------------------------------*/