#include "B.h"
#define T X(, )

typedef T const X(, c);

typedef int (*X(, cmpfn))(const X(, ) *, const X(, ) *);

fun void X(HEAP, UpAt)(X(, sc) heap, size_t a) {
    if ($len(heap) == 0) return;
    while (a) {
        size_t b = (a - 1) / 2;  // parent
        int cmp = X(, cmp)(*heap + b, *heap + a);
        if (cmp <= 0) break;
        X(, Swap)(*heap + a, *heap + b);
        a = b;
    }
}

fun void X(HEAP, Up)(X(, sc) heap) { X(HEAP, UpAt)(heap, $len(heap) - 1); }

fun void X(HEAP, DownAt)(X($c, ) heap, size_t i) {
    size_t n = $len(heap);
    do {
        size_t left = 2 * i + 1;
        if (left >= n || left < i) break;
        size_t j = left;
        size_t right = left + 1;
        if (right < n && X(, cmp)(*heap + j, *heap + right) > 0) j = right;
        if (X(, cmp)(*heap + i, *heap + j) <= 0) break;
        X(, Swap)(*heap + i, *heap + j);
        i = j;
    } while (1);
}

fun void X(HEAP, Down)(X($c, ) heap) { return X(HEAP, DownAt)(heap, 0); }

fun ok64 X(HEAP, Pop)(T *v, X(, bp) buf) {
    T **data = X(, bData)(buf);
    if ($empty(data)) return Bnodata;
    X(, mv)(v, $head(data));
    X(, Swap)($head(data), $last(data));
    --$term(data);
    X(HEAP, Down)(data);
    return OK;
}

fun ok64 X(HEAP, Push)(X(, bp) buf, T const *v) {
    ok64 o = X(, bFeedP)(buf, v);
    if (o != OK) return o;
    X(HEAP, Up)(X(, bData)(buf));
    return OK;
}

fun ok64 X(HEAP, Push1)(X(, bp) buf, T v) {
    ok64 o = X(, bFeed1)(buf, v);
    if (o != OK) return o;
    X(HEAP, Up)(X(, bData)(buf));
    return OK;
}

fun void X(HEAP, UpAtZ)(X(, sc) heap, X(, cmpfn) fn, size_t at) {
    if ($len(heap) == 0) return;
    while (at) {
        size_t b = (at - 1) / 2;  // parent
        int cmp = fn(*heap + b, *heap + at);
        if (cmp <= 0) break;
        X(, Swap)(*heap + at, *heap + b);
        at = b;
    }
}

fun void X(HEAP, UpZ)(X(, sc) heap, X(, cmpfn) fn) {
    if ($len(heap) == 0) return;
    size_t a = $len(heap) - 1;
    while (a) {
        size_t b = (a - 1) / 2;  // parent
        int cmp = fn(*heap + b, *heap + a);
        if (cmp <= 0) break;
        X(, Swap)(*heap + a, *heap + b);
        a = b;
    }
}

fun void X(HEAP, DownZ)(X(, sc) heap, X(, cmpfn) fn) {
    size_t i = 0;
    size_t n = $len(heap);
    do {
        size_t left = 2 * i + 1;
        if (left >= n || left < i) break;
        size_t j = left;
        size_t right = left + 1;
        if (right < n && fn(*heap + j, *heap + right) > 0) j = right;
        if (fn(*heap + i, *heap + j) <= 0) break;
        X(, Swap)(*heap + i, *heap + j);
        i = j;
    } while (1);
}

fun ok64 X(HEAP, PopZ)(T *v, X(, bp) buf, X(, cmpfn) fn) {
    T **data = X(, bData)(buf);
    if ($empty(data)) return Bnodata;
    X(, mv)(v, $head(data));
    X(, Swap)($head(data), $last(data));
    --$term(data);
    X(HEAP, DownZ)(data, fn);
    return OK;
}

fun ok64 X(HEAP, PushZ)(X(, bp) buf, T const *v, X(, cmpfn) fn) {
    ok64 o = X(, bFeedP)(buf, v);
    if (o != OK) return o;
    X(HEAP, UpZ)(X(, bData)(buf), fn);
    return OK;
}

fun ok64 X(HEAP, Push1Z)(X(, bp) buf, T v, X(, cmpfn) fn) {
    ok64 o = X(, bFeed1)(buf, v);
    if (o != OK) return o;
    X(HEAP, UpZ)(X(, bData)(buf), fn);
    return OK;
}
