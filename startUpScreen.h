// startUpScreen.h

#ifndef _STARTUPSCREEN_h
#define _STARTUPSCREEN_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

void startUpScreen(TFT_eSPI& tft);

#endif

