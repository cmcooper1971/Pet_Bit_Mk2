// 
// 
// 

#include <TFT_eSPI.h>
#include "startScreen.h"
#include "startUpScreen.h"

void startUpScreen(TFT_eSPI& tft) {

	tft.fillScreen(ILI9341_WHITE);

	int h = 128, w = 128, row, col, buffidx = 0;

	for (row = 128; row < h; row++) {		// For each scanline.

		for (col = 128; col < w; col++) {	// For each pixel, read from Flash Memory, since image is stored as uint16_t, pgm_read_word is used as it uses 16bit address.
			tft.drawPixel(col, row, pgm_read_word(startScreen + buffidx));
			buffidx++;

		}

	}

}  // Close function.