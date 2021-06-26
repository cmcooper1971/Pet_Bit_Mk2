// 
// 
// 

#include <TFT_eSPI.h>
#include "colours.h"
#include "Free_Fonts.h"	
#include "xphDial.h"
#include "format_function.h"

/*-----------------------------------------------------------------*/

void XphDialScreen(TFT_eSPI& tft, int cx, int cy, int r, int loval, double hival, double inc, double sa, double curval, int dig, int dec, unsigned int needlecolor, unsigned int dialcolor, unsigned int  textcolor, String label, boolean& redraw) {

	double ix, iy, ox, oy, tx, ty, lx, rx, ly, ry, i, offset, stepval, data, angle;
	double degtorad = .0174532778;
	static double px = cx, py = cy, pix = cx, piy = cy, plx = cx, ply = cy, prx = cx, pry = cy;

	// Draw the dial only one time--this will minimize flicker.

	if (redraw == true) {
		redraw = false;
		tft.fillCircle(cx, cy, r - 2, dialcolor);
		tft.drawCircle(cx, cy, r, needlecolor);
		tft.drawCircle(cx, cy, r - 1, needlecolor);
		tft.setTextColor(textcolor, dialcolor);
		tft.setTextSize(1);
		//TFT.setFont(&FreeSans9pt7b);
		tft.setCursor(cx - 15, cy + 30);
		tft.println(label);
		//TFT.setFont();
	}

	// Draw the current value.

	tft.setTextSize(1);
	tft.setTextColor(textcolor, dialcolor);
	tft.setCursor(cx - 7, cy + 40);
	tft.println(Format(curval, dig, dec));

	// Center the scale about the vertical axis--and use this to offset the needle, and scale text.

	offset = (270 + sa / 2) * degtorad;

	// Find the scale step value based on the hival low val and the scale sweep angle.
	// Deducting a small value to eliminate round off errors, this val may need to be adjusted.

	stepval = (inc) * (double(sa) / (double(hival - loval))) + .00;

	// Draw the scale and numbers, note draw this each time to repaint where the needle was.

	for (i = 0; i <= sa; i += stepval) {
		angle = (i * degtorad);
		angle = offset - angle;
		ox = (r - 2) * cos(angle) + cx;
		oy = (r - 2) * sin(angle) + cy;
		ix = (r - 10) * cos(angle) + cx;
		iy = (r - 10) * sin(angle) + cy;
		tx = (r - 10) * cos(angle) + cx;
		ty = (r - 15) * sin(angle) + cy;
		tft.drawLine(ox, oy, ix, iy, textcolor);
		tft.setTextSize(1.5);
		tft.setTextColor(textcolor, dialcolor);
		tft.setCursor(tx - 7, ty);
		data = hival - (i * (inc / stepval));
		tft.println(Format(data, dig, dec));
	}

	// Compute and draw the needle.

	angle = (sa * (1 - (((curval - loval) / (hival - loval)))));
	angle = angle * degtorad;
	angle = offset - angle;
	ix = (r - 10) * cos(angle) + cx;
	iy = (r - 10) * sin(angle) + cy;

	// Draw a triangle for the needle (compute and store 3 vertiticies).

	lx = 5 * cos(angle - 90 * degtorad) + cx;
	ly = 5 * sin(angle - 90 * degtorad) + cy;
	rx = 5 * cos(angle + 90 * degtorad) + cx;
	ry = 5 * sin(angle + 90 * degtorad) + cy;

	// First draw the previous needle in dial color to hide it, note draw performance for triangle is pretty bad.

	//d.fillTriangle (pix, piy, plx, ply, prx, pry, dialcolor);
	//d.fillTriangle (pix, piy, plx, ply, prx, pry, dialcolor);

	//d.fillTriangle (pix, piy, plx, ply, prx - 20, pry - 20, dialcolor);
	//d.fillTriangle (pix, piy, prx, pry, prx + 20, pry + 20, dialcolor);

	tft.fillTriangle(pix, piy, plx, ply, prx, pry, dialcolor);
	tft.drawTriangle(pix, piy, plx, ply, prx, pry, dialcolor);

	// Then draw the old needle in need color to display it.

	tft.fillTriangle(ix, iy, lx, ly, rx, ry, needlecolor);
	tft.drawTriangle(ix, iy, lx, ly, rx, ry, textcolor);

	// Draw a cute little dial center.

	tft.fillCircle(cx, cy, 8, textcolor);

	//Save all current to old so the previous dial can be hidden.

	pix = ix;
	piy = iy;
	plx = lx;
	ply = ly;
	prx = rx;
	pry = ry;

} // Close function.

/*-----------------------------------------------------------------*/
