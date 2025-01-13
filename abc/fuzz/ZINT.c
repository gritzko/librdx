#include "ZINT.h"

#include "TEST.h"

fuzz(u64, ZINTfuzz) {
    sane(1);
    if ($len(input) & 1) --input[1];
    a$dup(u64c, tozip, input);
    $eat(tozip) {
        u128 a;
        a._64[0] = **tozip;
        ++*tozip;
        a._64[1] = **tozip;
        aBpad(u8, pad, 16);
        u8$ into = Bu8idle(pad);
        u8c$ data = Bu8cdata(pad);
        ZINTu128feed(into, a);
        u128 b = {};
        ZINTu128drain(&b, data);
        sane(0 == u128cmp(&a, &b));
    }
    done;
}
