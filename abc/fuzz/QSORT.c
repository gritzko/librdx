#include "INT.h"
#include "PRO.h"
#include "TEST.h"

#define MAXLEN 4096

FUZZ(u64, QSORTfuzz) {
    sane(1);
    if ($len(input) > MAXLEN) input[1] = input[0] + MAXLEN;
    size_t n = $len(input);
    if (n == 0) done;

    // Copy input twice: one for QSORTx, one for stdlib
    u64 a[MAXLEN], b[MAXLEN];
    memcpy(a, input[0], n * sizeof(u64));
    memcpy(b, input[0], n * sizeof(u64));

    u64s as = {a, a + n};
    u64s bs = {b, b + n};

    // Sort both
    u64sSort(as);
    $sort(bs, u64cmp);

    // Must match
    for (size_t i = 0; i < n; i++)
        must(a[i] == b[i], "sort mismatch");

    // Verify sorted
    for (size_t i = 1; i < n; i++)
        must(a[i - 1] <= a[i], "not sorted");

    // Dedup
    u64sDedup(as);
    size_t ulen = $len(as);

    // Verify dedup: no adjacent duplicates
    for (size_t i = 1; i < ulen; i++)
        must(as[0][i - 1] < as[0][i], "dedup failed");

    // Verify dedup preserves all unique values
    size_t expected = 1;
    for (size_t i = 1; i < n; i++)
        if (b[i] != b[i - 1]) expected++;
    must(ulen == expected, "dedup count mismatch");

    done;
}
