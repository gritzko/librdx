#ifndef LIBRDX_SM_H
#define LIBRDX_SM_H

#include "json/BASON.h"

con ok64 SMFAIL = 0x7163ca495;
con ok64 SMBAD = 0x1c58b28d;

#define SM_MAXDEPTH 16

typedef struct {
    u8 kb[11];
    u8 klen;
} SMkeyframe;

typedef struct {
    u8cs data;
    u8bp buf;
    u64bp idx;
    u8 divstack[64];
    int depth;
    u8 prevdiv[64];
    int prevdepth;
    SMkeyframe kframes[SM_MAXDEPTH];
    int klevel;
    b8 incode;
    u8 inline_mode;
} SMstate;

ok64 SMLexer(SMstate* state);

ok64 SMParse(u8bp buf, u64bp idx, u8csc source);

#endif
