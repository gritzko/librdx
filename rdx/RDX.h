#ifndef RDX_RDX_H
#define RDX_RDX_H
#include <abc/SHA.h>
#include <abc/TLV.h>
#include <abc/UTF8.h>
#include <abc/ZINT.h>

#include "abc/01.h"
#include "abc/BUF.h"
#include "abc/NACL.h"

#define RDX_MAX_NESTING 64
#define RDX_MAX_INPUTS 64
#define RDX_MAX_STACKING 32

typedef enum {
    RDX_TYPE_ROOT = 0,
    // collection types
    RDX_TYPE_TUPLE = 1,
    RDX_TYPE_LINEAR = 2,
    RDX_TYPE_EULER = 3,
    RDX_TYPE_MULTIX = 4,
    RDX_TYPE_PLEX_LEN = 5,
    // primitive types
    RDX_TYPE_FLOAT = 5,
    RDX_TYPE_INT = 6,
    RDX_TYPE_REF = 7,
    RDX_TYPE_STRING = 8,
    RDX_TYPE_TERM = 9,
    RDX_TYPE_LEN = 10,
    RDX_TYPE_BLOB = 10,
} RDX_TYPE;

static const u8 RDX_TYPE_LIT[] = {
    0, 'P', 'L', 'E', 'X', 'F', 'I', 'R', 'S', 'T', 'B',
};
#define SKIL_LIT 'K'
extern const u8 RDX_TYPE_LIT_REV[];
extern const u8 RDX_TYPE_BRACKET_REV[];
static const char* RDX_TYPE_BRACKET_OPEN = " ([{<";
static const char* RDX_TYPE_BRACKET_CLOSE = " )]}>";

con ok64 RDXBAD = 0x6cd84b28d;
con ok64 RDXBADNEST = 0x6cd84b28d5ce71d;
con ok64 RDXBADFILE = 0x6cd84b28d3d254e;
con ok64 RDXNOROOM = 0x1b3615d86d8616;
con ok64 RDXBADORDR = 0x6cd84b28d61b35b;
con ok64 RDXBADARG = 0x1b3612ca34a6d0;

typedef enum {
    RDX_FMT_TLV = 0,  // FIXME NONE = 0
    RDX_FMT_WRITE = 1,
    RDX_FMT_JDR = 2,
    RDX_FMT_SKIL = 4,
    RDX_FMT_WAL = 6,
    RDX_FMT_MEM = 8,
    RDX_FMT_JDR_PIN = 10,
    RDX_FMT_Y = 12,
    RDX_FMT_LEN = 14,
} RDX_FMT;

typedef enum {
    RDX_UTF_ENC_UTF8 = 0,
    RDX_UTF_ENC_UTF16 = 1,
    RDX_UTF_ENC_UTF8_ESC = 2,
    RDX_UTF_ENC_UTF8_ESC_ML = 3,
    RDX_UTF_ENC_LEN = 4,
} RDX_UTF_ENC;

typedef struct {
    u64 src, seq;
} id128;

#define id128RevMask 63
#define id128SeqMask ((1UL << 60) - 1)  // - 63)
#define id128SrcMask ((1UL << 60) - 1)
typedef id128 const id128c;
typedef id128* id128p;
typedef id128c* id128cp;
fun b8 id128Z(id128cp a, id128cp b) {
    u64 at = a->seq & ~id128RevMask;
    u64 bt = b->seq & ~id128RevMask;
    return at == bt ? (a->src < b->src) : (at < bt);
}
fun b8 id128Eq(id128cp a, id128cp b) {
    u64 at = a->seq & ~id128RevMask;
    u64 bt = b->seq & ~id128RevMask;
    return at == bt && a->src == b->src;
}
fun b8 id128Empty(id128cp a) { return a->src == 0 && a->seq == 0; }
fun u64 id128Hash(id128cp a) { return u64hash2(a->src, a->seq); }

// fixme double-type
ok64 utf8sFeedID(utf8s u, id128cp a);
ok64 utf8sDrainID(utf8cs u, id128p a);

fun ok64 RDXu8sFeedID(u8s into, id128cp ref) {
    return ZINTu8sFeed128(into, ref->src, ref->seq);
}

fun ok64 RDXu8csDrainID(u8cs from, id128p ref) {
    return ZINTu8sDrain128(from, &ref->src, &ref->seq);
}

ok64 RDXutf8sFeedID(utf8s into, id128cp ref);
ok64 RDXutf8sDrainID(utf8cs from, id128p ref);

typedef int64_t RDXint;
typedef double RDXfloat;
typedef id128 RDXref;
typedef u8cs RDXstring;
typedef u8cs RDXterm;

typedef struct rdx rdx;

/**
 *  [ |  | ] memory owning buffer u8b
 *    ^
 *  [ ^rdx0 <-- rdx1 <-- rdx2 ... ] rdxb
 *
 *  Replicator Iterator Bufferophagous Omnivorous
 *
 *  A parsed RDX element, the basic building block of everything here
 *  and the ultimate hourglass waist. Serves as a stacked/hierarchical
 *  iterator over any kind of RDX store: SKIL, in-memory list, WAL,
 *  plain TLV RDX. Because of RDX tree structure, most of the time
 *  one uses rdxb (a stack of iterators). In the baseline scenario,
 *  the 0th element in the stack refers to the underlying memory
 *  owning buffer (mmap, etc). Lower elements consume a byte slice
 *  borrowed from the parent element.
 **/
typedef struct rdx {
    u8 format;       // RDX_FMT
    u8 type;         // RDX_TYPE (4:4)
    u8 cformat;      // RDX_FMT (for PLEX), RDX_UTF_ENC (for S)
    u8 ptype;        // parent type
    u32 len;         // length (format-dependant)
    id128 id;        // element id
    union {          // parsed values
        f64 f;       // float
        i64 i;       // integer
        id128 r;     // reference
        u8cs s;      // string
        u8cs t;      // term
        u8s plex;    // plex inners
        u8cs plexc;  // plex inners
        rdx* y[2];   //
    };
    union {
        struct {
            voidp extra;  // format-specific
            union {
                u8s into;   // same (non 0th, writers)
                u8cs data;  // data range (non 0th, readers)
            };
        };
        rdx* ins[3];
    };
} rdx;

typedef rdx* rdxp;
typedef rdx const* rdxcp;
typedef rdxp const* rdxpcp;
typedef rdxp* rdxpp;
typedef rdxp** rdxppp;

#define a_rdx(n, s, fmt)                     \
    rdx n = {.format = fmt | RDX_FMT_WRITE}; \
    u8sFork(n.into, s);

#define a_rdxc(n, s, fmt)    \
    rdx n = {.format = fmt}; \
    u8csFork(n.data, s);

#define a_rdxcb(n, s, fmt)          \
    a_pad(rdx, n, RDX_MAX_NESTING); \
    zerob(n);                       \
    rdxbFed1(n);                    \
    (**n).format = fmt;             \
    u8cgOf((**n).datag, s);

#define a_rdxb(n, s, fmt)               \
    a_pad(rdx, n, RDX_MAX_NESTING);     \
    zerob(n);                           \
    rdxbFed1(n);                        \
    (**n).format = fmt | RDX_FMT_WRITE; \
    u8gOf((**n).intog, s);

fun b8 rdxTypePlex(rdxcp p) { return p->type && p->type < RDX_TYPE_PLEX_LEN; }

fun b8 rdxWritable(rdxcp p) { return p->format & RDX_FMT_WRITE; }

fun int id128cmp(id128cp a, id128cp b) { return 0; }
fun int rdxcmp(rdxcp a, rdxcp b) { return id128cmp(&a->id, &b->id); }

fun void rdxMv(rdxp into, rdxcp from) {
    into->type = from->type;
    into->cformat = from->cformat;
    into->id = from->id;
    into->r = from->r;
}

fun ok64 rdxZ(rdxcp a, rdxcp b);

// rdxb stack: [PAST PEND(0/1) IDLE]
#define X(M, name) M##rdx##name
#include "abc/Bx.h"
#include "abc/HEAPx.h"
#undef X

fun int rdxpcmp(rdxp const* a, rdxp const* b) { return rdxcmp(*a, *b); }

fun ok64 rdxZ(rdxcp a, rdxcp b);

fun ok64 rdxpZ(rdxpcp a, rdxpcp b) { return rdxZ(*a, *b); }

#define X(M, name) M##rdxp##name
#include "abc/Bx.h"
#include "abc/HEAPx.h"
#undef X

/*
fun int rdxbpcmp(rdxbp const* a, rdxbp const* b) { return 0; }

#define X(M, name) M##rdxbp##name
#include "abc/Bx.h"
#undef X
*/

fun b8 rdxbWritable(rdxbp b) {
    u64 l = rdxbDataLen(b);
    return l && rdxWritable(rdxbAtP(b, l - 1));
}

fun u8 rdxbType(rdxbp x) {
    if (rdxbEmpty(x)) return RDX_TYPE_ROOT;
    return rdxbLast(x)->type;
}

fun b8 rdxbTypePlex(rdxbp x) {
    u8 t = rdxbType(x);
    return t && t < RDX_TYPE_PLEX_LEN;
}

typedef ok64 (*rdxz)(rdxcp a, rdxcp b);

typedef ok64 (*rdxf)(rdxp x);
typedef ok64 (*rdx2f)(rdxp c, rdxp p);
typedef ok64 (*rdxbf)(rdxbp x);

// 1. Next moves horizontally      -->
// 2. Into/Outo move by the stack  ^ v
ok64 rdxNextTLV(rdxp x);
ok64 rdxIntoTLV(rdxp c, rdxp p);
// Outo is optional when reading (a child iterator may not
// reach the END of sequence or Outo() may not be invoked
// or there might be several child iterators)
ok64 rdxOutoTLV(rdxp c, rdxp p);

ok64 JDRlexer(rdxp x);
ok64 rdxSkipJDR(rdxp x);
fun ok64 rdxNextJDR(rdxp x) {
    if (x->cformat != 0 && rdxTypePlex(x)) {
        ok64 o = rdxSkipJDR(x);
        if (o != OK) return o;
    }
    x->type = 0;
    ok64 o = JDRlexer(x);
    if (o == NEXT) {
        o = OK;
    } else if (o == OK && x->type == 0) {
        o = END;
    }
    return o;
}
ok64 rdxIntoJDR(rdxp c, rdxp p);
ok64 rdxOutoJDR(rdxp c, rdxp p);

ok64 rdxNextSKIL(rdxp x);
ok64 rdxIntoSKIL(rdxp c, rdxp p);
ok64 rdxOutoSKIL(rdxp c, rdxp p);

ok64 rdxNextWAL(rdxp x);
ok64 rdxIntoWAL(rdxp c, rdxp p);
ok64 rdxOutoWAL(rdxp c, rdxp p);

ok64 rdxNextMEM(rdxp x);
ok64 rdxIntoMEM(rdxp c, rdxp p);
ok64 rdxOutoMEM(rdxp c, rdxp p);

ok64 rdxNextY(rdxp x);
ok64 rdxIntoY(rdxp c, rdxp p);
ok64 rdxOutoY(rdxp c, rdxp p);

ok64 rdxWriteNextTLV(rdxp x);
ok64 rdxWriteIntoTLV(rdxp c, rdxp p);
ok64 rdxWriteOutoTLV(rdxp c, rdxp p);

ok64 rdxWriteNextJDR(rdxp x);
ok64 rdxWriteIntoJDR(rdxp c, rdxp p);
ok64 rdxWriteOutoJDR(rdxp c, rdxp p);

ok64 rdxWriteNextSKIL(rdxp x);
ok64 rdxWriteIntoSKIL(rdxp c, rdxp p);
ok64 rdxWriteOutoSKIL(rdxp c, rdxp p);

ok64 rdxWriteNextWAL(rdxp x);
ok64 rdxWriteIntoWAL(rdxp c, rdxp p);
ok64 rdxWriteOutoWAL(rdxp c, rdxp p);

ok64 rdxWriteNextMEM(rdxp x);
ok64 rdxWriteIntoMEM(rdxp c, rdxp p);
ok64 rdxWriteOutoMEM(rdxp c, rdxp p);

fun ok64 rdxNoNext(rdxp c) { return NOTIMPLYET; }
fun ok64 rdxNoInto(rdxp c, rdxp p) { return NOTIMPLYET; }
fun ok64 rdxNoOuto(rdxp c, rdxp p) { return NOTIMPLYET; }

static const rdxf VTABLE_NEXT[RDX_FMT_LEN] = {
    rdxNextTLV,  rdxWriteNextTLV,  rdxNextJDR, rdxWriteNextJDR,
    rdxNextSKIL, rdxWriteNextSKIL, rdxNextWAL, rdxWriteNextWAL,
    rdxNextMEM,  rdxWriteNextMEM,  rdxNextJDR, rdxWriteNextJDR,
    rdxNextY,    rdxNoNext};

static const rdx2f VTABLE_INTO[RDX_FMT_LEN] = {
    rdxIntoTLV,  rdxWriteIntoTLV,  rdxIntoJDR, rdxWriteIntoJDR,
    rdxIntoSKIL, rdxWriteIntoSKIL, rdxIntoWAL, rdxWriteIntoWAL,
    rdxIntoMEM,  rdxWriteIntoMEM,  rdxIntoJDR, rdxWriteIntoJDR,
    rdxIntoY,    rdxNoInto,
};

static const rdx2f VTABLE_OUTO[RDX_FMT_LEN] = {
    rdxOutoTLV,  rdxWriteOutoTLV,  rdxOutoJDR, rdxWriteOutoJDR,
    rdxOutoSKIL, rdxWriteOutoSKIL, rdxOutoWAL, rdxWriteOutoWAL,
    rdxOutoMEM,  rdxWriteOutoMEM,  rdxOutoJDR, rdxWriteOutoJDR,
    rdxOutoY,    rdxNoOuto,
};

fun ok64 rdxNext(rdxp x) { return VTABLE_NEXT[x->format](x); }
fun ok64 rdxInto(rdxp c, rdxp p) { return VTABLE_INTO[p->format](c, p); }
fun ok64 rdxOuto(rdxp c, rdxp p) { return VTABLE_OUTO[p->format](c, p); }

fun ok64 rdxRootZ(rdxcp a, rdxcp b) { return NO; }
fun ok64 rdxTupleZ(rdxcp a, rdxcp b) { return NO; }
ok64 rdx1Z(rdxcp a, rdxcp b);
fun ok64 rdxLinearZ(rdxcp a, rdxcp b) {
    u64 ao = (a->id.seq & id128SeqMask) - 1;
    u64 bo = (b->id.seq & id128SeqMask) - 1;
    if (ao == bo) {
        ao = a->id.src & id128SrcMask;
        bo = b->id.src & id128SrcMask;
    }
    if (ao != bo) {
        return u64Z(&ao, &bo);
    }
    return NO;
    // return rdx1Z(a, b);
}
ok64 rdxEulerTupleZ(rdxcp a, rdxcp b);
ok64 rdxEulerZ(rdxcp a, rdxcp b);
fun ok64 rdxMultixZ(rdxcp a, rdxcp b) { return u64Z(&a->id.src, &b->id.src); }
static const rdxz ZTABLE[RDX_TYPE_PLEX_LEN] = {
    rdxRootZ, rdxTupleZ, rdxLinearZ, rdxEulerZ, rdxMultixZ,
};
fun ok64 rdxZ(rdxcp a, rdxcp b) {
    rdxz Z = ZTABLE[a->ptype];
    return Z(a, b);
}
fun ok64 rdxWinZ(rdxcp a, rdxcp b) {
    // sane(a && b && a->ptype == b->ptype);
    u64 aseq = a->id.seq & id128SeqMask;
    u64 bseq = b->id.seq & id128SeqMask;
    if (aseq != bseq) return aseq > bseq;
    u64 asrc = a->id.src & id128SrcMask;
    u64 bsrc = b->id.src & id128SrcMask;
    if (asrc != bsrc) return asrc > bsrc;
    if (a->type != b->type) return u8Z(&a->type, &b->type);
    if (!rdxTypePlex(a)) return !rdx1Z(a, b);
    return NO;
}

ok64 rdxCopyF(rdxp into, rdxp from, voidf f, voidp p);
ok64 rdxCopy(rdxp into, rdxp from);
ok64 rdxbCopy(rdxbp into, rdxbp from);
ok64 rdxMerge(rdxp into, rdxg inputs);
ok64 rdxDiff(rdxp into, rdxp was, rdxp is);
ok64 rdxHash(sha256p hash, rdxp root);

typedef enum {
    UTF8_ENCODER_ONE = 0,
    UTF8_ENCODER_ALL = 1,
    UTF8_DECODER_ONE = 2,
    UTF8_DECODER_ALL = 3,
    UTF8_CODER_LEN = 4,
} UTF8_CODER_TYPE;

typedef ok64 (*UTFRecode)(u8s into, u8cs from);

fun ok64 UTF8Tokenizer(u8s into, u8cs from) {
    return utf8sDrain1utf8(into, from);
}
fun ok64 UTF8NImpl(u8s into, u8cs from) { return NOTIMPLYET; }
ok64 UTF8Escape(u8s into, u8cs from);
ok64 UTF8UnEscape(u8s into, u8cs from);
ok64 UTF8EscapeAll(u8s into, u8cs from);
ok64 UTF8UnEscapeAll(u8s into, u8cs from);
ok64 UTF8EscTick(u8s into, u8cs from);
ok64 UTF8UnEscTick(u8s into, u8cs from);
ok64 UTF8EscTickAll(u8s into, u8cs from);
ok64 UTF8UnEscTickAll(u8s into, u8cs from);

static UTFRecode UTABLE[RDX_UTF_ENC_LEN][UTF8_CODER_LEN] = {
    {UTF8Tokenizer, u8sFeedSome, UTF8Tokenizer, u8sFeedSome},
    {UTF8NImpl, UTF8NImpl, UTF8NImpl, UTF8NImpl},
    {
        UTF8Escape,
        UTF8EscapeAll,
        UTF8UnEscape,
        UTF8UnEscapeAll,
    },
    {
        UTF8EscTick,
        UTF8EscTickAll,
        UTF8UnEscTick,
        UTF8UnEscTickAll,
    }};

ok64 rdxbInto(rdxb b);
ok64 rdxbNext(rdxb b);
ok64 rdxbOuto(rdxb its);

ok64 rdxCopy(rdxp into, rdxp from);
ok64 rdxVerify(rdxp x);
ok64 rdxVerifyAll(rdxp from);

ok64 rdxStrip(rdxp into, rdxp from);

ok64 rdxHashBlake(rdxp of, blake256* hash);

#endif
