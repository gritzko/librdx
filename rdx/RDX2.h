#ifndef RDX_RDX_H
#define RDX_RDX_H
#include <abc/BUF.h>
#include <abc/TLV.h>
#include <abc/ZINT.h>
#include "abc/UTF8.h"
#include "abc/01.h"
#include "abc/OK.h"
#include "abc/S.h"

#ifndef LIBRDX_RDX_H
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

typedef enum {
    RDX_ENC_UTF8 = '8',
    RDX_ENC_UTF16 = '6',
    RDX_ENC_UTF8_ESC = 'e',
    RDX_ENC_UTF8_ESC_ML = 'm',
} RDXEncoding;

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

con ok64 RDXbad = 0x4cd6e6968;
con ok64 RDXnoroom = 0x1335bcb3db3cf1;
con ok64 RDXnodata = 0x1b361cb3a25e25;
con ok64 RDXfail = 0x1335baa5b70;
con ok64 RDXbadF = 0x1335b9a5a0f;
con ok64 RDXbadI = 0x1335b9a5a12;
con ok64 RDXbadR = 0x1335b9a5a1b;
con ok64 RDXbadS = 0x1335b9a5a1c;
con ok64 RDXbadT = 0x1335b9a5a1d;
con ok64 RDXbadnest = 0x4cd6e6968ca9df8;
con ok64 RDXsyntax = 0x1335bdfdcb897c;
con ok64 RDXsogood = OK;

#endif

typedef struct {
    u64 src, seq;
} ref128;

#define ref128RevMask 63
typedef ref128 const ref128c;
typedef ref128* ref128p;
typedef ref128c* ref128cp;
fun b8 ref128Z(ref128cp a, ref128cp b) {
    u64 at = a->seq & ~ref128RevMask;
    u64 bt = b->seq & ~ref128RevMask;
    return at == bt ? a->src < b->src : at < bt;
}
fun b8 ref128Eq(ref128cp a, ref128cp b) {
    u64 at = a->seq & ~ref128RevMask;
    u64 bt = b->seq & ~ref128RevMask;
    return at == bt && a->src == b->src;
}

fun ok64 RDXu8sFeedID(u8s into, ref128cp ref) {
    return ZINTu8sFeed128(into, ref->src, ref->seq);
}

fun ok64 RDXu8csDrainID(u8cs from, ref128p ref) {
    return ZINTu8sDrain128(from, &ref->src, &ref->seq);
}

ok64 RDXutf8sFeedID(utf8s into, ref128cp ref);
ok64 RDXutf8sDrainID(utf8cs from, ref128p ref);

typedef int64_t RDXint;
typedef double RDXfloat;
// typedef ref128 RDXref;
typedef u8cs RDXstring;
typedef u8cs RDXterm;

typedef struct {
    u8 type;
    u8 enc;
    u64 reclen;
    u8cs rest;
    ref128 id;
    union {
        f64 f;
        i64 i;
        ref128 r;
        u8cs s;
        u8cs t;
        u8cs plex;
    };
} rdx;

#define RDX_MAX_NESTING 64
#define RDX_MAX_INPUTS 64

typedef const rdx rdxc;
typedef const rdx* rdxcp;

fun b8 rdxIsPLEX(rdxcp k) {
    u8 lit = k->type;
    return lit == RDX_TUPLE || lit == RDX_LINEAR || lit == RDX_EULER ||
           lit == RDX_MULTIX;
}

fun int ref128cmp(ref128cp a, ref128cp b) { return 0; }
fun int rdxcmp(rdxcp a, rdxcp b) { return ref128cmp(&a->id, &b->id); }

#define X(M, name) M##rdx##name
#include "abc/Bx.h"
#undef X

fun int rdxpcmp(rdxp const* a, rdxp const* b) { return rdxcmp(*a, *b); }

#define X(M, name) M##rdxp##name
#include "abc/Bx.h"
#undef X

extern u8cs RDX_ROOT_REC;

#define LESS 1
#define GREQ 0
typedef b8 (*rdxZ)(rdxcp a, rdxcp b);

fun void rdxInit(rdxp it, u8csc data) {
    zerop(it);
    $mv(it->plex, data);
}
ok64 rdxNext(rdxp it);
fun ok64 rdxInto(rdxp it, u8cs rest) {
    u8csDup(rest, it->rest);
    u8csDup(it->rest, it->plex);
    return rdxNext(it);
}
fun ok64 rdxOuto(rdxp it, u8csc rest) {
    u8csDup(it->rest, rest);
    return rdxNext(it);
}

ok64 rdxbInit(rdxb reader, u8cs data);
fun u8 rdxbType(rdxb reader) { return rdxbLast(reader)->type; }
ok64 rdxbNext(rdxb reader);
ok64 rdxbInto(rdxb reader);
ok64 rdxbOuto(rdxb reader);

ok64 rdxpsUpAt(rdxps heap, size_t ndx, rdxZ z);
fun ok64 rdxpsUp(rdxps heap, rdxZ z) {
    return rdxpsUpAt(heap, $len(heap) - 1, z);
}
ok64 rdxpsDownAt(rdxps heap, size_t ndx, rdxZ z);
fun ok64 rdxpsDown(rdxps heap, rdxZ z) { return rdxpsDownAt(heap, 0, z); }
ok64 rdxpsEqs(rdxps heap, u32p eqs, rdxZ z);
ok64 rdxpsNexts(rdxps heap, u32 eqs, rdxZ z);  // ejects

ok64 RDXu8sFeed(u8s rdx, rdxcp it);

ok64 RDXu8sDrain(rdxp it, u8s rdx);

ok64 RDXu8ssMonoFeed(u8css spans, u8cs input, rdxZ z);

fun b8 u64Z(u64cp a, u64cp b) { return a < b; }
b8 rdxTypeZ(rdxcp a, rdxcp b);
b8 rdx1Z(rdxcp a, rdxcp b);
b8 rdxTupleZ(rdxcp a, rdxcp b);
b8 rdxLinearZ(rdxcp a, rdxcp b);
b8 rdxEulerZ(rdxcp a, rdxcp b);
b8 rdxMultixZ(rdxcp a, rdxcp b);
b8 rdxLastWriteWinsZ(rdxcp a, rdxcp b);

fun ok64 RDXu8bFeed(u8b builder, rdxcp what) {
    return RDXu8sFeed(u8bIdle(builder), what);
}
ok64 RDXu8bInto(u8b builder, rdxcp what);
ok64 RDXu8bOuto(u8b builder, rdxcp what);

ok64 RDXu8bFeedDeep(u8b builder, rdxb reader);
ok64 RDXu8bFeedAll(u8s into, u8cs from);

ok64 RDXu8bMergeLWW(u8b merged, rdxpsc eqs);

ok64 RDXu8bMerge(u8b merged, rdxps inputs, rdxZ z);

ok64 RDXu8sMergeZ(u8s merged, u8css inputs, rdxZ z);

fun ok64 RDXu8sMerge(u8s merged, u8css inputs) {
    return RDXu8sMergeZ(merged, inputs, rdxTupleZ);
}

// . . . . . . . . RDX TLV FIRST . . . . . . . .

ok64 RDXu8sFeed1(u8s elem, rdxcp rdx);
ok64 RDXu8sFeedF(u8s elem, rdxcp rdx);
ok64 RDXu8sFeedI(u8s elem, rdxcp rdx);
ok64 RDXu8sFeedR(u8s elem, rdxcp rdx);
// the reason to use rdxcp in the signature: encoding may vary (u8, json etc)
ok64 RDXu8sFeedS(u8s elem, rdxcp rdx);
ok64 RDXu8sFeedT(u8s elem, rdxcp rdx);

ok64 RDXu8sDrainF(u8csc elem, rdxp rdx);
ok64 RDXu8sDrainI(u8csc elem, rdxp rdx);
ok64 RDXu8sDrainR(u8csc elem, rdxp rdx);
ok64 RDXu8sDrainS(u8csc elem, rdxp rdx);
ok64 RDXu8sDrainT(u8csc elem, rdxp rdx);

#endif
