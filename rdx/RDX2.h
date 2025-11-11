#ifndef RDX_RDX_H
#define RDX_RDX_H
#include <abc/BUF.h>
#include <abc/TLV.h>
#include <abc/ZINT.h>

#include "abc/01.h"
#include "abc/OK.h"
#include "abc/S.h"
#include "abc/UTF8.h"

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
con ok64 RDXbadrec = 0x1b3619a5a36a67;
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
con ok64 RDXeof = 0x6cd869cea;

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
fun b8 ref128Empty(ref128cp a) { return a->src == 0 && a->seq == 0; }

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

typedef ok64 (*rdxZ)(rdxcp a, rdxcp b);

ok64 rdxNext(rdxp it);
fun b8 rdxOK(rdxp it) { return it->reclen > 0; }
fun ok64 rdxInit(rdxp it, u8csc data) {
    zerop(it);
    u8csDup(it->rest, data);
    return OK;
}
fun ok64 rdxInto(rdxp it, u8cs rest) {
    u8csDup(rest, it->rest);
    u8csDup(it->rest, it->plex);
    return rdxNext(it);
}
fun ok64 rdxOuto(rdxp it, u8csc rest) {
    u8csDup(it->rest, rest);
    return rdxNext(it);
}
fun b8 rdxEmpty(rdxp it) { return $empty(it->rest); }

ok64 rdxbInit(rdxb reader, u8cs data);
fun u8 rdxbType(rdxb reader) { return rdxbLast(reader)->type; }
fun ok64 rdxbNext(rdxb reader) {
#ifndef ABC_INSANE
    if (unlikely(reader == NULL || Bdatalen(reader) <= 0)) return FAILsanity;
#endif
    return rdxNext(Blastp(reader));
}
ok64 rdxbInto(rdxb reader);
ok64 rdxbOuto(rdxb reader);

ok64 rdxpsUpAt(rdxps heap, size_t ndx, rdxz z);
fun ok64 rdxpsUp(rdxps heap, rdxz z) {
    return rdxpsUpAt(heap, $len(heap) - 1, z);
}
ok64 rdxpsDownAt(rdxps heap, size_t ndx, rdxz z);
fun ok64 rdxpsDown(rdxps heap, rdxz z) { return rdxpsDownAt(heap, 0, z); }
ok64 rdxpsEqs(rdxps heap, u32p eqs, rdxz z);
ok64 rdxpsNexts(rdxps heap, u32 eqs, rdxz z);  // ejects

ok64 RDXu8sFeed(u8s rdx, rdxcp it);

ok64 RDXu8sDrain(rdxp it, u8s rdx);

ok64 RDXu8ssMonoFeed(u8css spans, u8cs input, rdxz z);

fun ok64 u64Z(u64cp a, u64cp b) { return a < b; }
ok64 rdxTypeZ(rdxcp a, rdxcp b);
ok64 rdx1Z(rdxcp a, rdxcp b);
ok64 rdxTupleZ(rdxcp a, rdxcp b);
ok64 rdxLinearZ(rdxcp a, rdxcp b);
ok64 rdxEulerZ(rdxcp a, rdxcp b);
ok64 rdxMultixZ(rdxcp a, rdxcp b);
ok64 rdxLastWriteWinsZ(rdxcp a, rdxcp b);

fun ok64 RDXu8bFeed(u8bp builder, rdxcp what) {
    return RDXu8sFeed(u8bIdle(builder), what);
}
ok64 RDXu8bInto(u8bp builder, rdxcp what);
ok64 RDXu8bOuto(u8bp builder, rdxcp what);

ok64 RDXu8bFeedDeep(u8bp builder, rdxb reader);
ok64 RDXu8bFeedAll(u8bp into, u8cs from);

ok64 RDXu8bMergeLWW(u8b merged, rdxpsc eqs);

ok64 RDXu8bMerge(u8b merged, rdxps inputs, rdxz z);

ok64 RDXu8bMergeZ(u8bp merged, u8css inputs, rdxz z);

fun ok64 RDXu8sMerge(u8bp merged, u8css inputs) {
    return RDXu8bMergeZ(merged, inputs, rdxTupleZ);
}

// . . . . . . . . RDX TLV FIRST . . . . . . . .

ok64 RDXu8sFeedFIRST(u8s elem, rdxcp rdx);
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


// . . . . . . . . DIFF . . . . . . . .

#define DIFF_MAX_STEPS 1<<20

ok64 RDXu8bDiff(u8b diff, u8cs a, u8cs b);

#endif
