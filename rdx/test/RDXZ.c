#include "RDXZ.h"

#include <unistd.h>

#include "../UNIT.h"
#include "JDR.h"
#include "RDX.h"
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
    aBcpad(u8cs, elem, 64);
    a$dup(u8c, tlv, tlvdata);
    int i = 0, j = 0;
    while (!$empty(tlv)) {
        u8cs prev = {};
        call(TLVDrain$, prev, tlv);
        ++i;
        j = i;
        a$dup(u8c, rest, tlv);
        while (!$empty(rest)) {
            u8cs rec = {};
            call(TLVDrain$, rec, rest);
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

static u8cssp zcasesdata;
static u8cssp zcasesidle;

con ok64 FAILz = 0x3e55228f;

ok64 zfn($cu8c cases) {
    sane($ok(cases));
    a$dup(u8c, c, cases);
    ok64 o = OK;
    while (!$empty(c) && o == OK) {
        u8cs rec;
        o = TLVDrain$(rec, c);
        if (o != OK) break;
        for (u8cs* p = $head(zcasesdata); p < $term(zcasesdata); ++p) {
            int self = RDXZvalue(p, p);
            if (self != 0) {
                UNITfail(*p, *p);
                o = FAILz;
            }
            int z = RDXZvalue(p, &rec);
            if (z >= 0) {
                UNITfail(*p, rec);
                o = FAILz;
            }
        }
        u8cssFeedP(zcasesidle, &rec);
    }
    return o;
}

pro(RDXZtest) {
    aBpad(u8cs, zcases, 256);
    sane(1);
    a$rg(path, 1);
    Bu8 rdxjbuf = {};
    call(FILEmapro, rdxjbuf, path);
    call(UNITdrain, rdxjbuf, zfn);
    call(FILEunmap, rdxjbuf);
    done;
}

MAIN(RDXZtest);
