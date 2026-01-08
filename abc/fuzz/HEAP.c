#include <stdlib.h>

#include "INT.h"
#include "PRO.h"
#include "TEST.h"

#define X(M, name) M##u32##name
#include "HEAPx.h"
#undef X

fuzz(u32, HEAPfuzz) {
    sane(1);
    if ($len(input) > 1024) input[1] = input[0] + 1024;

    aBpad(u32, sorted, 1024);
    call(u32bFeed, sorted, input);
    $sort(u32bDataC(sorted), &u32cmp);

    aBpad(u32, heap, 1024);
    aBpad(u32, heaped, 1024);
    $for(u32c, p, input) call(HEAPu32Push1, heap, *p);
    u32 **from = u32bData(heap);
    while (!$empty(from)) {
        u32 v = 0;
        call(HEAPu32Pop, &v, heap);
        call(u32bFeed1, heaped, v);
    }

    assert(0 == $cmp(Bdata(heaped), Bdata(sorted)));
    done;
}
