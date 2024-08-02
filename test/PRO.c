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

con ok64 Xbadarg = 0x2bda5a259a1;
con ok64 XYbadarg = 0xaf69689668a1;
con ok64 XYZbadarg = 0x2bda5a259a38a1;
con ok64 XYZAbadarg = 0xaf69689662a38a1;

pro(PROis) {
    test(ok64is(XYZAbadarg, badarg), badarg);
    test(ok64is(XYZbadarg, badarg), badarg);
    test(ok64is(XYbadarg, badarg), badarg);
    test(ok64is(Xbadarg, badarg), badarg);
    test(ok64is(badarg, badarg), badarg);
    done;
}

pro(pro_test) {
    mute(fail_test(), badarg);
    call(PROis);
    done;
}

TEST(pro_test);
