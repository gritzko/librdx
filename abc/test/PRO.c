#include "PRO.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "INT.h"
#include "TEST.h"

pro(fail_test) {
    sane(1);
    fail(BADarg);
    done;
}

con ok64 Xbadarg = 0x219a5a25dab;
con ok64 XYbadarg = 0x8629a5a25dab;
con ok64 XYZbadarg = 0x218a39a5a25dab;
con ok64 XYZAbadarg = 0x8628ca9a5a25dab;

pro(PROis) {
    sane(1);
    test(ok64is(XYZAbadarg, BADarg), BADarg);
    test(ok64is(XYZbadarg, BADarg), BADarg);
    test(ok64is(XYbadarg, BADarg), BADarg);
    test(ok64is(Xbadarg, BADarg), BADarg);
    test(ok64is(BADarg, BADarg), BADarg);
    done;
}

pro(pro_test) {
    sane(1);
    mute(fail_test(), BADarg);
    call(PROis);
    done;
}

TEST(pro_test);
