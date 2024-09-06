
#include "RDX1.h"

#include <unistd.h>

#include "B.h"
#include "FILE.h"
#include "PRO.h"
#include "RDX.h"
#include "TEST.h"
pro(RDXFtest) {
    sane(1);
#define RDXFlen 3
    RDXfloat inputs[RDXFlen] = {0, 12345.6789, 1.2e+20};
    for (int i = 0; i < RDXFlen; ++i) {
        RDXfloat c = inputs[i];
        aRDXid(id, i, i);
        aBpad(u8, tlv, 64);
        call(RDXFc2tlv, Bu8idle(tlv), c, id);
        RDXfloat c2 = 0;
        id128 id2 = {};
        call(RDXFtlv2c, &c2, &id2, Bu8cdata(tlv));
        printf("%lf %lf\n", c, c2);
        same(c, c2);
        same(RDXtime(id), RDXtime(id2));
        same(RDXsrc(id), RDXsrc(id2));
        aBpad(u8, txt, 32);
        id128 id3 = {};
        call(RDXFtlv2txt, Bu8idle(txt), &id3, Bu8cdata(tlv));
        a$str(str, "text RDX: $s\n");
        FILEfeedf(STDOUT_FILENO, str, Bu8cdata(txt));
        aBpad(u8, tlv2, 32);
        call(RDXFtxt2tlv, Bu8idle(tlv2), Bu8cdata(txt), id3);
        $testeq(Bu8cdata(tlv), Bu8cdata(tlv2));
    }
    done;
}

pro(RDXItest) {
    sane(1);
#define RDXFlen 3
    RDXint inputs[RDXFlen] = {0, 12345, INT64_MIN};
    for (int i = 0; i < RDXFlen; ++i) {
        RDXint c = inputs[i];
        aRDXid(id, i, i);
        aBpad(u8, tlv, 64);
        call(RDXIc2tlv, Bu8idle(tlv), c, id);
        RDXint c2 = 0;
        id128 id2 = {};
        call(RDXItlv2c, &c2, &id2, Bu8cdata(tlv));
        printf("%li %li\n", c, c2);
        same(c, c2);
        same(RDXtime(id), RDXtime(id2));
        same(RDXsrc(id), RDXsrc(id2));
        aBpad(u8, txt, 32);
        id128 id3 = {};
        call(RDXItlv2txt, Bu8idle(txt), &id3, Bu8cdata(tlv));
        a$str(str, "text RDX: $s\n");
        FILEfeedf(STDOUT_FILENO, str, Bu8cdata(txt));
        aBpad(u8, tlv2, 32);
        call(RDXItxt2tlv, Bu8idle(tlv2), Bu8cdata(txt), id3);
        $testeq(Bu8cdata(tlv), Bu8cdata(tlv2));
    }
    done;
}

pro(RDXRtest) {
    sane(1);
#define RDXRlen 3
    u64 inputs[RDXRlen][2] = {{0, 0}, {100, 200}, {UINT64_MAX, UINT64_MAX}};
    for (int i = 0; i < RDXRlen; ++i) {
        aRDXid(c, inputs[i][0], inputs[i][1]);
        aRDXid(id, i, i);
        aBpad(u8, tlv, 64);
        call(RDXRc2tlv, Bu8idle(tlv), c, id);
        RDXref c2 = {};
        id128 id2 = {};
        call(RDXRtlv2c, &c2, &id2, Bu8cdata(tlv));
        want(id128cmp(&c, &c2) == 0);
        same(RDXtime(id), RDXtime(id2));
        same(RDXsrc(id), RDXsrc(id2));
        aBpad(u8, txt, 64);
        id128 id3 = {};
        call(RDXRtlv2txt, Bu8idle(txt), &id3, Bu8cdata(tlv));
        a$str(str, "text RDX: $s\n");
        FILEfeedf(STDOUT_FILENO, str, Bu8cdata(txt));
        aBpad(u8, tlv2, 32);
        call(RDXRtxt2tlv, Bu8idle(tlv2), Bu8cdata(txt), id3);
        $testeq(Bu8cdata(tlv), Bu8cdata(tlv2));
    }
    done;
}

pro(RDX1test) {
    sane(1);
    call(RDXFtest);
    call(RDXItest);
    call(RDXRtest);
    done;
}

TEST(RDX1test);
