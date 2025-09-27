#ifndef ABC_SKIP_H
#define ABC_SKIP_H

#include "abc/S.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/OK.h"
#include "abc/TLV.h"

con ok64 SKIPnotyet = 0x714499cb3e3da78;
con ok64 SKIPbad = 0x1c512666968;
con ok64 SKIPmiss = 0x714499c6ddf7;
con ok64 SKIPnodata = 0x714499cb3a25e25;
con ok64 SKIPtoofar = 0x714499e33cea976;
con ok64 SKIPbof = 0x1c512666cea;
con ok64 SKIPnone = 0x714499cb3ca9;
con ok64 SKIPnoroom = 0x714499cb3db3cf1;

#define SKIP_TLV_TYPE 'K'

#endif  // ABC_SKIP_H
