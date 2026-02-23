#ifndef ABC_AREA_H
#define ABC_AREA_H

#include "BUF.h"

fun ok64 AREAOpen(u8b area, size_t bytes) { return u8bMap(area, bytes); }
fun ok64 AREAClose(u8b area) { return u8bUnMap(area); }
fun b8 AREAContains(u8b area, void const *p) {
    return (u8cp)p >= (u8cp)area[0] && (u8cp)p < (u8cp)area[3];
}

// Carve typed buffer from arena.  Idle pointer advances past the carved range.
#define AREACarve(area, T, buf, count)                     \
    do {                                                   \
        size_t _n = (size_t)(count);                       \
        size_t _align = _Alignof(T);                       \
        u8p _p = (u8p)(((size_t)(area)[2] + _align - 1)   \
                        & ~(_align - 1));                  \
        T *_base = (T *)_p;                                \
        T **_b = (T **)(buf);                              \
        _b[0] = _b[1] = _b[2] = _base;                    \
        _b[3] = _base + _n;                                \
        assert((u8p)(_base + _n) <= (u8p)(area)[3]);       \
        u8p *_a = (u8p *)(area);                           \
        _a[1] = _a[2] = (u8p)(_base + _n);                \
    } while (0)

#endif  // ABC_AREA_H
