#include "JDR2.h"
#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/PRO.h"

pro(JDRtest1) {
    sane(1);
    const char* inputs[] = {
        "{1:2,ab-3:4.5E0}",
        "ab-123:0e",
        "{@b0b-1}",
        "123",
        "1.2345E2",
        "ab-123",
        "\"string\"",
        "[1,2,3]",
        "123@ab-45",
        "[1@ab-12,1.23E0@cd-34,\"str\"@ef-56,ab-123@78-90]",
        "\"line\\n\\tmore\\n\"",
        "<nested,tu:ple>",
        "~@b0b-1:~@a1ec-2",
        "2345@2e:-1@b0b-1",
        "[1:2:3]",
        "<@b0b-1 4:5:6>",
        "7:8@b0b-1:9",
        "<@3 <<>,<>>>",
        //"5.2,0.3 1.23 1.2",
        //"::::::::::::::::::::::::::::::::",
        NULL,
    };

    for (int i = 0; inputs[i]; ++i) {
        aBcpad(u8, pad, 1024);
        aBcpad(u64, stack, 1024);
        a_pad(u8, tlv, PAGESIZE);
        a_pad(u8, jdr2, PAGESIZE);

        u8csc input = $u8str(inputs[i]);
        fprintf(stderr, "#%i. %s\n", i, inputs[i]);
        a_dup(u8c, text, input);

        ok64 o = RDXutf8sParse(text, tlv, NULL);

        if (o != OK) {
            $print(text);
            fail(o);
        }

        o = RDXutf8sFeedRaw(jdr2_idle, tlv_datac);

        if (o == OK && 0 != $cmp(input, jdr2_datac)) o = faileq;

        if (o != OK) {
            $println(input);
            $println(jdr2_datac);
            fail(o);
        }

        $testeq(input, jdr2_datac);
    }
    done;
}

pro(JDRtest2) {
    sane(1);
    u8cs ml = $u8str("`multi\nline\n\nstring\n`");
    a_dup(u8c, dup, ml);
    a_pad(u8, tlv, PAGESIZE);
    call(RDXutf8sParse, dup, tlv, NULL);
    testeq(u8bDataLen(tlv), $len(ml) - 2 + 3);
    done;
}

pro(JDRtest) {
    sane(1);
    call(JDRtest1);
    call(JDRtest2);
    done;
}

MAIN(JDRtest);
