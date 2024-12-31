#include "JDR.h"

#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

pro(RDXtest1) {
    sane(1);
#define LEN1 16
    $u8c inputs[LEN1] = {
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
        $u8str("nested:<tuple>"),
        $u8str("(~@b0b-1,~@a1ec-2)"),
        $u8str("(-1@b0b-1,2345@2e)"),
        $u8str("[1:2:3]"),
        $u8str("<@b0b-1 4:5:6>"),
        $u8str("7:8@b0b-1:9"),
    };

    for (int i = 0; i < LEN1; ++i) {
        aBcpad(u8, pad, 1024);
        aBcpad(u64, stack, 1024);
        aBcpad(u8, tlv, PAGESIZE);
        aBcpad(u8, rdxj2, PAGESIZE);

        $u8c text = $dup(inputs[i]);
        ok64 o = JDRdrain(tlvidle, text);

        if (o != OK) {
            $print(text);
            fail(o);
        }

        o = JDRfeed(rdxj2idle, tlvdata);

        if (o == OK && 0 != $cmp(inputs[i], rdxj2data)) o = faileq;

        if (o != OK) {
            $print(rdxj2data);
            fail(o);
        }

        $testeq(inputs[i], rdxj2data);
    }
    done;
}

pro(RDXtest) {
    sane(1);
    call(RDXtest1);
    done;
}

TEST(RDXtest);
