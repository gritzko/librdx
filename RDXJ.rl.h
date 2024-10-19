#include "PRO.h"
#include "INT.h"
#include "RDXJ.h"

enum {
	RDXJInt = RDXJenum+9,
	RDXJFloat = RDXJenum+10,
	RDXJRef = RDXJenum+11,
	RDXJString = RDXJenum+12,
	RDXJTerm = RDXJenum+13,
	RDXJOpenObject = RDXJenum+14,
	RDXJCloseObject = RDXJenum+15,
	RDXJOpenArray = RDXJenum+16,
	RDXJCloseArray = RDXJenum+17,
	RDXJOpenVector = RDXJenum+18,
	RDXJCloseVector = RDXJenum+19,
	RDXJStamp = RDXJenum+20,
	RDXJComma = RDXJenum+21,
	RDXJColon = RDXJenum+22,
	RDXJFIRST = RDXJenum+24,
	RDXJRoot = RDXJenum+25,
};
ok64 RDXJonInt ($cu8c tok, RDXJstate* state);
ok64 RDXJonFloat ($cu8c tok, RDXJstate* state);
ok64 RDXJonRef ($cu8c tok, RDXJstate* state);
ok64 RDXJonString ($cu8c tok, RDXJstate* state);
ok64 RDXJonTerm ($cu8c tok, RDXJstate* state);
ok64 RDXJonOpenObject ($cu8c tok, RDXJstate* state);
ok64 RDXJonCloseObject ($cu8c tok, RDXJstate* state);
ok64 RDXJonOpenArray ($cu8c tok, RDXJstate* state);
ok64 RDXJonCloseArray ($cu8c tok, RDXJstate* state);
ok64 RDXJonOpenVector ($cu8c tok, RDXJstate* state);
ok64 RDXJonCloseVector ($cu8c tok, RDXJstate* state);
ok64 RDXJonStamp ($cu8c tok, RDXJstate* state);
ok64 RDXJonComma ($cu8c tok, RDXJstate* state);
ok64 RDXJonColon ($cu8c tok, RDXJstate* state);
ok64 RDXJonFIRST ($cu8c tok, RDXJstate* state);
ok64 RDXJonRoot ($cu8c tok, RDXJstate* state);


