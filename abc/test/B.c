#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "01.h"
#include "BUF.h"
#include "INT.h"
#include "PRO.h"
#include "TEST.h"

ok64 fail_test() {
    sane(1);
    fail(badarg);
    done;
}

ok64 Bmap_test() {
    sane(1);
    Bu8 buf = {};
    call(Bu8map, buf, 1024);
    Bat(buf, 100) = 100;
    testeq(Bat(buf, 100), 100);
    call(Bu8unmap, buf);
    done;
}

ok64 B$_test() {
    sane(1);
    aBpad(u8cp, slices, 8);
    $u8c hello = $u8str("Hello");
    Bu8cpfeed2(slices, hello[0], hello[1]);
    done;
}

ok64 Bndx_test() {
    sane(YES);
    Bu64 buf = {};
    Bu64alloc(buf, 1024);
    for (u64 i = 0; i < 1000; ++i) {
        call(Bu64feed1, buf, i);
        sane(Blast(buf) == i);
    }
    Bu64free(buf);
    done;
}

ok64 Breserve_test() {
    sane(1);
    Bu8 buf = {};
    call(Bu8alloc, buf, 1024);
    for (int i = 0; i < (1 << 20); i++) {
        otry(Bu8feed2, buf, '1', '2');
        ofix(Bnoroom) call(Bu8reserve, buf, 1024);
        ocry();
    }
    call(Bu8free, buf);
    done;
}

ok64 B$test() {
    sane(1);
    a$$pad(pad, 128, 8);
    $$call($u8feedstr, pad, "one");
    $$call(u64decfeed, pad, 2);
    $$call($u8feedstr, pad, "three");
    a$str(templ, "First $1, then $2, then $3!");
    a$str(correct, "First one, then 2, then three!");
    aBpad2(u8, res, 128);
    $$feedf(residle, templ, pad$data);
    $testeq(correct, resdata);
    done;
}

ok64 BBtest() {
    sane(1);
    aBpad(Bu8, buff, 4);
    testeq(sizeof(Bat(buff, 0)), sizeof(Bvoid));
    done;
}
ok64 Btest() {
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
