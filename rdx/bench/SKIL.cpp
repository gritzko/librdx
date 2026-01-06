//
// (c) Victor Grishchenko, 2020-2023
//
#include <benchmark/benchmark.h>

extern "C" {
#include "abc/INT.h"
#include "abc/PRO.h"
#include "rdx/RDX.h"
}

using namespace std;

uint8_t _pro_depth = 0;

ok64 SKILBinarySearch(benchmark::State &state, u8bp pad) {
    sane(1);

    // Test seeking to specific positions using SKIL binary search
    a_pad(u64, tabs, PAGESIZE);
    u8sp pad_idle = u8bIdle(pad);
    u8sp pad_data = u8bData(pad);

    rdx e = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE, .extra = (void *)tabs};
    $mv(e.into, pad_idle);
    e.type = RDX_TYPE_EULER;
    call(rdxNext, &e);

    rdx i = {};
    call(rdxInto, &i, &e);
    // Write multiples of 10: 0, 10, 20, 30, ..., 9990
    i64 j = 0;
    for (auto _ : state) {
        i.type = RDX_TYPE_INT;
        i.i = j;
        i.id.seq = j;
        i.id.src = 0;
        call(rdxNext, &i);
        ++j;
    }
    call(rdxOuto, &i, &e);
    $mv(pad_idle, e.into);

    // Read back and verify using binary search
    rdx e2 = {.format = RDX_FMT_SKIL};
    $mv(e2.data, pad_data);
    call(rdxNext, &e2);
    test(e2.type == RDX_TYPE_EULER, RDXBAD);

    // Test seeking to various positions
    // int test_values[] = {0, 100, 500, 5000, 9990};
    // for (int k = 0; k < 5; k++) {
    printf("to %li\n", j);
    for (i64 i = 0; i < j; i++) {
        rdx target = {};
        target.type = RDX_TYPE_INT;
        target.i = i;

        call(rdxInto, &target, &e2);
        // Should find the exact value
        test(target.type == RDX_TYPE_INT, RDXBAD);
        test(target.i == i, NOEQ);
        test(target.id.seq == i, NOEQ);
        call(rdxOuto, &target, &e2);
    }

    done;
}

static void BM_SomeFunction(benchmark::State &state) {
    u8b pad = {};
    u8bMap(pad, GB);
    ok64 o = SKILBinarySearch(state, pad);
    if (o != OK) fprintf(stderr, "%s\n", ok64str(o));
    u8bUnMap(pad);
}

BENCHMARK(BM_SomeFunction);
//->Range(8 << 10, 8 << 20);

BENCHMARK_MAIN();
