#include <unistd.h>

#include "JDR.h"
#include "RDX.h"
#include "RDXC.h"
#include "Y.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/INT.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

pro(RDXFtest) {
    sane(1);
#define RDXFlen 3
    RDXfloat inputs[RDXFlen] = {0, 12345.6789, 1.2e+20};
    for (int i = 0; i < RDXFlen; ++i) {
        RDXfloat c = inputs[i];
        aRDXid(id, i, i);
        aBpad(u8, tlv, 64);
        call(RDXCfeedF, u8bIdle(tlv), c, id);
        RDXfloat c2 = 0;
        id128 id2 = {};
        call(RDXCdrainF, &c2, &id2, u8cbData(tlv));
        printf("%lf %lf\n", c, c2);
        same(c, c2);
        same(id128time(id), id128time(id2));
        same(id128src(id), id128src(id2));
        aBpad(u8, txt, 32);
        call(RDXFtlv2txt, u8bIdle(txt), u8cbData(tlv));
        a$str(str, "text RDX: $s\n");
        FILEFeedf(STDOUT_FILENO, str, u8cbData(txt));
        aBpad(u8, tlv2, 32);
        id128 id3 = {i, i};
        call(RDXFtxt2tlv, u8bIdle(tlv2), u8cbData(txt), id3);
        $testeq(u8cbData(tlv), Bu8cdata(tlv2));
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
        call(RDXCfeedI, u8bIdle(tlv), c, id);
        RDXint c2 = 0;
        id128 id2 = {};
        call(RDXCdrainI, &c2, &id2, u8cbData(tlv));
        printf("%li %li\n", c, c2);
        same(c, c2);
        same(id128time(id), id128time(id2));
        same(id128src(id), id128src(id2));
        aBpad(u8, txt, 32);
        call(RDXItlv2txt, u8bIdle(txt), u8cbData(tlv));
        a$str(str, "text RDX: $s\n");
        FILEFeedf(STDOUT_FILENO, str, u8cbData(txt));
        aBpad(u8, tlv2, 32);
        id128 id3 = {i, i};
        call(RDXItxt2tlv, u8bIdle(tlv2), u8cbData(txt), id3);
        $testeq(u8cbData(tlv), Bu8cdata(tlv2));
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
        call(RDXCfeedR, u8bIdle(tlv), c, id);
        RDXref c2 = {};
        id128 id2 = {};
        call(RDXCdrainR, &c2, &id2, u8cbData(tlv));
        want(id128cmp(&c, &c2) == 0);
        same(id128time(id), id128time(id2));
        same(id128src(id), id128src(id2));
        aBpad(u8, txt, 64);
        call(RDXRtlv2txt, u8bIdle(txt), u8cbData(tlv));
        a$str(str, "text RDX: $s\n");
        FILEFeedf(STDOUT_FILENO, str, u8cbData(txt));
        aBpad(u8, tlv2, 32);
        id128 id3 = {i, i};
        call(RDXRtxt2tlv, u8bIdle(tlv2), u8cbData(txt), id3);
        $testeq(u8cbData(tlv), Bu8cdata(tlv2));
    }
    done;
}

pro(RDXStest) {
    sane(1);
#define RDXSlen 3
    u8cs inputs[RDXRlen] = {$u8str(""), $u8str("a"), $u8str("abcdef")};
    for (int i = 0; i < RDXRlen; ++i) {
        u8c$ c = inputs[i];
        aRDXid(id, i, i);
        aBpad(u8, tlv, 64);
        call(RDXCfeedS, u8bIdle(tlv), c, id);
        id128 id2 = {};
        u8cs c2 = {};
        call(RDXCdrainS, c2, u8cbData(tlv), &id2);
        want($eq(c, c2));
        same(id128time(id), id128time(id2));
        same(id128src(id), id128src(id2));
        same($empty(c) ? 3 : $len(c) + 3 + 2, Bdatalen(tlv));
        aBcpad(u8, txt, 32);
        id128 id3;
        u8cs text = {};
        call(RDXCdrainS, text, u8cbData(tlv), &id3);
        aBcpad(u8, tlv2, 32);
        call(RDXCfeedS, tlv2idle, text, id);
        $testeq(u8cbData(tlv), tlv2data);
    }
    done;
}

pro(RDX1) {
    sane(1);
    B(u8, testbuf);
    u8c** path = STD_ARGS[1] + 2;
    // a$str(path, "RDX1.rdx");
    call(FILEmapro, (voidB)testbuf, path);
    fprintf(stdout, "OK\n");
    $print(u8cbData(testbuf));
    aBcpad(u8, tlv, PAGESIZE);
    aBcpad(u64, stack, 1024);
    aBcpad(u8, pad, PAGESIZE);  // FIXME
    JDRstate state = {
        .text = $dup(u8cbData(testbuf)),
        .tlv = $dup(tlvidle),
        .nest = 1,
    };
    call(JDRlexer, &state);
    aBcpad(u8, rdxj, PAGESIZE);
    call(JDRdrain, rdxjidle, tlvdata);

    a$dup(u8c, inputs, rdxjdata);
    aBpad2(u8cs, ins, 64);
    while (!$empty(inputs)) {
        u8 lit;
        u8cs rec = {};
        call(RDXdrain$, &lit, rec, inputs);
        if (lit != RDX_TERM) {
            u8cssFeed1(insidle, rec);
            continue;
        }
        u8cs correct = $dup(Blast(insbuf));
        u8csbPop(insbuf);
        aBcpad(u8, merged, PAGESIZE);
        call(YmergeFIRST, mergedidle, insdata);
        $testeq(correct, mergeddata);
        Breset(insbuf);
    }

    call(FILEunmap, (voidB)testbuf);
    done;
}

pro(RDX1test) {
    sane(1);
    call(RDXFtest);
    call(RDXItest);
    call(RDXRtest);
    call(RDXStest);
    call(RDX1);
    done;
}

MAIN(RDX1test);
