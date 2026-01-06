//
// (c) Victor Grishchenko, 2020-2023
//
#include <benchmark/benchmark.h>

#include "01.h"

extern "C" {
#include "SORT.h"
}

using namespace std;

#define LEN1 (1 << 17)

uint8_t _pro_depth = 0;

static void QSort(benchmark::State &state) {
    aB(u64, ints);
    size_t n = state.range(0);
    u64bAllocate(intsbuf, n);
    u64 r = 57;
    for (auto _ : state) {
        for (u64 i = 0; i < n; ++i) {
            u64sFeed1(intsidle, i ^ r);
        }
        $sort(intsdata, u64cmp);
        assert(*$head(intsdata) <= *$last(intsdata));
        assert($len(intsdata) == n);
        Breset(intsbuf);
    }
    u64bFree(intsbuf);
}

BENCHMARK(QSort)->Range(8 << 10, 8 << 20);

static void YSort(benchmark::State &state) {
    aB(u64, ints);
    aB(u64, ints2);
    size_t n = state.range(0);
    u64bAlloc(intsbuf, n);
    u64bAlloc(ints2buf, n);
    u64 r = 57;
    u64 i = 0;
    for (auto _ : state) {
        for (u64 i = 0; i < n; ++i) {
            u64sFeed1(intsidle, i ^ r);
        }
        SORTu64(ints2idle, intsdata);
        assert(*$head(ints2data) <= *$last(ints2data));
        assert($len(ints2data) == n);
        Breset(intsbuf);
        Breset(ints2buf);
    }
    u64bFree(intsbuf);
    u64bFree(ints2buf);
}

BENCHMARK(YSort)->Range(8 << 10, 8 << 20);

BENCHMARK_MAIN();
