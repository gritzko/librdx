//
// Created by gritzko on 11/20/25.
//
#include "RDX.h"

#include "abc/PRO.h"
#include "abc/TEST.h"

ok64 RDXTestBasics() {
    sane(1);
    testeq(sizeof(rdx), 48);
    done;
}

ok64 RDXid128test() {
    sane(1);
#define RDXIDINLEN 3
    id128 inputs[RDXIDINLEN] = {{0, 0},      //
                                {100, 200},  //
                                {ron60Max, ron60Max}};
    for (int i = 0; i < RDXIDINLEN; ++i) {
        aBpad(u8, ron, 64);
        zerob(ron);
        call(RDXutf8sFeedID, u8bIdle(ron), &inputs[i]);
        id128 in2 = {};
        call(RDXutf8sDrainID, u8bDataC(ron), &in2);
        want(id128Eq(&inputs[i], &in2));
    }
    done;
}

/*   tlv1 --> jdr1 --> tlv4
 *    |
 *   tlv2 --> jdr2
 *    |
 *   tlv3
 */
ok64 RDXTestTLV() {
    sane(1);
    a_u8cs(uno, 'i', 2, 0, 2);
    a_u8cs(duos, 's', 12, 0, 'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l',
           'd');
    a_u8cs(tres, 'r', 3, 2, 1, 2);
    a_u8cs(tuple, 'p', 5, 0, 'i', 2, 0, 8);
    a_u8cs(euler, 'e', 18, 0, 'p', 5, 0, 'i', 2, 0, 8, 'p', 8, 2, 2, 3, 'i', 3,
           0, 1, 2);
    u8csp inputs[] = {
        tuple, uno, duos, tres, tuple, euler, NULL,
    };
    int i = 0;
    while (inputs[i]) {
        a_dup(u8c, in, inputs[i]);

        a_pad(u8, tlv2, 256);
        a_rdx(tlv1it, in, RDX_FORMAT_TLV);
        a_rdxw(tlv2wit, u8bIdle(tlv2), RDX_FORMAT_TLV);

        call(rdxbCopy, tlv2wit, tlv1it);

        $testeq(inputs[i], tlv2_datac);

        i++;
    }

    done;
}

ok64 RDXTestJDR() {
    sane(1);
    a_cstr(oneint, "1");
    a_cstr(oneterm, "a1");
    a_cstr(oneid, "a-1");
    a_cstr(tuple, "(1,two,3E0)");
    u8csp inputs[] = {
        tuple,
        oneint,
        oneterm,
        oneid,
        NULL,
    };
    int i = 0;
    while (inputs[i]) {
        a_dup(u8c, in, inputs[i]);

        a_rdx(jdr1it, in, RDX_FORMAT_JDR);
        a_pad(u8, tlv1, 256);
        a_rdxw(tlv1wit, u8bIdle(tlv1), RDX_FORMAT_TLV);

        call(rdxbCopy, tlv1wit, jdr1it);

        a_rdx(tlv1it, u8bDataC(tlv1), RDX_FORMAT_TLV);
        a_pad(u8, jdr2, 256);
        a_rdxw(jdr2wit, u8bIdle(jdr2), RDX_FORMAT_JDR);

        call(rdxbCopy, jdr2wit, tlv1it);

        $testeq(inputs[i], jdr2_datac);

        i++;
    }

    done;
}

pro(RDXtest) {
    sane(1);
    call(RDXTestBasics);
    call(RDXid128test);
    call(RDXTestTLV);
    call(RDXTestJDR);
    done;
}

TEST(RDXtest);
