#include "BIN.h"
#include "PRO.h"
#include "TEST.h"

pro(BINtest1) {
    sane(bin64contains(bin64of(2, 0), bin64of(0, 2)));
    sane(bin64daughter(11) == 9);
    sane(bin64son(5) == 6);
    sane(bin64size(7) == 8);
    sane(bin64level(11) == 2);
    done;
}

pro(BINtest) {
    call(BINtest1);
    done;
}

TEST(BINtest);
