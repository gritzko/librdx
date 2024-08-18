#include "PRO.h"
#include "INT.h"
#include "MARK2.h"

enum {
	MARK2Ref0 = MARK2enum+8,
	MARK2Ref1 = MARK2enum+9,
	MARK2Em = MARK2enum+10,
	MARK2StA0 = MARK2enum+11,
	MARK2StA1 = MARK2enum+12,
	MARK2Root = MARK2enum+14,
};
ok64 MARK2onRef0 ($cu8c tok, MARK2state* state);
ok64 MARK2onRef1 ($cu8c tok, MARK2state* state);
ok64 MARK2onEm ($cu8c tok, MARK2state* state);
ok64 MARK2onStA0 ($cu8c tok, MARK2state* state);
ok64 MARK2onStA1 ($cu8c tok, MARK2state* state);
ok64 MARK2onRoot ($cu8c tok, MARK2state* state);


