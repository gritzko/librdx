#include "RDXZ.h"

#include <unistd.h>

#include "RDX.h"
#include "RDXJ.h"
#include "abc/$.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/INT.h"
#include "abc/OK.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

pro(RDXZtestvalue, Bu8 testbuf) {
    sane(1);
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
                printf("OPA %i!=%i\n", i, j);
            }
            test(z < 0, FAILsanity);
        }
    }
    done;
}

pro(RDXZtest) {
    sane(1);
    B(u8, testbuf);
    a$rg(path, 1);
    // a$str(path, "RDXZ.rdx");
    int fd = FILE_CLOSED;
    call(FILEmapro, testbuf, &fd, path);
    $print(Bu8cdata(testbuf));
    ok64 o = RDXZtestvalue(testbuf);
    FILEunmap(testbuf);
    FILEclose(&fd);
    return o;
}

MAIN(RDXZtest);
