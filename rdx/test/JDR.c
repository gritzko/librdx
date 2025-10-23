#include "JDR2.h"

#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/PRO.h"

pro(JDRtest1) {
    sane(1);
#define LEN1 18
    u8cs inputs[LEN1] = {
        $u8str("{@b0b-1}"),
        $u8str("123"),
        $u8str("1.2345E2"),
        $u8str("ab-123"),
        $u8str("ab-123:0e"),
        $u8str("\"string\""),
        $u8str("[1,2,3]"),
        $u8str("{1:2,ab-3:4.5E0}"),
        $u8str("123@ab-45"),
        $u8str("[1@ab-12,1.23E0@cd-34,\"str\"@ef-56,ab-123@78-90]"),
        $u8str("\"line\\n\\tmore\\n\""),
        $u8str("<nested,tu:ple>"),
        $u8str("(~@b0b-1,~@a1ec-2)"),
        $u8str("(2345@2e,-1@b0b-1)"),
        $u8str("[1:2:3]"),
        $u8str("<@b0b-1 4:5:6>"),
        $u8str("7:8@b0b-1:9"),
        $u8str("<@3 <<>,<>>>"),
        //$u8str("5.2,0.3 1.23 1.2"),
        //$u8str("::::::::::::::::::::::::::::::::"),
    };

    for (int i = 0; i < LEN1; ++i) {
        aBcpad(u8, pad, 1024);
        aBcpad(u64, stack, 1024);
        a_pad(u8, tlv, PAGESIZE);
        a_pad(u8, jdr2, PAGESIZE);

        u8cs text = $dup(inputs[i]);
        ok64 o = RDXutf8sParse( text, tlv, NULL);

        if (o != OK) {
            $print(text);
            fail(o);
        }

        o = RDXutf8sFeedRaw(jdr2_idle, tlv_datac);

        if (o == OK && 0 != $cmp(inputs[i], jdr2_datac)) o = faileq;

        if (o != OK) {
            $println(inputs[i]);
            $println(jdr2_datac);
            fail(o);
        }

        $testeq(inputs[i], jdr2_datac);
    }
    done;
}

pro(JDRtest2) {
    sane(1);
    u8cs ml = $u8str("`multi\nline\n\nstring\n`");
    a$dup(u8c, dup, ml);
    aBcpad(u8, tlv, PAGESIZE);
    call(RDXutf8sParse, dup, tlvidle, NULL);
    testeq($len(tlvdata), $len(ml) - 2 + 3);
    done;
}

pro(JDRtest) {
    sane(1);
    call(JDRtest1);
    call(JDRtest2);
    done;
}

MAIN(JDRtest);
