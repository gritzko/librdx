#include "RDXJ.h"

#include "B.h"
#include "FILE.h"
#include "TEST.h"

pro(RDXtest1) {
    sane(1);
#define LEN1 8
    $u8c inputs[LEN1] = {
        $u8str("123"),
        $u8str("1.2345E2"),
        $u8str("ab-123"),
        $u8str("\"string\""),
        $u8str("[1,2,3]"),
        $u8str("{1:2,ab-3:4.5E0}"),
        $u8str("123@ab-45"),
        $u8str("[1@ab-12,1.23E0@cd-34,\"str\"@ef-56,ab-123@78-90]"),
    };

    for (int i = 0; i < LEN1; ++i) {
        aBcpad(u8, pad, 1024);
        aBcpad(u64, stack, 1024);
        aBcpad(u8, tlv, PAGESIZE);
        aBcpad(u8, rdxj2, PAGESIZE);

        RDXJstate state = {.text = {inputs[i][0], inputs[i][1]},
                           .tlv = (u8B)tlvbuf,
                           .stack = (u64B)stackbuf,
                           .pad = (u8B)padbuf,
                           .id = {}};

        call(Bu64feed1, state.stack, 0);

        call(RDXJlexer, &state);

        call(RDXJfromTLV, rdxj2idle, tlvdata);

        FILEerr(rdxj2data);
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
