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
    call(Bu32feed$, sorted, input);
    $sort(Bu32data(sorted), &u32cmp);

    aBpad(u32, heap, 1024);
    aBpad(u32, heaped, 1024);
    $for(u32c, p, input) call(HEAPu32push1, heap, *p);
    u32 **from = Bu32data(heap);
    while (!$empty(from)) {
        u32 v = 0;
        call(HEAPu32pop, &v, heap);
        call(Bu32feed1, heaped, v);
    }

    assert(0 == $cmp(Bdata(heaped), Bdata(sorted)));
    done;
}
