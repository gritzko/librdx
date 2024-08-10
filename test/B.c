#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "INT.h"
#include "PRO.h"
#include "TEST.h"

pro(fail_test) {
    fail(badarg);
    done;
}

pro(Bmap_test) { done; }

pro(B$_test) {
    aBpad(u8cp, slices, 8);
    $u8c hello = $u8str("Hello");
    Bu8cpfeed2(slices, hello[0], hello[1]);
    done;
}

pro(Breserve_test) {
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

pro(Btest) {
    call(Bmap_test);
    call(B$_test);
    call(Breserve_test);
    done;
}

TEST(Btest)
