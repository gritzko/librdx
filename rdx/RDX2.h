#ifndef RDX_RDX_H
#define RDX_RDX_H
#include <abc/BUF.h>
#include <abc/TLV.h>
#include <abc/ZINT.h>

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

#endif

con ok64 RDXnodata = 0x1b361cb3a25e25;

typedef struct {
    u64 src, seq;
} ref128;

#define ref128RevMask 63
typedef ref128 const ref128c;
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
fun ok64 ref128Feed(u8s into, ref128cp ref) {
    return ZINTu8sFeed128(into, ref->src, ref->seq);
}

typedef int64_t RDXint;
typedef double RDXfloat;
// typedef ref128 RDXref;
typedef u8cs RDXstring;
typedef u8cs RDXterm;

typedef struct {
    u8cs rec;
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

fun u8 rdxType(rdxcp k) { return $empty(k->rec) ? 0 : (**(k->rec) & ~TLVaA); }

fun b8 rdxIsPLEX(rdxcp k) {
    u8 lit = rdxType(k);
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
    $mv(it->rec, RDX_ROOT_REC);
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
fun u8 rdxbType(rdxb reader) { return rdxType(rdxbLast(reader)); }
ok64 rdxbNext(rdxb reader);
ok64 rdxbInto(rdxb reader);
ok64 rdxbOuto(rdxb reader);

void rdxpsUpAt(rdxps heap, int ndx, rdxZ z);
fun void rdxpsUp(rdxps heap, rdxZ z) { rdxpsUpAt(heap, $len(heap) - 1, z); }
void rdxpsDownAt(rdxps heap, int ndx, rdxZ z);
fun void rdxpsDown(rdxps heap, rdxZ z) { rdxpsDownAt(heap, 0, z); }
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
    return RDXu8sFeed(u8b_idle(builder), what);
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

#endif
