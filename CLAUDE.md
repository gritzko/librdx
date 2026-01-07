#   Coding guidelines

0.  Dependencies: libsodium, gtest, google-benchmark.
1.  Please follow the ABC coding style:
      - no pointer arithmetics, use slices where possible
      - prefer typed slice functions over generic (e.g. u8bReset() vs Breset())
      - use slice iteration where possible, $for and $rof
      - stdlib legacy use is discouraged
      - POSIX is normally wrapped (e.g. FILE.h, POL.h)
2.  Function naming convention: `MOD typ8 VerbStuff ()`, e.g. `HEXu8sFeed()` 
      - MOD is the module (header)
      - ABC record types have fixed bit layout, their name most often ends
        with the bit width, e.g. sha256
      - Verb is the function name per se
      - Stuff may be combinatorial (the Go way), e.g. see the Feed() functions
3.  For anything implemented we must ensure it actually works (test, bench, fuzz)
