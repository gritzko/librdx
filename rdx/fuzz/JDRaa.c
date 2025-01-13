#include <stdlib.h>
#include <unistd.h>

#include "../test/UNIT.h"
#include "RDX.h"
#include "abc/FILE.h"
#include "abc/TLV.h"
#include "rdx/JDR.h"

ok64 RDXvalid($cu8c data) {
    sane($ok(data));
    a$dup(u8c, d, data);
    while (!$empty(d)) {
        u8 t;
        $u8c key, val;
        call(TLVdrainkv, &t, key, val, d);
        test($len(key) <= 16, JDRbad + 1);
        if (RDXisPLEX(t)) {
            call(RDXvalid, val);
        } else if (RDXisFIRST(t)) {
            switch (t) {
                case RDX_FLOAT:
                    test($len(val) <= 8, JDRbad + 2);
                    break;

                case RDX_INT:
                    test($len(val) <= 8, JDRbad + 3);
                    break;

                case RDX_REF:
                    test($len(val) <= 16, JDRbad + 4);
                    break;

                case RDX_STRING:
                    // todo UTF8
                    break;

                case RDX_TERM:
                    // TODO base64
                    break;
            }
        } else {
            fail(JDRbad + 5);
        }
    }
    done;
}

uint8_t _pro_depth = 0;
int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    $u8c input = {(u8c *)Data, (u8c *)Data + Size};
    aBcpad(u8, rdx, PAGESIZE);

    ok64 o = JDRdrain(rdxidle, input);
    if (o != OK) return 0;

    o = RDXvalid(rdxdata);

    if (OK != o) {
        aBcpad(u8, hex, 1 << 16);
        a$str(msg, "invalid RDX: ");
        ok64feed(hexidle, o);
        $u8feed2(hexidle, '\n', '\n');
        HEXdump(hexidle, rdxdata);
        FILEfeed(STDERR_FILENO, hexdata);
        abort();
    }

    return 0;
}
