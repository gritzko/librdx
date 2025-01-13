//
// (c) Victor Grishchenko, 2020-2023
//
#include "INT.h"
#include "KV.h"
#include "PRO.h"

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

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size < 2) return 0;
    srandom(*Data);
    Bu32 dictbuf = {};
    Bu32alloc(dictbuf, 16);
    u32$ dict = Bu32data(dictbuf);
    void **b = (void **)dictbuf;
    b[2] = b[3];
    Bu32 copybuf = {};
    Bu32alloc(copybuf, 16);

    u8 *ins = (u8 *)malloc(Size);
    memcpy(ins, Data, Size);

    Play(dict, ins, Size);

    $u32feedall(Bu32idle(copybuf), Bu32cdata(dictbuf));

    for (int j = 0; j < 1000; ++j) {
        Bzero(dictbuf);
        int p = random() % (Size - 1);
        if (ins[p] != (128 ^ ins[p + 1])) u8swap(ins + p, ins + p + 1);

        Play(dict, ins, Size);

        assert($eq(Bu32data(dictbuf), Bu32data(copybuf)));
    }

    Bu32free(dictbuf);
    Bu32free(copybuf);
    free(ins);

    return 0;
}
