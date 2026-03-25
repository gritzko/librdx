#ifndef ABC_SKIP_H
#define ABC_SKIP_H

#include "abc/S.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/OK.h"
#include "abc/TLV.h"

con ok64 SKIPNOTYET = 0x7144995d876239d;
con ok64 SKIPBAD = 0x1c51264b28d;
con ok64 SKIPMISS = 0x71449959271c;
con ok64 SKIPNODATA = 0x7144995d834a74a;
con ok64 SKIPTOOFAR = 0x71449975860f29b;
con ok64 SKIPBOF = 0x1c51264b60f;
con ok64 SKIPNONE = 0x7144995d85ce;
con ok64 SKIPNOROOM = 0x7144995d86d8616;

#define SKIP_TLV_TYPE 'K'

#endif  // ABC_SKIP_H
