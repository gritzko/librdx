#include "abc/INT.h"
#include "abc/PRO.h"
#include "JSON.h"

enum {
	JSONLiteral = JSONenum+7,
	JSONString = JSONenum+8,
	JSONNumber = JSONenum+9,
	JSONOpenObject = JSONenum+10,
	JSONCloseObject = JSONenum+11,
	JSONOpenArray = JSONenum+12,
	JSONCloseArray = JSONenum+13,
	JSONComma = JSONenum+14,
	JSONColon = JSONenum+15,
	JSONJSON = JSONenum+18,
	JSONRoot = JSONenum+19,
};
ok64 JSONonLiteral ($cu8c tok, JSONstate* state);
ok64 JSONonString ($cu8c tok, JSONstate* state);
ok64 JSONonNumber ($cu8c tok, JSONstate* state);
ok64 JSONonOpenObject ($cu8c tok, JSONstate* state);
ok64 JSONonCloseObject ($cu8c tok, JSONstate* state);
ok64 JSONonOpenArray ($cu8c tok, JSONstate* state);
ok64 JSONonCloseArray ($cu8c tok, JSONstate* state);
ok64 JSONonComma ($cu8c tok, JSONstate* state);
ok64 JSONonColon ($cu8c tok, JSONstate* state);
ok64 JSONonJSON ($cu8c tok, JSONstate* state);
ok64 JSONonRoot ($cu8c tok, JSONstate* state);


