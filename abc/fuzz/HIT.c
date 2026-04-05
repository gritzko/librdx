#include "INT.h"
#include "PRO.h"
#include "TEST.h"

// Manual Swap for u64cs (array type, can't use Sx.h)
fun void u64csSwap(u64cs *a, u64cs *b) {
    u64c *t0 = (*a)[0], *t1 = (*a)[1];
    (*a)[0] = (*b)[0];
    (*a)[1] = (*b)[1];
    (*b)[0] = t0;
    (*b)[1] = t1;
}

#define X(M, name) M##u64##name
#include "HITx.h"
#undef X

// MSETx for cross-validation
#define X(M, name) M##u64##name
#include "MSETx.h"
#undef X

#define LEN 512
#define MAXRUNS 32

FUZZ(u64, HITfuzz) {
    sane(1);
    if ($len(input) > LEN) input[1] = input[0] + LEN;
    size_t n = $len(input);
    if (n < 2) done;

    // --- Split input into sorted runs ---
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

    // --- Test A: Merge via HIT, cross-check vs MSET ---
    u64cs uruns[MAXRUNS];
    for (size_t i = 0; i < nruns; i++) {
        uruns[i][0] = runs[i][0];
        uruns[i][1] = runs[i][1];
    }
    u64css uheap = {uruns, uruns + nruns};
    HITu64Start(uheap);
    u64 ubuf[LEN];
    u64p uout = ubuf;
    HITu64Merge(uheap, &uout);
    size_t ulen = uout - ubuf;
    for (size_t i = 0; i + 1 < ulen; i++)
        must(ubuf[i] < ubuf[i + 1], "merge not sorted");

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
    must(mlen == ulen, "merge length mismatch");
    for (size_t i = 0; i < mlen; i++)
        must(mbuf[i] == ubuf[i], "merge value mismatch");

    // --- Test B: Intersect via HIT ---
    u64cs iruns[MAXRUNS];
    for (size_t i = 0; i < nruns; i++) {
        iruns[i][0] = runs[i][0];
        iruns[i][1] = runs[i][1];
    }
    u64css iheap = {iruns, iruns + nruns};
    HITu64Start(iheap);
    u64 ibuf[LEN];
    u64p iout = ibuf;
    HITu64Intersect(iheap, &iout, nruns);
    size_t ilen = iout - ibuf;
    // intersection strictly sorted
    for (size_t i = 0; i + 1 < ilen; i++)
        must(ibuf[i] < ibuf[i + 1], "intersection not sorted");
    // every intersection element must be in every run
    for (size_t j = 0; j < ilen; j++) {
        for (size_t r = 0; r < nruns; r++) {
            b8 found = NO;
            for (u64c *p = runs[r][0]; p < runs[r][1]; p++) {
                if (*p == ibuf[j]) { found = YES; break; }
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

    // --- Test C: Seek + Merge cross-validation vs MSET ---
    u64 seekkey = input[0][0];

    u64cs sruns[MAXRUNS];
    for (size_t i = 0; i < nruns; i++) {
        sruns[i][0] = runs[i][0];
        sruns[i][1] = runs[i][1];
    }
    u64css sheap = {sruns, sruns + nruns};
    HITu64Start(sheap);
    if (!$empty(sheap))
        HITu64Seek(sheap, &seekkey);
    u64 sbuf[LEN];
    u64p sout = sbuf;
    HITu64Merge(sheap, &sout);
    size_t slen = sout - sbuf;
    for (size_t i = 0; i + 1 < slen; i++)
        must(sbuf[i] < sbuf[i + 1], "seek merge not sorted");
    for (size_t i = 0; i < slen; i++)
        must(sbuf[i] >= seekkey, "seek merge element < key");

    // MSET path
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
    must(mslen == slen, "seek length mismatch");
    for (size_t i = 0; i < mslen; i++)
        must(msbuf[i] == sbuf[i], "seek value mismatch");

    // --- Test D: sIntersectMerge cross-validation ---
    // Split runs into 2 groups, run sIntersectMerge, compare against
    // individual Merge + two-pointer intersect.
    if (nruns >= 2) {
        size_t half = nruns / 2;

        // sIntersectMerge path
        u64cs ir1[MAXRUNS], ir2[MAXRUNS];
        for (size_t i = 0; i < half; i++) {
            ir1[i][0] = runs[i][0]; ir1[i][1] = runs[i][1];
        }
        for (size_t i = half; i < nruns; i++) {
            ir2[i - half][0] = runs[i][0]; ir2[i - half][1] = runs[i][1];
        }
        u64cs *oha[2][2];
        oha[0][0] = ir1; oha[0][1] = ir1 + half;
        oha[1][0] = ir2; oha[1][1] = ir2 + (nruns - half);
        HITu64Start(oha[0]);
        HITu64Start(oha[1]);
        u64csss imh = {oha, oha + 2};
        u64 imbuf[LEN]; u64p imout = imbuf;
        HITu64sIntersectMerge(imh, &imout);
        size_t imlen = imout - imbuf;
        for (size_t i = 0; i + 1 < imlen; i++)
            must(imbuf[i] < imbuf[i + 1], "sIM not sorted");

        // Reference: Merge each half, then two-pointer intersect
        u64cs mr1[MAXRUNS], mr2[MAXRUNS];
        for (size_t i = 0; i < half; i++) {
            mr1[i][0] = runs[i][0]; mr1[i][1] = runs[i][1];
        }
        for (size_t i = half; i < nruns; i++) {
            mr2[i - half][0] = runs[i][0]; mr2[i - half][1] = runs[i][1];
        }
        u64css mh1 = {mr1, mr1 + half};
        u64css mh2 = {mr2, mr2 + (nruns - half)};
        HITu64Start(mh1); HITu64Start(mh2);
        u64 m1[LEN]; u64p m1o = m1;
        u64 m2[LEN]; u64p m2o = m2;
        HITu64Merge(mh1, &m1o);
        HITu64Merge(mh2, &m2o);
        size_t m1len = m1o - m1, m2len = m2o - m2;

        u64 refbuf[LEN]; size_t reflen = 0;
        size_t p1 = 0, p2 = 0;
        while (p1 < m1len && p2 < m2len) {
            if (m1[p1] < m2[p2]) p1++;
            else if (m1[p1] > m2[p2]) p2++;
            else { refbuf[reflen++] = m1[p1]; p1++; p2++; }
        }
        must(imlen == reflen, "sIM length mismatch");
        for (size_t i = 0; i < reflen; i++)
            must(imbuf[i] == refbuf[i], "sIM value mismatch");
    }

    done;
}
