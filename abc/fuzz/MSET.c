#include "INT.h"
#include "PRO.h"
#include "TEST.h"

#define X(M, name) M##u64##name
#include "MSETx.h"
#undef X

#define LEN 512
#define MAXRUNS 32
#define BUFLEN (LEN * 8)

FUZZ(u64, MSETfuzz) {
    sane(1);
    if ($len(input) > LEN) input[1] = input[0] + LEN;
    size_t n = $len(input);
    if (n < 1) done;

    // reference copy for verification
    aBpad2(u64, ref, LEN);
    $u64feedall(refidle, input);

    // working copy for sorted runs
    aBpad2(u64, d, LEN);
    $u64feedall(didle, input);

    // compaction output buffer
    u64 cbuf[BUFLEN];
    u64s cinto = {cbuf, cbuf + BUFLEN};

    // chunk size from first element
    u64 first = *ddata[0];
    size_t chunk = (first % 16) + 1;
    if (chunk > n) chunk = n;

    // build stack feeding sorted runs, compact as needed
    u64cs runs[MAXRUNS];
    size_t nruns = 0;
    u64 *base = *ddata;
    size_t pos = 0;

    while (pos < n) {
        size_t end = pos + chunk;
        if (end > n) end = n;
        u64s run = {base + pos, base + end};
        MSETu64Sort(run);
        runs[nruns][0] = base + pos;
        runs[nruns][1] = base + end;
        nruns++;

        u64css stack = {runs, runs + nruns};
        while (!MSETu64IsCompact(stack)) {
            call(MSETu64Compact, stack, cinto);
            nruns = stack[1] - stack[0];
        }
        pos = end;
    }

    // every input element must be findable via Get
    u64css stack = {runs, runs + nruns};
    for (size_t i = 0; i < n; i++) {
        u64 key = u64sAt(refdata, i);
        ok64 o = MSETu64Get(stack, key);
        must(o == OK, "element not found");
    }
    done;
}
