#ifndef RDX_RDX_H
#define RDX_RDX_H
#include <abc/SHA.h>
#include <abc/TLV.h>
#include <abc/UTF8.h>
#include <abc/ZINT.h>

#include "abc/BUF.h"

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
    RDX_FORMAT_TLV = 0,  // FIXME NONE = 0
    RDX_FORMAT_JDR = 2,
    RDX_FORMAT_LSM = 4,
    RDX_FORMAT_WAL = 6,
    RDX_FORMAT_MEM = 8,
    RDX_FORMAT_JDR_PIN = 10,
    RDX_FORMAT_LEN = 12,
} RDX_FORMAT;

#define RDX_FORMAT_WRITE 1

typedef enum {
    RDX_UTF_ENC_UTF8 = 0,
    RDX_UTF_ENC_UTF16 = 1,
    RDX_UTF_ENC_UTF8_ESC = 2,
    RDX_UTF_ENC_UTF8_ESC_ML = 3,
    RDX_UTF_ENC_LEN = 4,
} RDX_UTF_ENC;

typedef enum {
    RDX_RW_READ = 0,
    RDX_RW_WRITE = 1,
    RDX_RW_LEN = 2,
} RDX_RW;

typedef struct {
    u64 src, seq;
} id128;

#define id128RevMask 63
typedef id128 const id128c;
typedef id128* id128p;
typedef id128c* id128cp;
fun b8 id128Z(id128cp a, id128cp b) {
    u64 at = a->seq & ~id128RevMask;
    u64 bt = b->seq & ~id128RevMask;
    return at == bt ? a->src < b->src : at < bt;
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

/**
 *  [ |  | ] memory owning buffer u8b
 *    ^
 *  [ ^rdx0 <-- rdx1 <-- rdx2 ... ] rdxb
 *
 *  A parsed RDX element, the basic building block of everything here
 *  and the ultimate hourglass waist. Serves as a stacked/hierarchical
 *  iterator over any kind of RDX store: LSM, in-memory list, WAL,
 *  plain TLV RDX. Because of RDX tree structure, most of the time
 *  one uses rdxb (a stack of iterators). In the baseline scenario,
 *  the 0th element in the stack refers to the underlying memory
 *  owning buffer (mmap, etc). Lower elements consume a byte slice
 *  borrowed from the parent element.
 **/
typedef struct {
    u8 format;      // RDX_FORMAT
    u8 type;        // RDX_TYPE (4:4)
    u8 cformat;     // RDX_FORMAT (for PLEX), RDX_UTF_ENC (for S)
    u8 ptype;       // parent type
    u32 len;        // length (format-dependant)
    id128 id;       // element id
    union {         // parsed values
        f64 f;      // float
        i64 i;      // integer
        id128 r;    // reference
        u8cs s;     // string
        u8cs t;     // term
        u8cs plex;  // plex inners
    };
    union {
        u8csp data;  // data range (non 0th, readers)
        u8sp into;   // same (non 0th, writers)
    };
} rdx;

typedef rdx* rdxp;
typedef rdx const* rdxcp;
typedef rdxp const* rdxpcp;
typedef rdxp* rdxpp;
typedef rdxp** rdxppp;

#define a_rdx(n, b, fmt)            \
    a_pad(rdx, n, RDX_MAX_NESTING); \
    zerob(n);                       \
    rdxbFed1(n);                    \
    (**n).format = fmt;             \
    (**n).host = b;

#define a_rdxr(n, s, fmt)           \
    a_pad(rdx, n, RDX_MAX_NESTING); \
    zerob(n);                       \
    rdxbFed1(n);                    \
    (**n).format = fmt;             \
    (**n).data = s;

#define a_rdxw(n, s, fmt)                  \
    a_pad(rdx, n, RDX_MAX_NESTING);        \
    zerob(n);                              \
    rdxbFed1(n);                           \
    (**n).format = fmt | RDX_FORMAT_WRITE; \
    (**n).into = s;

fun RDX_TYPE rdxTypePlex(rdxcp p) {
    return p->type && p->type < RDX_TYPE_PLEX_LEN;
}

fun b8 rdxWritable(rdxcp p) { return p->format & RDX_FORMAT_WRITE; }

fun int id128cmp(id128cp a, id128cp b) { return 0; }
fun int rdxcmp(rdxcp a, rdxcp b) { return id128cmp(&a->id, &b->id); }

fun void rdxMv(rdxp into, rdxcp from) {
    into->type = from->type;
    into->cformat = from->cformat;
    into->id = from->id;
    into->r = from->r;
}

// rdxb stack: [PAST PEND(0/1) IDLE]
#define X(M, name) M##rdx##name
#include "abc/Bx.h"
#undef X

static const rdxz ZTABLE[RDX_TYPE_PLEX_LEN];
fun ok64 rdxZ(rdxcp a, rdxcp b) {
    rdxz Z = ZTABLE[a->type];
    return NOTIMPLYET;
}

fun int rdxpcmp(rdxp const* a, rdxp const* b) { return rdxcmp(*a, *b); }
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

// 1. Next operates the data      -->
// 2. Into/Outo operate the stack ^ v
// 3. Seek moves in the data      <- ->
ok64 rdxNextTLV(rdxp x);
ok64 rdxIntoTLV(rdxp c, rdxp p);
ok64 rdxOutoTLV(rdxp c, rdxp p);

ok64 JDRlexer(rdxp x);
ok64 rdxSkipJDR(rdxp x);
fun ok64 rdxNextJDR(rdxp x) {
    if (x->cformat != 0 && rdxTypePlex(x)) {
        return rdxSkipJDR(x);
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

ok64 rdxNextLSM(rdxp x);
ok64 rdxIntoLSM(rdxp c, rdxp p);
ok64 rdxOutoLSM(rdxp c, rdxp p);

ok64 rdxNextWAL(rdxp x);
ok64 rdxIntoWAL(rdxp c, rdxp p);
ok64 rdxOutoWAL(rdxp c, rdxp p);

ok64 rdxNextMEM(rdxp x);
ok64 rdxIntoMEM(rdxp c, rdxp p);
ok64 rdxOutoMEM(rdxp c, rdxp p);

ok64 rdxWriteNextTLV(rdxp x);
ok64 rdxWriteIntoTLV(rdxp c, rdxp p);
ok64 rdxWriteOutoTLV(rdxp c, rdxp p);

ok64 rdxWriteNextJDR(rdxp x);
ok64 rdxWriteIntoJDR(rdxp c, rdxp p);
ok64 rdxWriteOutoJDR(rdxp c, rdxp p);

ok64 rdxWriteNextLSM(rdxp x);
ok64 rdxWriteIntoLSM(rdxp c, rdxp p);
ok64 rdxWriteOutoLSM(rdxp c, rdxp p);

ok64 rdxWriteNextWAL(rdxp x);
ok64 rdxWriteIntoWAL(rdxp c, rdxp p);
ok64 rdxWriteOutoWAL(rdxp c, rdxp p);

ok64 rdxWriteNextMEM(rdxp x);
ok64 rdxWriteIntoMEM(rdxp c, rdxp p);
ok64 rdxWriteOutoMEM(rdxp c, rdxp p);

static const rdxf VTABLE_NEXT[RDX_FORMAT_LEN] = {
    rdxNextTLV, rdxWriteNextTLV, rdxNextJDR, rdxWriteNextJDR,
    rdxNextLSM, rdxWriteNextLSM, rdxNextWAL, rdxWriteNextWAL,
    rdxNextMEM, rdxWriteNextMEM, rdxNextJDR, rdxWriteNextJDR,
};

static const rdx2f VTABLE_INTO[RDX_FORMAT_LEN] = {
    rdxIntoTLV, rdxWriteIntoTLV, rdxIntoJDR, rdxWriteIntoJDR,
    rdxIntoLSM, rdxWriteIntoLSM, rdxIntoWAL, rdxWriteIntoWAL,
    rdxIntoMEM, rdxWriteIntoMEM, rdxIntoJDR, rdxWriteIntoJDR,
};

static const rdx2f VTABLE_OUTO[RDX_FORMAT_LEN] = {
    rdxOutoTLV, rdxWriteOutoTLV, rdxOutoJDR, rdxWriteOutoJDR,
    rdxOutoLSM, rdxWriteOutoLSM, rdxOutoWAL, rdxWriteOutoWAL,
    rdxOutoMEM, rdxWriteOutoMEM, rdxOutoJDR, rdxWriteOutoJDR,
};

fun ok64 rdxNext(rdxp x) { return VTABLE_NEXT[x->format](x); }
fun ok64 rdxInto(rdxp c, rdxp p) { return VTABLE_INTO[p->format](c, p); }
fun ok64 rdxOuto(rdxp c, rdxp p) { return VTABLE_OUTO[p->format](c, p); }

fun ok64 rdxRootZ(rdxcp a, rdxcp b) { return NO; }
fun ok64 rdxTupleZ(rdxcp a, rdxcp b) { return NO; }
fun ok64 rdxLinearZ(rdxcp a, rdxcp b) {
    u64 ao = a->id.seq - 1;
    u64 bo = b->id.seq - 1;
    return u64Z(&ao, &bo);
}
ok64 rdx1Z(rdxcp a, rdxcp b);
fun ok64 rdxEulerZ(rdxcp a, rdxcp b) {
    if (a->type != b->type) return u8Z(&a->type, &b->type);
    if (a->type < RDX_TYPE_PLEX_LEN) return id128Z(&a->id, &b->id);
    return rdx1Z(a, b);
}
fun ok64 rdxMultixZ(rdxcp a, rdxcp b) { return u64Z(&a->id.src, &b->id.src); }
static const rdxz ZTABLE[RDX_TYPE_PLEX_LEN] = {
    rdxRootZ, rdxTupleZ, rdxLinearZ, rdxEulerZ, rdxMultixZ,
};

ok64 rdxCopy(rdxp into, rdxp from);
ok64 rdxbCopy(rdxbp into, rdxbp from);
ok64 rdxMerge(rdxp into, rdxbp inputs);
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

#endif
