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
} RDX_TYPE;

static const u8 RDX_TYPE_LIT[] = {
    0, 'P', 'L', 'E', 'X', 'F', 'I', 'R', 'S', 'T',
};
extern const u8 RDX_TYPE_LIT_REV[];

#define RDX_WRITABLE 1

con ok64 RDXbad = 0x6cd866968;

typedef enum {
    RDX_FORMAT_TLV = 0,
    RDX_TLV_WRITER = RDX_FORMAT_TLV | RDX_WRITABLE,
    RDX_FORMAT_JDR = 2,
    RDX_JDR_WRITER = RDX_FORMAT_JDR | RDX_WRITABLE,
    RDX_FORMAT_LSM = 4,
    RDX_LSM_WRITER = RDX_FORMAT_LSM | RDX_WRITABLE,
    RDX_FORMAT_WAL = 6,
    RDX_WAL_WRITER = RDX_FORMAT_WAL | RDX_WRITABLE,
    RDX_FORMAT_RAM = 8,
    RDX_RAM_WRITER = RDX_FORMAT_RAM | RDX_WRITABLE,
    RDX_FORMAT_LEN = 10,
} RDX_FORMAT;

typedef enum {
    RDX_METHOD_NEXT = 0,
    RDX_METHOD_INTO = 1,
    RDX_METHOD_OUTO = 2,
    RDX_METHOD_SEEK = 3,
    RDX_METHOD_LEN = 4,
} RDX_METHOD;

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

typedef struct {
    u8 format;      // RDX_FORMAT
    u8 type;        // RDX_TYPE (4:4)
    u8 enc;         // RDX_UTF_ENC
    u8 mark;        // misc use
    u32 len;        // record length
    u64 pos;        // pos in the data range
    id128 id;       // element id
    union {         // parsed values
        f64 f;      // float
        i64 i;      // integer
        id128 r;    // reference
        u8cs s;     // string
        u8cs t;     // term
        u8cs plex;  // plex inners (?)
    };
    union {
        u8cs data;  // data range
        u8s into;   // same
    };
} rdx;

typedef rdx* rdxp;
typedef rdx const* rdxcp;

fun RDX_TYPE rdxType(rdxcp p) { return 0xf & p->type; }

fun RDX_TYPE rdxTypePlex(rdxcp p) { return rdxType(p) < RDX_TYPE_PLEX_LEN; }

fun b8 rdxWritable(rdxcp p) { return p->format & RDX_WRITABLE; }

fun int id128cmp(id128cp a, id128cp b) { return 0; }
fun int rdxcmp(rdxcp a, rdxcp b) { return id128cmp(&a->id, &b->id); }

fun void rdxMv(rdxp into, rdxcp from) {
    into->type = from->type;
    into->id = from->id;
    into->r = from->r;
}

// rdxb stack: [PAST PEND(0/1) IDLE]
#define X(M, name) M##rdx##name
#include "abc/Bx.h"
#undef X

fun int rdxpcmp(rdxp const* a, rdxp const* b) { return rdxcmp(*a, *b); }

#define X(M, name) M##rdxp##name
#include "abc/Bx.h"
#undef X

fun int rdxbpcmp(rdxbp const* a, rdxbp const* b) { return 0; }

#define X(M, name) M##rdxbp##name
#include "abc/Bx.h"
#undef X

fun b8 rdxbWritable(rdxbp b) {
    return rdxbDataLen(b) && rdxWritable(rdxbLast(b));
}

fun u8 rdxbType(rdxbp x) {
    if (rdxbEmpty(x)) return RDX_TYPE_ROOT;
    return rdxType(rdxbLast(x));
}

fun b8 rdxbTypePlex(rdxbp x) { return rdxbType(x) < RDX_TYPE_PLEX_LEN; }

typedef ok64 (*rdxz)(rdxcp a, rdxcp b);

typedef ok64 (*rdxf)(rdxb x);

// 1. Next operates the data      -->
// 2. Into/Outo operate the stack ^ v
// 3. Seek moves in the data      <- ->
ok64 RDXNextTLV(rdxb x);
ok64 RDXIntoTLV(rdxb x);
ok64 RDXOutoTLV(rdxb x);
ok64 RDXSeekTLV(rdxb x);

ok64 RDXNextJDR(rdxb x);
ok64 RDXIntoJDR(rdxb x);
ok64 RDXOutoJDR(rdxb x);
ok64 RDXSeekJDR(rdxb x);

ok64 RDXNextLSM(rdxb x);
ok64 RDXIntoLSM(rdxb x);
ok64 RDXOutoLSM(rdxb x);
ok64 RDXSeekLSM(rdxb x);

ok64 RDXNextWAL(rdxb x);
ok64 RDXIntoWAL(rdxb x);
ok64 RDXOutoWAL(rdxb x);
ok64 RDXSeekWAL(rdxb x);

ok64 RDXNextRAM(rdxb x);
ok64 RDXIntoRAM(rdxb x);
ok64 RDXOutoRAM(rdxb x);
ok64 RDXSeekRAM(rdxb x);

ok64 RDXWriteNextTLV(rdxb x);
ok64 RDXWriteIntoTLV(rdxb x);
ok64 RDXWriteOutoTLV(rdxb x);
ok64 RDXWriteSeekTLV(rdxb x);

ok64 RDXWriteNextJDR(rdxb x);
ok64 RDXWriteIntoJDR(rdxb x);
ok64 RDXWriteOutoJDR(rdxb x);
ok64 RDXWriteSeekJDR(rdxb x);

ok64 RDXWriteNextLSM(rdxb x);
ok64 RDXWriteIntoLSM(rdxb x);
ok64 RDXWriteOutoLSM(rdxb x);
ok64 RDXWriteSeekLSM(rdxb x);

ok64 RDXWriteNextWAL(rdxb x);
ok64 RDXWriteIntoWAL(rdxb x);
ok64 RDXWriteOutoWAL(rdxb x);
ok64 RDXWriteSeekWAL(rdxb x);

ok64 RDXWriteNextRAM(rdxb x);
ok64 RDXWriteIntoRAM(rdxb x);
ok64 RDXWriteOutoRAM(rdxb x);
ok64 RDXWriteSeekRAM(rdxb x);

static const rdxf VTABLE_NEXT[RDX_FORMAT_LEN] = {
    RDXNextTLV, RDXWriteNextTLV, RDXNextJDR, RDXWriteNextJDR,
    RDXNextLSM, RDXWriteNextLSM, RDXNextWAL, RDXWriteNextWAL,
    RDXNextRAM, RDXWriteNextRAM,
};

static const rdxf VTABLE_SEEK[RDX_FORMAT_LEN] = {
    RDXSeekTLV, RDXWriteSeekTLV, RDXSeekJDR, RDXWriteSeekJDR,
    RDXSeekLSM, RDXWriteSeekLSM, RDXSeekWAL, RDXWriteSeekWAL,
    RDXSeekRAM, RDXWriteSeekRAM,
};

fun rdxp rdxbPending(rdxbp x) { return x[2]; }

fun ok64 RDXInto(rdxb x) {
    if (!rdxbIdleLen(x)) return $noroom;
    rdxp lo = $term(rdxbData(x));
    if (rdxbDataLen(x)) {
        zerop(lo);
        rdxp hi = lo - 1;
        $mv(lo->data, hi->plex);
        lo->format = hi->format;
    }
    return rdxbFed1(x);
}

fun ok64 RDXOuto(rdxb x) {
    if (!rdxbDataLen(x)) return $nodata;
    --*rdxbIdle(x);
    rdxp lo = *rdxbIdle(x);
    if (rdxbDataLen(x)) {
        rdxp hi = lo - 1;
        $mv(hi->plex, lo->data);
    }
    return OK;
}

// convention: RDXInto on an empty buf expects the 0th entry have `data` set
fun ok64 RDXOpen(rdxb x, RDX_FORMAT fmt, u8cs data) {
    rdxp first = rdxbAtP(x, 0);
    first->format = fmt;
    $mv(first->data, (u8**)data);
    return RDXInto(x);
}

fun ok64 RDXWriteOpen(rdxb x, RDX_FORMAT fmt, u8s data) {
    rdxp first = rdxbAtP(x, 0);
    fmt |= RDX_WRITABLE;
    first->format = fmt;
    $mv(first->data, (u8**)data);
    return RDXInto(x);
}

// convention: RDXOuto that pops the last entry, sets data->(used space)
fun ok64 RDXClose(rdxb x, u8csp rest) {
    assert(rdxbDataLen(x) == 1);
    return RDXOuto(x);
}

fun ok64 RDXWriteClose(rdxb x, u8sp rest) {
    assert(rdxbDataLen(x) == 1);
    rdxp last = rdxbLast(x);
    $mv(rest, last->into);
    rest[0] += last->pos;
    return RDXOuto(x);
}

fun ok64 RDXNext(rdxb x) { return VTABLE_NEXT[rdxbLast(x)->format](x); }

fun ok64 RDXSeek(rdxb x) { return VTABLE_SEEK[rdxbLast(x)->format](x); }

fun ok64 RDXRootZ(rdxcp a, rdxcp b) { return NOTIMPLYET; }
fun ok64 RDXTupleZ(rdxcp a, rdxcp b) { return NOTIMPLYET; }
fun ok64 RDXLinearZ(rdxcp a, rdxcp b) { return NOTIMPLYET; }
fun ok64 RDXEulerZ(rdxcp a, rdxcp b) { return NOTIMPLYET; }
fun ok64 RDXMultixZ(rdxcp a, rdxcp b) { return NOTIMPLYET; }

static const rdxz ZTABLE[RDX_TYPE_PLEX_LEN] = {
    RDXRootZ, RDXTupleZ, RDXLinearZ, RDXEulerZ, RDXMultixZ,
};

ok64 RDXCopy(rdxb into, rdxb from);
ok64 RDXMerge(rdxb into, rdxbps heap);
ok64 RDXHash(sha256p hash, rdxb root);

// An RDX file minimalistic header, 16 bytes.
typedef struct {
    struct {
        u8 litT;    // always T
        u8 format;  // RDX_FORMAT
        u8 index;   // index format
        u8 crypto;  // crypto model
    };
    u32 meta_len;
    u64 data_len;
} rdxmeta128;

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

#endif
