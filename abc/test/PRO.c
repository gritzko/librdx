#include "PRO.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "INT.h"
#include "TEST.h"

pro(fail_test) {
    sane(1);
    fail(BADARG);
    done;
}

con ok64 BBADARG = 0xb2ca34a6d0;
con ok64 ABBADARG = 0x28b2ca34a6d0;
con ok64 ABCBADARG = 0xa2cc2ca34a6d0;
con ok64 ABCDBADARG = 0x28b30d2ca34a6d0;

pro(PROis) {
    sane(1);
    test(ok64is(ABCDBADARG, BADARG), BADARG);
    test(ok64is(ABCBADARG, BADARG), BADARG);
    test(ok64is(ABBADARG, BADARG), BADARG);
    test(ok64is(BBADARG, BADARG), BADARG);
    test(ok64is(BADARG, BADARG), BADARG);
    done;
}

pro(pro_test) {
    sane(1);
    mute(fail_test(), BADARG);
    call(PROis);
    done;
}

TEST(pro_test);
