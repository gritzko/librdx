#include "B.h"
#define T X(, )

typedef T const X(, c);

typedef z32 (*X(, Zfn))(const X(, ) *, const X(, ) *);

fun void X(HEAP, up)(X($c, ) heap) {
    if ($len(heap) == 0) return;
    size_t a = $len(heap) - 1;
    while (a) {
        size_t b = (a - 1) / 2;  // parent
        int cmp = X(, cmp)(*heap + b, *heap + a);
        if (cmp <= 0) break;
        X(, swap)(*heap + a, *heap + b);
        a = b;
    }
}

fun void X(HEAP, down)(X($c, ) heap) {
    size_t i = 0;
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
    ok64 o = X(B, feedp)(buf, v);
    if (o != OK) return o;
    X(HEAP, up)(X(B, data)(buf));
    return OK;
}

fun ok64 X(HEAP, push1)(X(B, ) buf, T v) {
    ok64 o = X(B, feed1)(buf, v);
    if (o != OK) return o;
    X(HEAP, up)(X(B, data)(buf));
    return OK;
}

fun void X(HEAP, upf)(X($c, ) heap, X(, Zfn) fn) {
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

fun void X(HEAP, downf)(X($c, ) heap, X(, Zfn) fn) {
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

fun ok64 X(HEAP, popf)(T *v, X(B, ) buf, X(, Zfn) fn) {
    T **data = X(B, data)(buf);
    if ($empty(data)) return Bnodata;
    X(, mv)(v, $head(data));
    X(, swap)($head(data), $last(data));
    --$term(data);
    X(HEAP, downf)(data, fn);
    return OK;
}

fun ok64 X(HEAP, pushf)(X(B, ) buf, T const *v, X(, Zfn) fn) {
    ok64 o = X(B, feedp)(buf, v);
    if (o != OK) return o;
    X(HEAP, upf)(X(B, data)(buf), fn);
    return OK;
}

fun ok64 X(HEAP, push1f)(X(B, ) buf, T v, X(, Zfn) fn) {
    ok64 o = X(B, feed1)(buf, v);
    if (o != OK) return o;
    X(HEAP, upf)(X(B, data)(buf), fn);
    return OK;
}
