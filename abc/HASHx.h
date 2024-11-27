//
// (c) Victor Grishchenko, 2020-2024
//
#include "HASH.h"

#define T X(, )

#ifndef lineX
// the maximum scan length (cell displacement)
#define lineX 16
#endif

fun pro(X(HASH, scan), size_t *ndx, X($, ) data, T const *rec) {
    const size_t mask = lineX - 1;
    sane($ok(data) && ndx != nil && 0 == ($len(data) & mask));
    T zz;
    zero(zz);
    size_t off = (*ndx) & mask;
    size_t base = (*ndx) & ~mask;
    for (size_t i = off + 1; i < off + lineX; ++i) {
        *ndx = base + (i & mask);
        if (X(, cmp)(*data + *ndx, &zz) == 0) return HASHnone;
        if (X(, cmp)(*data + *ndx, rec) == 0) return OK;
    }
    return HASHnoroom;
}

// OK, HASHnone, HASHnoroom
fun ok64 X(HASH, find)(size_t *ndx, X($, ) data, T const *rec) {
    u64 hash = X(, hash)(rec);
    *ndx = hash % $len(data);
    if (X(, cmp)(rec, *data + *ndx) == 0) return OK;
    T zz;
    zero(zz);
    if (X(, cmp)(&zz, *data + *ndx) == 0) return HASHnone;
    return X(HASH, scan)(ndx, data, rec);
}

fun ok64 X(HASH, get)(T *rec, X($, ) data) {
    size_t ndx = 0;
    ok64 o = X(HASH, find)(&ndx, data, rec);
    if (o == OK) {
        *rec = data[0][ndx];
    } else if (o == HASHnoroom) {
        o = HASHnone;
    }
    return o;
}

fun ok64 X(HASH, put)(X($, ) data, T const *rec) {
    size_t ndx = 0;
    ok64 o = X(HASH, find)(&ndx, data, rec);
    if (o == OK || o == HASHnone) {
        data[0][ndx] = *rec;
        o = OK;
    }
    return o;
}

fun pro(X(HASH, shift), X($, ) data, size_t ndx) {
    sane($ok(data) && ndx < $len(data));
    T zz;
    zero(zz);
    const size_t mask = lineX - 1;
    size_t off = ndx & mask;
    size_t base = ndx & ~mask;
    for (size_t i = off + 1; i < off + lineX; ++i) {
        size_t x = base + (i & mask);  // all pow 2
        if (X(, cmp)(*data + x, &zz) == 0) {
            break;
        }
        size_t nom = X(, hash)(*data + x) % $len(data);
        if ((x > ndx && (nom <= ndx || nom > x)) ||
            (x < ndx && (nom <= ndx && nom > x))) {
            Bat(data, ndx) = Bat(data, x);
            ndx = x;
        }
    }
    zero(Bat(data, ndx));
    done;
}

fun ok64 X(HASH, del)(X($, ) data, T const *rec) {
    size_t ndx = 0;
    ok64 o = X(HASH, find)(&ndx, data, rec);
    if (o == HASHnone || o == HASHnoroom) return OK;
    return X(HASH, shift)(data, ndx);
}

#undef T
