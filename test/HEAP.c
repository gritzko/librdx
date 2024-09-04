#include "INT.h"
#include "PRO.h"
#include "TEST.h"

// Instantiate the HEAP template for u32
#define X(M, name) M##u32##name
#include "HEAPx.h"
#undef X

pro(HEAPtest1) {
    sane(1);
    // Make a buffer on the stack
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
    testeq(one, 1);
    testeq(two, 2);
    testeq(three, 3);
    done;
}
pro(HEAPtest) {
    sane(1);
    call(HEAPtest1);
    done;
}

TEST(HEAPtest)
