#include "PRO.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "INT.h"
#include "TEST.h"

pro(fail_test) {
    sane(1);
    fail(badarg);
    done;
}

static const ok64 Xbadarg = 0x219a5a25dab;
static const ok64 XYbadarg = 0x8629a5a25dab;
static const ok64 XYZbadarg = 0x218a39a5a25dab;
static const ok64 XYZAbadarg = 0x8628ca9a5a25dab;

pro(PROis) {
    sane(1);
    test(ok64is(XYZAbadarg, badarg), badarg);
    test(ok64is(XYZbadarg, badarg), badarg);
    test(ok64is(XYbadarg, badarg), badarg);
    test(ok64is(Xbadarg, badarg), badarg);
    test(ok64is(badarg, badarg), badarg);
    done;
}

pro(pro_test) {
    sane(1);
    mute(fail_test(), badarg);
    call(PROis);
    done;
}

TEST(pro_test);
