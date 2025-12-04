#include "B.h"
#include "abc/01.h"
#define T X(, )

typedef T const X(, c);

fun void X(HEAP, UpAtZ)(X(, sc) heap, size_t at, X(, z) z) {
    if ($len(heap) == 0) return;
    while (at) {
        size_t b = (at - 1) / 2;  // parent
        if (z(*heap + b, *heap + at)) break;
        X(, Swap)(*heap + at, *heap + b);
        at = b;
    }
}

fun void X(HEAP, UpAt)(X(, sc) heap, size_t at) {
    return X(HEAP, UpAtZ)(heap, at, X(, Z));
}

fun void X(HEAP, Up)(X(, sc) heap) { X(HEAP, UpAt)(heap, $len(heap) - 1); }

fun void X(HEAP, UpZ)(X(, sc) heap, X(, z) z) {
    X(HEAP, UpAtZ)(heap, $len(heap) - 1, z);
}

fun void X(HEAP, DownAtZ)(X(, sc) heap, size_t at, X(, z) z) {
    size_t n = $len(heap);
    size_t i = at;
    do {
        size_t left = 2 * i + 1;
        if (left >= n || left < i) break;
        size_t right = left + 1;
        size_t j = left;
        if (right < n && z(*heap + right, *heap + left)) j = right;
        if (!z(*heap + j, *heap + i)) break;
        X(, Swap)(*heap + i, *heap + j);
        i = j;
    } while (1);
}

fun void X(HEAP, DownAt)(X($c, ) heap, size_t at) {
    return X(HEAP, DownAtZ)(heap, at, X(, Z));
}

fun void X(HEAP, DownZ)(X(, sc) heap, X(, z) z) {
    X(HEAP, DownAtZ)(heap, 0, z);
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

fun ok64 X(HEAP, PopZ)(T *v, X(, bp) buf, X(, z) z) {
    T **data = X(, bData)(buf);
    if ($empty(data)) return Bnodata;
    X(, mv)(v, $head(data));
    X(, Swap)($head(data), $last(data));
    --$term(data);
    X(HEAP, DownZ)(data, z);
    return OK;
}

fun ok64 X(HEAP, PushZ)(X(, bp) buf, T const *v, X(, z) z) {
    ok64 o = X(, bFeedP)(buf, v);
    if (o != OK) return o;
    X(HEAP, UpZ)(X(, bData)(buf), z);
    return OK;
}

fun ok64 X(HEAP, Push1Z)(X(, bp) buf, T v, X(, z) z) {
    ok64 o = X(, bFeed1)(buf, v);
    if (o != OK) return o;
    X(HEAP, UpZ)(X(, bData)(buf), z);
    return OK;
}

fun void X(HEAP, EqsZ)(X(, sc) heap, X(, sp) eqs, X(, z) z) {
    size_t l = $len(heap);
    size_t eqlen = 1;
    size_t lim = 2;
    for (size_t i = 1; i < l && i <= lim; ++i) {
        if (z(*heap, *heap + i)) continue;
        if (eqlen != i) {
            X(, Swap)(*heap + eqlen, *heap + i);
            X(HEAP, DownAtZ)(heap, i, z);
            --i;
        } else {
            lim = i * 2 + 2;
        }
        eqlen++;
    }
}

fun void X(HEAP, MakeZ)(X(, sc) heap, X(, z) z) {
    size_t l = $len(heap);
    for (size_t i = 0; i < l; i++) X(HEAP, UpAtZ)(heap, i, z);
}
