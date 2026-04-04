// HITx.h — generic heap iterator template
// Requires Sx.h included for T before inclusion.
// Uses X(, x) advancer, X(, y) merge policy, X(, z) comparator, X(, Swap).
// Heap is X(, s) = T *[2].

#include "OK.h"
#include "S.h"

#define T X(, )

// Sift down at position `at` with custom comparator z.
fun void X(HIT, DownYZ)(X(, s) heap, size_t at, X(, z) z) {
    size_t n = $len(heap);
    size_t i = at;
    for (;;) {
        size_t left = 2 * i + 1;
        if (left >= n || left < i) break;
        size_t right = left + 1;
        size_t j = left;
        if (right < n && z($atp(heap, right), $atp(heap, left)))
            j = right;
        if (!z($atp(heap, j), $atp(heap, i))) break;
        X(, Swap)($atp(heap, i), $atp(heap, j));
        i = j;
    }
}

// Bubble up at position with custom comparator z.
fun void X(HIT, UpYZ)(X(, s) heap, size_t at, X(, z) z) {
    while (at) {
        size_t b = (at - 1) / 2;
        if (z($atp(heap, b), $atp(heap, at))) break;
        X(, Swap)($atp(heap, b), $atp(heap, at));
        at = b;
    }
}

// Floyd's heapify with custom comparator z.
fun void X(HIT, HeapYZ)(X(, s) heap, X(, z) z) {
    size_t n = $len(heap);
    for (size_t i = n / 2; i > 0; --i)
        X(HIT, DownYZ)(heap, i - 1, z);
}

// Find all entries equal to minimum (per z), move to front.
// eqs = sub-slice heap[0..ntops).
fun ok64 X(HIT, TopsYZ)(X(, s) heap, X(, s) eqs, X(, z) z) {
    size_t l = $len(heap);
    if (l == 0) return MISS;
    size_t eqlen = 1;
    size_t lim = 2;
    for (size_t i = 1; i < l && i <= lim; ++i) {
        if (z($atp(heap, 0), $atp(heap, i))) continue;
        if (eqlen != i) {
            X(, Swap)($atp(heap, eqlen), $atp(heap, i));
            X(HIT, DownYZ)(heap, i, z);
            --i;
        } else {
            lim = i * 2 + 2;
        }
        eqlen++;
    }
    eqs[0] = heap[0];
    eqs[1] = $atp(heap, eqlen);
    return OK;
}

// Eject entry at position. Swap with last, shrink.
fun void X(HIT, Eject)(X(, s) heap, size_t at) {
    size_t last = $len(heap) - 1;
    if (at != last) X(, Swap)($atp(heap, at), $atp(heap, last));
    --heap[1];
}

// Drain one value from the heap iterator.
//   x — advancer: x(&entry, &min) returns OK (more data) or NODATA (exhausted)
//   y — merge policy: y(&out, tops) returns OK (emit) or MISS (skip)
//   z — comparator
// Flow:
//   1. TopsYZ — find equal-minimum group
//   2. y(out, tops) — decide emit or skip
//   3. For each top: x(&entry, &min) — advance past min
//      NODATA → eject, OK → sift down
//   4. If y returned OK, advance out
// Returns y's result; NODATA when heap empty.
fun ok64 X(HIT, NextYZ)(X(, s) heap, X(, s) out, X(, x) x, X(, y) y, X(, z) z) {
    if ($empty(heap)) return NODATA;
    // 1. find tops
    X(, s) tops = {NULL, NULL};
    ok64 o = X(HIT, TopsYZ)(heap, tops, z);
    if (o != OK) return o;
    // 2. merge policy
    ok64 yr = y(*out, tops);
    // 3. advance each top entry (reverse order — eject swaps with end)
    size_t ntops = $len(tops);
    for (size_t j = ntops; j > 0; --j) {
        size_t i = j - 1;
        ok64 xr = x($atp(heap, i), $atp(heap, i));
        if (xr != OK) {
            X(HIT, Eject)(heap, i);
            // sift the replacement entry
            if (i < $len(heap)) X(HIT, DownYZ)(heap, i, z);
        } else {
            X(HIT, DownYZ)(heap, i, z);
        }
    }
    // 4. advance output if emitted
    if (yr == OK) ++out[0];
    return $empty(heap) && yr != OK ? NODATA : yr;
}

#ifdef HIT_ENTRY_IS_SLICE
// Filter empty entries, compact, heapify.
// Only for entry types where $empty(entry) is valid (slice types).
fun void X(HIT, StartZ)(X(, s) heap, X(, z) z) {
    T *w = heap[0];
    for (T *r = heap[0]; r < heap[1]; r++) {
        if (!$empty(*r)) {
            if (w != r) X(, Swap)(w, r);
            w++;
        }
    }
    heap[1] = w;
    X(HIT, HeapYZ)(heap, z);
}
#endif

// Seek forward until heap top >= key.
// x seeks within one entry: x(entry, key) returns OK or NODATA.
fun ok64 X(HIT, SeekXZ)(X(, s) heap, T const *key, X(, x) x, X(, z) z) {
    while (!$empty(heap) && z(heap[0], key)) {
        ok64 xr = x(heap[0], key);
        if (xr != OK) {
            X(HIT, Eject)(heap, 0);
            if ($empty(heap)) break;
        }
        X(HIT, DownYZ)(heap, 0, z);
    }
    return $empty(heap) ? NODATA : OK;
}

#undef T
