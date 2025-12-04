//
// Created by gritzko on 11/20/25.
//
#include "RDX.h"

#include "abc/01.h"
#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/S.h"
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
        a_rdxr(tlv1it, in, RDX_FORMAT_TLV);
        a_rdxw(tlv2wit, u8bIdle(tlv2), RDX_FORMAT_TLV);

        call(rdxbCopy, tlv2wit, tlv1it);

        $testeq(inputs[i], tlv2_datac);

        i++;
    }

    done;
}

ok64 RDXTestJDR() {
    sane(1);
    a_cstr(oneint, "1\n");
    a_cstr(oneterm, "a1\n");
    a_cstr(oneid, "a-1\n");
    a_cstr(tuple, "(1,two,3E0)\n");
    u8csp inputs[] = {
        tuple, oneint, oneterm, oneid, NULL,
    };
    int i = 0;
    while (inputs[i]) {
        a_dup(u8c, in, inputs[i]);

        a_rdxr(jdr1it, in, RDX_FORMAT_JDR);
        a_pad(u8, tlv1, 256);
        a_rdxw(tlv1wit, u8bIdle(tlv1), RDX_FORMAT_TLV);

        call(rdxbCopy, tlv1wit, jdr1it);

        a_rdxr(tlv1it, u8bDataC(tlv1), RDX_FORMAT_TLV);
        a_pad(u8, jdr2, 256);
        a_rdxw(jdr2wit, u8bIdle(jdr2), RDX_FORMAT_JDR);

        call(rdxbCopy, jdr2wit, tlv1it);

        u8sFeed1(jdr2_idle, '\n');
        $testeq(inputs[i], jdr2_datac);

        i++;
    }

    done;
}

#include <test/ZE.h>

ok64 RDXTestZE() {
    sane(1);
    a_pad(rdx, elems, 64);
    a_pad(u8cs, ple, 64);
    a_cstr(in, EULERZ_TEST);
    a_rdxr(it, in, RDX_FORMAT_JDR);
    call(rdxbNext, it);
    test(rdxbType(it) == RDX_TYPE_EULER, RDXBAD);
    call(rdxbInto, it);

    u8cp p = it[0][1].data[0];
    scan(rdxbNext, it) {
        u8cp p2 = it[0][1].data[0];
        u8csbFeed1(ple, it[0][0].plex);
        call(rdxbFeedP, elems, &it[0][1]);
        rdxbLast(elems)->data = *u8csbLast(ple);
        u8cs e = {p, p2};
        p = p2;
    }
    seen(END);

    fprintf(stderr, "Stored %lu elements\n", rdxbDataLen(elems));

    // N**N compare
    rdxsp data = rdxbData(elems);
    ok64 eu = rdxEulerZ(*data + 30, *data + 31);
    for (rdxp i = data[0]; i < data[1]; ++i) {
        // printf("%lu\n", i - *data);
        for (rdxp j = i + 1; j < data[1]; ++j) {
            test(YES == rdxEulerZ(i, j), RDXBADORDR);
            /*if (!rdxEulerZ(i, j)) {
                a_pad(u8, out, 128);
                i->into = u8bIdle(out);
                j->into = u8bIdle(out);
                rdxWriteNextJDR(i);
                u8bFeed1(out, '<');
                rdxWriteNextJDR(j);
                u8bFeed1(out, 0);
                // printf("oops %lu < %lu, %s\n", (i - *data), (j - *data),
                // *out);
            }*/
        }
    }

    done;
}

#include <test/Y1.h>
#include <test/YE.h>

ok64 RDXTestY(u8cs test[][8]) {
    sane(1);
    for (int i = 0; test[i][0][0]; i++) {
        a_pad(rdx, inputs, 16);
        a_dup(u8c, correct, test[i][0]);
        for (int j = 0; test[i][j][0]; j++) {
            rdx it = {.format = RDX_FORMAT_JDR, .data = test[i][j]};
            // call(rdxNext, &it);
            call(rdxbFeedP, inputs, &it);
        }
        rdxsFed1(rdxbData(inputs));
        a_pad(u8, res, PAGESIZE);
        rdx w = {.format = RDX_FORMAT_JDR | RDX_FORMAT_WRITE, .into = res_idle};
        call(rdxMerge, &w, rdxbDataIdle(inputs));
        $println(res_datac);
        test(0 == $cmp(res_datac, correct), RDXBAD);
    }
    done;
}

pro(RDXtest) {
    sane(1);
    call(RDXTestBasics);
    call(RDXid128test);
    call(RDXTestTLV);
    call(RDXTestJDR);
    call(RDXTestZE);
    call(RDXTestY, FIRSTY_TEST);
    call(RDXTestY, YE_TEST);
    done;
}

TEST(RDXtest);
