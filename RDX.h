#ifndef LIBRDX_RDX_H
#define LIBRDX_RDX_H

#include "$.h"
#include "01.h"
#include "B.h"
#include "HEX.h"
#include "INT.h"
#include "TLV.h"
#include "ZINT.h"

con ok64 RDXnospace = 0xa67974df3ca135b;
con ok64 RDXbad = 0xa259a135b;

typedef enum {
    RDX_FLOAT = 'F',
    RDX_INT = 'I',
    RDX_REF = 'R',
    RDX_STRING = 'S',
    RDX_TERM = 'T',
    RDX_NAT = 'N',
    RDX_Z = 'Z',
    RDX_SET = 'E',
    RDX_LINEAR = 'L',
    RDX_MAP = 'M',
} RDXtype;

typedef int64_t RDXint;
typedef double RDXfloat;
typedef u128 id128;
typedef id128 RDXref;
typedef $u8c RDXstring;
typedef $u8c RDXterm;

typedef $u8 rdx$;

typedef ok64 RDXmergefn($u8 into, $u8cp from);

#define id128cmp u128cmp
#define aRDXid(n, time, src) id128 n = {._64 = {src, time}};
#define RDXtime(t) ((t)._64[1])
#define RDXsrc(t) ((t)._64[0])

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

$u8c ID128DELIM = $u8str("-");

fun ok64 RDXid128feed($u8 txt, id128 id) {
    u8* p = *txt;
    ok64 o = u64hexfeed(txt, RDXtime(id));
    if ($len(txt) <= 1) o = RDXnospace;
    if (o == OK) {
        $feed1(txt, **ID128DELIM);
        o = u64hexfeed(txt, RDXsrc(id));
    }
    if (o != OK) *txt = p;
    return o;
}

fun ok64 RDXid128drain(id128* id, $cu8c txt) {
    u8c* p = $u8find(txt, *ID128DELIM);
    if (p == nil) return RDXbad;
    $u8c time = {txt[0], p};
    $u8c src = {p + 1, txt[1]};
    id128 res = {};
    ok64 o = u64hexdrain(&RDXtime(res), time);
    if (o == OK) o = u64hexdrain(&RDXsrc(res), src);
    if (o == OK) *id = res;
    return o;
}

#endif
