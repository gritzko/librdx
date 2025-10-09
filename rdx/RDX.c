#include "RDX.h"

#include "abc/TLV.h"

ok64 RDXallFIRST($cu8c rdx) {
    sane($ok(rdx));
    a$dup(u8c, r, rdx);
    while (!$empty(r)) {
        u8cs next;
        call(TLVDrain$, next, r);
        if (!RDXisFIRST(**next)) fail(FAIL);
    }
    done;
}
