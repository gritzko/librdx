#ifndef RDX_RDX_H
#define RDX_RDX_H
#include <abc/DIFF.h>
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
    // collection types, PLEX types (tuPle, Linear, Eulex, multiX)
    RDX_TYPE_TUPLE = 1,     // a tuple, (key, "value"), same as key:"value"
    RDX_TYPE_LINEAR = 2,    // an array, [1, 2, 3, 4, 5]
    RDX_TYPE_EULER = 3,     // a set or a map, {1, 2, 3} or {1:one, 2:two}
    RDX_TYPE_MULTIX = 4,    // a version vector, a counter, <1@alice-1, 2@bob-2>
    RDX_TYPE_PLEX_LEN = 5,  // less than this are PLEX elements (containers)
    // primitive types, FIRST types (Float, Int, Ref, String, Term)
    RDX_TYPE_FLOAT = 5,
    RDX_TYPE_INT = 6,
    RDX_TYPE_REF = 7,
    RDX_TYPE_STRING = 8,
    RDX_TYPE_TERM = 9,
    RDX_TYPE_LEN = 10,
    RDX_TYPE_BLOB = 10,
} RDX_TYPE;

static const u8 RDX_TYPE_LIT[] = {
    'O', 'P', 'L', 'E', 'X', 'F', 'I', 'R', 'S', 'T', 'B',
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
con ok64 RDXOVERBAD = 0x6cd8587ce6cb28d;
con ok64 BACK = 0x2ca314;

typedef enum {
    // uninitialized / empty slot
    RDX_FMT_NONE = 0,
    RDX_FMT_WRITE = 1,
    // type-length-value binary
    RDX_FMT_TLV = 2,
    // JSON like text format
    RDX_FMT_JDR = 4,
    // TLV with skiplists
    RDX_FMT_SKIL = 6,
    // todo WAL (log)
    RDX_FMT_WAL = 8,
    // todo
    RDX_FMT_MEM = 10,
    // JDR - colon notation subformat
    RDX_FMT_JDR_PIN = 12,
    // generic merge iterator
    RDX_FMT_Y = 14,
    // red black tree
    RDX_FMT_RB = 16,
    // metadata/tombstone stripping iterator
    RDX_FMT_STRIP = 18,
    // SLIK: unified SKIL+TLV format (no prefix length for containers)
    RDX_FMT_SLIK = 20,
    RDX_FMT_LEN = 22,
} RDX_FMT;

// File extension to format mapping
u8 RDXFmtFromExt(u8cs path);
u8csp RDXExtFromFmt(u8 fmt);

typedef enum {
    RDX_UTF_ENC_UTF8 = 0,
    RDX_UTF_ENC_UTF16 = 1,
    RDX_UTF_ENC_UTF8_ESC = 2,
    RDX_UTF_ENC_UTF8_ESC_ML = 3,
    RDX_UTF_ENC_LEN = 4,
    RDX_UTF_ENC_BITS = 3,  // mask for 2-bit encoding (0-3)
} RDX_UTF_ENC;

typedef struct {
    u64 src, seq;
} id128;

#define RON60_MASK ((1UL << 60) - 1)
typedef id128 const id128c;
typedef id128* id128p;
typedef id128c* id128cp;
fun b8 id128Z(id128cp a, id128cp b) {
    u64 at = a->seq & RON60_MASK;
    u64 bt = b->seq & RON60_MASK;
    if (at == bt) {
        at = a->src & RON60_MASK;
        bt = b->src & RON60_MASK;
    }
    return at < bt;
}
fun b8 id128RevZ(id128cp a, id128cp b) {
    u64 at = a->seq & (RON60_MASK - 1);
    u64 bt = b->seq & (RON60_MASK - 1);
    if (at == bt) {
        at = a->src & RON60_MASK;
        bt = b->src & RON60_MASK;
    }
    return at < bt;
}
fun b8 id128Eq(id128cp a, id128cp b) {
    u64 at = a->seq & RON60_MASK;
    u64 bt = b->seq & RON60_MASK;
    if (at == bt) {
        at = a->src & RON60_MASK;
        bt = b->src & RON60_MASK;
    }
    return at == bt;
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

// Location type: even values are positions, odd values are insertion points
typedef u32 loc32;

// Legacy RB index entry (24 bytes) - DEPRECATED, see rdx/RB.h for new format
// TODO: migrate RB.c to new self-contained format
#ifndef RB_RED_BIT
#define RB_RED_BIT (1U << 31)
#endif
#ifndef RB_NIL
#define RB_NIL (RB_RED_BIT - 1)
#endif
typedef struct rdxRB {
    u32 rb_left;
    u32 rb_right;
    u32 rb_parent;
    u32 rdx_parent;
    u32 body_off;
    u32 body_len;
} rdxRB;
typedef rdxRB* rdxRBp;
typedef rdxRB const* rdxRBcp;
typedef rdxRBp rdxRBs[2];
typedef rdxRBcp rdxRBcs[2];
typedef rdxRBs rdxRBb[2];
typedef rdxRBb* rdxRBbp;
fun b8 rdxRBred(rdxRBcp e) { return (e->rb_parent & RB_RED_BIT) != 0; }
fun u32 rdxRBparent(rdxRBcp e) { return e->rb_parent & ~RB_RED_BIT; }

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
 *  one uses rdxb (a stack of iterators).
 **/
typedef struct rdx {
    u8 format;       // RDX_FMT
    u8 flags;        // RDX_FMT (for PLEX), RDX_UTF_ENC (for S)
    u8 type;         // RDX_TYPE (4:4)
    u8 ptype;        // parent type
    loc32 loc;       // location in children list, u32max for END
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
            u8cp next;   // read/write position
            u8p  opt;    // range end (readers), write end (writers)
            u8bp bulk;  // buffer pointer (TLV/SKIL/RB)
        };
        rdx* ins[3];   // FMT_Y: constituent iterators of a merge
    };
} rdx;

typedef rdx* rdxp;
typedef rdx const* rdxcp;
typedef rdxp const* rdxpcp;
typedef rdxp* rdxpp;
typedef rdxp** rdxppp;

#define a_rdx(n, s, fmt)                     \
    rdx n = {.format = fmt | RDX_FMT_WRITE,  \
             .next = (u8p)(s)[0], .opt = (s)[1]};

#define a_rdxc(n, s, fmt)                    \
    rdx n = {.format = fmt,                   \
             .next = (u8p)(s)[0], .opt = (u8p)(s)[1]};

#define a_rdxcb(n, s, fmt)          \
    a_pad(rdx, n, RDX_MAX_NESTING); \
    zerob(n);                       \
    rdxbFed1(n);                    \
    (**n).format = fmt;             \
    (**n).next = (u8p)(s)[0];       \
    (**n).opt = (u8p)(s)[1];

#define a_rdxb(n, s, fmt)               \
    a_pad(rdx, n, RDX_MAX_NESTING);     \
    zerob(n);                           \
    rdxbFed1(n);                        \
    (**n).format = fmt | RDX_FMT_WRITE; \
    (**n).next = (u8p)(s)[0];           \
    (**n).opt = (s)[1];

fun b8 rdxTypePlex(rdxcp p) { return p->type && p->type < RDX_TYPE_PLEX_LEN; }

fun b8 rdxWritable(rdxcp p) { return p->format & RDX_FMT_WRITE; }

fun int id128cmp(id128cp a, id128cp b) { return 0; }
fun int rdxcmp(rdxcp a, rdxcp b) { return id128cmp(&a->id, &b->id); }

fun void rdxMv(rdxp into, rdxcp from) {
    into->type = from->type;
    into->flags = from->flags;
    into->id = from->id;
    into->r = from->r;
}

fun b8 rdxZ(rdxcp a, rdxcp b);

// rdxb stack: [PAST PEND(0/1) IDLE]
#define X(M, name) M##rdx##name
#include "abc/Bx.h"
#include "abc/HEAPx.h"
#undef X

fun int rdxpcmp(rdxp const* a, rdxp const* b) { return rdxcmp(*a, *b); }

fun b8 rdxZ(rdxcp a, rdxcp b);

fun b8 rdxpZ(rdxpcp a, rdxpcp b) { return rdxZ(*a, *b); }

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

typedef b8 (*rdxz)(rdxcp a, rdxcp b);

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

// Parse FIRST value into x based on x->type (already set)
ok64 rdxParseVal(rdxp x, u8cs val);

// Drain one TLV record from stream into x (advances stream)
ok64 rdxDrainTLV(rdxp x, u8cs stream);

ok64 JDRLexer(rdxp x);
ok64 rdxSkipJDR(rdxp x);
fun ok64 rdxNextJDR(rdxp x) {
    if (rdxTypePlex(x) && x->plexc[0] == (u8cp)x->next) {
        ok64 o = rdxSkipJDR(x);
        if (o != OK) return o;
    }
    x->type = 0;
    x->flags = 0;
    ok64 o = JDRLexer(x);
    if (o == NEXT) {
        if (x->flags == RDX_FMT_JDR_PIN)
            x->next = (u8p)x->plexc[0];  // fixme ugly
        o = OK;
    } else if (o == BACK) {
        // PIN tuple hit parent's delimiter; next was backed up by callback
        o = END;
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

ok64 rdxNextRB(rdxp x);
ok64 rdxIntoRB(rdxp c, rdxp p);
ok64 rdxOutoRB(rdxp c, rdxp p);

ok64 rdxNextSTRIP(rdxp x);
ok64 rdxIntoSTRIP(rdxp c, rdxp p);
ok64 rdxOutoSTRIP(rdxp c, rdxp p);

ok64 rdxWriteNextSTRIP(rdxp x);
ok64 rdxWriteIntoSTRIP(rdxp c, rdxp p);
ok64 rdxWriteOutoSTRIP(rdxp c, rdxp p);

void rdxInitSLIK(rdxp x, u8bp buf, u64bp stack);
ok64 rdxNextSLIK(rdxp x);
// Seeking: p must be positioned AT a container (after rdxNext returned it).
// To seek, set c->type and c->i (or c->s, etc) before calling rdxInto.
// EULER seeks by value (key value for tuples), TUPLE/LINEAR seek by 
// positional index (seek modes may or may not be implemented in all formats)
// If c->type==0, just enter and position before the first child.
ok64 rdxIntoSLIK(rdxp c, rdxp p);
ok64 rdxOutoSLIK(rdxp c, rdxp p);

ok64 rdxWriteNextSLIK(rdxp x);
ok64 rdxWriteIntoSLIK(rdxp c, rdxp p);
ok64 rdxWriteOutoSLIK(rdxp c, rdxp p);

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

ok64 rdxWriteNextRB(rdxp x);
ok64 rdxWriteIntoRB(rdxp c, rdxp p);
ok64 rdxWriteOutoRB(rdxp c, rdxp p);
// Diff base (with metadata) against neu (no metadata), produces patch
ok64 rdxDiffRB(rdxp patch, rdxp base, rdxp neu, u64p nextSeq);

fun ok64 rdxNoNext(rdxp c) { return NOTIMPLYET; }
fun ok64 rdxNoInto(rdxp c, rdxp p) { return NOTIMPLYET; }
fun ok64 rdxNoOuto(rdxp c, rdxp p) { return NOTIMPLYET; }

static const rdxf VTABLE_NEXT[RDX_FMT_LEN] = {
    rdxNoNext,    rdxNoNext,                         // NONE
    rdxNextTLV,   rdxWriteNextTLV,   rdxNextJDR,  rdxWriteNextJDR,
    rdxNextSKIL,  rdxWriteNextSKIL,  rdxNextWAL,  rdxWriteNextWAL,
    rdxNextMEM,   rdxWriteNextMEM,   rdxNextJDR,  rdxWriteNextJDR,
    rdxNextY,     rdxNoNext,         rdxNextRB,   rdxWriteNextRB,
    rdxNextSTRIP, rdxWriteNextSTRIP, rdxNextSLIK, rdxWriteNextSLIK,
};

static const rdx2f VTABLE_INTO[RDX_FMT_LEN] = {
    rdxNoInto,    rdxNoInto,                         // NONE
    rdxIntoTLV,   rdxWriteIntoTLV,   rdxIntoJDR,  rdxWriteIntoJDR,
    rdxIntoSKIL,  rdxWriteIntoSKIL,  rdxIntoWAL,  rdxWriteIntoWAL,
    rdxIntoMEM,   rdxWriteIntoMEM,   rdxIntoJDR,  rdxWriteIntoJDR,
    rdxIntoY,     rdxNoInto,         rdxIntoRB,   rdxWriteIntoRB,
    rdxIntoSTRIP, rdxWriteIntoSTRIP, rdxIntoSLIK, rdxWriteIntoSLIK,
};

static const rdx2f VTABLE_OUTO[RDX_FMT_LEN] = {
    rdxNoOuto,    rdxNoOuto,                         // NONE
    rdxOutoTLV,   rdxWriteOutoTLV,   rdxOutoJDR,  rdxWriteOutoJDR,
    rdxOutoSKIL,  rdxWriteOutoSKIL,  rdxOutoWAL,  rdxWriteOutoWAL,
    rdxOutoMEM,   rdxWriteOutoMEM,   rdxOutoJDR,  rdxWriteOutoJDR,
    rdxOutoY,     rdxNoOuto,         rdxOutoRB,   rdxWriteOutoRB,
    rdxOutoSTRIP, rdxWriteOutoSTRIP, rdxOutoSLIK, rdxWriteOutoSLIK,
};

fun ok64 rdxNext(rdxp x) { return VTABLE_NEXT[x->format](x); }
fun ok64 rdxInto(rdxp c, rdxp p) { return VTABLE_INTO[p->format](c, p); }
fun ok64 rdxOuto(rdxp c, rdxp p) { return VTABLE_OUTO[p->format](c, p); }

// Buffer initialization functions
typedef void (*rdxInitFn)(rdxp, u8bp);
fun void rdxNoInit(rdxp x, u8bp buf) {
    (void)x;
    (void)buf;
}
fun void rdxInitReadTLV(rdxp x, u8bp buf) {
    x->next = buf[1];
    x->opt = buf[2];
}
fun void rdxInitWriteTLV(rdxp x, u8bp buf) { x->bulk = buf; }
fun void rdxInitReadSKIL(rdxp x, u8bp buf) {
    x->next = buf[1];
    x->opt = buf[2];
    x->bulk = buf;
}
fun void rdxInitWriteSKIL(rdxp x, u8bp buf) { x->bulk = buf; }
fun void rdxInitReadJDR(rdxp x, u8bp buf) {
    x->next = buf[1];
    x->opt = buf[2];
}
fun void rdxInitWriteJDR(rdxp x, u8bp buf) { x->bulk = buf; }

// Full SLIK init (use these)
void rdxInitSLIK(rdxp x, u8bp buf, u64bp stack);       // read
void rdxWriteInitSLIK(rdxp x, u8bp buf, u64bp stack);  // write
ok64 rdxWriteFinishSLIK(rdxp x);                       // finalize root

// Simple inits for VTABLE (just set bulk)
fun void rdxInitReadSLIK(rdxp x, u8bp buf) { x->next = buf[1]; x->bulk = buf; }
fun void rdxInitWriteSLIK(rdxp x, u8bp buf) { x->bulk = buf; }

fun void rdxInitReadRB(rdxp x, u8bp buf) {
    x->next = NULL;  // not positioned yet
    x->bulk = buf;
    // Tree root is in buffer header (first 4 bytes) - no copy needed
}
fun void rdxInitWriteRB(rdxp x, u8bp buf) {
    x->next = NULL;
    x->bulk = buf;
    // Initialize header: tree root = NIL, advance write pos past header
    if (buf[1] + 4 <= buf[3]) {
        *(u32*)buf[1] = RB_NIL;
        ((u8**)buf)[2] = buf[1] + 4;
    }
}

static const rdxInitFn VTABLE_INIT[RDX_FMT_LEN] = {
    rdxNoInit,       rdxNoInit,                            // NONE
    rdxInitReadTLV,  rdxInitWriteTLV,  rdxInitReadJDR,  rdxInitWriteJDR,
    rdxInitReadSKIL, rdxInitWriteSKIL, rdxNoInit,       rdxNoInit,
    rdxNoInit,       rdxNoInit,        rdxInitReadJDR,  rdxInitWriteJDR,
    rdxNoInit,       rdxNoInit,        rdxInitReadRB,   rdxInitWriteRB,
    rdxNoInit,       rdxNoInit,        rdxInitReadSLIK, rdxInitWriteSLIK,
};

fun void rdxInit(rdxp x, u8 fmt, u8bp buf) {
    *x = (rdx){.format = fmt};
    VTABLE_INIT[fmt](x, buf);
}
fun void rdxWriteInit(rdxp x, u8 fmt, u8bp buf) {
    rdxInit(x, fmt | RDX_FMT_WRITE, buf);
}

// SLIK read requires a skip stack with end-of-data sentinel.
// Call after rdxInit() for SLIK read mode.
// skipbuf: u64b buffer to use as skip stack (will be modified)
// Sets x->opt to the skip buffer with initial sentinel.
#define RDX_SLIK_ENTRY_CLOSE 3
#define RDX_SLIKEntryEncode(pos, type) (((pos) << 2) | ((type) & 3))
fun void rdxInitOptSLIK(rdxp x, u64b skipbuf) {
    u64 end_offset = u8bDataLen(x->bulk);
    u64bFeed1(skipbuf, RDX_SLIKEntryEncode(end_offset, RDX_SLIK_ENTRY_CLOSE));
    x->opt = (u8p)skipbuf;
}

fun b8 rdxRootZ(rdxcp a, rdxcp b) { return NO; }
fun b8 rdxTupleZ(rdxcp a, rdxcp b) { return NO; }
b8 rdx1Z(rdxcp a, rdxcp b);
fun b8 rdxLinearZ(rdxcp a, rdxcp b) { return id128RevZ(&a->id, &b->id); }
b8 rdxTupleKeyWinZ(rdxcp a, rdxcp b);
b8 rdxEulerZ(rdxcp a, rdxcp b);
b8 rdxIsEmptyTuple(rdxcp x);
fun b8 rdxMultixZ(rdxcp a, rdxcp b) { return u64Z(&a->id.src, &b->id.src); }
static const rdxz ZTABLE[RDX_TYPE_PLEX_LEN] = {
    rdxRootZ, rdxTupleZ, rdxLinearZ, rdxEulerZ, rdxMultixZ,
};
// a LESS comparator for any rdx'es
fun b8 rdxZ(rdxcp a, rdxcp b) {
    rdxz Z = ZTABLE[a->ptype];
    return Z(a, b);
}
fun b8 rdxWinZ(rdxcp a, rdxcp b) {
    // sane(a && b && a->ptype == b->ptype);
    u64 aseq = a->id.seq & RON60_MASK;
    u64 bseq = b->id.seq & RON60_MASK;
    if (aseq != bseq) return aseq > bseq;
    u64 asrc = a->id.src & RON60_MASK;
    u64 bsrc = b->id.src & RON60_MASK;
    if (asrc != bsrc) return asrc > bsrc;
    if (a->type != b->type) return u8Z(&a->type, &b->type);
    if (!rdxTypePlex(a)) {
        return rdx1Z(b, a);
    } else if (a->type == RDX_TYPE_TUPLE) {
        return rdxTupleKeyWinZ(a, b);
    }
    return NO;
}

ok64 rdxCopyF(rdxp into, rdxp from, voidf f, voidp p);
ok64 rdxCopy1(rdxp into, rdxp from);  // Copy current element and contents
ok64 rdxCopy(rdxp into, rdxp from);
ok64 rdxbCopy(rdxbp into, rdxbp from);
ok64 rdxMerge(rdxp into, rdxg inputs);

// Normalize: rdxMerge with single input (convenience wrapper)
// Uses a buffer to provide space for child iterators during merge
fun ok64 rdxNormalize(rdxp into, rdxp from) {
    a_pad(rdx, inputs, 4 * 64);  // buffer with idle space
    rdxbZero(inputs);
    **inputs = *from;
    rdxbFed1(inputs);
    return rdxMerge(into, rdxbDataIdle(inputs));
}

// Stream copy to SKIL with data pumping (flushes to fd when buffer fills)
ok64 rdxCopyToSKIL(rdxp from, int fd, u8bp buf, size_t threshold);

// Merge multiple SKIL inputs to SKIL output, no vtable lookups
ok64 rdxCopySKILs(rdxg inputs, int fd, u8bp buf, size_t threshold);

// Convert: read from one format, write to another (also normalizes)
ok64 rdxConvert(rdxp into, rdxp from);
ok64 rdxDiff(rdxp into, rdxp was, rdxp is);
ok64 rdxDiff2(rdxp patch, rdxp doc, rdxp neu, u8g oud_buf, u8g neu_buf,
              u64g hash_buf, i32s work, e32g edl);
ok64 rdxHash(sha256p hash, rdxp root);

typedef enum {
    UTF8_ENCODER_ONE = 0,
    UTF8_ENCODER_ALL = 1,
    UTF8_DECODER_ONE = 2,
    UTF8_DECODER_ALL = 3,
    UTF8_CODER_LEN = 4,
} UTF8_CODER_TYPE;

typedef ok64 (*UTFRecode)(u8s into, u8cs from);
typedef ok64 (*u8csCB)(u8cs chunk, voidp ctx);

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
ok64 UTFRecodeCB(u8cs from, u8 enc, u8 coder, u8csCB cb, voidp ctx);

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
// Type-specific validators (callable from parsers)
ok64 rdxVerifyFloat(f64 f);
ok64 rdxVerifyInt(i64 i);
ok64 rdxVerifyRef(id128 r);
ok64 rdxVerifyId(id128 id);
ok64 rdxVerifyString(u8cs str, u8 enc);
ok64 rdxVerifyTerm(u8cs term);
// Full element and sequence verification
ok64 rdxVerify(rdxp x, u8cp rec);
ok64 rdxVerifyTLV(rdxp x, u8cp rec);
ok64 rdxVerifyAll(rdxp from);

ok64 rdxStrip(rdxp into, rdxp from);

ok64 rdxHashBlake(rdxp of, blake256* hash);

// Path-dependent u64 hashes (pre-order traversal)
// Modes:
//   0 (FIRST)    - only FIRST elements (ignores PLEX containers)
//   1 (PREORDER) - PLEX hash before contents (standard pre-order)
//   2 (BRACKET)  - PLEX hash before AND after contents
// RAP hashing modes
#define RDX_HASH_FIRST 0     // only FIRST elements
#define RDX_HASH_PREORDER 1  // PLEX open before contents
#define RDX_HASH_BRACKET 2   // PLEX open AND close around contents

// Hash layout: [marker:2][depth:6][hash:56]
// - bits 62-63: marker (00=FIRST, 01=OPEN, 10=CLOSE)
// - bits 56-61: nesting depth (0-63)
// - bits 0-55: hash value
#define RDX_HASH_MARK_MASK (3ULL << 62)
#define RDX_HASH_DEPTH_MASK (63ULL << 56)
#define RDX_HASH_VALUE_MASK ((1ULL << 56) - 1)
#define RDX_HASH_META_MASK (RDX_HASH_MARK_MASK | RDX_HASH_DEPTH_MASK)

#define RDX_HASH_MARK_FIRST (0ULL << 62)
#define RDX_HASH_MARK_OPEN (1ULL << 62)
#define RDX_HASH_MARK_CLOSE (2ULL << 62)

// Apply marker and depth to hash
#define RDX_HASH_AS_FIRST(h, d) \
    (((h) & RDX_HASH_VALUE_MASK) | RDX_HASH_MARK_FIRST | ((u64)(d) << 56))
#define RDX_HASH_AS_OPEN(h, d) \
    (((h) & RDX_HASH_VALUE_MASK) | RDX_HASH_MARK_OPEN | ((u64)(d) << 56))
#define RDX_HASH_AS_CLOSE(h, d) \
    (((h) & RDX_HASH_VALUE_MASK) | RDX_HASH_MARK_CLOSE | ((u64)(d) << 56))

// Legacy compatibility (depth=0)
#define RDX_HASH_MASK RDX_HASH_MARK_MASK
ok64 rdxRapidHashesF(u64s hashes, rdxp from, u8 flags);
ok64 rdxRapidHashes(u64s hashes, rdxp from);

#endif
