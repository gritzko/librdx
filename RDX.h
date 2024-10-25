#ifndef LIBRDX_RDX_H
#define LIBRDX_RDX_H

#include "$.h"
#include "01.h"
#include "B.h"
#include "HEX.h"
#include "INT.h"
#include "OK.h"
#include "TLV.h"
#include "ZINT.h"

con ok64 RDXnospace = 0xa67974df3ca135b;
con ok64 RDXbad = 0xa259a135b;

#define RDX_MAX_NEST 64

typedef enum {
    RDX_FLOAT = 'F',
    RDX_INT = 'I',
    RDX_REF = 'R',
    RDX_STRING = 'S',
    RDX_TERM = 'T',
    RDX_TUPLE = 'P',
    RDX_LINEAR = 'L',
    RDX_EULER = 'E',
    RDX_MULTIX = 'X',
} RDXtype;

typedef int64_t RDXint;
typedef double RDXfloat;
typedef u128 id128;
typedef id128 RDXref;
typedef $u8c RDXstring;
typedef $u8c RDXterm;

#define id128cmp u128cmp
#define aRDXid(n, time, src) id128 n = {._64 = {src, time}};
#define RDXtime(t) ((t)._64[1])
#define RDXsrc(t) ((t)._64[0])

fun b8 RDXisFIRST(u8 l) {
    return l == 'F' || l == 'I' || l == 'R' || l == 'S' || l == 'T';
}
fun b8 RDXisPLEX(u8 l) { return l == 'P' || l == 'L' || l == 'E' || l == 'X'; }

// #define X(M, name) M##$u8c##name
// #include "HEAPx.h"
// #undef X

typedef ok64 rdxnext(u8c$ next, $u8c input);

fun u64 RDXtick(u128* clock) { return ++RDXtime(*clock); }
fun u64 RDXtock(u128* clock, u128 see) {
    if (RDXtime(*clock) < RDXtime(see)) RDXtime(*clock) = RDXtime(see);
    return RDXtime(*clock);
}

fun pro(RDXfeed, $u8 tlv, u8 t, id128 id, $cu8c value) {
    aBpad(u8, idpad, 16);
    ZINTu128feed(Bu8idle(idpad), id);
    return TLVfeedkv(tlv, t, Bu8cdata(idpad), value);
}

fun ok64 RDXdrain(u8* t, id128* id, $u8c value, $u8c tlv) {
    $u8c idbody = {};
    ok64 o = TLVdrainkv(t, idbody, value, tlv);
    if (likely(o == OK)) o = ZINTu128drain(id, idbody);
    return o;  // TODO untouched on error
}

fun ok64 RDXdrain$(u8* t, $u8c rec, $u8c rdx) {
    ok64 o = TLVdrain$(rec, rdx);
    if (unlikely(o != OK)) return o;
    *t = **rec;
    if (*t >= 'a' && *t <= 'z') *t -= TLVaa;
    return OK;
}

static $u8c ID128DELIM = $u8str("-");

#endif
