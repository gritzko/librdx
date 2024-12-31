#include "abc/INT.h"
#include "abc/PRO.h"
#include "JDR.h"

enum {
	JDRInt = JDRenum+9,
	JDRFloat = JDRenum+10,
	JDRRef = JDRenum+11,
	JDRString = JDRenum+12,
	JDRTerm = JDRenum+13,
	JDRStamp = JDRenum+14,
	JDROpenP = JDRenum+15,
	JDRCloseP = JDRenum+16,
	JDROpenL = JDRenum+17,
	JDRCloseL = JDRenum+18,
	JDROpenE = JDRenum+19,
	JDRCloseE = JDRenum+20,
	JDROpenX = JDRenum+21,
	JDRCloseX = JDRenum+22,
	JDRComma = JDRenum+23,
	JDRColon = JDRenum+24,
	JDROpen = JDRenum+25,
	JDRClose = JDRenum+26,
	JDRInter = JDRenum+27,
	JDRFIRST = JDRenum+29,
	JDRRoot = JDRenum+30,
};
ok64 JDRonInt ($cu8c tok, JDRstate* state);
ok64 JDRonFloat ($cu8c tok, JDRstate* state);
ok64 JDRonRef ($cu8c tok, JDRstate* state);
ok64 JDRonString ($cu8c tok, JDRstate* state);
ok64 JDRonTerm ($cu8c tok, JDRstate* state);
ok64 JDRonStamp ($cu8c tok, JDRstate* state);
ok64 JDRonOpenP ($cu8c tok, JDRstate* state);
ok64 JDRonCloseP ($cu8c tok, JDRstate* state);
ok64 JDRonOpenL ($cu8c tok, JDRstate* state);
ok64 JDRonCloseL ($cu8c tok, JDRstate* state);
ok64 JDRonOpenE ($cu8c tok, JDRstate* state);
ok64 JDRonCloseE ($cu8c tok, JDRstate* state);
ok64 JDRonOpenX ($cu8c tok, JDRstate* state);
ok64 JDRonCloseX ($cu8c tok, JDRstate* state);
ok64 JDRonComma ($cu8c tok, JDRstate* state);
ok64 JDRonColon ($cu8c tok, JDRstate* state);
ok64 JDRonOpen ($cu8c tok, JDRstate* state);
ok64 JDRonClose ($cu8c tok, JDRstate* state);
ok64 JDRonInter ($cu8c tok, JDRstate* state);
ok64 JDRonFIRST ($cu8c tok, JDRstate* state);
ok64 JDRonRoot ($cu8c tok, JDRstate* state);


