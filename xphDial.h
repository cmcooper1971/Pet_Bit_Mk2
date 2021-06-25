// xphDial.h


#ifndef _XPHDIAL_h
#define _XPHDIAL_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

void XphDialScreen(TFT_eSPI& tft, int cx, int cy, int r, int loval, double hival, double inc, double sa, double curval, int dig, int dec, unsigned int needlecolor, unsigned int dialcolor, unsigned int  textcolor, String label, boolean& redraw);

#endif

