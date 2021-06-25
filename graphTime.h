// graphTime.h

#ifndef _GRAPHTIME_h
#define _GRAPHTIME_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

void ptSessionTimeV1(TFT_eSPI& tft, double x, double y, double w, double h, double loval, double hival, double inc, double curval, int dig, int dec, unsigned int barcolor, unsigned int voidcolor, unsigned int bordercolor, unsigned int textcolor, unsigned int backcolor, String label, boolean& redraw);

void ptSessionTimeV2(TFT_eSPI& tft, double x, double y, double w, double h, double loval, double hival, double inc, double curval, int dig, int dec, unsigned int barcolor, unsigned int voidcolor, unsigned int bordercolor, unsigned int textcolor, unsigned int backcolor, String label, boolean& redraw);

void ptSessionTimeV3(TFT_eSPI& tft, double x, double y, double w, double h, double loval, double hival, double inc, double curval, int dig, int dec, unsigned int barcolor, unsigned int voidcolor, unsigned int bordercolor, unsigned int textcolor, unsigned int backcolor, String label, boolean& redraw);

#endif

