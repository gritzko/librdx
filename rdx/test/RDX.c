#include <stdint.h>
#include <unistd.h>

#include "abc/B.h"
#include "abc/TEST.h"
#include "rdx/JDR2.h"
#include "rdx/RDX2.h"

pro(RDXtest0) {
    sane(1);
    want(RDXisPLEX('P'));
    want(RDXisPLEX('p'));
    want(!RDXisPLEX('*'));
    want(!RDXisPLEX('f'));
    want(RDXisFIRST('f'));
    want(RDXisFIRST('F'));
    want(RDXisFIRST('t'));
    want(RDXisFIRST('T'));
    want(!RDXisFIRST('P'));
    done;
}

pro(RDXid128test) {
    sane(1);
#define RDXIDINLEN 3
    ref128 inputs[RDXIDINLEN] = {{0, 0},      //
                                 {100, 200},  //
                                 {ron60Max, ron60Max}};
    for (int i = 0; i < RDXIDINLEN; ++i) {
        aBpad(u8, ron, 64);
        Bzero(ron);
        call(RDXutf8sFeedID, u8bIdle(ron), &inputs[i]);
        ref128 in2 = {};
        call(RDXutf8sDrainID, u8bDataC(ron), &in2);
        want(ref128Eq(&inputs[i], &in2));
    }
    done;
}

pro(RDXTestF) {
    sane(1);
    RDXfloat inputs[] = {0, 12345.6789, 1.2e+20};
    for (int i = 0; i < sizeof(inputs) / sizeof(*inputs); ++i) {
        a_pad(utf8, txt, 64);
        rdx r1 = {.type = RDX_FLOAT, .f = inputs[i]}, r2 = {}, r3 = {};
        call(RDXutf8sFeedF, utf8bIdle(txt), &r1);
        call(RDXutf8sDrainF, utf8bDataC(txt), &r2);
        testeqv(r1.f, r2.f, "%f");

        a_pad(u8, tlv, 64);
        call(RDXu8sFeedF, utf8bIdle(tlv), &r1);
        call(RDXu8sDrainF, utf8bDataC(tlv), &r3);
        testeqv(r1.f, r3.f, "%f");
    }
    done;
}

pro(RDXtest) {
    sane(1);
    call(RDXtest0);
    call(RDXid128test);
    call(RDXTestF);
    done;
}

TEST(RDXtest);
