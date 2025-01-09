#include "RDXZ.h"

#include <unistd.h>

#include "JDR.h"
#include "RDX.h"
#include "UNIT.h"
#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/OK.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

pro(RDXZtestvalue, Bu8 testbuf) {
    sane(1);
    aBcpad(u8, tlv, PAGESIZE);
    aBcpad(u64, stack, 1024);
    aBcpad(u8, pad, PAGESIZE);
    ok64 o = JDRdrain(tlvidle, Bu8cdata(testbuf));
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

static $u8c$ zcasesdata;
static $u8c$ zcasesidle;

con ok64 FAILz = 0x3e55228f;

ok64 zfn($cu8c cases) {
    a$dup(u8c, c, cases);
    ok64 o = OK;
    while (!$empty(c) && o == OK) {
        $u8c rec;
        o = TLVdrain$(rec, c);
        if (o != OK) break;
        for ($u8cc* p = $head(zcasesdata); p < $term(zcasesdata); ++p) {
            int z = RDXZvalue(p, &rec);
            if (z >= 0) {
                UNITfail(*p, rec);
                o = FAILz;
            }
        }
        $$u8cfeedp(zcasesidle, &rec);
    }
    return o;
}

pro(RDXZtest) {
    aBpad($u8c, zcases, 256);
    zcasesidle = B$u8c$2(zcases);
    zcasesdata = B$u8c$1(zcases);
    sane(1);
    a$rg(path, 1);
    Bu8 rdxjbuf = {};
    int fd = FILE_CLOSED;
    call(FILEmapro, rdxjbuf, &fd, path);
    call(UNITdrain, rdxjbuf, zfn);
    call(FILEunmap, rdxjbuf);
    call(FILEclose, &fd);
    done;
}

MAIN(RDXZtest);
