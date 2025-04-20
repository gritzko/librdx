#ifndef ABC_SKIP_H
#define ABC_SKIP_H

#include "abc/$.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/OK.h"
#include "abc/TLV.h"

static const ok64 SKIPnotyet = 0x714499cb3e3da78;
static const ok64 SKIPbad = 0x1c512666968;
static const ok64 SKIPmiss = 0x714499c6ddf7;
static const ok64 SKIPnodata = 0x714499cb3a25e25;
static const ok64 SKIPtoofar = 0x714499e33cea976;
static const ok64 SKIPbof = 0x1c512666cea;
static const ok64 SKIPnone = 0x714499cb3ca9;
static const ok64 SKIPnoroom = 0x714499cb3db3cf1;

#define SKIP_TLV_TYPE 'K'

#endif  // ABC_SKIP_H
