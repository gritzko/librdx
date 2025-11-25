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
static const char* RDX_PLEX_OPEN_LIT = " ([{<";
static const char* RDX_PLEX_CLOSE_LIT = " )]}>";

con ok64 RDXbad = 0x6cd866968;
con ok64 RDXBADNEST = 0x6cd84b28d5ce71d;

typedef enum {
    RDX_FORMAT_TLV = 0,
    RDX_FORMAT_JDR = 2,
    RDX_FORMAT_LSM = 4,
    RDX_FORMAT_WAL = 6,
    RDX_FORMAT_RAM = 8,
    RDX_FORMAT_LEN = 10,
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

typedef struct {
    u8 format;      // RDX_FORMAT
    u8 type;        // RDX_TYPE (4:4)
    u8 enc;         // RDX_UTF_ENC
    u8 prnt;        // parent type
    u32 len;        // record length
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
        u8csp data;  // data range
        u8sp into;   // same
    };
} rdx;

typedef rdx* rdxp;
typedef rdx const* rdxcp;

#define a_rdx(n, s, fmt)            \
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

fun RDX_TYPE rdxType(rdxcp p) { return 0xf & p->type; }

fun RDX_TYPE rdxTypePlex(rdxcp p) { return rdxType(p) < RDX_TYPE_PLEX_LEN; }

fun b8 rdxWritable(rdxcp p) { return p->format & RDX_FORMAT_WRITE; }

fun int id128cmp(id128cp a, id128cp b) { return 0; }
fun int rdxcmp(rdxcp a, rdxcp b) { return id128cmp(&a->id, &b->id); }

fun void rdxMv(rdxp into, rdxcp from) {
    into->type = from->type;
    into->enc = from->enc;
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
    u64 l = rdxbDataLen(b);
    return l && rdxWritable(rdxbAtP(b, l - 1));
}

fun u8 rdxbType(rdxbp x) {
    if (rdxbEmpty(x)) return RDX_TYPE_ROOT;
    return rdxType(rdxbLast(x));
}

fun b8 rdxbTypePlex(rdxbp x) { return rdxbType(x) < RDX_TYPE_PLEX_LEN; }

typedef ok64 (*rdxz)(rdxcp a, rdxcp b);

typedef ok64 (*rdxf)(rdxp x);
typedef ok64 (*rdxbf)(rdxbp x);

// 1. Next operates the data      -->
// 2. Into/Outo operate the stack ^ v
// 3. Seek moves in the data      <- ->
ok64 rdxNextTLV(rdxp x);
ok64 rdxSeekTLV(rdxp x);

ok64 rdxNextJDR(rdxp x);
ok64 rdxSeekJDR(rdxp x);

ok64 rdxNextLSM(rdxp x);
ok64 rdxSeekLSM(rdxp x);

ok64 rdxNextWAL(rdxp x);
ok64 rdxSeekWAL(rdxp x);

ok64 rdxNextRAM(rdxp x);
ok64 rdxSeekRAM(rdxp x);

ok64 rdxWriteNextTLV(rdxp x);
ok64 rdxWriteSeekTLV(rdxp x);

ok64 rdxWriteNextJDR(rdxp x);
ok64 rdxWriteSeekJDR(rdxp x);

ok64 rdxWriteNextLSM(rdxp x);
ok64 rdxWriteSeekLSM(rdxp x);

ok64 rdxWriteNextWAL(rdxp x);
ok64 rdxWriteSeekWAL(rdxp x);

ok64 rdxWriteNextRAM(rdxp x);
ok64 rdxWriteSeekRAM(rdxp x);

static const rdxf VTABLE_NEXT[RDX_FORMAT_LEN] = {
    rdxNextTLV, rdxWriteNextTLV, rdxNextJDR, rdxWriteNextJDR,
    rdxNextLSM, rdxWriteNextLSM, rdxNextWAL, rdxWriteNextWAL,
    rdxNextRAM, rdxWriteNextRAM,
};

static const rdxf VTABLE_SEEK[RDX_FORMAT_LEN] = {
    rdxSeekTLV, rdxWriteSeekTLV, rdxSeekJDR, rdxWriteSeekJDR,
    rdxSeekLSM, rdxWriteSeekLSM, rdxSeekWAL, rdxWriteSeekWAL,
    rdxSeekRAM, rdxWriteSeekRAM,
};

fun ok64 rdxNext(rdxp x) { return VTABLE_NEXT[x->format](x); }

fun ok64 rdxSeek(rdxp x) { return VTABLE_SEEK[x->format](x); }

fun ok64 rdxRootZ(rdxcp a, rdxcp b) { return NOTIMPLYET; }
fun ok64 rdxTupleZ(rdxcp a, rdxcp b) { return NOTIMPLYET; }
fun ok64 rdxLinearZ(rdxcp a, rdxcp b) { return NOTIMPLYET; }
fun ok64 rdxEulerZ(rdxcp a, rdxcp b) { return NOTIMPLYET; }
fun ok64 rdxMultixZ(rdxcp a, rdxcp b) { return NOTIMPLYET; }

static const rdxz ZTABLE[RDX_TYPE_PLEX_LEN] = {
    rdxRootZ, rdxTupleZ, rdxLinearZ, rdxEulerZ, rdxMultixZ,
};

ok64 rdxCopy(rdxp into, rdxp from);
ok64 rdxbCopy(rdxb into, rdxb from);
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

ok64 rdxbInto(rdxb b);
ok64 rdxbOuto(rdxb its);

#endif
