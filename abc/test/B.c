#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "01.h"
#include "BUF.h"
#include "INT.h"
#include "PRO.h"
#include "TEST.h"

pro(fail_test) {
    sane(1);
    fail(BADARG);
    done;
}

pro(Bmap_test) {
    sane(1);
    Bu8 buf = {};
    call(u8bMap, buf, 1024);
    Bat(buf, 100) = 100;
    testeq(Bat(buf, 100), 100);
    call(u8bUnMap, buf);
    done;
}

pro(B$_test) {
    sane(1);
    aBpad(u8cp, slices, 8);
    u8cs hello = $u8str("Hello");
    u8cpbFeed2(slices, hello[0], hello[1]);
    done;
}

pro(Bndx_test) {
    sane(YES);
    Bu64 buf = {};
    u64bAllocate(buf, 1024);
    for (u64 i = 0; i < 1000; ++i) {
        call(u64bFeed1, buf, i);
        sane(Blast(buf) == i);
    }
    u64bFree(buf);
    done;
}

pro(Breserve_test) {
    sane(1);
    Bu8 buf = {};
    call(u8bAllocate, buf, 1024);
    for (int i = 0; i < (1 << 20); i++) {
        otry(u8bFeed2, buf, '1', '2');
        ofix(Bnoroom) call(u8bReserve, buf, 1024);
        ocry();
    }
    call(u8bFree, buf);
    done;
}

pro(B$test) {
    sane(1);
    a$$pad(pad, 128, 8);
    $$call(u8sFeedCStr, pad, "one");
    $$call(u64decfeed, pad, 2);
    $$call(u8sFeedCStr, pad, "three");
    a$str(templ, "First $1, then $2, then $3!");
    a$str(correct, "First one, then 2, then three!");
    aBpad2(u8, res, 128);
    $$feedf(residle, templ, pad$data);
    $testeq(correct, resdata);
    done;
}

pro(BBtest) {
    sane(1);
    aBpad(u8b, buff, 4);
    testeq(sizeof(Bat(buff, 0)), sizeof(Bvoid));
    done;
}
pro(Btest) {
    sane(1);
    call(Bmap_test);
    call(B$_test);
    call(Bndx_test);
    call(Breserve_test);
    call(B$test);
    call(BBtest);
    done;
}

TEST(Btest)
