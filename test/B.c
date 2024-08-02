#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "PRO.h"
#include "TEST.h"
#include "INT.h"

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

pro(Btest) {
    call(Bmap_test);
    call(B$_test);
    done;
}

TEST(Btest)
