#include "ZINT.h"

#include "TEST.h"

fuzz(u64, ZINTfuzz) {
    sane(1);
    if ($len(input) & 1) --input[1];
    a_dup(u64c, tozip, input);
    $eat(tozip) {
        u128 a;
        a._64[0] = **tozip;
        ++*tozip;
        a._64[1] = **tozip;
        aBpad(u8, pad, 16);
        u8sp into = u8bIdle(pad);
        u8csp data = u8bDataC(pad);
        ZINTu128feed(into, &a);
        u128 b = {};
        ZINTu128drain(&b, data);
        sane(0 == u128cmp(&a, &b));
    }
    done;
}
