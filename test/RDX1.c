
#include "RDX1.h"

#include <unistd.h>

#include "B.h"
#include "FILE.h"
#include "INT.h"
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
        call(RDXFtlv2txt, Bu8idle(txt), Bu8cdata(tlv));
        a$str(str, "text RDX: $s\n");
        FILEfeedf(STDOUT_FILENO, str, Bu8cdata(txt));
        aBpad(u8, tlv2, 32);
        id128 id3 = {i, i};
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
        call(RDXItlv2txt, Bu8idle(txt), Bu8cdata(tlv));
        a$str(str, "text RDX: $s\n");
        FILEfeedf(STDOUT_FILENO, str, Bu8cdata(txt));
        aBpad(u8, tlv2, 32);
        id128 id3 = {i, i};
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
        call(RDXRtlv2txt, Bu8idle(txt), Bu8cdata(tlv));
        a$str(str, "text RDX: $s\n");
        FILEfeedf(STDOUT_FILENO, str, Bu8cdata(txt));
        aBpad(u8, tlv2, 32);
        id128 id3 = {i, i};
        call(RDXRtxt2tlv, Bu8idle(tlv2), Bu8cdata(txt), id3);
        $testeq(Bu8cdata(tlv), Bu8cdata(tlv2));
    }
    done;
}

pro(RDXStest) {
    sane(1);
#define RDXSlen 3
    $u8c inputs[RDXRlen] = {$u8str(""), $u8str("a"), $u8str("abcdef")};
    for (int i = 0; i < RDXRlen; ++i) {
        u8c$ c = inputs[i];
        aRDXid(id, i, i);
        aBpad(u8, tlv, 64);
        call(RDXSc2tlv, Bu8idle(tlv), c, id);
        id128 id2 = {};
        $u8c c2 = {};
        call(RDXStlv2c, c2, &id2, Bu8cdata(tlv));
        want($eq(c, c2));
        same(RDXtime(id), RDXtime(id2));
        same(RDXsrc(id), RDXsrc(id2));
        same($empty(c) ? 3 : $len(c) + 3 + 2, Bdatalen(tlv));
        aBcpad(u8, txt, 32);
        call(RDXStlv2txt, txtidle, Bu8cdata(tlv));
        aBcpad(u8, tlv2, 32);
        call(RDXStxt2tlv, tlv2idle, txtdata, id);
        $testeq(Bu8cdata(tlv), tlv2data);
    }
    done;
}

pro(RDX1test) {
    sane(1);
    call(RDXFtest);
    call(RDXItest);
    call(RDXRtest);
    call(RDXStest);
    done;
}

TEST(RDX1test);
