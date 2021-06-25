// format_function.h

#ifndef _FORMAT_FUNCTION_h
#define _FORMAT_FUNCTION_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

String Format(double val, int dec, int dig);

#endif

