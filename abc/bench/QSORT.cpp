#include <benchmark/benchmark.h>

extern "C" {
#include "INT.h"
}

uint8_t _pro_depth = 0;

// Fill buffer with pseudo-random data
static void fill_random(u64 *arr, size_t n, u64 seed) {
    for (size_t i = 0; i < n; i++) {
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        arr[i] = seed;
    }
}

static void fill_random_dups(u64 *arr, size_t n, u64 seed) {
    for (size_t i = 0; i < n; i++) {
        seed = seed * 6364136223846793005ULL + 1;
        arr[i] = seed % (n / 4 + 1);
    }
}

// --- stdlib qsort (function pointer comparator) ---

static void BM_StdlibQSort(benchmark::State &state) {
    size_t n = (size_t)state.range(0);
    u64 *arr = new u64[n];
    for (auto _ : state) {
        state.PauseTiming();
        fill_random(arr, n, 42);
        state.ResumeTiming();
        u64s data = {arr, arr + n};
        $sort(data, u64cmp);
        benchmark::DoNotOptimize(arr[0]);
    }
    delete[] arr;
    state.SetItemsProcessed((int64_t)state.iterations() * (int64_t)n);
}

// --- inline introsort (no function pointer) ---

static void BM_QSort(benchmark::State &state) {
    size_t n = (size_t)state.range(0);
    u64 *arr = new u64[n];
    for (auto _ : state) {
        state.PauseTiming();
        fill_random(arr, n, 42);
        state.ResumeTiming();
        u64s data = {arr, arr + n};
        u64sSort(data);
        benchmark::DoNotOptimize(arr[0]);
    }
    delete[] arr;
    state.SetItemsProcessed((int64_t)state.iterations() * (int64_t)n);
}

// --- sort + dedup with duplicates ---

static void BM_StdlibSortDedup(benchmark::State &state) {
    size_t n = (size_t)state.range(0);
    u64 *arr = new u64[n];
    for (auto _ : state) {
        state.PauseTiming();
        fill_random_dups(arr, n, 42);
        state.ResumeTiming();
        u64s data = {arr, arr + n};
        $sort(data, u64cmp);
        // manual dedup
        u64 *w = arr + 1;
        for (u64 *r = arr + 1; r < data[1]; r++)
            if (*r != *(r - 1)) *w++ = *r;
        benchmark::DoNotOptimize(w);
    }
    delete[] arr;
    state.SetItemsProcessed((int64_t)state.iterations() * (int64_t)n);
}

static void BM_QSortDedup(benchmark::State &state) {
    size_t n = (size_t)state.range(0);
    u64 *arr = new u64[n];
    for (auto _ : state) {
        state.PauseTiming();
        fill_random_dups(arr, n, 42);
        state.ResumeTiming();
        u64s data = {arr, arr + n};
        u64sSort(data);
        u64sDedup(data);
        benchmark::DoNotOptimize(data[1]);
    }
    delete[] arr;
    state.SetItemsProcessed((int64_t)state.iterations() * (int64_t)n);
}

BENCHMARK(BM_StdlibQSort)->Range(1 << 10, 1 << 20);
BENCHMARK(BM_QSort)->Range(1 << 10, 1 << 20);
BENCHMARK(BM_StdlibSortDedup)->Range(1 << 10, 1 << 20);
BENCHMARK(BM_QSortDedup)->Range(1 << 10, 1 << 20);

BENCHMARK_MAIN();
