// QSORTx.h — Inline-comparator introsort + dedup template
// Instantiate: #define ABC_QSORT_X
//              #define X(M, name) M##u64##name
//              #include "abc/Bx.h"   (or Sx.h — must come after ABC_QSORT_X)
//              #include "abc/QSORTx.h"
//              #undef X
//              #undef ABC_QSORT_X
// Requires: X(,Z)(cp a, cp b) -> b8  (inline less-than comparator)
//           X(,s), X(,g), X(,b) types from Sx.h/Bx.h

#include "S.h"

#define T X(, )

#define QS_ISORT_THRESH 24

// --- Insertion sort for small arrays ---

fun void X(QS, Isort)(T *lo, T *hi) {
    for (T *i = lo + 1; i < hi; i++) {
        T key = *i;
        T *j = i;
        while (j > lo && X(, Z)(&key, j - 1)) {
            *j = *(j - 1);
            --j;
        }
        *j = key;
    }
}

// --- Heapsort fallback ---

fun void X(QS, HDown)(T *arr, size_t n, size_t i) {
    for (;;) {
        size_t c = 2 * i + 1;
        if (c >= n) break;
        if (c + 1 < n && X(, Z)(arr + c, arr + c + 1)) c++;
        if (!X(, Z)(arr + i, arr + c)) break;
        T t = arr[i];
        arr[i] = arr[c];
        arr[c] = t;
        i = c;
    }
}

fun void X(QS, Hsort)(T *lo, T *hi) {
    size_t n = (size_t)(hi - lo);
    if (n < 2) return;
    for (size_t i = n / 2; i > 0; --i) X(QS, HDown)(lo, n, i - 1);
    for (size_t i = n - 1; i > 0; --i) {
        T t = lo[0];
        lo[0] = lo[i];
        lo[i] = t;
        X(QS, HDown)(lo, i, 0);
    }
}

// --- Median of 3 ---

fun T *X(QS, Med3)(T *a, T *b, T *c) {
    if (X(, Z)(a, b)) {
        if (X(, Z)(b, c)) return b;
        return X(, Z)(a, c) ? c : a;
    } else {
        if (X(, Z)(c, b)) return b;
        return X(, Z)(c, a) ? c : a;
    }
}

// --- Introsort core ---

fun void X(QS, Core)(T *lo, T *hi, int depth) {
    while (hi - lo > QS_ISORT_THRESH) {
        if (depth <= 0) {
            X(QS, Hsort)(lo, hi);
            return;
        }
        --depth;

        T *mid = lo + (hi - lo) / 2;
        T *piv = X(QS, Med3)(lo, mid, hi - 1);
        T pval = *piv;
        *piv = *(hi - 1);
        *(hi - 1) = pval;

        T *i = lo, *j = hi - 2;
        for (;;) {
            while (i <= j && X(, Z)(i, &pval)) i++;
            while (i < j && X(, Z)(&pval, j)) j--;
            if (i >= j) break;
            T t = *i;
            *i = *j;
            *j = t;
            i++;
            j--;
        }
        T t = *i;
        *i = *(hi - 1);
        *(hi - 1) = t;

        // Recurse on smaller partition, loop on larger
        if (i - lo < hi - (i + 1)) {
            X(QS, Core)(lo, i, depth);
            lo = i + 1;
        } else {
            X(QS, Core)(i + 1, hi, depth);
            hi = i;
        }
    }
    X(QS, Isort)(lo, hi);
}

// --- depth limit: 2 * floor(log2(n)) ---

fun int X(QS, DepthLimit)(size_t n) {
    int d = 0;
    while (n > 1) {
        n >>= 1;
        d++;
    }
    return d * 2;
}

// --- Public API ---

fun void X(, sSort)(X(, s) data) {
    size_t n = (size_t)(data[1] - data[0]);
    if (n < 2) return;
    X(QS, Core)(data[0], data[1], X(QS, DepthLimit)(n));
}

fun void X(, gSort)(X(, g) g) {
    X(, s) left = {g[0], g[1]};
    X(, sSort)(left);
}

fun void X(, bSort)(X(, b) buf) {
    T *data[2] = {buf[1], buf[2]};
    X(, sSort)(data);
}

// --- Dedup: shrink sorted slice, removing adjacent duplicates ---

fun void X(, sDedup)(X(, s) data) {
    if (data[0] >= data[1]) return;
    T *w = data[0] + 1;
    for (T *r = data[0] + 1; r < data[1]; r++) {
        if (X(, Z)(r - 1, r) || X(, Z)(r, r - 1)) *w++ = *r;
    }
    data[1] = w;
}

fun void X(, gDedup)(X(, g) g) {
    X(, s) left = {g[0], g[1]};
    X(, sDedup)(left);
    g[1] = left[1];
}

fun void X(, bDedup)(X(, b) buf) {
    T *data[2] = {buf[1], buf[2]};
    X(, sDedup)(data);
    ((T **)buf)[2] = data[1];
}

#undef T
#undef QS_ISORT_THRESH
