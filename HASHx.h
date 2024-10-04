//
// (c) Victor Grishchenko, 2020-2024
//
#include "HASH.h"

typedef X(, ) T;

#ifndef lineX
// CHAINLEN is the maximum chain scan length (cell displacement)
#define lineX 8
#endif

fun pro(X(HASH, scan), size_t *ndx, X($, ) data, T const *rec) {
    sane($ok(data) && ndx != nil);
    T zero = {};
    const size_t mask = lineX - 1;
    size_t off = *ndx & mask;
    size_t base = *ndx & ~mask;
    for (size_t i = off + 1; i < off + lineX; ++i) {
        size_t x = base + (i & mask);
        if (X(, cmp)(*data + x, &zero) == 0) fail(HASHnone);
        if (X(, cmp)(*data + x, rec) == 0) skip;
    }
    fail(HASHnoroom);
    done;
}

// OK, HASHnone, HASHnoroom
fun ok64 X(HASH, find)(size_t *ndx, X($, ) data, T const *rec) {
    u64 hash = X(, hash)(rec);
    *ndx = hash % $len(data);
    if (X(, cmp)(rec, *data + *ndx) == 0) return OK;
    const T zero = {};
    if (X(, cmp)(&zero, *data + *ndx) == 0) return HASHnone;
    return X(HASH, scan)(ndx, data, rec);
}

fun ok64 X(HASH, get)(T *rec, X($, ) data) {
    size_t ndx = 0;
    ok64 o = X(HASH, find)(&ndx, data, rec);
    if (o == OK) *rec = data[0][ndx];
    return o;
}

fun ok64 X(HASH, put)(X($, ) data, T const *rec) {
    size_t ndx = 0;
    ok64 o = X(HASH, find)(&ndx, data, rec);
    if (o == HASHnone) o = OK;
    if (o != OK) return o;
    data[0][ndx] = *rec;
    return OK;
}

fun pro(X(HASH, shift), X($, ) data, size_t ndx) {
    sane($ok(data) && ndx < $len(data));
    T zero = {};
    const size_t mask = lineX - 1;
    size_t off = ndx & mask;
    size_t base = ndx & ~mask;
    u64 fit = 0;
    for (size_t i = off + 1; i < off + lineX; ++i) {
        size_t x = base + (i & mask);  // FIXME all pow 2?
        if (X(, cmp)(*data + x, &zero) == 0) {
            break;
        }
        size_t nom = X(, hash)(*data + x) % $len(data);
        nom &= mask;
        size_t bit = 1 << nom;
        if (nom == (x & mask)) {
            fit |= bit;
        } else if (0 == (fit & bit)) {
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
    if (o == HASHnone) return OK;
    return X(HASH, shift)(data, ndx);
}
