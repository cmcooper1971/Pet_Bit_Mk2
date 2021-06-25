// drawBatteryBitmap.h

#ifndef _DRAWBATTERYBITMAP_h
#define _DRAWBATTERYBITMAP_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

void drawBattery(TFT_eSPI& tft, int x, int y, const uint16_t* bitmap, int bw, int bh);

#endif

