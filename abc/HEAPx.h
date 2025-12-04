#include "B.h"
#include "abc/01.h"
#define T X(, )

typedef T const X(, c);

fun ok64 X(, sUpAtZ)(X(, sc) heap, size_t at, X(, z) z) {
    if (unlikely(!X(, sOK)(heap) || at >= $len(heap))) return MISS;
    while (at) {
        size_t b = (at - 1) / 2;  // parent
        if (z(*heap + b, *heap + at)) break;
        X(, Swap)(*heap + at, *heap + b);
        at = b;
    }
    return OK;
}

fun ok64 X(, sUpAt)(X(, sc) heap, size_t at) {
    return X(, sUpAtZ)(heap, at, X(, Z));
}

fun ok64 X(, sUp)(X(, sc) heap) { return X(, sUpAt)(heap, $len(heap) - 1); }

fun ok64 X(, sUpZ)(X(, sc) heap, X(, z) z) {
    return X(, sUpAtZ)(heap, $len(heap) - 1, z);
}

fun ok64 X(, sDownAtZ)(X(, sc) heap, size_t at, X(, z) z) {
    if (unlikely(!X(, sOK)(heap) || at >= $len(heap))) return MISS;
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
    return OK;
}

fun ok64 X(, sDownAt)(X($c, ) heap, size_t at) {
    return X(, sDownAtZ)(heap, at, X(, Z));
}

fun ok64 X(, sDownZ)(X(, sc) heap, X(, z) z) {
    return X(, sDownAtZ)(heap, 0, z);
}

fun ok64 X(, sDown)(X($c, ) heap) { return X(, sDownAt)(heap, 0); }

fun ok64 X(HEAP, Pop)(T *v, X(, bp) buf) {
    T **data = X(, bData)(buf);
    if ($empty(data)) return Bnodata;
    X(, mv)(v, $head(data));
    X(, Swap)($head(data), $last(data));
    --$term(data);
    if ($empty(data)) return OK;
    return X(, sDown)(data);
}

fun ok64 X(HEAP, Push)(X(, bp) buf, T const *v) {
    ok64 o = X(, bFeedP)(buf, v);
    if (o != OK) return o;
    return X(, sUp)(X(, bData)(buf));
}

fun ok64 X(HEAP, Push1)(X(, bp) buf, T v) {
    ok64 o = X(, bFeed1)(buf, v);
    if (o != OK) return o;
    return X(, sUp)(X(, bData)(buf));
}

fun ok64 X(HEAP, PopZ)(T *v, X(, bp) buf, X(, z) z) {
    T **data = X(, bData)(buf);
    if ($empty(data)) return Bnodata;
    X(, mv)(v, $head(data));
    X(, Swap)($head(data), $last(data));
    --$term(data);
    return X(, sDownZ)(data, z);
}

fun ok64 X(HEAP, PushZ)(X(, bp) buf, T const *v, X(, z) z) {
    ok64 o = X(, bFeedP)(buf, v);
    if (o != OK) return o;
    return X(, sUpZ)(X(, bData)(buf), z);
}

fun ok64 X(HEAP, Push1Z)(X(, bp) buf, T v, X(, z) z) {
    ok64 o = X(, bFeed1)(buf, v);
    if (o != OK) return o;
    return X(, sUpZ)(X(, bData)(buf), z);
}

fun ok64 X(, sTopsZ)(X(, sc) heap, X(, sp) eqs, X(, z) z) {
    size_t l = $len(heap);
    size_t eqlen = 1;
    size_t lim = 2;
    for (size_t i = 1; i < l && i <= lim; ++i) {
        if (z(*heap, *heap + i)) continue;
        if (eqlen != i) {
            X(, Swap)(*heap + eqlen, *heap + i);
            ok64 o = X(, sDownAtZ)(heap, i, z);
            if (o != OK) return o;
            --i;
        } else {
            lim = i * 2 + 2;
        }
        eqlen++;
    }
    eqs[0] = heap[0];
    eqs[1] = eqs[0] + eqlen;
    return OK;
}

fun ok64 X(, sTops)(X(, sc) heap, X(, sp) eqs) {
    return X(, sTopsZ)(heap, eqs, X(, Z));
}

fun ok64 X(, sHeapZ)(X(, sc) heap, X(, z) z) {
    size_t l = $len(heap);
    ok64 o = OK;
    for (size_t i = 0; i < l && o == OK; i++) {
        o = X(, sUpAtZ)(heap, i, z);
    }
    return o;
}

fun ok64 X(, sHeap)(X(, sc) heap, X(, z) z) {
    return X(, sHeapZ)(heap, X(, Z));
}
