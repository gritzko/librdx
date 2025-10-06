
#include "ZINT.h"

#include <stdint.h>
#include <unistd.h>

#include "S.h"
#include "PRO.h"
#include "TEST.h"

pro(ZINTtest1) {
    sane(1);
    i64 i[] = {
        -126446, 65536, 0, INT64_MAX, INT64_MIN, -1, 1,
    };
    a$(i64, is, i);
    $eat(is) {
        u64 u = ZINTzigzag(**is);
        i64 i2 = ZINTzagzig(u);
        sane(**is == i2);
    }

    done;
}

pro(ZINTtest2) {
    sane(1);
    i64 i[] = {
        0,          0,

        126446,     65536,

        0,          UINT64_MAX,

        UINT64_MAX, 0,

        0xffff,     0xff,

        0xff,       0xffff,

        0xffffff,   0,

        0,          0xffffff,

        0xffffff,   0xffff,

        0xffff,     0xffffff,

        1,          1,
    };
    a$(i64, tozip, i);
    $eat(tozip) {
        u128 a;
        a._64[0] = **tozip;
        ++*tozip;
        a._64[1] = **tozip;
        aBpad(u8, pad, 16);
        u8$ into = Bu8idle(pad);
        u8c$ data =Bu8cdata(pad);
        ZINTu128feed(into, &a);
        u128 b = {};
        ZINTu128drain(&b, data);
        sane(0 == u128cmp(&a, &b));
    }

    done;
}

pro(ZINTtest) {
    sane(1);
    call(ZINTtest1);
    call(ZINTtest2);
    done;
}

TEST(ZINTtest);
