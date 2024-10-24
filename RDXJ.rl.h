#include "PRO.h"
#include "INT.h"
#include "RDXJ.h"

enum {
	RDXJInt = RDXJenum+9,
	RDXJFloat = RDXJenum+10,
	RDXJRef = RDXJenum+11,
	RDXJString = RDXJenum+12,
	RDXJTerm = RDXJenum+13,
	RDXJStamp = RDXJenum+14,
	RDXJOpenP = RDXJenum+15,
	RDXJCloseP = RDXJenum+16,
	RDXJOpenL = RDXJenum+17,
	RDXJCloseL = RDXJenum+18,
	RDXJOpenE = RDXJenum+19,
	RDXJCloseE = RDXJenum+20,
	RDXJOpenX = RDXJenum+21,
	RDXJCloseX = RDXJenum+22,
	RDXJComma = RDXJenum+23,
	RDXJColon = RDXJenum+24,
	RDXJOpen = RDXJenum+25,
	RDXJClose = RDXJenum+26,
	RDXJInter = RDXJenum+27,
	RDXJFIRST = RDXJenum+29,
	RDXJRoot = RDXJenum+30,
};
ok64 RDXJonInt ($cu8c tok, RDXJstate* state);
ok64 RDXJonFloat ($cu8c tok, RDXJstate* state);
ok64 RDXJonRef ($cu8c tok, RDXJstate* state);
ok64 RDXJonString ($cu8c tok, RDXJstate* state);
ok64 RDXJonTerm ($cu8c tok, RDXJstate* state);
ok64 RDXJonStamp ($cu8c tok, RDXJstate* state);
ok64 RDXJonOpenP ($cu8c tok, RDXJstate* state);
ok64 RDXJonCloseP ($cu8c tok, RDXJstate* state);
ok64 RDXJonOpenL ($cu8c tok, RDXJstate* state);
ok64 RDXJonCloseL ($cu8c tok, RDXJstate* state);
ok64 RDXJonOpenE ($cu8c tok, RDXJstate* state);
ok64 RDXJonCloseE ($cu8c tok, RDXJstate* state);
ok64 RDXJonOpenX ($cu8c tok, RDXJstate* state);
ok64 RDXJonCloseX ($cu8c tok, RDXJstate* state);
ok64 RDXJonComma ($cu8c tok, RDXJstate* state);
ok64 RDXJonColon ($cu8c tok, RDXJstate* state);
ok64 RDXJonOpen ($cu8c tok, RDXJstate* state);
ok64 RDXJonClose ($cu8c tok, RDXJstate* state);
ok64 RDXJonInter ($cu8c tok, RDXJstate* state);
ok64 RDXJonFIRST ($cu8c tok, RDXJstate* state);
ok64 RDXJonRoot ($cu8c tok, RDXJstate* state);


