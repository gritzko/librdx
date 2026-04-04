#ifndef LIBRDX_B_h
#define LIBRDX_B_h
#define _BSD_SOURCE 1
#define _DEFAULT_SOURCE 1
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "01.h"
#include "OK.h"
#include "S.h"

con ok64 MMAPFAIL = 0x5962993ca495;
con ok64 BMAPFAIL = 0x2d62993ca495;
con ok64 BNOTNULL = 0x2d761d5de555;
con ok64 BISNULL = 0xb49c5de555;
con ok64 BNOTMAP = 0xb5d8756299;
con ok64 BALLOCFAIL = 0x2ca55560c3ca495;
con ok64 BNOTALLOC = 0xb5d874a55560c;
con ok64 BNOROOM = 0xb5d86d8616;
con ok64 BNODATA = 0xb5d834a74a;
con ok64 BBADARG = 0xb2ca34a6d0;
con ok64 BMISS = 0xb59271c;
con ok64 BADARENA	= 0x2ca34a6ce5ca;

#define B_MAX_LEN_BITS 48
#define B_MAX_LEN (1UL << B_MAX_LEN_BITS)

#define BTYPE(T) typedef T *const B##T[4];

BTYPE(void);
typedef void *const *void$;
typedef void const *const *voidc$;
typedef void *voidb[4];
typedef void **voidbp;

#define B(T, b) B##T b = {0, 0, 0, 0}

#define Bbusy(b) {b[0], b[2]}

#define Bpast(b) (b + 0)
#define Bdata(b) (b + 1)
#define Bidle(b) (b + 2)
#define $past(b) (b + 0)
#define $data(b) (b + 1)
#define $idle(b) (b + 2)

#define Blastp(b) ((b)[2] - 1)
#define Blast(b) (*Blastp(b))

#define Bi(b) *(b[2])
#define Bd(b) *(b[1])

#define Blen(b) (b[3] - b[0])
#define Bsize(b) ((u8 *)(b[3]) - (u8 *)(b[0]))
#define BDataSize(b) ((u8 *)(b[2]) - (u8 *)(b[1]))
#define Busylen(b) (b[2] - b[0])
#define Busysize(b) ((u8 const *)b[2] - (u8 const *)b[0])
#define Bpastlen(b) $len(Bpast(b))
#define Bdatalen(b) $len(Bdata(b))
#define Bidlelen(b) $len(Bidle(b))
#define Bempty(b) $empty(Bdata(b))

#define Bok(b) \
    (b != NULL && b[0] != NULL && b[0] <= b[1] && b[1] <= b[2] && b[2] <= b[3])
#define BNULL(b) (b == NULL || b[0] == NULL)
#define Bhasroom(b) (b[2] < b[3])

#define aBpad(T, n, l) \
    T _##n[(l)];       \
    B##T n = {_##n, _##n, _##n, _##n + (l)};

#define aBpad2(T, n, l)                           \
    T _##n[(l)];                                  \
    B##T n##buf = {_##n, _##n, _##n, _##n + (l)}; \
    T##$ n##idle = T##bIdle(n##buf);              \
    T##$ n##data = T##bData(n##buf);

#define a_pad(T, n, l)                       \
    T _##n[(l)];                             \
    T##b n = {_##n, _##n, _##n, _##n + (l)}; \
    T##sp n##_idle = T##bIdle(n);            \
    T##sp n##_data = T##bData(n);            \
    T##csp n##_datac = T##cbData((T const **)n);

#define a_pad0(T, n, l) \
    a_pad(T, n, l);     \
    zerob(n);

#define s_pad(T, n, l)                              \
    static T _##n[(l)];                             \
    static T *n[4] = {_##n, _##n, _##n, _##n + (l)}

#define aBcpad(T, n, l)                           \
    T _##n[(l)];                                  \
    B##T n##buf = {_##n, _##n, _##n, _##n + (l)}; \
    T##$ n##idle = T##bIdle(n##buf);              \
    T##c##$ n##data = T##bDataC(n##buf);

#define aB(T, name)                                    \
    T *name##buf[4] = {};                              \
    T **name##data = name##buf + 1;                    \
    T const **name##cdata = (T const **)name##buf + 1; \
    T **name##idle = name##buf + 2;

#define aBusy(T, name, buf) T *name[2] = {buf[0], buf[2]};

#define a_fake(T, name, s) T *name[4] = {s[0], s[0], s[1], s[1]};

#define zerob(buf) memset(buf[0], 0, Bsize(buf))

#define Bwithin(b, s) (s[0] >= b[0] && s[1] <= b[3])

fun void _Brebase(Bvoid buf, void *newhead, size_t newlen) {
    u8 **b = (u8 **)buf;
    size_t data = b[1] - b[0];
    if (data > newlen) data = newlen;
    size_t idle = b[2] - b[0];
    if (idle > newlen) idle = newlen;
    b[0] = (u8 *)newhead;
    b[1] = b[0] + data;
    b[2] = b[0] + idle;
    b[3] = b[0] + newlen;
}

fun ok64 Balloc(Bvoid b, size_t sz) {
    if (!BNULL(b)) return BNOTNULL;
    u8 *p = (u8 *)malloc(sz);
    if (p == NULL) return BALLOCFAIL;
    u8 **buf = (u8 **)b;
    buf[0] = buf[1] = buf[2] = p;
    buf[3] = p + sz;
    return OK;
}

fun ok64 Brealloc(Bvoid b, size_t sz) {
    void *m = realloc(b[0], sz);
    if (m == NULL) return NOROOM;
    _Brebase(b, m, sz);
    return OK;
}

fun ok64 Breserve(Bvoid b, size_t sz) {
    size_t i = $size(Bidle(b));
    if (i >= sz) return OK;
    return Brealloc(b, roundup(Busysize(b) + sz, 256));
}

#define BNULLify(buf) memset((void **)buf, 0, sizeof(Bvoid));

fun ok64 Bfree(Bvoid buf) {
    if (BNULL(buf)) return BISNULL;
    free(buf[0]);
    BNULLify(buf);
    return OK;
}

#define Bump(b, l)                      \
    {                                   \
        u8 **bb = (u8 **)b;             \
        size_t bytes = l * sizeof(**b); \
        if (bb[2] + bytes > bb[3])      \
            bb[2] = bb[3];              \
        else                            \
            bb[2] += bytes;             \
    }

#define Back(b)             \
    {                       \
        u8 **bb = (u8 **)b; \
        bb[2] = bb[1];      \
    }

#define Backpast(b)         \
    {                       \
        u8 **bb = (u8 **)b; \
        bb[1] = bb[0];      \
    }

#define eatB(T, i, b) for (T *i = b[1]; i < b[2]; i++)

#define Bate(b)              \
    {                        \
        u8c **v = (u8c **)b; \
        v[1] = v[2];         \
    }

#define Brewind(b, past, data)          \
    {                                   \
        assert(past + data <= Blen(b)); \
        b[1] = b[0] + past;             \
        b[2] = b[0] + past + data;      \
    }

#define Breset(buf)          \
    {                        \
        u8 **b = (u8 **)buf; \
        b[1] = b[2] = b[0];  \
    }

#define Breset1(buf)         \
    {                        \
        u8 **b = (u8 **)buf; \
        b[1] = b[0];         \
        b[2] = b[3];         \
    }

#define aB$(T, n, buf, from, till) T *n[2] = {buf[0] + from, buf[0] + till};

#define Batp(buf, ndx) (buf[0] + ndx)
#define Bat(buf, ndx) (buf[0][(ndx)])

#define Bpush(buf) (buf[2]++)
#define Bpop(buf) (--buf[2])
#define Btop(buf) (buf[2] - 1)

typedef struct {
    u64 from;
    u64 till;
} range64;

// Byte offset range [lo, hi)
typedef struct { u32 lo; u32 hi; } range32;
con range32 range32Z = {};

fun int range32cmp(range32 const *a, range32 const *b) {
    if (a->lo != b->lo) return (a->lo > b->lo) - (a->lo < b->lo);
    return (a->hi > b->hi) - (a->hi < b->hi);
}

// Match range: pairs haystack and needle byte ranges
typedef struct { range32 hay; range32 ndl; } match32;
con match32 match32Z = {};

fun int match32cmp(match32 const *a, match32 const *b) {
    return memcmp(a, b, sizeof(match32));
}

#endif
