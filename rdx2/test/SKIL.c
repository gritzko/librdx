#include "abc/01.h"
#include "abc/TEST.h"
#include "rdx2/RDX.h"

ok64 SKILTest1() {
    con int length = 100;
    sane(1);
    a_pad(u8, pad, PAGESIZE);
    a_pad(u64, tabs, PAGESIZE);
    rdx e = {.format = RDX_FMT_SKIL | RDX_FMT_WRITE, .extra = (void*)tabs};
    $mv(e.into, pad_idle);
    e.type = RDX_TYPE_EULER;
    call(rdxNext, &e);
    rdx i = {};
    call(rdxInto, &i, &e);
    for (int j = 0; j < length; j++) {
        i.type = RDX_TYPE_INT;
        i.i = j;
        i.id.seq = j;
        i.id.src = 0;
        call(rdxNext, &i);
    }
    call(rdxOuto, &i, &e);
    $mv(pad_idle, e.into);

    rdx e2 = {.format = RDX_FMT_SKIL};
    $mv(e2.data, pad_data);
    call(rdxNext, &e2);
    test(e2.type == RDX_TYPE_EULER, RDXBAD);
    for (int j = 0; j < length; j++) {
        rdx i2 = {};
        i2.type = RDX_TYPE_INT;
        i2.i = j;
        call(rdxInto, &i2, &e2);
        test(i2.id.seq == j, NOEQ);
        test(i2.id.src == 0, NOEQ);
        call(rdxOuto, &i2, &e2);
    }

    done;
}

ok64 SKILTests() {
    sane(1);
    call(SKILTest1);
    done;
}

TEST(SKILTests);
