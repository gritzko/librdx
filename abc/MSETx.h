#include "MSET.h"

#define T X(, )

// --- Z-parameterized core heap ops ---

// Sift down at position `at` with custom comparator z.
fun void X(MSET, _DownZ)(X(, css) heap, size_t at, X(, z) z) {
    X(, cs) *h = heap[0];
    size_t n = $len(heap);
    size_t i = at;
    for (;;) {
        size_t left = 2 * i + 1;
        if (left >= n || left < i) break;
        size_t right = left + 1;
        size_t j = left;
        if (right < n && z(h[right][0], h[left][0]))
            j = right;
        if (!z(h[j][0], h[i][0])) break;
        T const *t0 = h[i][0], *t1 = h[i][1];
        h[i][0] = h[j][0];
        h[i][1] = h[j][1];
        h[j][0] = t0;
        h[j][1] = t1;
        i = j;
    }
}

// Bubble up at position with custom comparator z.
fun void X(MSET, _UpZ)(X(, css) heap, size_t at, X(, z) z) {
    X(, cs) *h = heap[0];
    while (at) {
        size_t b = (at - 1) / 2;
        if (z(h[b][0], h[at][0])) break;
        T const *t0 = h[at][0], *t1 = h[at][1];
        h[at][0] = h[b][0];
        h[at][1] = h[b][1];
        h[b][0] = t0;
        h[b][1] = t1;
        at = b;
    }
}

// Floyd's heapify with custom comparator z.
fun void X(MSET, HeapZ)(X(, css) heap, X(, z) z) {
    size_t n = $len(heap);
    for (size_t i = n / 2; i > 0; --i)
        X(MSET, _DownZ)(heap, i - 1, z);
}

// --- Wrappers using default comparator ---

fun void X(MSET, _Down)(X(, css) h, size_t at) {
    X(MSET, _DownZ)(h, at, X(, Z));
}

// Sort a slice in-place using the element comparator.
fun void X(MSET, Sort)(X(, s) data) { $sort(data, X(, cmp)); }

// --- Composable iteration primitives ---

// Find all equal-minimum entries (per z), move them to positions 0..ntops-1.
// eqs[0]..eqs[1] = the equal-minimum sub-range.
fun ok64 X(MSET, TopZ)(X(, css) heap, X(, css) eqs, X(, z) z) {
    size_t l = $len(heap);
    if (l == 0) return MISS;
    X(, cs) *h = heap[0];
    size_t eqlen = 1;
    size_t lim = 2;
    for (size_t i = 1; i < l && i <= lim; ++i) {
        if (z(h[0][0], h[i][0])) continue;
        if (eqlen != i) {
            T const *t0 = h[eqlen][0], *t1 = h[eqlen][1];
            h[eqlen][0] = h[i][0];
            h[eqlen][1] = h[i][1];
            h[i][0] = t0;
            h[i][1] = t1;
            X(MSET, _DownZ)(heap, i, z);
            --i;
        } else {
            lim = i * 2 + 2;
        }
        eqlen++;
    }
    eqs[0] = h;
    eqs[1] = h + eqlen;
    return OK;
}

// Advance ntops front entries (++s[0]), eject empties, compact, re-heapify.
// Returns MSETNODATA if heap becomes empty.
fun ok64 X(MSET, AdvZ)(X(, css) heap, size_t ntops, X(, z) z) {
    X(, cs) *h = heap[0];
    size_t old_n = $len(heap);
    // Advance all top slices
    for (size_t i = 0; i < ntops; i++)
        ++h[i][0];
    // Compact: keep alive tops, shift non-tops forward
    size_t alive = 0;
    for (size_t i = 0; i < ntops; i++) {
        if (!$empty(h[i])) {
            if (alive != i) {
                h[alive][0] = h[i][0];
                h[alive][1] = h[i][1];
            }
            alive++;
        }
    }
    for (size_t i = ntops; i < old_n; i++) {
        if (alive != i) {
            h[alive][0] = h[i][0];
            h[alive][1] = h[i][1];
        }
        alive++;
    }
    heap[1] = h + alive;
    X(MSET, HeapZ)(heap, z);
    return $empty(heap) ? MSETNODATA : OK;
}

// Eject entry at position. Swap with last, shrink, re-heapify.
fun ok64 X(MSET, EjectAtZ)(X(, css) heap, size_t at, X(, z) z) {
    size_t len = $len(heap);
    if (at >= len) return MISS;
    X(, cs) *h = heap[0];
    size_t last = len - 1;
    if (at != last) {
        T const *t0 = h[at][0], *t1 = h[at][1];
        h[at][0] = h[last][0];
        h[at][1] = h[last][1];
        h[last][0] = t0;
        h[last][1] = t1;
    }
    --heap[1];
    if (at < $len(heap)) {
        X(MSET, _UpZ)(heap, at, z);
        X(MSET, _DownZ)(heap, at, z);
    }
    return OK;
}

// --- Z-parameterized versions of existing functions ---

// Remove empties + Floyd's heapify with custom comparator.
fun void X(MSET, StartZ)(X(, css) iter, X(, z) z) {
    X(, cs) *w = iter[0];
    for (X(, cs) *r = iter[0]; r < iter[1]; r++) {
        if (!$empty(*r)) {
            if (w != r) {
                (*w)[0] = (*r)[0];
                (*w)[1] = (*r)[1];
            }
            w++;
        }
    }
    iter[1] = w;
    X(MSET, HeapZ)(iter, z);
}

// Build a min-heap iterator (default comparator).
fun void X(MSET, Start)(X(, css) iter) {
    X(MSET, StartZ)(iter, X(, Z));
}

// Seek to >= key with custom comparator.
fun ok64 X(MSET, SeekZ)(X(, css) iter, T key, X(, z) z) {
    while (!$empty(iter) && z(***iter, &key)) {
        X(, cs) *top = iter[0];
        X($c, c) run = {(*top)[0], (*top)[1]};
        T const *pos = X($, findge)(run, &key);
        (*top)[0] = pos;
        if ($empty(*top)) {
            X(, cs) *last = iter[1] - 1;
            (*top)[0] = (*last)[0];
            (*top)[1] = (*last)[1];
            --iter[1];
            if ($empty(iter)) break;
        }
        X(MSET, _DownZ)(iter, 0, z);
    }
    return $empty(iter) ? MSETNODATA : OK;
}

// Seek forward to the first element >= key (default comparator).
fun ok64 X(MSET, Seek)(X(, css) iter, T key) {
    return X(MSET, SeekZ)(iter, key, X(, Z));
}

// Advance past the current top element, maintain the heap.
// Duplicates across runs count as one; all copies are skipped.
// Returns OK normally, MSETNODATA if already empty.
fun ok64 X(MSET, Next)(X(, css) iter) {
    if ($empty(iter)) return MSETNODATA;
    T const *cur = ***iter;
    do {
        X(, cs) *top = iter[0];
        ++(*top)[0];
        if ($empty(*top)) {
            X(, cs) *last = iter[1] - 1;
            (*top)[0] = (*last)[0];
            (*top)[1] = (*last)[1];
            --iter[1];
        }
        if (!$empty(iter)) X(MSET, _Down)(iter, 0);
    } while (!$empty(iter) && !X(, Z)(cur, ***iter) && !X(, Z)(***iter, cur));
    return OK;
}

// Merge all runs into flat sorted output.
// Calls Start, then drains the iterator into `into`.
fun ok64 X(MSET, Merge)(X(, s) into, X(, css) iter) {
    X(MSET, Start)(iter);
    while (!$empty(iter)) {
        if ($empty(into)) return MSETNOROOM;
        X(, mv)(*into, ***iter);
        ++*into;
        X(MSET, Next)(iter);
    }
    return OK;
}

// Whether the LSM stack satisfies the 1/8 invariant:
// stack is oldest-first; each newer (higher-index) run < 1/8 of preceding.
fun b8 X(MSET, IsCompact)(X(, css) stack) {
    size_t n = $len(stack);
    for (size_t i = 0; i + 1 < n; i++) {
        if ($len(stack[0][i + 1]) * 8 > $len(stack[0][i]))
            return NO;
    }
    return YES;
}

// Merge youngest runs to restore the 1/8 invariant.
// Stack is oldest-first. Youngest runs are at the tail.
// Cascades: if merging tail m runs still violates against
// the run before them, includes that run too, and so on.
// `into` must have room for the merged elements.
fun ok64 X(MSET, Compact)(X(, css) stack, X(, s) into) {
    size_t n = $len(stack);
    if (n < 2) return OK;
    size_t m = 1;
    size_t total = $len(stack[0][n - 1]);
    while (m < n && total * 8 > $len(stack[0][n - 1 - m])) {
        total += $len(stack[0][n - 1 - m]);
        m++;
    }
    if (m < 2) return OK;
    if ($len(into) < total) return MSETNOROOM;
    T *base = *into;
    X(, css) sub = {stack[0] + (n - m), stack[0] + n};
    X(MSET, Start)(sub);
    while (!$empty(sub)) {
        X(, mv)(*into, ***sub);
        ++*into;
        X(MSET, Next)(sub);
    }
    stack[0][n - m][0] = base;
    stack[0][n - m][1] = *into;
    stack[1] = stack[0] + (n - m + 1);
    return OK;
}

// Find key in the stack. Copies runs onto C stack, seeks.
// Returns OK if found, MSETNODATA if not.
fun ok64 X(MSET, Get)(X(, css) stack, T key) {
    size_t n = $len(stack);
    if (n == 0) return MSETNONE;
    X(, cs) runs[MSET_MAX_LEVELS];
    if (n > MSET_MAX_LEVELS) n = MSET_MAX_LEVELS;
    for (size_t i = 0; i < n; i++) {
        runs[i][0] = stack[0][i][0];
        runs[i][1] = stack[0][i][1];
    }
    X(, css) iter = {runs, runs + n};
    X(MSET, Start)(iter);
    ok64 o = X(MSET, Seek)(iter, key);
    if (o != OK) return MSETNONE;
    if (X(, Z)(&key, ***iter)) return MSETNONE;
    return OK;
}

#undef T
