// 
// 
// 

#include <TFT_eSPI.h>
#include "startScreen.h"
#include "startUpScreen.h"

void startUpScreen(TFT_eSPI& tft) {

	tft.fillScreen(ILI9341_WHITE);

	int h = 80, w = 112, row, col, buffidx = 0;

	for (row = 16; row < h; row++) {		// For each scanline.

		for (col = 48; col < w; col++) {	// For each pixel, read from Flash Memory, since image is stored as uint16_t, pgm_read_word is used as it uses 16bit address.
			tft.drawPixel(col, row, pgm_read_word(startScreen + buffidx));
			buffidx++;

		} // End for loop.

	} // End for loop.

}  // Close function.