// 
// 
// 

#include <TFT_eSPI.h>
#include "colours.h"
#include "Free_Fonts.h"				// Free fonts for use with eTFT.
#include "graphDistance.h"
#include "format_function.h"

/*-----------------------------------------------------------------*/

void ptSessionDistanceV1(TFT_eSPI& tft, double x, double y, double w, double h, double loval, double hival, double inc, double curval, int dig, int dec, unsigned int barcolor, unsigned int voidcolor, unsigned int bordercolor, unsigned int textcolor, unsigned int backcolor, String label, boolean& redraw)
{

	tft.setFreeFont(&FreeSans9pt7b);
	tft.setTextSize(1);
	tft.setTextColor(WHITE);
	tft.setCursor(23, 34);
	tft.println("Daily Distances");
	tft.setFreeFont();

	double stepval, range;
	double my, level;
	double i, data;
	// draw the border, scale, and label once.
	// avoid doing this on every update to minimize flicker.
	if (redraw == true) {
		redraw = false;

		tft.drawRect(x - 1, y - h - 1, w + 2, h + 2, bordercolor);
		tft.setTextColor(textcolor, backcolor);
		tft.setTextSize(1);
		tft.setCursor(x + 2, y + 7);
		tft.println(label);
		// step val basically scales the hival and low val to the height
		// deducting a small value to eliminate round off errors
		// this val may need to be adjusted.
		stepval = (inc) * (double(h) / (double(hival - loval))) - 0;
		for (i = 0; i <= h; i += stepval) {
			my = y - h + i;
			tft.drawFastHLine(x + w + 1, my, 5, textcolor);
			// draw lables
			tft.setTextSize(1);
			tft.setTextColor(BLACK, backcolor);
			tft.setCursor(x + w + 12, my - 3);
			data = hival - (i * (inc / stepval));
			tft.println(Format(data, dig, dec));
			//Screen.setFont();
		}
	}
	// compute level of bar graph that is scaled to the  height and the hi and low vals
	// this is needed to accompdate for +/- range
	level = (h * (((curval - loval) / (hival - loval))));
	// draw the bar graph
	// write a upper and lower bar to minimize flicker cause by blanking out bar and redraw on update
	tft.fillRect(x, y - h, w, h - level, voidcolor);
	tft.fillRect(x, y - level + 1, w, level, barcolor);
	// write the current value
	/*
	  tft.setTextColor(textcolor, backcolor);
	  tft.setTextSize(1);
	  tft.setCursor(x , y - h - 23);
	  tft.println(Format(curval, dig, dec));
	*/

}  // Close function.

/*-----------------------------------------------------------------*/

void ptSessionDistanceV2(TFT_eSPI& tft, double x, double y, double w, double h, double loval, double hival, double inc, double curval, int dig, int dec, unsigned int barcolor, unsigned int voidcolor, unsigned int bordercolor, unsigned int textcolor, unsigned int backcolor, String label, boolean& redraw)
{

	double stepval, range;
	double my, level;
	double i, data;
	// draw the border, scale, and label once
	// avoid doing this on every update to minimize flicker
	if (redraw == true) {
		redraw = false;

		tft.drawRect(x - 1, y - h - 1, w + 2, h + 2, bordercolor);
		tft.setTextColor(textcolor, backcolor);
		tft.setTextSize(1);
		tft.setCursor(x + 2, y + 7);
		tft.println(label);
		// step val basically scales the hival and low val to the height
		// deducting a small value to eliminate round off errors
		// this val may need to be adjusted
		stepval = (inc) * (double(h) / (double(hival - loval))) - 0;
		for (i = 0; i <= h; i += stepval) {
			my = y - h + i;
			tft.drawFastHLine(x + w + 1, my, 5, textcolor);
			// draw lables
			tft.setTextSize(1);
			tft.setTextColor(BLACK, backcolor);
			tft.setCursor(x + w + 12, my - 3);
			data = hival - (i * (inc / stepval));
			tft.println(Format(data, dig, dec));
		}
	}
	// compute level of bar graph that is scaled to the  height and the hi and low vals
	// this is needed to accompdate for +/- range
	level = (h * (((curval - loval) / (hival - loval))));
	// draw the bar graph
	// write a upper and lower bar to minimize flicker cause by blanking out bar and redraw on update
	tft.fillRect(x, y - h, w, h - level, voidcolor);
	tft.fillRect(x, y - level + 1, w, level, barcolor);
	// write the current value
	/*
	  tft.setTextColor(textcolor, backcolor);
	  tft.setTextSize(1);
	  tft.setCursor(x , y - h - 23);
	  tft.println(Format(curval, dig, dec));
	*/

} // Close function.

/*-----------------------------------------------------------------*/

void ptSessionDistanceV3(TFT_eSPI& tft, double x, double y, double w, double h, double loval, double hival, double inc, double curval, int dig, int dec, unsigned int barcolor, unsigned int voidcolor, unsigned int bordercolor, unsigned int textcolor, unsigned int backcolor, String label, boolean& redraw)
{

	double stepval, range;
	double my, level;
	double i, data;
	// draw the border, scale, and label once
	// avoid doing this on every update to minimize flicker
	if (redraw == true) {
		redraw = false;

		tft.drawRect(x - 1, y - h - 1, w + 2, h + 2, bordercolor);
		tft.setTextColor(textcolor, backcolor);
		tft.setTextSize(1);
		tft.setCursor(x + 2, y + 7);
		tft.println(label);
		// step val basically scales the hival and low val to the height
		// deducting a small value to eliminate round off errors
		// this val may need to be adjusted
		stepval = (inc) * (double(h) / (double(hival - loval))) - 0;
		for (i = 0; i <= h; i += stepval) {
			my = y - h + i;
			tft.drawFastHLine(x + w + 1, my, 5, textcolor);
			// draw lables
			tft.setTextSize(1);
			tft.setTextColor(textcolor, backcolor);
			tft.setCursor(x + w + 7, my - 3);
			data = hival - (i * (inc / stepval));
			tft.println(Format(data, dig, dec));
		}
	}
	// compute level of bar graph that is scaled to the  height and the hi and low vals
	// this is needed to accompdate for +/- range
	level = (h * (((curval - loval) / (hival - loval))));
	// draw the bar graph
	// write a upper and lower bar to minimize flicker cause by blanking out bar and redraw on update
	tft.fillRect(x, y - h, w, h - level, voidcolor);
	tft.fillRect(x, y - level + 1, w, level, barcolor);
	// write the current value
	/*
	  tft.setTextColor(textcolor, backcolor);
	  tft.setTextSize(1);
	  tft.setCursor(x , y - h - 23);
	  tft.println(Format(curval, dig, dec));
	*/

} // Close function.

/*-----------------------------------------------------------------*/
