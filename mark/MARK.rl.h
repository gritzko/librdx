#include "abc/PRO.h"
#include "abc/INT.h"
#include "MARK.h"

enum {
	MARKHLine = MARKenum+2,
	MARKIndent = MARKenum+3,
	MARKOList = MARKenum+4,
	MARKUList = MARKenum+5,
	MARKH1 = MARKenum+6,
	MARKH2 = MARKenum+7,
	MARKH3 = MARKenum+8,
	MARKH4 = MARKenum+9,
	MARKH = MARKenum+10,
	MARKQuote = MARKenum+11,
	MARKCode = MARKenum+12,
	MARKLink = MARKenum+14,
	MARKDiv = MARKenum+17,
	MARKLine = MARKenum+18,
	MARKRoot = MARKenum+19,
};
ok64 MARKonHLine ($cu8c tok, MARKstate* state);
ok64 MARKonIndent ($cu8c tok, MARKstate* state);
ok64 MARKonOList ($cu8c tok, MARKstate* state);
ok64 MARKonUList ($cu8c tok, MARKstate* state);
ok64 MARKonH1 ($cu8c tok, MARKstate* state);
ok64 MARKonH2 ($cu8c tok, MARKstate* state);
ok64 MARKonH3 ($cu8c tok, MARKstate* state);
ok64 MARKonH4 ($cu8c tok, MARKstate* state);
ok64 MARKonH ($cu8c tok, MARKstate* state);
ok64 MARKonQuote ($cu8c tok, MARKstate* state);
ok64 MARKonCode ($cu8c tok, MARKstate* state);
ok64 MARKonLink ($cu8c tok, MARKstate* state);
ok64 MARKonDiv ($cu8c tok, MARKstate* state);
ok64 MARKonLine ($cu8c tok, MARKstate* state);
ok64 MARKonRoot ($cu8c tok, MARKstate* state);


