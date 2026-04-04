#include "INT.h"
#include "PRO.h"
#include "TEST.h"

#define X(M, name) M##u64##name
#include "MSETx.h"
#undef X

#define LEN 512
#define MAXRUNS 32

FUZZ(u64, MSETfuzz) {
    sane(1);
    if ($len(input) > LEN) input[1] = input[0] + LEN;
    size_t n = $len(input);
    if (n < 2) done;

    // --- Split input on 0s into sorted runs ---
    u64 work[LEN];
    u64cs runs[MAXRUNS];
    size_t nruns = 0;
    size_t wpos = 0;
    size_t rstart = 0;
    for (size_t i = 0; i < n; i++) {
        u64 v = input[0][i];
        if (v == 0 || i == n - 1) {
            if (v != 0) work[wpos++] = v;
            if (wpos > rstart && nruns < MAXRUNS) {
                u64s run = {work + rstart, work + wpos};
                MSETu64Sort(run);
                runs[nruns][0] = work + rstart;
                runs[nruns][1] = work + wpos;
                nruns++;
                rstart = wpos;
            }
        } else {
            work[wpos++] = v;
        }
    }
    if (nruns < 1) done;

    // --- Test 1: Dedup-union via TopZ+AdvZ ---
    u64cs uruns[MAXRUNS];
    for (size_t i = 0; i < nruns; i++) {
        uruns[i][0] = runs[i][0];
        uruns[i][1] = runs[i][1];
    }
    u64css uheap = {uruns, uruns + nruns};
    MSETu64StartZ(uheap, u64Z);
    u64 ubuf[LEN];
    size_t ulen = 0;
    while (!$empty(uheap)) {
        must(ulen < LEN, "union overflow");
        ubuf[ulen] = ****uheap;
        // skip all copies of this value (dedup across and within runs)
        do {
            u64css eqs;
            ok64 o = MSETu64TopZ(uheap, eqs, u64Z);
            must(o == OK, "TopZ fail in union");
            MSETu64AdvZ(uheap, $len(eqs), u64Z);
        } while (!$empty(uheap) &&
                 !u64Z(&ubuf[ulen], ***uheap) &&
                 !u64Z(***uheap, &ubuf[ulen]));
        ulen++;
    }
    // union must be strictly sorted (deduped)
    for (size_t i = 0; i + 1 < ulen; i++)
        must(ubuf[i] < ubuf[i + 1], "union not strictly sorted");
    // every element from every run must appear in union
    for (size_t r = 0; r < nruns; r++) {
        for (u64c *p = runs[r][0]; p < runs[r][1]; p++) {
            b8 found = NO;
            for (size_t j = 0; j < ulen; j++) {
                if (ubuf[j] == *p) { found = YES; break; }
            }
            must(found, "union missing element");
        }
    }

    // --- Test 2: Intersection via TopZ+AdvZ ---
    u64cs iruns[MAXRUNS];
    for (size_t i = 0; i < nruns; i++) {
        iruns[i][0] = runs[i][0];
        iruns[i][1] = runs[i][1];
    }
    u64css iheap = {iruns, iruns + nruns};
    MSETu64StartZ(iheap, u64Z);
    u64 ibuf[LEN];
    size_t ilen = 0;
    while (!$empty(iheap) && $len(iheap) >= nruns) {
        u64css eqs;
        ok64 o = MSETu64TopZ(iheap, eqs, u64Z);
        must(o == OK, "TopZ fail in intersection");
        b8 emit = ($len(eqs) == nruns);
        u64 val = ****iheap;
        // advance and skip further duplicates of val
        do {
            MSETu64AdvZ(iheap, $len(eqs), u64Z);
            if ($empty(iheap)) break;
            if (u64Z(&val, ***iheap) || u64Z(***iheap, &val)) break;
            o = MSETu64TopZ(iheap, eqs, u64Z);
            must(o == OK, "TopZ fail in intersection skip");
        } while (1);
        if (emit) {
            must(ilen < LEN, "intersection overflow");
            ibuf[ilen++] = val;
        }
    }
    // intersection must be strictly sorted
    for (size_t i = 0; i + 1 < ilen; i++)
        must(ibuf[i] < ibuf[i + 1], "intersection not strictly sorted");
    // every intersection element must be in every run
    for (size_t j = 0; j < ilen; j++) {
        for (size_t r = 0; r < nruns; r++) {
            b8 found = NO;
            for (u64c *p = runs[r][0]; p < runs[r][1]; p++) {
                if (*p == ibuf[j]) { found = YES; break; }
            }
            must(found, "intersection element not in run");
        }
    }
    // every element in ALL runs must be in intersection
    for (size_t r = 0; r < nruns; r++) {
        for (u64c *p = runs[r][0]; p < runs[r][1]; p++) {
            b8 in_all = YES;
            for (size_t r2 = 0; r2 < nruns; r2++) {
                b8 found = NO;
                for (u64c *q = runs[r2][0]; q < runs[r2][1]; q++) {
                    if (*q == *p) { found = YES; break; }
                }
                if (!found) { in_all = NO; break; }
            }
            if (!in_all) continue;
            b8 in_result = NO;
            for (size_t j = 0; j < ilen; j++) {
                if (ibuf[j] == *p) { in_result = YES; break; }
            }
            must(in_result, "missing from intersection");
        }
    }

    // --- Test 3: Merge (dedup union) matches TopZ union ---
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
    must(mlen == ulen, "merge/union length mismatch");
    for (size_t i = 0; i < mlen; i++)
        must(mbuf[i] == ubuf[i], "merge/union value mismatch");

    done;
}
