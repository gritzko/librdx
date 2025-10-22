
#include "HASH.h"

#include <stdint.h>
#include <unistd.h>

#include "01.h"
#include "INT.h"
#include "KV.h"
#include "OK.h"
#include "TEST.h"

fun u64 u32hash(u32 const *v) { return mix32(*v); }

#define X(M, name) M##u32##name
#define ABC_HASH_CONVERGE 1
#include "HASHx.h"
#undef X

#define X(M, name) M##kv32##name
#include "HASHx.h"
#undef X

pro(HASH0) {
    sane(1);
    aBcpad(u32, pad, 1024);
    Bzero(padbuf);
    u32 one = 1;
    call(HASHu32put, padidle, &one);
    call(HASHu32get, &one, padidle);
    call(HASHu32del, padidle, &one);
    want(HASHnone == HASHu32get(&one, padidle));
    done;
}

pro(HASH1) {
    sane(1);
    aBcpad(u32, pad, 1024 + 128);
    Bzero(padbuf);
    for (u32 i = 1; i < 1000; i += 2) {
        call(HASHu32put, padidle, &i);
    }
    for (u32 i = 2; i < 1000; i += 2) {
        u32 n = i;
        ok64 o = HASHu32get(&n, padidle);
        want(HASHnone == o);
    }
    for (u32 i = 1; i < 1000; i += 2) {
        u32 v = i;
        ok64 o = HASHu32get(&v, padidle);
        want(o == OK);
        want(v == i);
    }
    for (u32 i = 1; i < 1000; i += 4) {
        call(HASHu32del, padidle, &i);
    }
    for (u32 i = 1; i < 1000; i += 4) {
        u32 n = i;
        want(HASHnone == HASHu32get(&n, padidle));
    }
    for (u32 i = 3; i < 1000; i += 4) {
        u32 v = i;
        call(HASHu32get, &v, padidle);
        want(v == i);
    }
    done;
}

pro(HASH3) {
    sane(1);
    Bkv32 dictbuf = {};
    kv32bAllocate(dictbuf, 1024);
    kv32$ dict = kv32bIdle(dictbuf);

    kv32 a = {.key = 6220, .val = 2};
    call(HASHkv32put, dict, &a);
    kv32 b = {.key = 22, .val = 3};
    call(HASHkv32put, dict, &b);

    kv32 a2 = {.key = 6220};
    kv32 b2 = {.key = 22};
    call(HASHkv32get, &a2, dict);
    call(HASHkv32get, &b2, dict);
    testeq(a2.val, 2);
    testeq(b2.val, 3);

    call(HASHkv32del, dict, &b);

    a.val = 0;
    call(HASHkv32get, &a, dict);
    testeq(a.val, 2);
    mute(HASHkv32get(&b, dict), HASHnone);

    kv32bFree(dictbuf);
    done;
}

#define LENd 24

pro(HASHd) {
    sane(1);
    Bu32 dictbuf = {};
    u32bAllocate(dictbuf, 16);
    u32$ dict = u32bData(dictbuf);
    void **b = (void **)dictbuf;
    b[2] = b[3];
    Bu32 copybuf = {};
    u32bAllocate(copybuf, 16);

    i32 ins[LENd] = {1,  2,  3,  4,   -4, 5,   -5, 5,   6,  7,  8,  9,
                     10, 11, 12, -12, 12, -12, 13, -13, 14, 15, 15, 15};

    for (int i = 0; i < LENd; ++i) {
        if (ins[i] > 0) {
            u32 rec = ins[i];
            HASHu32put(dict, &rec);
        } else {
            u32 rec = -ins[i];
            HASHu32del(dict, &rec);
        }
    }
    u32sFeed(u32bIdle(copybuf), u32cbData(dictbuf));

    for (int j = 0; j < 1000; ++j) {
        Bzero(dictbuf);
        int p = rand() % (LENd - 1);
        if (ins[p] != -ins[p + 1]) i32Swap(ins + p, ins + p + 1);
        for (int i = 0; i < LENd; ++i) {
            if (ins[i] > 0) {
                u32 rec = ins[i];
                HASHu32put(dict, &rec);
            } else {
                u32 rec = -ins[i];
                HASHu32del(dict, &rec);
            }
        }
        if (!$eq(u32bData(dictbuf), u32bData(copybuf))) {
            fprintf(stderr, "INS\t");
            for (int i = 0; i < LENd; ++i) fprintf(stderr, "%i\t", ins[i]);
            fprintf(stderr, "\n");
            fprintf(stderr, "COPY\t");
            for (int i = 0; i < 16; ++i)
                fprintf(stderr, "%i\t", Bat(copybuf, i));
            fprintf(stderr, "\n");
            fprintf(stderr, "DICT\t");
            for (int i = 0; i < 16; ++i)
                fprintf(stderr, "%i\t", Bat(dictbuf, i));
            fprintf(stderr, "\n");
            fprintf(stderr, "FIT\n");
            for (u32 u = 0; u < 16; ++u)
                fprintf(stderr, "%u\t%lu\n", u, u32hash(&u) % 16);

            Bzero(dictbuf);
            for (int i = 0; i < LENd; ++i) {
                if (ins[i] > 0) {
                    u32 rec = ins[i];
                    HASHu32put(dict, &rec);
                } else {
                    u32 rec = -ins[i];
                    HASHu32del(dict, &rec);
                }
                fprintf(stderr, "#%i\t", i);
                for (int i = 0; i < 16; ++i)
                    fprintf(stderr, "%i\t", Bat(dictbuf, i));
                fprintf(stderr, "\n");
            }
        }
        $testeq(u32bData(dictbuf), u32bData(copybuf));
    }

    u32bFree(dictbuf);
    u32bFree(copybuf);
    done;
}

pro(HASH) {
    sane(1);
    call(HASH0);
    call(HASH1);
    call(HASH3);
    call(HASHd);
    done;
}

TEST(HASH);
