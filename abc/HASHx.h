//
// (c) Victor Grishchenko, 2020-2024
//
#include "$.h"
#include "01.h"
#include "HASH.h"

#define T X(, )

#ifndef lineX
// the maximum scan length (cell displacement)
#define lineX 16
#endif

#define MASK (lineX - 1)

fun pro(X(HASH, scan), size_t *ndx, X($, ) data, T const *rec) {
    sane($ok(data) && ndx != nil && 0 == ($len(data) & MASK));
    size_t off = (*ndx) & MASK;
    size_t base = (*ndx) & ~MASK;
    for (size_t i = off + 1; i < off + lineX; ++i) {
        *ndx = base + (i & MASK);
        if (X($, is0)(data, *ndx) == 0) return HASHnone;
        if (X(, cmp)(*data + *ndx, rec) == 0) return OK;
    }
    return HASHnoroom;
}

// OK, HASHnone, HASHnoroom
fun ok64 X(HASH, find)(size_t *ndx, X($, ) data, T const *rec) {
    u64 hash = X(, hash)(rec);
    *ndx = hash % $len(data);
    if (X(, cmp)(rec, $atp(data, *ndx)) == 0) return OK;
    if (X($, is0)(data, *ndx)) return HASHnone;
    return X(HASH, scan)(ndx, data, rec);
}

fun ok64 X(HASH, _get)(T *rec, X($, ) data, size_t ndx) {
    size_t off = ndx & MASK;
    size_t base = ndx & ~MASK;
    for (size_t i = (off + 1) & MASK; i != off; i = (i + 1) & MASK) {
        ndx = base + i;
        if (X(, cmp)(rec, $atp(data, ndx)) == 0) {
            *rec = $at(data, ndx);
            return OK;
        }
        if (X($, is0)(data, ndx)) return HASHnone;
    }
    return HASHnone;
}

fun ok64 X(HASH, get)(T *rec, X($, ) data) {
    u64 hash = X(, hash)(rec);
    size_t ndx = hash % $len(data);
    if (X($, is0)(data, ndx)) return HASHnone;
    if (X(, cmp)(rec, $atp(data, ndx)) == 0) {
        *rec = $at(data, ndx);  // TODO mv
        return OK;
    }
    return X(HASH, _get)(rec, data, ndx);
}

fun ok64 X(HASH, _put)(T const *rec, X($, ) data, size_t hash) {
    size_t ndx = hash % $len(data);
    size_t off = ndx & MASK;
    size_t base = ndx & ~MASK;
    for (size_t i = (off + 1) & MASK; i != off; i = (i + 1) & MASK) {
        ndx = base + i;
        if (X($, is0)(data, ndx) || X(, cmp)(rec, $atp(data, ndx)) == 0) {
            X(, mv)($atp(data, ndx), rec);
            return OK;
        }
    }
    return HASHnoroom;
}

fun ok64 X(HASH, put)(X($, ) data, T const *rec) {
    u64 hash = X(, hash)(rec);
    size_t ndx = hash % $len(data);
    if (X($, is0)(data, ndx) || X(, cmp)(rec, $atp(data, ndx)) == 0) {
        X(, mv)($atp(data, ndx), rec);
        return OK;
    }
    return X(HASH, _put)(rec, data, hash);
}

fun pro(X(HASH, shift), X($, ) data, size_t ndx) {
    sane($ok(data) && ndx < $len(data));
    size_t off = ndx & MASK;
    size_t base = ndx & ~MASK;
    for (size_t i = off + 1; i < off + lineX; ++i) {
        size_t x = base + (i & MASK);  // all pow 2
        if (X($, is0)(data, x)) {
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

#undef MASK
#undef lineX
#undef T
