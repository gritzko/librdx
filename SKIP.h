#ifndef ABC_SKIP_H
#define ABC_SKIP_H

#include "$.h"
#include "01.h"
#include "B.h"
#include "OK.h"
#include "TLV.h"

con ok64 SKIPnotyet = 0xe29f78cf265251c;
con ok64 SKIPbad = 0x2896665251c;
con ok64 SKIPmiss = 0xdf7b7165251c;
con ok64 SKIPnodata = 0x978968cf265251c;
con ok64 SKIPtoofar = 0xda5ab3cf865251c;
con ok64 SKIPbof = 0x2ace665251c;
con ok64 SKIPnone = 0xa72cf265251c;
con ok64 SKIPnoroom = 0xc73cf6cf265251c;

#define SKIP_MASK 0xffff
#define SKIP_TERM_LEN 3
#define SKIP_TLV_TYPE 'Z'

typedef $u8ccmpfn SKIPcmpfn;

typedef struct {
    // The last skip record.
    u64 pos;
    u8 len;
    u8 tlvlen;
    u16 off[40];
} SKIPs;

// Avg skip record: 2 entries, 4+1 bytes,
// OVerhead 1% => gap 1<<9=512
#define X(M, name) M##bl09##name
#include "SKIPx.h"
#undef X

#endif  // ABC_SKIP_H
