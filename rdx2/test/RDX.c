//
// Created by gritzko on 11/20/25.
//
#include "RDX.h"

#include "abc/PRO.h"
#include "abc/TEST.h"

ok64 RDXTestBasics() {
    sane(1);
    testeq(sizeof(rdx), 64);
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
#define TEST_LEN 128
ok64 RDXTestTLV() {
    sane(1);
    a_u8cs(uno, 'i', 2, 0, 2);
    a_u8cs(duos, 's', 12, 0, 'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l',
           'd');
    a_u8cs(tres, 'r', 3, 2, 1, 2);
    a_u8cs(tuple, 'p', 5, 0, 'i', 2, 0, 8);
    u8csp inputs[] = {
        tuple, uno, duos, tres, tuple, NULL,
    };
    int i = 0;
    while (inputs[i]) {
        u8csp in = inputs[i];

        a_pad(rdx, tlv1rdx, 8);
        zerob(tlv1rdx);
        zerob(tlv1rdx);
        call(RDXOpen, tlv1rdx, RDX_FORMAT_TLV, in);
        a_pad(rdx, tlv2rdx, 8);
        zerob(tlv2rdx);
        a_pad(u8, tlv2, TEST_LEN);
        zerob(tlv2);
        call(RDXWriteOpen, tlv2rdx, RDX_FORMAT_TLV, tlv2_idle);

        call(RDXCopy, tlv2rdx, tlv1rdx);

        call(RDXClose, tlv1rdx, NULL);
        call(RDXWriteClose, tlv2rdx, tlv2_idle);

        $testeq(in, tlv2_datac);

        i++;
    }

    done;
}

pro(RDXtest) {
    sane(1);
    call(RDXTestBasics);
    call(RDXid128test);
    call(RDXTestTLV);
    done;
}

TEST(RDXtest);
