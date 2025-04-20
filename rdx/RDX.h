#ifndef LIBRDX_RDX_H
#define LIBRDX_RDX_H

#include "abc/01.h"
#include "abc/B.h"
#include "abc/HEX.h"
#include "abc/INT.h"
#include "abc/OK.h"
#include "abc/S.h"
#include "abc/TLV.h"
#include "abc/ZINT.h"

static const ok64 RDXnoroom = 0x1b361cb3db3cf1;
static const ok64 RDXbad = 0x6cd866968;
static const ok64 RDXwrong = 0x6cd87bdb3cab;

#define RDX_MAX_NEST 256

typedef enum {
    // primitive types
    RDX_FLOAT = 'F',
    RDX_INT = 'I',
    RDX_REF = 'R',
    RDX_STRING = 'S',
    RDX_TERM = 'T',
    // collection types
    RDX_TUPLE = 'P',
    RDX_LINEAR = 'L',
    RDX_EULER = 'E',
    RDX_MULTIX = 'X',
} RDXtype;

static const u64 RDX_PLEX_BITS =
    (1 << (RDX_TUPLE - 'A')) | (1 << (RDX_LINEAR - 'A')) |
    (1 << (RDX_EULER - 'A')) | (1 << (RDX_MULTIX - 'A'));

typedef int64_t RDXint;
typedef double RDXfloat;
typedef u128 id128;
typedef id128 RDXref;
typedef $u8c RDXstring;
typedef $u8c RDXterm;

// FIXME big: seq, lil: src
#define id128cmp u128cmp
#define aRDXid(n, time, src) id128 n = {._64 = {src, time}};
#define id128src(t) ((t)._64[0])
#define id128time(t) ((t)._64[1])
#define id128seq(t) ((t)._64[1])

fun b8 id128empty(id128 id) { return id._64[0] == 0 && id._64[1] == 0; }
fun u8 RDXrdt($u8c rdx) { return $empty(rdx) ? 0 : TLVup(**rdx); }
fun b8 RDXisFIRST(u8 l) {
    l &= ~TLVaA;
    return l == RDX_FLOAT || l == RDX_INT || l == RDX_REF || l == RDX_STRING ||
           l == RDX_TERM;
}
fun b8 RDXisPLEX(u8 l) {
    l &= ~TLVaA;
    return l == RDX_TUPLE || l == RDX_LINEAR || l == RDX_EULER ||
           l == RDX_MULTIX;
}

// #define X(M, name) M##$u8c##name
// #include "abc/HEAPx.h"
// #undef X

typedef ok64 rdxnext(u8c$ next, $u8c input);

fun u64 RDXtick(u128* clock) { return ++id128time(*clock); }
fun u64 RDXtock(u128* clock, u128 see) {
    if (id128time(*clock) < id128time(see)) id128time(*clock) = id128time(see);
    return id128time(*clock);
}

fun ok64 RDXfeed($u8 tlv, u8 t, id128 id, $cu8c value) {
    aBpad(u8, idpad, 16);
    ZINTu128feed(Bu8idle(idpad), &id);
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
    if (*t >= 'a' && *t <= 'z') *t -= TLVaA;
    return OK;
}

ok64 RDXflatfeed($u8 into, $u8c rdx);

ok64 RDXallFIRST($cu8c rdx);

static $u8c ID128DELIM = $u8str("-");

#endif
