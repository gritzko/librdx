
#include "HASH.h"

#include <stdint.h>
#include <unistd.h>

#include "01.h"
#include "INT.h"
#include "OK.h"
#include "TEST.h"

fun u64 u32hash(u32 const *v) { return mix32(*v); }

#define X(M, name) M##u32##name
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

pro(HASH) {
    sane(1);
    call(HASH0);
    call(HASH1);
    done;
}

TEST(HASH);
