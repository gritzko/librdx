#include "RDXZ.h"

#include <unistd.h>

#include "$.h"
#include "01.h"
#include "B.h"
#include "FILE.h"
#include "INT.h"
#include "OK.h"
#include "PRO.h"
#include "RDX.h"
#include "RDXJ.h"
#include "TEST.h"

pro(RDXZtestvalue) {
    sane(1);
    B(u8, testbuf);
    a$rg(path, 1);
    // a$str(path, "RDXZ.rdx");
    call(FILEmapro, (voidB)testbuf, path);
    $print(Bu8cdata(testbuf));
    aBcpad(u8, tlv, PAGESIZE);
    aBcpad(u64, stack, 1024);
    aBcpad(u8, pad, PAGESIZE);
    ok64 o = RDXJdrain(tlvidle, Bu8cdata(testbuf));
    if (o != OK) {
        $print(Bu8cdata(testbuf));  // state.text);
        fail(o);
    }
    aBcpad($u8c, elem, 64);
    a$dup(u8c, tlv, tlvdata);
    int i = 0, j = 0;
    while (!$empty(tlv)) {
        $u8c prev = {};
        call(TLVdrain$, prev, tlv);
        ++i;
        j = i;
        a$dup(u8c, rest, tlv);
        while (!$empty(rest)) {
            $u8c rec = {};
            call(TLVdrain$, rec, rest);
            ++j;
            int z = RDXZvalue(&prev, &rec);
            if (z >= 0) {
                printf("OPA\n");
            }
            test(z < 0, FAILsanity);
        }
    }

    nedo(FILEunmap((voidB)testbuf));
}

pro(RDXZtest) {
    sane(1);
    call(RDXZtestvalue);
    done;
}

MAIN(RDXZtest);
