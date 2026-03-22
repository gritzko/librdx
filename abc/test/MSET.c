#include "INT.h"
#include "PRO.h"
#include "TEST.h"

#define X(M, name) M##u64##name
#include "MSETx.h"
#undef X

ok64 MSET0() {
    sane(1);
    u64 arr[] = {5, 3, 1, 4, 2};
    u64s data = {arr, arr + 5};
    MSETu64Sort(data);
    for (int i = 0; i < 4; i++) want(arr[i] <= arr[i + 1]);
    done;
}

ok64 MSET1() {
    sane(1);
    u64 a[] = {1, 3, 5, 7};
    u64 b[] = {2, 4, 6, 8};
    u64cs runs[2] = {{a, a + 4}, {b, b + 4}};
    u64css iter = {runs, runs + 2};
    u64 out[8];
    u64s into = {out, out + 8};
    call(MSETu64Merge, into, iter);
    for (int i = 0; i < 8; i++) testeq(out[i], (u64)(i + 1));
    done;
}

#define BULK_N 1000
#define BULK_K 7

ok64 MSET2() {
    sane(1);
    u64 input[BULK_N], ref[BULK_N];
    u64 r = 42;
    for (int i = 0; i < BULK_N; i++) {
        r = r * 6364136223846793005ULL + 1;
        input[i] = ref[i] = r;
    }
    u64s refs = {ref, ref + BULK_N};
    $sort(refs, u64cmp);
    u64cs runs[BULK_K];
    size_t chunk = BULK_N / BULK_K;
    for (int i = 0; i < BULK_K; i++) {
        size_t from = i * chunk;
        size_t till = (i == BULK_K - 1) ? BULK_N : from + chunk;
        u64s run = {input + from, input + till};
        MSETu64Sort(run);
        runs[i][0] = input + from;
        runs[i][1] = input + till;
    }
    u64css iter = {runs, runs + BULK_K};
    u64 out[BULK_N];
    u64s into = {out, out + BULK_N};
    call(MSETu64Merge, into, iter);
    for (int i = 0; i < BULK_N; i++) testeq(out[i], ref[i]);
    done;
}

ok64 MSET3() {
    sane(1);
    // empty runs
    u64cs runs0[3] = {{NULL, NULL}, {NULL, NULL}, {NULL, NULL}};
    u64css iter0 = {runs0, runs0 + 3};
    u64 out0[1];
    u64s into0 = {out0, out0 + 1};
    call(MSETu64Merge, into0, iter0);
    testeq(into0[0], (u64 *)out0);

    // single-element runs
    u64 a[] = {3}, b[] = {1}, c[] = {2};
    u64cs runs1[3] = {{a, a + 1}, {b, b + 1}, {c, c + 1}};
    u64css iter1 = {runs1, runs1 + 3};
    u64 out1[3];
    u64s into1 = {out1, out1 + 3};
    call(MSETu64Merge, into1, iter1);
    testeq(out1[0], (u64)1);
    testeq(out1[1], (u64)2);
    testeq(out1[2], (u64)3);

    // duplicates: {1,1,3} + {1,2,3} => {1,2,3}
    u64 d[] = {1, 1, 3}, e[] = {1, 2, 3};
    u64cs runs2[2] = {{d, d + 3}, {e, e + 3}};
    u64css iter2 = {runs2, runs2 + 2};
    u64 out2[6];
    u64s into2 = {out2, out2 + 6};
    call(MSETu64Merge, into2, iter2);
    testeq((size_t)(into2[0] - out2), (size_t)3);
    testeq(out2[0], (u64)1);
    testeq(out2[1], (u64)2);
    testeq(out2[2], (u64)3);
    done;
}

ok64 MSET4() {
    sane(1);
    u64 a[] = {1, 3, 5, 7, 9};
    u64 b[] = {2, 4, 6, 8, 10};
    u64cs runs[2] = {{a, a + 5}, {b, b + 5}};
    u64css iter = {runs, runs + 2};
    MSETu64Start(iter);
    testeq(****iter, (u64)1);
    call(MSETu64Seek, iter, (u64)5);
    testeq(****iter, (u64)5);
    call(MSETu64Seek, iter, (u64)7);
    testeq(****iter, (u64)7);
    ok64 o = MSETu64Seek(iter, 100);
    testeq(o, MSETNODATA);
    done;
}

ok64 MSET5() {
    sane(1);
    u64 a[] = {1, 3};
    u64 b[] = {2, 4};
    u64cs runs[2] = {{a, a + 2}, {b, b + 2}};
    u64css iter = {runs, runs + 2};
    MSETu64Start(iter);
    testeq(****iter, (u64)1);
    call(MSETu64Next, iter);
    testeq(****iter, (u64)2);
    call(MSETu64Next, iter);
    testeq(****iter, (u64)3);
    call(MSETu64Next, iter);
    testeq(****iter, (u64)4);
    call(MSETu64Next, iter);
    want($empty(iter));
    ok64 o = MSETu64Next(iter);
    testeq(o, MSETNODATA);
    done;
}

ok64 MSET6() {
    sane(1);
    // oldest-first: 100, 10, 1 elements — each 1/10 of preceding, ok
    u64 a[100], b[10], c[1];
    for (int i = 0; i < 100; i++) a[i] = (u64)i;
    for (int i = 0; i < 10; i++) b[i] = (u64)(100 + i);
    c[0] = 200;
    u64cs runs[3] = {{a, a + 100}, {b, b + 10}, {c, c + 1}};
    u64css stack = {runs, runs + 3};
    want(MSETu64IsCompact(stack) == YES);

    // violating stack: 8, 8 — second is not < 1/8 of first
    u64 d[8], e[8];
    for (int i = 0; i < 8; i++) d[i] = (u64)i;
    for (int i = 0; i < 8; i++) e[i] = (u64)(8 + i);
    u64cs runs2[2] = {{d, d + 8}, {e, e + 8}};
    u64css stack2 = {runs2, runs2 + 2};
    want(MSETu64IsCompact(stack2) == NO);

    // single run is always compact
    u64cs runs3[1] = {{a, a + 100}};
    u64css stack3 = {runs3, runs3 + 1};
    want(MSETu64IsCompact(stack3) == YES);
    done;
}

ok64 MSET7() {
    sane(1);
    // oldest-first: run of 1000, then run of 5, then run of 8
    // 8 * 8 = 64 > 5, so last two must merge => 13 elements
    // 13 * 8 = 104 < 1000, so stop
    u64 big[1000];
    for (int i = 0; i < 1000; i++) big[i] = (u64)(i * 3);
    u64 med[] = {1, 4, 7, 10, 13};
    u64 sml[] = {0, 2, 5, 8, 11, 14, 17, 20};
    u64cs runs[3] = {
        {big, big + 1000},
        {med, med + 5},
        {sml, sml + 8}
    };
    u64css stack = {runs, runs + 3};
    want(MSETu64IsCompact(stack) == NO);

    u64 buf[13];
    u64s into = {buf, buf + 13};
    call(MSETu64Compact, stack, into);
    // should now have 2 runs: big[1000] and merged[13]
    testeq($len(stack), (size_t)2);
    testeq($len(stack[0][0]), (size_t)1000);
    testeq($len(stack[0][1]), (size_t)13);
    want(MSETu64IsCompact(stack) == YES);
    // merged run must be sorted
    for (int i = 0; i + 1 < 13; i++) want(buf[i] <= buf[i + 1]);
    done;
}

ok64 MSET8() {
    sane(1);
    // cascading: oldest-first 10, 10, 10 — all equal, must all merge
    u64 a[10], b[10], c[10];
    for (int i = 0; i < 10; i++) a[i] = (u64)(i * 3);
    for (int i = 0; i < 10; i++) b[i] = (u64)(i * 3 + 1);
    for (int i = 0; i < 10; i++) c[i] = (u64)(i * 3 + 2);
    u64cs runs[3] = {{a, a + 10}, {b, b + 10}, {c, c + 10}};
    u64css stack = {runs, runs + 3};
    u64 buf[30];
    u64s into = {buf, buf + 30};
    call(MSETu64Compact, stack, into);
    // all three merged into one
    testeq($len(stack), (size_t)1);
    testeq($len(stack[0][0]), (size_t)30);
    want(MSETu64IsCompact(stack) == YES);
    for (int i = 0; i + 1 < 30; i++) want(buf[i] <= buf[i + 1]);
    done;
}

ok64 MSET9() {
    sane(1);
    u64 a[] = {1, 3, 5, 7, 9};
    u64 b[] = {2, 4, 6, 8, 10};
    u64cs runs[2] = {{a, a + 5}, {b, b + 5}};
    u64css stack = {runs, runs + 2};
    // find every element
    for (u64 v = 1; v <= 10; v++)
        testeq(MSETu64Get(stack, v), OK);
    // missing elements
    testeq(MSETu64Get(stack, (u64)0), MSETNODATA);
    testeq(MSETu64Get(stack, (u64)11), MSETNODATA);
    // empty stack
    u64css empty = {runs, runs};
    testeq(MSETu64Get(empty, (u64)1), MSETNODATA);
    done;
}

// Next skips duplicates across runs
ok64 MSETa() {
    sane(1);
    u64 a[] = {1, 2, 3};
    u64 b[] = {1, 2, 3};
    u64cs runs[2] = {{a, a + 3}, {b, b + 3}};
    u64css iter = {runs, runs + 2};
    MSETu64Start(iter);
    testeq(****iter, (u64)1);
    call(MSETu64Next, iter);
    testeq(****iter, (u64)2);
    call(MSETu64Next, iter);
    testeq(****iter, (u64)3);
    call(MSETu64Next, iter);
    want($empty(iter));
    done;
}

// Merge deduplicates identical elements
ok64 MSETb() {
    sane(1);
    u64 a[] = {1, 1, 1};
    u64 b[] = {1, 1, 1};
    u64cs runs[2] = {{a, a + 3}, {b, b + 3}};
    u64css iter = {runs, runs + 2};
    u64 out[6];
    u64s into = {out, out + 6};
    call(MSETu64Merge, into, iter);
    testeq((size_t)(into[0] - out), (size_t)1);
    testeq(out[0], (u64)1);
    done;
}

// Compact deduplicates across merged runs
ok64 MSETc() {
    sane(1);
    // oldest: 100 distinct, then two small runs with overlapping values
    u64 big[100];
    for (int i = 0; i < 100; i++) big[i] = (u64)(i * 2);
    u64 sml1[] = {1, 3, 5, 7, 9};
    u64 sml2[] = {1, 3, 5, 7, 9};
    u64cs runs[3] = {
        {big, big + 100},
        {sml1, sml1 + 5},
        {sml2, sml2 + 5}
    };
    u64css stack = {runs, runs + 3};
    u64 buf[10];
    u64s into = {buf, buf + 10};
    call(MSETu64Compact, stack, into);
    testeq($len(stack), (size_t)2);
    // 5 unique values after dedup
    testeq($len(stack[0][1]), (size_t)5);
    for (int i = 0; i + 1 < 5; i++) want(buf[i] < buf[i + 1]);
    done;
}

ok64 MSETtest() {
    sane(1);
    call(MSET0);
    call(MSET1);
    call(MSET2);
    call(MSET3);
    call(MSET4);
    call(MSET5);
    call(MSET6);
    call(MSET7);
    call(MSET8);
    call(MSET9);
    call(MSETa);
    call(MSETb);
    call(MSETc);
    done;
}

TEST(MSETtest);
