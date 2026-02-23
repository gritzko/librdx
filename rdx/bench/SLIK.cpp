//
// SLIK benchmark: write N records, then seek to each one
//
#include <benchmark/benchmark.h>

extern "C" {
#include "abc/INT.h"
#include "abc/PRO.h"
#include "rdx/RDX.h"
}

using namespace std;

uint8_t _pro_depth = 0;

// Write N records, then seek to each
ok64 SLIKBenchSeek(benchmark::State &state, u8bp pad, u64bp tabs, i64 N) {
    sane(1);

    // Write N INT records
    rdx e;
    rdxWriteInitSLIK(&e, pad, tabs);
    e.type = RDX_TYPE_EULER;
    call(rdxWriteNextSLIK, &e);

    rdx c = {};
    call(rdxWriteIntoSLIK, &c, &e);
    for (i64 j = 0; j < N; j++) {
        c.type = RDX_TYPE_INT;
        c.i = j * 7;
        c.id.seq = j;
        c.id.src = 0;
        call(rdxWriteNextSLIK, &c);
    }
    call(rdxWriteOutoSLIK, &c, &e);
    call(rdxWriteFinishSLIK, &e);

    // Setup read
    a_pad0(u64, readstack, PAGESIZE);
    rdx r;
    rdxInitSLIK(&r, pad, readstack);
    call(rdxNextSLIK, &r);
    test(r.type == RDX_TYPE_EULER, RDXBAD);

    // Benchmark: seek to each record
    i64 k = 0;
    for (auto _ : state) {
        rdx seek = {};
        seek.type = RDX_TYPE_INT;
        seek.i = k * 7;
        call(rdxIntoSLIK, &seek, &r);
        benchmark::DoNotOptimize(seek.i);
        call(rdxOutoSLIK, &seek, &r);
        k = (k + 1) % N;
    }

    done;
}

static void BM_SLIKSeek_10K(benchmark::State &state) {
    u8b pad = {};
    u64b tabs = {};
    u8bMap(pad, MB * 4);
    u64bMap(tabs, PAGESIZE * 4);
    ok64 o = SLIKBenchSeek(state, pad, tabs, 10000);
    if (o != OK) fprintf(stderr, "ERR: %s\n", ok64str(o));
    u64bUnMap(tabs);
    u8bUnMap(pad);
}

static void BM_SLIKSeek_100K(benchmark::State &state) {
    u8b pad = {};
    u64b tabs = {};
    u8bMap(pad, MB * 32);
    u64bMap(tabs, PAGESIZE * 4);
    ok64 o = SLIKBenchSeek(state, pad, tabs, 100000);
    if (o != OK) fprintf(stderr, "ERR: %s\n", ok64str(o));
    u64bUnMap(tabs);
    u8bUnMap(pad);
}

static void BM_SLIKSeek_1M(benchmark::State &state) {
    u8b pad = {};
    u64b tabs = {};
    u8bMap(pad, MB * 256);
    u64bMap(tabs, PAGESIZE * 4);
    ok64 o = SLIKBenchSeek(state, pad, tabs, 1000000);
    if (o != OK) fprintf(stderr, "ERR: %s\n", ok64str(o));
    u64bUnMap(tabs);
    u8bUnMap(pad);
}

BENCHMARK(BM_SLIKSeek_10K);
BENCHMARK(BM_SLIKSeek_100K);
BENCHMARK(BM_SLIKSeek_1M);

BENCHMARK_MAIN();
