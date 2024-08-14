//
// Created by gritzko on 5/11/24.
//
#include "01.h"

#include <assert.h>
#include <stdio.h>

#include "OK.h"
#include "PRO.h"
#include "TEST.h"

pro(BITStest1) {
    u64 a = 0xaabbccdd11223344;
    u64 b = flip64(a);
    sane(b == 0x44332211ddccbbaa);
    u32 c = 0xaabbccdd;
    sane(flip32(c) == 0xddccbbaa);

    u64 t7 = 128;
    sane(ctz64(t7) == 7);
    sane(clz64(t7) == 56);
    done;
}

pro(BITSbytelen) {
    sane(YES);
    testeq(2, u32bytelen(0x111));
    testeq(2, u32bytelen(0x1111));
    testeq(2, u64bytelen(0x111));
    testeq(2, u64bytelen(0x1111));
    testeq(5, u64bytelen(0x1122334455));
    testeq(6, u64bytelen(0x12233445566));
    testeq(0, u64bytelen(0x0));
    done;
}

pro(BITSstr) {
    sane(YES);
    const char* badarg = okstr($badarg);
    testeq(0, strcmp(badarg, "~badarg"));
    const char* ok = okstr(OK);
    testeq(0, strcmp(badarg, "OK"));
    done;
}

pro(BITStest) {
    call(BITStest1);
    call(BITSbytelen);
    call(BITSstr);
    done;
}

TEST(BITStest);
