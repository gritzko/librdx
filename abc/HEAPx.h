#include "B.h"
#define T X(, )

typedef T const X(, c);

typedef int (*X(, cmpfn))(const X(, ) *, const X(, ) *);

fun void X(HEAP, up_at)(X($c, ) heap, size_t a) {
    if ($len(heap) == 0) return;
    while (a) {
        size_t b = (a - 1) / 2;  // parent
        int cmp = X(, cmp)(*heap + b, *heap + a);
        if (cmp <= 0) break;
        X(, swap)(*heap + a, *heap + b);
        a = b;
    }
}

fun void X(HEAP, up)(X($c, ) heap) { X(HEAP, up_at)(heap, $len(heap) - 1); }

fun void X(HEAP, down_at)(X($c, ) heap, size_t i) {
    size_t n = $len(heap);
    do {
        size_t left = 2 * i + 1;
        if (left >= n || left < i) break;
        size_t j = left;
        size_t right = left + 1;
        if (right < n && X(, cmp)(*heap + j, *heap + right) > 0) j = right;
        if (X(, cmp)(*heap + i, *heap + j) <= 0) break;
        X(, swap)(*heap + i, *heap + j);
        i = j;
    } while (1);
}

fun void X(HEAP, down)(X($c, ) heap) { return X(HEAP, down_at)(heap, 0); }

fun ok64 X(HEAP, pop)(T *v, X(B, ) buf) {
    T **data = X(B, data)(buf);
    if ($empty(data)) return Bnodata;
    X(, mv)(v, $head(data));
    X(, swap)($head(data), $last(data));
    --$term(data);
    X(HEAP, down)(data);
    return OK;
}

fun ok64 X(HEAP, push)(X(B, ) buf, T const *v) {
    ok64 o = X(, B_feedp)(buf, v);
    if (o != OK) return o;
    X(HEAP, up)(X(B, data)(buf));
    return OK;
}

fun ok64 X(HEAP, push1)(X(B, ) buf, T v) {
    ok64 o = X(, B_feed1)(buf, v);
    if (o != OK) return o;
    X(HEAP, up)(X(B, data)(buf));
    return OK;
}

fun void X(HEAP, up_at_f)(X($c, ) heap, X(, cmpfn) fn, size_t at) {
    if ($len(heap) == 0) return;
    while (at) {
        size_t b = (at - 1) / 2;  // parent
        int cmp = fn(*heap + b, *heap + at);
        if (cmp <= 0) break;
        X(, swap)(*heap + at, *heap + b);
        at = b;
    }
}

fun void X(HEAP, upf)(X($c, ) heap, X(, cmpfn) fn) {
    if ($len(heap) == 0) return;
    size_t a = $len(heap) - 1;
    while (a) {
        size_t b = (a - 1) / 2;  // parent
        int cmp = fn(*heap + b, *heap + a);
        if (cmp <= 0) break;
        X(, swap)(*heap + a, *heap + b);
        a = b;
    }
}

fun void X(HEAP, downf)(X($c, ) heap, X(, cmpfn) fn) {
    size_t i = 0;
    size_t n = $len(heap);
    do {
        size_t left = 2 * i + 1;
        if (left >= n || left < i) break;
        size_t j = left;
        size_t right = left + 1;
        if (right < n && fn(*heap + j, *heap + right) > 0) j = right;
        if (fn(*heap + i, *heap + j) <= 0) break;
        X(, swap)(*heap + i, *heap + j);
        i = j;
    } while (1);
}

fun ok64 X(HEAP, popf)(T *v, X(B, ) buf, X(, cmpfn) fn) {
    T **data = X(B, data)(buf);
    if ($empty(data)) return Bnodata;
    X(, mv)(v, $head(data));
    X(, swap)($head(data), $last(data));
    --$term(data);
    X(HEAP, downf)(data, fn);
    return OK;
}

fun ok64 X(HEAP, pushf)(X(B, ) buf, T const *v, X(, cmpfn) fn) {
    ok64 o = X(, B_feedp)(buf, v);
    if (o != OK) return o;
    X(HEAP, upf)(X(B, data)(buf), fn);
    return OK;
}

fun ok64 X(HEAP, push1f)(X(B, ) buf, T v, X(, cmpfn) fn) {
    ok64 o = X(, B_feed1)(buf, v);
    if (o != OK) return o;
    X(HEAP, upf)(X(B, data)(buf), fn);
    return OK;
}
