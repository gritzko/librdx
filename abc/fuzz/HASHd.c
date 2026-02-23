//
// (c) Victor Grishchenko, 2020-2023
//
#include "INT.h"
#include "KV.h"
#include "PRO.h"
#include "TEST.h"

fun u64 u32hash(u32 const *v) { return mix32(*v); }

#define X(M, name) M##u32##name
#define ABC_HASH_CONVERGE 1
#include "HASHx.h"
#undef X

fun void Play($u32 dict, u8 *ins, size_t Size) {
    for (int i = 0; i < Size; ++i) {
        if (ins[i] == 0 || ins[i] == 128) break;
        if (ins[i] < 128) {
            u32 rec = ins[i];
            HASHu32put(dict, &rec);
        } else {
            u32 rec = ins[i] - 128;
            HASHu32del(dict, &rec);
        }
    }
}

FUZZ(u8, HASHdfuzz) {
    sane(1);
    int Size = $size(input);
    if (Size < 2) return 0;
    srandom(**input);
    Bu32 dictbuf = {};
    u32bAlloc(dictbuf, 16);
    u32$ dict = u32bData(dictbuf);
    void **b = (void **)dictbuf;
    b[2] = b[3];
    Bu32 copybuf = {};
    u32bAlloc(copybuf, 16);

    u8 *ins = (u8 *)malloc(Size);
    u8 *orig = (u8 *)malloc(Size);
    memcpy(ins, *input, Size);
    memcpy(orig, *input, Size);

    Play(dict, ins, Size);

    $u32feedall(u32bIdle(copybuf), u32bDataC(dictbuf));

    for (int j = 0; j < 1000; ++j) {
        u32bZero(dictbuf);
        // Reset ins from original each iteration to avoid cumulative swaps
        memcpy(ins, orig, Size);
        int p = random() % (Size - 1);
        // Skip swaps involving break markers (0 or 128)
        if (ins[p] == 0 || ins[p] == 128 || ins[p + 1] == 0 || ins[p + 1] == 128)
            continue;
        // Skip if swapping insert-X with delete-X (would change semantics)
        if (ins[p] == (128 ^ ins[p + 1])) continue;
        u8Swap(ins + p, ins + p + 1);

        Play(dict, ins, Size);

        must($eq(u32bData(dictbuf), u32bData(copybuf)), "dict mismatch");
    }
    free(orig);

    u32bFree(dictbuf);
    u32bFree(copybuf);
    free(ins);

    return 0;
}
