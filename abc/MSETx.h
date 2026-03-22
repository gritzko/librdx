#include "MSET.h"

#define T X(, )

// Sift down at position `at` in the min-heap of sorted runs.
// Heap is X(, css): a slice of X(, cs) runs ordered by head element.
fun void X(MSET, _Down)(X(, css) heap, size_t at) {
    X(, cs) *h = heap[0];
    size_t n = $len(heap);
    size_t i = at;
    for (;;) {
        size_t left = 2 * i + 1;
        if (left >= n || left < i) break;
        size_t right = left + 1;
        size_t j = left;
        if (right < n && X(, Z)(h[right][0], h[left][0]))
            j = right;
        if (!X(, Z)(h[j][0], h[i][0])) break;
        T const *t0 = h[i][0], *t1 = h[i][1];
        h[i][0] = h[j][0];
        h[i][1] = h[j][1];
        h[j][0] = t0;
        h[j][1] = t1;
        i = j;
    }
}

// Sort a slice in-place using the element comparator.
fun void X(MSET, Sort)(X(, s) data) { $sort(data, X(, cmp)); }

// Build a min-heap iterator from an array of sorted runs.
// Removes empty runs, then heapifies by head element (Floyd).
// After Start, ***iter points at the smallest element.
fun void X(MSET, Start)(X(, css) iter) {
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
    size_t n = $len(iter);
    for (size_t i = n / 2; i > 0; --i)
        X(MSET, _Down)(iter, i - 1);
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

// Seek forward to the first element >= key.
// Returns OK if found (***iter points at it), MSETNODATA if none.
fun ok64 X(MSET, Seek)(X(, css) iter, T key) {
    while (!$empty(iter) && X(, Z)(***iter, &key)) {
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
        X(MSET, _Down)(iter, 0);
    }
    return $empty(iter) ? MSETNODATA : OK;
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
