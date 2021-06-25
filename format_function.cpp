// 
// 
// 

#include "format_function.h"

String Format(double val, int dec, int dig) {
	int addpad = 0;
	char sbuf[20];
	String condata = (dtostrf(val, dec, dig, sbuf));


	int slen = condata.length();
	for (addpad = 1; addpad <= dec + dig - slen; addpad++) {
		condata = " " + condata;
	}
	return (condata);

} // Close function.