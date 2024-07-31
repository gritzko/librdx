#include "PRO.h"
#include "TEST.h"
#include "INT.h"

#define X(M, name) M##u32##name
#include "HEAPx.h"
#undef X

pro(HEAPtest1) {
    aBpad(u32, pad, 32);
    u32$ heap = Bu32data(pad);
    // Pushes one entry into the heap buffer
    call(HEAPu32push1, pad, 3, u32cmp);
    call(HEAPu32push1, pad, 2, u32cmp);
    call(HEAPu32push1, pad, 1, u32cmp);
    u32 one, two, three;
    // Retrieves the least entry.
    // May also use **pad to read one.
    call(HEAPu32pop, &one, pad, u32cmp);
    call(HEAPu32pop, &two, pad, u32cmp);
    call(HEAPu32pop, &three, pad, u32cmp);
    sane(one == 1);
    sane(two == 2);
    sane(three == 3);
    done;
}
pro(HEAPtest) {
    call(HEAPtest1);
    done;
}

TEST(HEAPtest)
