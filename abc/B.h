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

con ok64 Bmapfail = 0x2f1974aa5b70;
con ok64 Bnotnull = 0x2f2cf8cb9c30;
con ok64 Bisnil = 0x2eddf2b70;
con ok64 Bnotmap = 0xbcb3e31974;
con ok64 Ballocfail = 0x2e5c30ce7aa5b70;
con ok64 Bnotalloc = 0xbcb3e25c30ce7;
con ok64 Bnoroom = 0xbcb3db3cf1;
con ok64 Bnodata = 0xbcb3a25e25;
con ok64 Bbadarg = 0xb9a5a25dab;
con ok64 Bmiss = 0xbc6ddf7;

#define B_MAX_LEN_BITS 48
#define B_MAX_LEN (1UL << B_MAX_LEN_BITS)

#define BTYPE(T) typedef T *const B##T[4];

BTYPE(void);
typedef void *const *void$;
typedef void const *const *voidc$;
typedef void *const *voidB;

#define B(T, b) B##T b = {0, 0, 0, 0}

#define Bbusy(b) \
    { b[0], b[2] }

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
#define Busylen(b) (b[2] - b[0])
#define Busysize(b) ((u8 const *)b[2] - (u8 const *)b[0])
#define Bpastlen(b) $len(Bpast(b))
#define Bdatalen(b) $len(Bdata(b))
#define Bidlelen(b) $len(Bidle(b))
#define Bempty(b) $empty(Bdata(b))

#define Bok(b) \
    (b != nil && b[0] != nil && b[0] <= b[1] && b[1] <= b[2] && b[2] <= b[3])
#define Bnil(b) (b == nil || b[0] == nil)
#define Bhasroom(b) (b[2] < b[3])

#define aBpad(T, n, l) \
    T _##n[(l)];       \
    B##T n = {_##n, _##n, _##n, _##n + (l)};

#define aBpad2(T, n, l)                           \
    T _##n[(l)];                                  \
    B##T n##buf = {_##n, _##n, _##n, _##n + (l)}; \
    T##$ n##idle = B##T##idle(n##buf);            \
    T##$ n##data = B##T##data(n##buf);

#define aBcpad(T, n, l)                           \
    T _##n[(l)];                                  \
    B##T n##buf = {_##n, _##n, _##n, _##n + (l)}; \
    T##$ n##idle = B##T##idle(n##buf);            \
    T##c##$ n##data = B##T##cdata(n##buf);

#define aB(T, name)                                    \
    T *name##buf[4] = {};                              \
    T **name##data = name##buf + 1;                    \
    T const **name##cdata = (T const **)name##buf + 1; \
    T **name##idle = name##buf + 2;

#define aBusy(T, name, buf) T *name[2] = {buf[0], buf[2]};

#define Bzero(buf) memset(buf[0], 0, Bsize(buf))

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
    if (!Bnil(b)) return Bnotnull;
    u8 *p = (u8 *)malloc(sz);
    if (p == NULL) return Ballocfail;
    u8 **buf = (u8 **)b;
    buf[0] = buf[1] = buf[2] = p;
    buf[3] = p + sz;
    return OK;
}

fun ok64 Brealloc(Bvoid b, size_t sz) {
    void *m = realloc(b[0], sz);
    if (m == NULL) return noroom;
    _Brebase(b, m, sz);
    return OK;
}

fun ok64 Breserve(Bvoid b, size_t sz) {
    size_t i = $size(Bidle(b));
    if (i >= sz) return OK;
    return Brealloc(b, roundup(Busysize(b) + sz, 256));
}

#define Bnilify(buf) memset((void **)buf, 0, sizeof(Bvoid));

fun ok64 Bfree(Bvoid buf) {
    if (Bnil(buf)) return Bisnil;
    free(buf[0]);
    Bnilify(buf);
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

#define Beat(b)              \
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
#define Bat(buf, ndx) (buf[0][ndx])

#define Bpush(buf) (buf[2]++)
#define Bpop(buf) (--buf[2])
#define Btop(buf) (buf[2] - 1)

typedef struct {
    u64 from;
    u64 till;
} range64;

#endif
