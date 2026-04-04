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

static size_t g_nruns;

// --- u64 advancer: single value, always consumed ---
fun ok64 u64NextX(u64p a, u64cp b) {
    (void)a;
    (void)b;
    return NODATA;
}

// --- u64 merge policies ---

// Union: always emit
fun ok64 u64UnionY(u64p a, u64s tops) {
    *a = *tops[0];
    return OK;
}

// Intersection: emit if count >= threshold
fun ok64 u64InterY(u64p a, u64s tops) {
    if ((size_t)$len(tops) < g_nruns) return MISS;
    *a = *tops[0];
    return OK;
}

// --- u64cs comparator ---
fun b8 u64csHeadZ(u64cs const *a, u64cs const *b) {
    return *(*a)[0] < *(*b)[0];
}

// --- u64cs advancer: advance slice past prev value ---
fun ok64 u64csNextX(u64cs *a, u64cs const *b) {
    u64 val = *(*b)[0];
    while (!$empty(*a) && *(*a)[0] == val) ++(*a)[0];
    return $empty(*a) ? NODATA : OK;
}

// --- u64cs merge policies ---

// Union: always emit
fun ok64 u64csUnionY(u64cs *a, u64css tops) {
    (*a)[0] = (*tops[0])[0];
    (*a)[1] = (*tops[0])[0] + 1;
    return OK;
}

// Intersection: emit if all runs present
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

// HIT0: u64 union [5,3,1,4,2] -> [1,2,3,4,5]
ok64 HIT0() {
    sane(1);
    u64 arr[] = {5, 3, 1, 4, 2};
    u64s heap = {arr, arr + 5};
    HITu64HeapYZ(heap, u64Z);
    u64 buf[5];
    u64s out = {buf, buf + 5};
    while (HITu64NextYZ(heap, out, u64NextX, u64UnionY, u64Z) == OK) {}
    size_t olen = out[0] - buf;
    testeq(olen, (size_t)5);
    testeq(buf[0], (u64)1);
    testeq(buf[1], (u64)2);
    testeq(buf[2], (u64)3);
    testeq(buf[3], (u64)4);
    testeq(buf[4], (u64)5);
    done;
}

// HIT1: u64 union with dups [3,1,3,2,1] -> [1,2,3]
ok64 HIT1() {
    sane(1);
    u64 arr[] = {3, 1, 3, 2, 1};
    u64s heap = {arr, arr + 5};
    HITu64HeapYZ(heap, u64Z);
    u64 buf[5];
    u64s out = {buf, buf + 5};
    while (HITu64NextYZ(heap, out, u64NextX, u64UnionY, u64Z) == OK) {}
    size_t olen = out[0] - buf;
    testeq(olen, (size_t)3);
    testeq(buf[0], (u64)1);
    testeq(buf[1], (u64)2);
    testeq(buf[2], (u64)3);
    done;
}

// HIT2: u64cs union of 3 sorted runs -> sorted deduped merge
ok64 HIT2() {
    sane(1);
    u64 a[] = {1, 3, 5, 7};
    u64 b[] = {2, 4, 6, 8};
    u64 c[] = {1, 4, 7, 10};
    u64cs runs[3] = {{a, a + 4}, {b, b + 4}, {c, c + 4}};
    u64css heap = {runs, runs + 3};
    HITu64csHeapYZ(heap, u64csHeadZ);
    u64cs vbuf[12];
    u64css out = {vbuf, vbuf + 12};
    while (HITu64csNextYZ(heap, out, u64csNextX, u64csUnionY, u64csHeadZ) == OK) {}
    size_t olen = out[0] - vbuf;
    // expected: 1,2,3,4,5,6,7,8,10
    testeq(olen, (size_t)9);
    testeq(*vbuf[0][0], (u64)1);
    testeq(*vbuf[1][0], (u64)2);
    testeq(*vbuf[2][0], (u64)3);
    testeq(*vbuf[3][0], (u64)4);
    testeq(*vbuf[4][0], (u64)5);
    testeq(*vbuf[5][0], (u64)6);
    testeq(*vbuf[6][0], (u64)7);
    testeq(*vbuf[7][0], (u64)8);
    testeq(*vbuf[8][0], (u64)10);
    done;
}

// HIT3: cross-validate u64cs union vs MSETu64Merge
ok64 HIT3() {
    sane(1);
    u64 a[] = {1, 3, 5, 7, 9};
    u64 b[] = {2, 3, 6, 8, 10};
    u64 c[] = {1, 4, 5, 9, 11};

    // MSET merge
    u64cs mruns[3] = {{a, a + 5}, {b, b + 5}, {c, c + 5}};
    u64css miter = {mruns, mruns + 3};
    u64 mbuf[15];
    u64s minto = {mbuf, mbuf + 15};
    call(MSETu64Merge, minto, miter);
    size_t mlen = minto[0] - mbuf;

    // HIT union
    u64cs hruns[3] = {{a, a + 5}, {b, b + 5}, {c, c + 5}};
    u64css hheap = {hruns, hruns + 3};
    HITu64csHeapYZ(hheap, u64csHeadZ);
    u64cs hvbuf[15];
    u64css hout = {hvbuf, hvbuf + 15};
    while (HITu64csNextYZ(hheap, hout, u64csNextX, u64csUnionY, u64csHeadZ) == OK) {}
    size_t hlen = hout[0] - hvbuf;

    testeq(hlen, mlen);
    for (size_t i = 0; i < mlen; i++)
        testeq(*hvbuf[i][0], mbuf[i]);
    done;
}

// HIT4: u64 intersection of 3 runs
ok64 HIT4() {
    sane(1);
    u64 a[] = {1, 2, 3, 5, 7};
    u64 b[] = {2, 3, 4, 5, 8};
    u64 c[] = {1, 3, 5, 6, 9};
    u64 all[15];
    memcpy(all, a, sizeof(a));
    memcpy(all + 5, b, sizeof(b));
    memcpy(all + 10, c, sizeof(c));
    u64s heap = {all, all + 15};
    HITu64HeapYZ(heap, u64Z);
    g_nruns = 3;
    u64 buf[15];
    u64s out = {buf, buf + 15};
    ok64 r;
    while ((r = HITu64NextYZ(heap, out, u64NextX, u64InterY, u64Z)) != NODATA) {}
    size_t olen = out[0] - buf;
    // intersection {1,2,3,5,7} & {2,3,4,5,8} & {1,3,5,6,9} = {3,5}
    testeq(olen, (size_t)2);
    testeq(buf[0], (u64)3);
    testeq(buf[1], (u64)5);
    done;
}

// HIT5: u64cs intersection of 3 runs
ok64 HIT5() {
    sane(1);
    u64 a[] = {1, 2, 3, 5, 7};
    u64 b[] = {2, 3, 4, 5, 8};
    u64 c[] = {1, 3, 5, 6, 9};
    u64cs runs[3] = {{a, a + 5}, {b, b + 5}, {c, c + 5}};
    u64css heap = {runs, runs + 3};
    HITu64csHeapYZ(heap, u64csHeadZ);
    g_nruns = 3;
    u64cs vbuf[15];
    u64css out = {vbuf, vbuf + 15};
    ok64 r;
    while ((r = HITu64csNextYZ(heap, out, u64csNextX, u64csInterY, u64csHeadZ)) != NODATA) {}
    size_t olen = out[0] - vbuf;
    // intersection {1,2,3,5,7} & {2,3,4,5,8} & {1,3,5,6,9} = {3,5}
    testeq(olen, (size_t)2);
    testeq(*vbuf[0][0], (u64)3);
    testeq(*vbuf[1][0], (u64)5);
    done;
}

// HIT6: empty -> NODATA
ok64 HIT6() {
    sane(1);
    u64s heap = {NULL, NULL};
    u64 buf[1];
    u64s out = {buf, buf + 1};
    ok64 r = HITu64NextYZ(heap, out, u64NextX, u64UnionY, u64Z);
    testeq(r, NODATA);
    testeq(out[0], buf);
    done;
}

// HIT7: single entry drain
ok64 HIT7() {
    sane(1);
    u64 arr[] = {42};
    u64s heap = {arr, arr + 1};
    HITu64HeapYZ(heap, u64Z);
    u64 buf[1];
    u64s out = {buf, buf + 1};
    ok64 r = HITu64NextYZ(heap, out, u64NextX, u64UnionY, u64Z);
    testeq(r, OK);
    testeq(buf[0], (u64)42);
    r = HITu64NextYZ(heap, out, u64NextX, u64UnionY, u64Z);
    testeq(r, NODATA);
    done;
}

// HIT8: Start filters empties, heap is correct
ok64 HIT8() {
    sane(1);
    u64 a[] = {1, 3, 5};
    u64 b[] = {2, 4, 6};
    u64cs runs[4] = {
        {a, a + 3},
        {NULL, NULL},       // empty
        {b, b + 3},
        {a + 3, a + 3},    // empty (begin == end)
    };
    u64css heap = {runs, runs + 4};
    HITu64csStartZ(heap, u64csHeadZ);
    testeq($len(heap), (size_t)2);
    // top should be minimum (1)
    testeq(*(*heap[0])[0], (u64)1);
    done;
}

// HIT9: Start with empties + drain, cross-validate vs MSET
ok64 HIT9() {
    sane(1);
    u64 a[] = {1, 5, 9};
    u64 b[] = {2, 5, 8};
    u64 c[] = {3, 6, 7};

    // HIT: with empties interspersed
    u64cs hruns[5] = {
        {a, a + 3},
        {NULL, NULL},
        {b, b + 3},
        {c + 3, c + 3},    // empty
        {c, c + 3},
    };
    u64css hheap = {hruns, hruns + 5};
    HITu64csStartZ(hheap, u64csHeadZ);
    u64cs hvbuf[12];
    u64css hout = {hvbuf, hvbuf + 12};
    while (HITu64csNextYZ(hheap, hout, u64csNextX, u64csUnionY, u64csHeadZ) == OK) {}
    size_t hlen = hout[0] - hvbuf;

    // MSET: same data, no empties
    u64cs mruns[3] = {{a, a + 3}, {b, b + 3}, {c, c + 3}};
    u64css miter = {mruns, mruns + 3};
    u64 mbuf[12];
    u64s minto = {mbuf, mbuf + 12};
    call(MSETu64Merge, minto, miter);
    size_t mlen = minto[0] - mbuf;

    testeq(hlen, mlen);
    for (size_t i = 0; i < mlen; i++)
        testeq(*hvbuf[i][0], mbuf[i]);
    done;
}

// HIT10: Seek basic — all entries advance past key
ok64 HIT10() {
    sane(1);
    u64 a[] = {1, 3, 5, 7, 9};
    u64 b[] = {2, 4, 6, 8, 10};
    u64 c[] = {3, 5, 7, 11, 13};
    u64cs runs[3] = {{a, a + 5}, {b, b + 5}, {c, c + 5}};
    u64css heap = {runs, runs + 3};
    HITu64csStartZ(heap, u64csHeadZ);
    ok64 o = HITu64csSeek(heap, 5, u64csHeadZ);
    testeq(o, OK);
    // top must be >= 5
    test(*(*heap[0])[0] >= 5, FAILSANITY);
    // drain and verify all values >= 5
    u64cs vbuf[15];
    u64css out = {vbuf, vbuf + 15};
    while (HITu64csNextYZ(heap, out, u64csNextX, u64csUnionY, u64csHeadZ) == OK) {}
    size_t olen = out[0] - vbuf;
    for (size_t i = 0; i < olen; i++)
        test(*vbuf[i][0] >= 5, FAILSANITY);
    // expected: 5,6,7,8,9,10,11,13
    testeq(olen, (size_t)8);
    done;
}

// HIT11: Seek + drain, cross-validate vs MSET Seek
ok64 HIT11() {
    sane(1);
    u64 a[] = {10, 20, 30, 40, 50};
    u64 b[] = {15, 25, 35, 45, 55};
    u64 c[] = {12, 22, 32, 42, 52};

    // HIT: Start + Seek + drain
    u64cs hruns[3] = {{a, a + 5}, {b, b + 5}, {c, c + 5}};
    u64css hheap = {hruns, hruns + 3};
    HITu64csStartZ(hheap, u64csHeadZ);
    HITu64csSeek(hheap, 25, u64csHeadZ);
    u64cs hvbuf[15];
    u64css hout = {hvbuf, hvbuf + 15};
    while (HITu64csNextYZ(hheap, hout, u64csNextX, u64csUnionY, u64csHeadZ) == OK) {}
    size_t hlen = hout[0] - hvbuf;

    // MSET: Start + Seek + drain
    u64cs mruns[3] = {{a, a + 5}, {b, b + 5}, {c, c + 5}};
    u64css miter = {mruns, mruns + 3};
    MSETu64Start(miter);
    MSETu64Seek(miter, 25);
    u64 mbuf[15];
    size_t mlen = 0;
    while (!$empty(miter)) {
        mbuf[mlen++] = ****miter;
        MSETu64Next(miter);
    }

    testeq(hlen, mlen);
    for (size_t i = 0; i < mlen; i++)
        testeq(*hvbuf[i][0], mbuf[i]);
    done;
}

// HIT12: Seek past all data → NODATA
ok64 HIT12() {
    sane(1);
    u64 a[] = {1, 2, 3};
    u64 b[] = {4, 5, 6};
    u64cs runs[2] = {{a, a + 3}, {b, b + 3}};
    u64css heap = {runs, runs + 2};
    HITu64csStartZ(heap, u64csHeadZ);
    ok64 o = HITu64csSeek(heap, 100, u64csHeadZ);
    testeq(o, NODATA);
    testeq($len(heap), (size_t)0);
    done;
}

// HIT13: Seek to before all data (no-op)
ok64 HIT13() {
    sane(1);
    u64 a[] = {5, 10, 15};
    u64 b[] = {7, 12, 20};
    u64cs runs[2] = {{a, a + 3}, {b, b + 3}};
    u64css heap = {runs, runs + 2};
    HITu64csStartZ(heap, u64csHeadZ);
    ok64 o = HITu64csSeek(heap, 1, u64csHeadZ);
    testeq(o, OK);
    // top should still be 5 (unchanged)
    testeq(*(*heap[0])[0], (u64)5);
    done;
}

// HIT14: Seek to exact value present in data
ok64 HIT14() {
    sane(1);
    u64 a[] = {1, 3, 5, 7};
    u64 b[] = {2, 4, 6, 8};
    u64cs runs[2] = {{a, a + 4}, {b, b + 4}};
    u64css heap = {runs, runs + 2};
    HITu64csStartZ(heap, u64csHeadZ);
    HITu64csSeek(heap, 4, u64csHeadZ);
    // top should be exactly 4
    testeq(*(*heap[0])[0], (u64)4);
    // drain rest
    u64cs vbuf[8];
    u64css out = {vbuf, vbuf + 8};
    while (HITu64csNextYZ(heap, out, u64csNextX, u64csUnionY, u64csHeadZ) == OK) {}
    size_t olen = out[0] - vbuf;
    // expected: 4,5,6,7,8
    testeq(olen, (size_t)5);
    testeq(*vbuf[0][0], (u64)4);
    testeq(*vbuf[1][0], (u64)5);
    testeq(*vbuf[2][0], (u64)6);
    testeq(*vbuf[3][0], (u64)7);
    testeq(*vbuf[4][0], (u64)8);
    done;
}

// HIT15: Start on all-empty → empty heap
ok64 HIT15() {
    sane(1);
    u64cs runs[3] = {
        {NULL, NULL},
        {NULL, NULL},
        {NULL, NULL},
    };
    u64css heap = {runs, runs + 3};
    HITu64csStartZ(heap, u64csHeadZ);
    testeq($len(heap), (size_t)0);
    ok64 o = HITu64csSeek(heap, 1, u64csHeadZ);
    testeq(o, NODATA);
    done;
}

ok64 HITtest() {
    sane(1);
    call(HIT0);
    call(HIT1);
    call(HIT2);
    call(HIT3);
    call(HIT4);
    call(HIT5);
    call(HIT6);
    call(HIT7);
    call(HIT8);
    call(HIT9);
    call(HIT10);
    call(HIT11);
    call(HIT12);
    call(HIT13);
    call(HIT14);
    call(HIT15);
    done;
}

TEST(HITtest);
