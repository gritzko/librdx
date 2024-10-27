#include "RDXY.h"

#include <unistd.h>

#include "$.h"
#include "01.h"
#include "B.h"
#include "FILE.h"
#include "INT.h"
#include "OK.h"
#include "PRO.h"
#include "RDXJ.h"
#include "RDXZ.h"
#include "TEST.h"

fun b8 is_tilda($u8c data) {
    u8 _tilda[] = {'t', 2, 0, '~'};
    $u8c tilda = $u8raw(_tilda);
    return $eq(data, tilda);
}

pro(RDXY1) {
    sane(1);
    aB(u8c, rdxj);
    u8c** path = STD_ARGS[1] + 2;
    // a$str(path, "RDXZ.rdx");
    call(FILEmapro, (voidB)rdxjbuf, path);
    $print(rdxjdata);
    aBcpad(u8, tlv, PAGESIZE);
    aBcpad(u64, stack, 1024);
    aBcpad(u8, pad, PAGESIZE);
    otry(RDXJdrain, tlvidle, rdxjdata);
    oops $print(rdxjdata);
    ocry;
    a$dup(u8c, tlv, tlvdata);
    int i = 0, j = 0;
    while (!$empty(tlv)) {
        $u8c in = {};
        aBpad2($u8c, elem, 64);
        $u8c correct = {};
        aBcpad(u8, res, PAGESIZE);
        call(TLVdrain$, in, tlv);
        do {
            $$u8cfeed1(elemidle, in);
            call(TLVdrain$, in, tlv);
        } while (!is_tilda(in));
        call(TLVdrain$, correct, tlv);

        call(RDXY, residle, elemdata);

        if (!$eq(correct, resdata)) {
            aBcpad(u8, out, PAGESIZE);
            call(RDXJfeed, outidle, resdata);
            $println(outdata);
            fail(faileq);
        }
    }

    nedo(FILEunmap((voidB)rdxjbuf));
}

pro(RDXYtest) {
    sane(1);
    call(RDXY1);
    done;
}

MAIN(RDXYtest);
