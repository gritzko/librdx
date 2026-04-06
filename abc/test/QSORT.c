#include "INT.h"
#include "PRO.h"
#include "TEST.h"

// --- Test 0: basic sort ---
ok64 QSORT0() {
    sane(1);
    u64 arr[] = {5, 3, 1, 4, 2, 9, 7, 8, 6, 0};
    u64s data = {arr, arr + 10};
    u64sSort(data);
    for (int i = 0; i < 9; i++) want(arr[i] <= arr[i + 1]);
    for (int i = 0; i < 10; i++) testeq(arr[i], (u64)i);
    done;
}

// --- Test 1: already sorted ---
ok64 QSORT1() {
    sane(1);
    u64 arr[] = {1, 2, 3, 4, 5};
    u64s data = {arr, arr + 5};
    u64sSort(data);
    for (int i = 0; i < 5; i++) testeq(arr[i], (u64)(i + 1));
    done;
}

// --- Test 2: reverse sorted ---
ok64 QSORT2() {
    sane(1);
    u64 arr[] = {5, 4, 3, 2, 1};
    u64s data = {arr, arr + 5};
    u64sSort(data);
    for (int i = 0; i < 5; i++) testeq(arr[i], (u64)(i + 1));
    done;
}

// --- Test 3: all equal ---
ok64 QSORT3() {
    sane(1);
    u64 arr[] = {7, 7, 7, 7, 7};
    u64s data = {arr, arr + 5};
    u64sSort(data);
    for (int i = 0; i < 5; i++) testeq(arr[i], (u64)7);
    done;
}

// --- Test 4: single element ---
ok64 QSORT4() {
    sane(1);
    u64 arr[] = {42};
    u64s data = {arr, arr + 1};
    u64sSort(data);
    testeq(arr[0], (u64)42);
    done;
}

// --- Test 5: empty ---
ok64 QSORT5() {
    sane(1);
    u64 arr[1] = {};
    u64s data = {arr, arr};
    u64sSort(data);
    done;
}

// --- Test 6: large random, compare to stdlib qsort ---
#define QSORT_N 10000
ok64 QSORT6() {
    sane(1);
    u64 a[QSORT_N], b[QSORT_N];
    u64 r = 0x123456789ABCDEF0ULL;
    for (int i = 0; i < QSORT_N; i++) {
        r = r * 6364136223846793005ULL + 1442695040888963407ULL;
        a[i] = b[i] = r;
    }
    u64s as = {a, a + QSORT_N};
    u64s bs = {b, b + QSORT_N};
    u64sSort(as);
    $sort(bs, u64cmp);
    for (int i = 0; i < QSORT_N; i++) testeq(a[i], b[i]);
    done;
}

// --- Test 7: sDedup on sorted data ---
ok64 QSORT7() {
    sane(1);
    u64 arr[] = {1, 1, 2, 3, 3, 3, 4, 5, 5};
    u64s data = {arr, arr + 9};
    u64sDedup(data);
    testeq($len(data), (size_t)5);
    for (int i = 0; i < 5; i++) testeq(data[0][i], (u64)(i + 1));
    done;
}

// --- Test 8: sDedup all equal ---
ok64 QSORT8() {
    sane(1);
    u64 arr[] = {7, 7, 7, 7};
    u64s data = {arr, arr + 4};
    u64sDedup(data);
    testeq($len(data), (size_t)1);
    testeq(data[0][0], (u64)7);
    done;
}

// --- Test 9: sDedup no duplicates ---
ok64 QSORT9() {
    sane(1);
    u64 arr[] = {1, 2, 3, 4, 5};
    u64s data = {arr, arr + 5};
    u64sDedup(data);
    testeq($len(data), (size_t)5);
    done;
}

// --- Test 10: sDedup empty ---
ok64 QSORT10() {
    sane(1);
    u64 arr[1] = {};
    u64s data = {arr, arr};
    u64sDedup(data);
    testeq($len(data), (size_t)0);
    done;
}

// --- Test 11: sort+dedup end-to-end ---
ok64 QSORT11() {
    sane(1);
    u64 arr[] = {5, 3, 1, 3, 2, 5, 1, 4, 2};
    u64s data = {arr, arr + 9};
    u64sSort(data);
    u64sDedup(data);
    testeq($len(data), (size_t)5);
    for (int i = 0; i < 5; i++) testeq(data[0][i], (u64)(i + 1));
    done;
}

// --- Test 12: u32 sort ---
ok64 QSORT12() {
    sane(1);
    u32 arr[] = {10, 3, 7, 1, 5};
    u32s data = {arr, arr + 5};
    u32sSort(data);
    for (int i = 0; i < 4; i++) want(arr[i] <= arr[i + 1]);
    done;
}

// --- Test 13: bSort / bDedup ---
ok64 QSORT13() {
    sane(1);
    Bu64 buf = {};
    call(u64bAlloc, buf, 16);
    u64bFeed1(buf, 5);
    u64bFeed1(buf, 3);
    u64bFeed1(buf, 1);
    u64bFeed1(buf, 3);
    u64bFeed1(buf, 1);
    testeq(u64bDataLen(buf), (size_t)5);
    u64bSort(buf);
    u64bDedup(buf);
    testeq(u64bDataLen(buf), (size_t)3);
    u64 *d = u64bDataHead(buf);
    testeq(d[0], (u64)1);
    testeq(d[1], (u64)3);
    testeq(d[2], (u64)5);
    u64bFree(buf);
    done;
}

// --- Test 14: large random with many duplicates ---
#define QSORT_D 50000
ok64 QSORT14() {
    sane(1);
    u64 a[QSORT_D], b[QSORT_D];
    u64 r = 42;
    for (int i = 0; i < QSORT_D; i++) {
        r = r * 6364136223846793005ULL + 1;
        a[i] = b[i] = r % 1000;  // lots of duplicates
    }
    u64s as = {a, a + QSORT_D};
    u64s bs = {b, b + QSORT_D};
    u64sSort(as);
    $sort(bs, u64cmp);
    for (int i = 0; i < QSORT_D; i++) testeq(a[i], b[i]);

    u64sDedup(as);
    want($len(as) <= 1000);
    want($len(as) > 0);
    for (size_t i = 1; i < $len(as); i++) want(as[0][i - 1] < as[0][i]);
    done;
}

ok64 QSORTtest() {
    sane(1);
    call(QSORT0);
    call(QSORT1);
    call(QSORT2);
    call(QSORT3);
    call(QSORT4);
    call(QSORT5);
    call(QSORT6);
    call(QSORT7);
    call(QSORT8);
    call(QSORT9);
    call(QSORT10);
    call(QSORT11);
    call(QSORT12);
    call(QSORT13);
    call(QSORT14);
    done;
}

TEST(QSORTtest);
