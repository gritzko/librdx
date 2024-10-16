#ifndef ABC_RDXJ_H
#define ABC_RDXJ_H
#include "INT.h"
#include "RDX.h"
#include "TLV.h"

con int RDXJenum = 0;

con ok64 RDXJbad = 0x289664e135b;
con ok64 RDXJfail = 0xc2d96a4e135b;

typedef struct {
    u32 tlvpos;
    u32 toks : 27, node : 5;
} rdx64;

typedef struct {
    $u8c text;
    u8B tlv;
    u64B stack;

    u8 lit;
    u128 id;
    u8B pad;
} RDXJstate;

ok64 RDXJlexer(RDXJstate* state);
ok64 RDXJfromTLV($u8 rdxj, $u8c tlv);

#endif
