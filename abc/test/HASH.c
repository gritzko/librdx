
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
    Bkv32alloc(dictbuf, 1024);
    kv32$ dict = Bkv32idle(dictbuf);

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

    Bkv32free(dictbuf);
    done;
}

pro(HASH) {
    sane(1);
    call(HASH0);
    call(HASH1);
    call(HASH3);
    done;
}

TEST(HASH);
