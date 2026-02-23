#include "BIN.h"

#include "PRO.h"
#include "TEST.h"

ok64 BINtest1() {
    sane(1);
    testeq(YES, bin64contains(bin64of(2, 0), bin64of(0, 2)));
    testeq(bin64daughter(11), 9);
    testeq(bin64son(5), 6);
    testeq(bin64size(7), 8);
    testeq(bin64level(11), 2);
    done;
}

ok64 BINtest() {
    sane(1);
    call(BINtest1);
    done;
}

TEST(BINtest);
