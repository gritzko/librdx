// HITx.h — Heap of ITerators template
// A HIT is a min-heap of sorted slices (iterators).
// Instantiate with element type: #define X(M, name) M##u64##name
// Entry type: X(,cs) (e.g. u64cs = u64 const *[2])
// Heap type:  X(,css) (e.g. u64css = u64cs *[2])
// Comparator: X(,Z) on element pointers
// Swap:       X(,csSwap) on slice entries
// Advance:    ++(*entry)[0], eject when $empty(*entry)

#include "OK.h"
#include "S.h"

#define HIT_T X(, )
#define HIT_E X(, cs)    // entry = const slice
#define HIT_H X(, css)   // heap = slice of entries

// --- Comparator: compare entries by head element ---

fun b8 X(HIT, Z)(HIT_E const *a, HIT_E const *b) {
    return X(, Z)((*a)[0], (*b)[0]);
}

// --- Heap operations ---

fun void X(HIT, Down)(X(, css) heap, size_t at) {
    size_t n = $len(heap);
    size_t i = at;
    for (;;) {
        size_t left = 2 * i + 1;
        if (left >= n || left < i) break;
        size_t right = left + 1;
        size_t j = left;
        if (right < n && X(HIT, Z)($atp(heap, right), $atp(heap, left)))
            j = right;
        if (!X(HIT, Z)($atp(heap, j), $atp(heap, i))) break;
        X(, csSwap)($atp(heap, i), $atp(heap, j));
        i = j;
    }
}

fun void X(HIT, Heap)(X(, css) heap) {
    size_t n = $len(heap);
    for (size_t i = n / 2; i > 0; --i)
        X(HIT, Down)(heap, i - 1);
}

// --- Eject entry at position ---

fun void X(HIT, Eject)(X(, css) heap, size_t at) {
    size_t last = $len(heap) - 1;
    if (at != last) X(, csSwap)($atp(heap, at), $atp(heap, last));
    --heap[1];
}

// --- Start: filter empty entries, compact, heapify ---

fun void X(HIT, Start)(X(, css) heap) {
    HIT_E *w = heap[0];
    for (HIT_E *r = heap[0]; r < heap[1]; r++) {
        if (!$empty(*r)) {
            if (w != r) X(, csSwap)(w, r);
            w++;
        }
    }
    heap[1] = w;
    X(HIT, Heap)(heap);
}

// --- Step: advance top entry, eject if exhausted, re-heapify ---

fun void X(HIT, Step)(X(, css) heap) {
    ++(*heap[0])[0];
    if ($empty(*heap[0])) {
        X(HIT, Eject)(heap, 0);
        if ($empty(heap)) return;
    }
    X(HIT, Down)(heap, 0);
}

// --- Tops: find all entries with head equal to minimum ---
// Moves them to front of heap. Returns count.

fun size_t X(HIT, Tops)(X(, css) heap) {
    size_t l = $len(heap);
    if (l == 0) return 0;
    size_t eqlen = 1;
    size_t lim = 2;
    for (size_t i = 1; i < l && i <= lim; ++i) {
        if (X(HIT, Z)($atp(heap, 0), $atp(heap, i))) continue;
        if (eqlen != i) {
            X(, csSwap)($atp(heap, eqlen), $atp(heap, i));
            X(HIT, Down)(heap, i);
            --i;
        } else {
            lim = i * 2 + 2;
        }
        eqlen++;
    }
    return eqlen;
}

// --- AdvanceTops: advance ntops entries, eject exhausted ---

fun void X(HIT, AdvanceTops)(X(, css) heap, size_t ntops) {
    for (size_t j = ntops; j > 0; --j) {
        size_t i = j - 1;
        ++(*$atp(heap, i))[0];
        if ($empty(*$atp(heap, i))) {
            X(HIT, Eject)(heap, i);
            if (i < $len(heap)) X(HIT, Down)(heap, i);
        } else {
            X(HIT, Down)(heap, i);
        }
    }
}

// --- Merge: drain heap producing sorted deduplicated output ---

fun void X(HIT, Merge)(X(, css) heap, X(, p) *out) {
    while (!$empty(heap)) {
        HIT_T val = *(*heap[0])[0];
        **out = val;
        ++*out;
        size_t ntops = X(HIT, Tops)(heap);
        X(HIT, AdvanceTops)(heap, ntops);
        // skip remaining duplicates
        while (!$empty(heap) && !X(, Z)((*heap[0])[0], &val)
                              && !X(, Z)(&val, (*heap[0])[0]))
            X(HIT, Step)(heap);
    }
}

// --- Intersect: emit values present in ALL nruns iterators ---

fun void X(HIT, Intersect)(X(, css) heap, X(, p) *out, size_t nruns) {
    while (!$empty(heap)) {
        size_t ntops = X(HIT, Tops)(heap);
        HIT_T val = *(*heap[0])[0];
        if (ntops >= nruns) {
            **out = val;
            ++*out;
        }
        X(HIT, AdvanceTops)(heap, ntops);
        // skip remaining duplicates of val
        while (!$empty(heap) && !X(, Z)((*heap[0])[0], &val)
                              && !X(, Z)(&val, (*heap[0])[0]))
            X(HIT, Step)(heap);
    }
}

// --- Seek: advance all entries until heap top >= key ---

fun ok64 X(HIT, Seek)(X(, css) heap, X(, cp) key) {
    HIT_E keyentry = {key, key + 1};
    while (!$empty(heap) && X(HIT, Z)(heap[0], &keyentry)) {
        X(, c) *const run[2] = {(*heap[0])[0], (*heap[0])[1]};
        X(, c) *pos = X(, sFindGE)(run, key);
        (*heap[0])[0] = pos;
        if ($empty(*heap[0])) {
            X(HIT, Eject)(heap, 0);
            if ($empty(heap)) break;
        }
        X(HIT, Down)(heap, 0);
    }
    return $empty(heap) ? NODATA : OK;
}

// --- SeekRange: trim all entries to [lo, hi), eject empty, re-heapify ---
// After this, the heap only contains elements in [lo, hi).
// No prefix checks needed during the walk — just drain until empty.

fun ok64 X(HIT, SeekRange)(X(, css) heap, X(, cp) lo, X(, cp) hi) {
    HIT_E *w = heap[0];
    for (HIT_E *r = heap[0]; r < heap[1]; r++) {
        if ($empty(*r)) continue;
        HIT_T *sub[2];
        X(, sFindRange)(sub, *r, lo, hi);
        if (sub[0] < sub[1]) {
            (*w)[0] = sub[0];
            (*w)[1] = sub[1];
            w++;
        }
    }
    heap[1] = w;
    if ($empty(heap)) return NODATA;
    X(HIT, Heap)(heap);
    return OK;
}

// --- SkipValue: advance inner HIT past its current top merged value ---

fun void X(HIT, SkipValue)(X(, css) inner) {
    if ($empty(inner)) return;
    HIT_T val = *(*inner[0])[0];
    size_t ntops = X(HIT, Tops)(inner);
    X(HIT, AdvanceTops)(inner, ntops);
    while (!$empty(inner) && !X(, Z)((*inner[0])[0], &val)
                           && !X(, Z)(&val, (*inner[0])[0]))
        X(HIT, Step)(inner);
}

// --- IntersectMerge: intersect the merged outputs of N inner HITs ---
// Each inner HIT is a X(,css) (heap of sorted runs producing merged output).
// The outer heap walks N such inner HITs in lockstep: emit a value only
// when ALL inner HITs produce it. Zero intermediate allocations.
//
// Usage:
//   u64cs *outers[N][2];   // N inner HITs
//   outers[i][0] = runs_i; outers[i][1] = runs_i + nruns_i;
//   HITu64Start(outers[i]);  // start each inner HIT
//   u64csss oh = {outers, outers + N};
//   u64 buf[...]; u64p out = buf;
//   HITu64sIntersectMerge(oh, &out);

typedef X(, css) *X(, csss)[2];

// Top value pointer of an inner HIT
fun X(, cp) X(HIT, cssTop)(X(, css) *hit) {
    return (*(*hit)[0])[0];
}

fun void X(, cssSwap)(X(, css) *a, X(, css) *b) {
    X(, cs) *t0 = (*a)[0], *t1 = (*a)[1];
    (*a)[0] = (*b)[0]; (*a)[1] = (*b)[1];
    (*b)[0] = t0; (*b)[1] = t1;
}

fun void X(HIT, cssDown)(X(, csss) oh, size_t at) {
    size_t n = (size_t)$len(oh);
    size_t i = at;
    for (;;) {
        size_t left = 2 * i + 1;
        if (left >= n || left < i) break;
        size_t right = left + 1;
        size_t j = left;
        if (right < n && X(, Z)(X(HIT, cssTop)($atp(oh, right)),
                                 X(HIT, cssTop)($atp(oh, left))))
            j = right;
        if (!X(, Z)(X(HIT, cssTop)($atp(oh, j)),
                     X(HIT, cssTop)($atp(oh, i)))) break;
        X(, cssSwap)($atp(oh, i), $atp(oh, j));
        i = j;
    }
}

fun size_t X(HIT, cssTops)(X(, csss) oh) {
    size_t l = (size_t)$len(oh);
    if (l == 0) return 0;
    size_t eqlen = 1;
    size_t lim = 2;
    for (size_t i = 1; i < l && i <= lim; ++i) {
        if (X(, Z)(X(HIT, cssTop)($atp(oh, 0)),
                    X(HIT, cssTop)($atp(oh, i)))) continue;
        if (eqlen != i) {
            X(, cssSwap)($atp(oh, eqlen), $atp(oh, i));
            X(HIT, cssDown)(oh, i);
            --i;
        } else {
            lim = i * 2 + 2;
        }
        eqlen++;
    }
    return eqlen;
}

fun void X(HIT, sIntersectMerge)(X(, csss) oheap, X(, p) *out) {
    // Filter empty inner HITs
    X(, css) *w = oheap[0];
    for (X(, css) *r = oheap[0]; r < oheap[1]; r++) {
        if (!$empty(*r)) {
            if (w != r) X(, cssSwap)(w, r);
            w++;
        }
    }
    oheap[1] = w;
    size_t nruns = (size_t)$len(oheap);
    if (nruns == 0) return;

    // Heapify outer
    for (size_t i = nruns / 2; i > 0; --i)
        X(HIT, cssDown)(oheap, i - 1);

    while ((size_t)$len(oheap) == nruns) {
        size_t ntops = X(HIT, cssTops)(oheap);
        HIT_T val = *X(HIT, cssTop)($atp(oheap, 0));

        if (ntops >= nruns) {
            **out = val;
            ++*out;
        }

        // Advance top ntops inner HITs past val
        for (size_t j = ntops; j > 0; --j) {
            size_t i = j - 1;
            X(HIT, SkipValue)(*$atp(oheap, i));
            if ($empty(*$atp(oheap, i))) return;
            X(HIT, cssDown)(oheap, i);
        }
    }
}

#undef HIT_H
#undef HIT_E
#undef HIT_T
