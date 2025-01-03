#include "RDXY.h"

#include <unistd.h>

#include "JDR.h"
#include "RDXZ.h"
#include "abc/$.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/INT.h"
#include "abc/OK.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

fun b8 is_tilda($u8c data) {
    u8 _tilda[] = {'t', 2, 0, '~'};
    $u8c tilda = $u8raw(_tilda);
    return $eq(data, tilda);
}

pro(RDXY1) {
    sane(1);
    aB(u8, rdxj);
    a$rg(path, 1);
    aBcpad(u8, tlv, PAGESIZE);
    int fd = FILE_CLOSED;
    call(FILEmapro, rdxjbuf, &fd, path);
    aBcpad(u64, stack, 1024);
    aBcpad(u8, pad, PAGESIZE);
    ok64 o = JDRdrain(tlvidle, rdxjcdata);
    if (o != OK) {
        $print(rdxjcdata);
        fail(o);
    }
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
            call(JDRfeed, outidle, resdata);
            $u8feed2(outidle, '!', '=');
            call(JDRfeed, outidle, correct);
            $println(outdata);
            fail(faileq);
        }
    }

    FILEunmap(rdxjbuf);
    done;
}

pro(RDXYtest) {
    sane(1);
    call(RDXY1);
    done;
}

MAIN(RDXYtest);
