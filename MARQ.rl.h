#include "PRO.h"
#include "INT.h"
#include "MARK2.h"

enum {
	MARK2Ref0 = MARK2enum+9,
	MARK2Ref1 = MARK2enum+10,
	MARK2Em0 = MARK2enum+11,
	MARK2Em1 = MARK2enum+12,
	MARK2Em = MARK2enum+13,
	MARK2St0 = MARK2enum+14,
	MARK2St1 = MARK2enum+15,
	MARK2St = MARK2enum+16,
	MARK2Root = MARK2enum+18,
};
ok64 MARK2onRef0 ($cu8c tok, MARK2state* state);
ok64 MARK2onRef1 ($cu8c tok, MARK2state* state);
ok64 MARK2onEm0 ($cu8c tok, MARK2state* state);
ok64 MARK2onEm1 ($cu8c tok, MARK2state* state);
ok64 MARK2onEm ($cu8c tok, MARK2state* state);
ok64 MARK2onSt0 ($cu8c tok, MARK2state* state);
ok64 MARK2onSt1 ($cu8c tok, MARK2state* state);
ok64 MARK2onSt ($cu8c tok, MARK2state* state);
ok64 MARK2onRoot ($cu8c tok, MARK2state* state);


