#   Coding guidelines

0.  Dependencies: libsodium, gtest, google-benchmark.
    Recommended debug build with tracing enabled: 
    `CFLAGS="-DABC_TRACE=ON" cmake -DCMAKE_BUILD_TYPE=Debug ..`
1.  Please follow the ABC coding style:
      - no pointer arithmetics, use slices where possible
      - prefer typed slice functions over generic macros (e.g. `u8bReset()` over 
        `Breset()` or `u8sMv()` over `$mv()`),
      - use slice iteration where possible, `$for`, `$rof`, `$eat`
      - stdlib legacy use is discouraged
      - POSIX calls are normally wrapped (e.g. FILE.h, POL.h)
2.  Function naming convention: `MOD typ8 VerbStuff ()`, e.g. `HEXu8sFeed()` 
      - MOD is the module (header)
      - ABC record types have fixed bit layout, their name most often ends
        with the bit width, e.g. `sha256`
      - Verb is the function name per se
      - Stuff may be combinatorial (the Go way), e.g. see `FeedXYZ()` functions
3.  For anything implemented we must ensure it actually works (test, bench, fuzz).
    Each project has `test/`, `fuzz/` and `bench/` subdirs, follow that pattern.
      - tests: preferably Go-style, table driven property tests.
        It helps to fill that table before you start implementing the feature itself.
      - fuzz tests: based on respective property tests. Fuzz fails, once minimized, 
        get fed back into property tests to prevent regressions. Use clang/libfuzzer.
      - benchmarks: test important performance parameters, preferably in a way 
        that makes regressions brutally obvious.
