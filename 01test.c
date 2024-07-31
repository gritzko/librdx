//
// Created by gritzko on 5/11/24.
//
#include <assert.h>
#include <stdio.h>

#include "01.h"
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

pro(BITStest) {
    call(BITStest1);
    done;
}

TEST(BITStest);
