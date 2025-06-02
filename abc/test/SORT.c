#include "SORT.h"

#include <unistd.h>

#include "PRO.h"
#include "abc/B.h"
#include "abc/TEST.h"

#define LEN1 917

ok64 SORT1() {
    sane(1);
    u64 max = UINT64_MAX;
    u64 min = 0;
    testeq(u64z(&max, &min), z32gt);
    testeq(u64z(&min, &max), z32lt);
    testeq(u64z(&max, &max), z32eq);
    testeq(u64z(&min, &min), z32eq);
    aBpad2(u64, ints, LEN1);
    aBpad2(u64, ints2, LEN1);
    aBpad2(u64, ints3, LEN1);
    u64 r = 57;
    for (u64 i = 0; i < LEN1; ++i) {
        $u64feed1(intsidle, i ^ r);
        $u64feed1(ints3idle, i ^ r);
    }
    $sort(ints3data, u64z);
    call(SORTu64, ints2idle, intsdata);
    testeq(LEN1, $len(ints2data));
    for (u64 i = 0; i < LEN1; ++i) {
        testeq($u64at(ints3data, i), $u64at(ints2data, i));
    }
    done;
}

ok64 SORTtest() {
    sane(1);
    call(SORT1);
    done;
}

TEST(SORTtest);
