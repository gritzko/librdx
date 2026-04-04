#include "INT.h"
#include "PRO.h"
#include "TEST.h"

// HITx for u64 (primitives)
#define X(M, name) M##u64##name
#include "HITx.h"
#undef X

// Manual Swap for u64cs (array type, can't use Sx.h)
fun void u64csSwap(u64cs *a, u64cs *b) {
    u64c *t0 = (*a)[0], *t1 = (*a)[1];
    (*a)[0] = (*b)[0];
    (*a)[1] = (*b)[1];
    (*b)[0] = t0;
    (*b)[1] = t1;
}

typedef b8 (*u64csz)(u64cs const *, u64cs const *);
typedef ok64 (*u64csx)(u64cs *, u64cs const *);
typedef ok64 (*u64csy)(u64cs *, u64css);

// HITx for u64cs (sorted slices)
#define HIT_ENTRY_IS_SLICE
#define X(M, name) M##u64cs##name
#include "HITx.h"
#undef X
#undef HIT_ENTRY_IS_SLICE

// MSETx for cross-validation
#define X(M, name) M##u64##name
#include "MSETx.h"
#undef X

#define LEN 512
#define MAXRUNS 32

static size_t g_nruns;

// --- u64 advancer: single value, always consumed ---
fun ok64 u64NextX(u64p a, u64cp b) {
    (void)a;
    (void)b;
    return NODATA;
}

// --- u64 union policy ---
fun ok64 u64UnionY(u64p a, u64s tops) {
    *a = *tops[0];
    return OK;
}

// --- u64cs functions ---
fun b8 u64csHeadZ(u64cs const *a, u64cs const *b) {
    return *(*a)[0] < *(*b)[0];
}

fun ok64 u64csNextX(u64cs *a, u64cs const *b) {
    u64 val = *(*b)[0];
    while (!$empty(*a) && *(*a)[0] == val) ++(*a)[0];
    return $empty(*a) ? NODATA : OK;
}

fun ok64 u64csUnionY(u64cs *a, u64css tops) {
    (*a)[0] = (*tops[0])[0];
    (*a)[1] = (*tops[0])[0] + 1;
    return OK;
}

fun ok64 u64csInterY(u64cs *a, u64css tops) {
    if ((size_t)$len(tops) < g_nruns) return MISS;
    (*a)[0] = (*tops[0])[0];
    (*a)[1] = (*tops[0])[0] + 1;
    return OK;
}

// --- u64cs seek-within-entry: binary search ---
fun ok64 u64csSeekX(u64cs *a, u64cs const *b) {
    u64c *const run[2] = {(*a)[0], (*a)[1]};
    u64c *pos = $u64findge(run, (*b)[0]);
    (*a)[0] = pos;
    return $empty(*a) ? NODATA : OK;
}

// Convenience wrapper: Seek with raw u64 key
fun ok64 HITu64csSeek(u64css heap, u64 key, u64csz z) {
    u64cs keyentry = {&key, &key + 1};
    return HITu64csSeekXZ(heap, &keyentry, u64csSeekX, z);
}

FUZZ(u64, HITfuzz) {
    sane(1);
    if ($len(input) > LEN) input[1] = input[0] + LEN;
    size_t n = $len(input);
    if (n < 2) done;

    // --- Test A: Primitives ---
    u64 work[LEN];
    size_t wlen = 0;
    for (size_t i = 0; i < n && wlen < LEN; i++) {
        if (input[0][i] != 0) work[wlen++] = input[0][i];
    }
    if (wlen > 0) {
        u64 heap[LEN];
        memcpy(heap, work, wlen * sizeof(u64));
        u64s h = {heap, heap + wlen};
        HITu64HeapYZ(h, u64Z);
        u64 ubuf[LEN];
        u64s uout = {ubuf, ubuf + LEN};
        while (HITu64NextYZ(h, uout, u64NextX, u64UnionY, u64Z) == OK) {}
        size_t ulen = uout[0] - ubuf;
        // must be strictly sorted (deduped)
        for (size_t i = 0; i + 1 < ulen; i++)
            must(ubuf[i] < ubuf[i + 1], "prim union not sorted");
        // every input value must appear
        for (size_t i = 0; i < wlen; i++) {
            b8 found = NO;
            for (size_t j = 0; j < ulen; j++) {
                if (ubuf[j] == work[i]) {
                    found = YES;
                    break;
                }
            }
            must(found, "prim union missing element");
        }
        // cross-check: qsort + manual dedup
        u64 ref[LEN];
        memcpy(ref, work, wlen * sizeof(u64));
        u64s refs = {ref, ref + wlen};
        $sort(refs, u64cmp);
        size_t rlen = 0;
        for (size_t i = 0; i < wlen; i++) {
            if (i == 0 || ref[i] != ref[i - 1]) ref[rlen++] = ref[i];
        }
        must(rlen == ulen, "prim union length mismatch");
        for (size_t i = 0; i < rlen; i++)
            must(ref[i] == ubuf[i], "prim union value mismatch");
    }

    // --- Test B: Slices ---
    u64 swork[LEN];
    u64cs runs[MAXRUNS];
    size_t nruns = 0;
    size_t wpos = 0;
    size_t rstart = 0;
    for (size_t i = 0; i < n; i++) {
        u64 v = input[0][i];
        if (v == 0 || i == n - 1) {
            if (v != 0) swork[wpos++] = v;
            if (wpos > rstart && nruns < MAXRUNS) {
                u64s run = {swork + rstart, swork + wpos};
                MSETu64Sort(run);
                runs[nruns][0] = swork + rstart;
                runs[nruns][1] = swork + wpos;
                nruns++;
                rstart = wpos;
            }
        } else {
            swork[wpos++] = v;
        }
    }
    if (nruns < 1) done;

    // Union via HIT
    u64cs uruns[MAXRUNS];
    for (size_t i = 0; i < nruns; i++) {
        uruns[i][0] = runs[i][0];
        uruns[i][1] = runs[i][1];
    }
    u64css uheap = {uruns, uruns + nruns};
    HITu64csHeapYZ(uheap, u64csHeadZ);
    u64cs uvbuf[LEN];
    u64css uout = {uvbuf, uvbuf + LEN};
    while (HITu64csNextYZ(uheap, uout, u64csNextX, u64csUnionY, u64csHeadZ) == OK) {}
    size_t ulen = uout[0] - uvbuf;
    u64 ubuf[LEN];
    for (size_t i = 0; i < ulen; i++) ubuf[i] = *uvbuf[i][0];
    for (size_t i = 0; i + 1 < ulen; i++)
        must(ubuf[i] < ubuf[i + 1], "slice union not sorted");

    // Cross-check vs MSET Merge
    u64cs mruns[MAXRUNS];
    for (size_t i = 0; i < nruns; i++) {
        mruns[i][0] = runs[i][0];
        mruns[i][1] = runs[i][1];
    }
    u64css miter = {mruns, mruns + nruns};
    u64 mbuf[LEN];
    u64s minto = {mbuf, mbuf + LEN};
    call(MSETu64Merge, minto, miter);
    size_t mlen = minto[0] - mbuf;
    must(mlen == ulen, "slice union/merge length mismatch");
    for (size_t i = 0; i < mlen; i++)
        must(mbuf[i] == ubuf[i], "slice union/merge value mismatch");

    // Intersection via HIT
    u64cs iruns[MAXRUNS];
    for (size_t i = 0; i < nruns; i++) {
        iruns[i][0] = runs[i][0];
        iruns[i][1] = runs[i][1];
    }
    u64css iheap = {iruns, iruns + nruns};
    HITu64csHeapYZ(iheap, u64csHeadZ);
    g_nruns = nruns;
    u64cs ivbuf[LEN];
    u64css iout = {ivbuf, ivbuf + LEN};
    ok64 ir;
    while ((ir = HITu64csNextYZ(iheap, iout, u64csNextX, u64csInterY, u64csHeadZ)) != NODATA) {}
    size_t ilen = iout[0] - ivbuf;
    u64 ibuf[LEN];
    for (size_t i = 0; i < ilen; i++) ibuf[i] = *ivbuf[i][0];
    // intersection strictly sorted
    for (size_t i = 0; i + 1 < ilen; i++)
        must(ibuf[i] < ibuf[i + 1], "intersection not sorted");
    // every intersection element must be in every run
    for (size_t j = 0; j < ilen; j++) {
        for (size_t r = 0; r < nruns; r++) {
            b8 found = NO;
            for (u64c *p = runs[r][0]; p < runs[r][1]; p++) {
                if (*p == ibuf[j]) {
                    found = YES;
                    break;
                }
            }
            must(found, "inter element not in run");
        }
    }
    // every element in ALL runs must be in intersection
    for (size_t r = 0; r < nruns; r++) {
        for (u64c *p = runs[r][0]; p < runs[r][1]; p++) {
            b8 in_all = YES;
            for (size_t r2 = 0; r2 < nruns; r2++) {
                b8 found = NO;
                for (u64c *q = runs[r2][0]; q < runs[r2][1]; q++) {
                    if (*q == *p) {
                        found = YES;
                        break;
                    }
                }
                if (!found) {
                    in_all = NO;
                    break;
                }
            }
            if (!in_all) continue;
            b8 in_result = NO;
            for (size_t j = 0; j < ilen; j++) {
                if (ibuf[j] == *p) {
                    in_result = YES;
                    break;
                }
            }
            must(in_result, "missing from intersection");
        }
    }

    // --- Test C: Start + Seek cross-validation ---
    u64 seekkey = input[0][0];

    // HIT path: Start + Seek + drain union
    u64cs sruns[MAXRUNS];
    for (size_t i = 0; i < nruns; i++) {
        sruns[i][0] = runs[i][0];
        sruns[i][1] = runs[i][1];
    }
    u64css sheap = {sruns, sruns + nruns};
    HITu64csStartZ(sheap, u64csHeadZ);
    if (!$empty(sheap)) {
        HITu64csSeek(sheap, seekkey, u64csHeadZ);
    }
    u64cs svbuf[LEN];
    u64css sout = {svbuf, svbuf + LEN};
    while (HITu64csNextYZ(sheap, sout, u64csNextX, u64csUnionY,
                          u64csHeadZ) == OK) {
    }
    size_t slen = sout[0] - svbuf;
    u64 sbuf[LEN];
    for (size_t i = 0; i < slen; i++) sbuf[i] = *svbuf[i][0];
    for (size_t i = 0; i + 1 < slen; i++)
        must(sbuf[i] < sbuf[i + 1], "seek union not sorted");
    for (size_t i = 0; i < slen; i++)
        must(sbuf[i] >= seekkey, "seek union element < key");

    // MSET path: Start + Seek + Next drain
    u64cs msr[MAXRUNS];
    for (size_t i = 0; i < nruns; i++) {
        msr[i][0] = runs[i][0];
        msr[i][1] = runs[i][1];
    }
    u64css msiter = {msr, msr + nruns};
    MSETu64Start(msiter);
    MSETu64Seek(msiter, seekkey);
    u64 msbuf[LEN];
    size_t mslen = 0;
    while (!$empty(msiter) && mslen < LEN) {
        u64c *p = ***msiter;
        msbuf[mslen++] = *p;
        MSETu64Next(msiter);
    }
    must(mslen == slen, "seek start length mismatch");
    for (size_t i = 0; i < mslen; i++)
        must(msbuf[i] == sbuf[i], "seek start value mismatch");

    done;
}
