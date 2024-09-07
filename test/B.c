#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "INT.h"
#include "PRO.h"
#include "TEST.h"

pro(fail_test) {
    sane(1);
    fail(badarg);
    done;
}

pro(Bmap_test) {
    sane(1);
    done;
}

pro(B$_test) {
    sane(1);
    aBpad(u8cp, slices, 8);
    $u8c hello = $u8str("Hello");
    Bu8cpfeed2(slices, hello[0], hello[1]);
    done;
}

pro(Bndx_test) {
    sane(YES);
    Bu64 buf = {};
    Bu64alloc(buf, 1024);
    for (u64 i = 0; i < 1000; ++i) {
        call(Bu64feed1, buf, i);
        sane(Blast(buf) == i);
    }
    nedo(Bu64free(buf));
}

pro(Breserve_test) {
    sane(1);
    Bu8 buf = {};
    call(Bu8alloc, buf, 1024);
    for (int i = 0; i < (1 << 20); i++) {
        try(Bu8feed2, buf, '1', '2');
        on(noroom) call(Bu8reserve, buf, 1024);
        sure(OK);
    }
    call(Bu8free, buf);
    done;
}

pro(B$test) {
    sane(1);
    /*    aBpad(u8, pad, 1024);
        $u8feed2(Bu8idle(pad), 12, 34);
        aBpad($u8, $pad, 32);
        $feed(B$u8idle($pad), pad[1]);*/
    done;
}

pro(Btest) {
    sane(1);
    call(Bmap_test);
    call(B$_test);
    call(Bndx_test);
    call(Breserve_test);
    call(B$test);
    done;
}

TEST(Btest)
