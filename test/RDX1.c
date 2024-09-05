
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

pro(RDX1test) {
    sane(1);
    call(RDXFtest);
    done;
}

TEST(RDX1test);
