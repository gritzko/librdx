#include "PRO.h"
#include "INT.h"
#include "MARQ.h"

enum {
	MARQRef0 = MARQenum+9,
	MARQRef1 = MARQenum+10,
	MARQEm0 = MARQenum+11,
	MARQEm1 = MARQenum+12,
	MARQEm = MARQenum+13,
	MARQSt0 = MARQenum+14,
	MARQSt1 = MARQenum+15,
	MARQSt = MARQenum+16,
	MARQRoot = MARQenum+18,
};
ok64 MARQonRef0 ($cu8c tok, MARQstate* state);
ok64 MARQonRef1 ($cu8c tok, MARQstate* state);
ok64 MARQonEm0 ($cu8c tok, MARQstate* state);
ok64 MARQonEm1 ($cu8c tok, MARQstate* state);
ok64 MARQonEm ($cu8c tok, MARQstate* state);
ok64 MARQonSt0 ($cu8c tok, MARQstate* state);
ok64 MARQonSt1 ($cu8c tok, MARQstate* state);
ok64 MARQonSt ($cu8c tok, MARQstate* state);
ok64 MARQonRoot ($cu8c tok, MARQstate* state);


