#ifndef ABC_SKIP_H
#define ABC_SKIP_H

#include "abc/$.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/OK.h"
#include "abc/TLV.h"

con ok64 SKIPnotyet = 0xe29f78cf265251c;
con ok64 SKIPbad = 0x2896665251c;
con ok64 SKIPmiss = 0xdf7b7165251c;
con ok64 SKIPnodata = 0x978968cf265251c;
con ok64 SKIPtoofar = 0xda5ab3cf865251c;
con ok64 SKIPbof = 0x2ace665251c;
con ok64 SKIPnone = 0xa72cf265251c;
con ok64 SKIPnoroom = 0xc73cf6cf265251c;

#define SKIP_TLV_TYPE 'K'

#endif  // ABC_SKIP_H
