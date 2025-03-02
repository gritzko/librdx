#include "RDXY.h"

#include <unistd.h>

#include "JDR.h"
#include "UNIT.h"
#include "abc/$.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/OK.h"
#include "abc/PRO.h"

fun b8 is_tilda($u8c data) {
    u8 _tilda[] = {'t', 2, 0, '~'};
    $u8c tilda = $u8raw(_tilda);
    return $eq(data, tilda);
}

ok64 yfn($cu8c cases) {
    sane($ok(cases));
    a$dup(u8c, tlv, cases);
    while (!$empty(tlv)) {
        $u8c in = {};
        aBpad2($u8c, elem, PAGESIZE);
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
            UNITfail(correct, resdata);
            fail(FAILeq);
        }
    }
    done;
}

pro(RDXY2) {
    aBpad($u8c, zcases, PAGESIZE);
    sane(1);
    a$rg(path, 1);
    Bu8 rdxjbuf = {};
    call(FILEmapro, rdxjbuf, path);
    call(UNITdrain, rdxjbuf, yfn);
    call(FILEunmap, rdxjbuf);
    done;
}

pro(RDXYtest) {
    sane(1);
    call(RDXY2);
    done;
}

MAIN(RDXYtest);
