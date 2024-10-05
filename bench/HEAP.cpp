//
// (c) Victor Grishchenko, 2020-2023
//
#include <benchmark/benchmark.h>

#include "INT.h"
#include "PRO.h"
#include "TEST.h"

extern "C" {
#define X(M, name) M##u32##name
#include "../HEAPx.h"
#undef X
}

using namespace std;

static void HeapPush(benchmark::State &state) {
    aBpad(u32, pad, MB);
    u32$ heap = Bu32data(pad);
    u64 i = 0;
    for (auto _ : state) {
        ++i;
        if ((i & MB / 2) == 0) {
            Breset(pad);
        }
        u32 p = i * PRIME1;
        ok64 o = HEAPu32push1(pad, p);
    }
    i = 0;
    for (auto _ : state) {
        ++i;
        u32 p;
        ok64 o = HEAPu32pop(&p, pad);
    }
}

BENCHMARK(HeapPush);

static void HeapPushFn(benchmark::State &state) {
    aBpad(u32, pad, MB);
    u32$ heap = Bu32data(pad);
    u64 i = 0;
    for (auto _ : state) {
        ++i;
        if ((i & MB / 2) == 0) {
            Breset(pad);
        }
        u32 p = i * PRIME1;
        ok64 o = HEAPu32push1f(pad, p, u32cmp);
    }
    i = 0;
    for (auto _ : state) {
        ++i;
        u32 p;
        ok64 o = HEAPu32popf(&p, pad, u32cmp);
    }
}

BENCHMARK(HeapPushFn);

BENCHMARK_MAIN();
