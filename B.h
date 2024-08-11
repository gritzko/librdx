#ifndef LIBRDX_B_h
#define LIBRDX_B_h
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "$.h"
#include "01.h"
#include "OK.h"

con ok64 Bmapfail = 0xc2d96ad25c4b;
con ok64 Bnotnull = 0xc30e72e33c8b;
con ok64 Bisnull = 0x30c39cb7b4b;
con ok64 Bnotmap = 0x34971e33c8b;
con ok64 Ballocfail = 0xc2d96a9f3c3094b;
con ok64 Bnotalloc = 0x27cf0c25e33c8b;
con ok64 Bnoroom = 0x31cf3db3c8b;
con ok64 Bnodata = 0x25e25a33c8b;

enum {
    B_NONE = 0,
    B_MMAP = 1,
    B_FMAP = 2,
    B_MALLOC = 3,
};

#define BTYPE(T) typedef T *const B##T[4];

BTYPE(void);
typedef void *const *void$;
typedef void const *const *voidc$;

#define B(T, b) B##T b = {0, 0, 0, 0}

#define Bbusy(b) \
    { b[0], b[2] }

#define Bpast(b) (b + 0)
#define Bdata(b) (b + 1)
#define Bidle(b) (b + 2)
#define $past(b) (b + 0)
#define $data(b) (b + 1)
#define $idle(b) (b + 2)

#define Blast(b) (*(b[2] - 1))

#define Bi(b) *(b[2])
#define Bd(b) *(b[1])

#define Blen(b) (b[3] - b[0])
#define Bsize(b) ((u8 *)(b[3]) - (u8 *)(b[0]))
#define Busylen(b) (b[2] - b[0])
#define Busysize(b) ((u8 const *)b[2] - (u8 const *)b[0])
#define Bdatalen(b) $len(Bdata(b))

#define Bok(b) \
    (b != nil && b[0] != nil && b[0] <= b[1] && b[1] <= b[2] && b[2] <= b[3])
#define Bnil(b) (b == nil || b[0] == nil)
#define Bhasroom(b) (b[2] < b[3])

#define aBpad(T, n, l) \
    T _##n[(l)];       \
    B##T n = {_##n, _##n, _##n, _##n + (l)}

#define Bzero(buf) memset(buf[0], 0, ((void *)buf[3]) - ((void *)buf[0]))

fun void _Brebase(Bvoid buf, void *newhead, size_t newlen) {
    size_t data = buf[1] - buf[0];
    if (data > newlen) data = newlen;
    size_t idle = buf[2] - buf[0];
    if (idle > newlen) idle = newlen;
    u8 **b = (u8 **)buf;
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

fun ok64 Bfree(Bvoid buf) {
    if (Bnil(buf)) return Bisnull;
    free(buf[0]);
    memset((void **)buf, 0, sizeof(Bvoid));
    return OK;
}

#define Brewind(b, past, data)           \
    {                                   \
        assert(past + data <= Blen(b)); \
        b[1] = b[0] + past;             \
        b[2] = b[0] + past + data;      \
    }

#define Breset(b) \
    { Brewind(b, 0, 0); }

#define aB(T, n) T *n[4] = {0, 0, 0, 0};

#define Batp(buf, ndx) (buf[0] + ndx)
// TODO Bat is a value
#define Bat(buf, ndx) (buf[0] + ndx)

#define Bpop(buf) \
    { --buf[2]; }

// #define Blast(buf) (buf[2] - 1)

typedef struct {
    u32 from;
    u32 till;
} range64;

#endif
