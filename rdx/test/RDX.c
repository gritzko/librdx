//
// Created by gritzko on 11/20/25.
//
#include "RDX.h"

#include <strings.h>

#include "abc/01.h"
#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/S.h"
#include "abc/TEST.h"

ok64 u8csTestEq(u8cs a, u8cs b) {
    sane(u8csOK(a) && u8csOK(b));
#if NDEBUG
    if ($eq(a, b)) return OK;
#endif
    a_pad(u8, msg, PAGESIZE);
    a_cstr(wantmsg, "want:\t");
    a_cstr(factmsg, "fact:\t");
    call(u8bFeed, msg, wantmsg);
    call(HEXput, msg_idle, a);
    call(u8bFeed1, msg, '\n');
    call(u8bFeed, msg, factmsg);
    call(HEXput, msg_idle, b);
    call(u8bFeed1, msg, '\n');

    call(u8bFeed, msg, wantmsg);
    call(u8bFeed, msg, a);
    call(u8bFeed1, msg, '\n');
    call(u8bFeed, msg, factmsg);
    call(u8bFeed, msg, b);
    call(u8bFeed1, msg, '\n');

    $print(msg_datac);

    return $eq(a, b) ? OK : NOEQ;
}

ok64 RDXTestBasics() {
    sane(1);
    testeq(sizeof(rdx), 64);
    rdx a;
    rdxgp g = a.ins;
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
        tuple, uno, duos, tres, euler, NULL,
    };
    int i = 0;
    while (inputs[i]) {
        a_dup(u8c, in, inputs[i]);
        a_pad(u8, tlv2, 256);
        rdx itc = {.format = RDX_FMT_TLV};
        u8csFork(in, itc.data);
        rdx it = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
        u8sFork(tlv2_idle, it.into);

        call(rdxCopy, &it, &itc);

        call(u8sJoin, tlv2_idle, it.into);

        call(u8csTestEq, inputs[i], tlv2_datac);

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
        oneint, oneterm, oneid, tuple, NULL,
    };
    int i = 0;
    while (inputs[i]) {
        a_dup(u8c, jdrA, inputs[i]);
        a_pad(u8, tlv, 256);
        a_pad(u8, jdrB, 256);
        // 1. a_rdx(jdr1, in, ...) no rdxb
        // 2. u8csFork(orig, copy), u8csJoin(orig, copy)
        // 3. u8csIn(outer, inner) u8csIs(outer, inner)
        // 4. RDX_FMT_SKIP ((()))
        // 5. RDX_FMT_VEC 40b (diff, use len)
        // 6. RDX_FMT_MEM (prepared, threshold, ext str)
        // 7. g is s*2, no u8g API, a_gauge and .

        rdx jdr1 = {.format = RDX_FMT_JDR};
        u8csFork(jdrA, jdr1.data);
        rdx tlv1 = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
        u8sFork(tlv_idle, tlv1.into);

        call(rdxCopy, &tlv1, &jdr1);
        call(u8sJoin, tlv_idle, tlv1.into);

        rdx tlv2 = {.format = RDX_FMT_TLV};
        u8csFork(tlv_datac, tlv2.data);
        rdx jdr2 = {.format = RDX_FMT_JDR | RDX_FMT_WRITE};
        u8sFork(jdrB_idle, jdr2.into);

        call(rdxCopy, &jdr2, &tlv2);
        call(u8sJoin, jdrB_idle, jdr2.into);

        call(u8csTestEq, inputs[i], jdrB_datac);

        i++;
    }

    done;
}

#include <test/ZE.h>

ok64 RDXTestZE() {
    sane(1);
    a_pad(rdx, elems, 64);
    a_cstr(in, EULERZ_TEST);
    rdx it = {.format = RDX_FMT_JDR};
    u8csFork(in, it.data);
    call(rdxNext, &it);
    test(it.type == RDX_TYPE_EULER, RDXBAD);
    rdx c = {};
    call(rdxInto, &c, &it);

    scan(rdxNext, &c) call(rdxbFeedP, elems, &c);
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
#include <test/YL.h>
#include <test/YP.h>
#include <test/YX.h>

ok64 RDXTestY(u8cs test[][8]) {
    sane(1);
    for (int i = 0; test[i][0][0]; i++) {
        a_pad(rdx, inputs, 16);
        rdxbZero(inputs);
        a_dup(u8c, correct, test[i][0]);
        for (int j = 0; test[i][j][0]; j++) {
            rdxp it = 0;
            call(rdxbFedP, inputs, &it);
            it->format = RDX_FMT_JDR;
            $mv(it->data, test[i][j]);
        }
        rdxsFed1(rdxbData(inputs));
        a_pad(u8, res, PAGESIZE);
        rdx w = {.format = RDX_FMT_JDR | RDX_FMT_WRITE};
        u8sFork(res_idle, w.into);
        call(rdxMerge, &w, rdxbDataIdle(inputs));
        u8sJoin(res_idle, w.into);
        $println(res_datac);
        test(0 == $cmp(res_datac, correct), RDXBAD);
    }
    done;
}

ok64 RDXTestY2(u8cs test[][8]) {
    sane(1);
    for (int i = 0; test[i][0][0]; i++) {
        a_pad(rdx, inputs, 16);
        rdxbZero(inputs);
        a_dup(u8c, correct, test[i][0]);
        for (int j = 0; test[i][j][0]; j++) {
            rdxp it = 0;
            call(rdxbFedP, inputs, &it);
            it->format = RDX_FMT_JDR;
            $mv(it->data, test[i][j]);
        }
        rdxsFed1(rdxbData(inputs));
        rdx y = {.format = RDX_FMT_Y};
        rdxgMv(y.ins, rdxbDataIdle(inputs));
        a_pad(u8, res, PAGESIZE);
        rdx w = {.format = RDX_FMT_JDR | RDX_FMT_WRITE};
        u8sFork(res_idle, w.into);
        call(rdxCopy, &w, &y);
        u8sJoin(res_idle, w.into);
        $println(res_datac);
        test(0 == $cmp(res_datac, correct), RDXBAD);
    }
    done;
}

ok64 RDXtest() {
    sane(1);
    call(RDXTestBasics);
    call(RDXid128test);
    call(RDXTestTLV);
    call(RDXTestJDR);
    call(RDXTestZE);
    call(RDXTestY, FIRSTY_TEST);
    call(RDXTestY, YP_TEST);
    call(RDXTestY, YL_TEST);
    call(RDXTestY, YE_TEST);
    call(RDXTestY, YX_TEST);

    call(RDXTestY2, FIRSTY_TEST);
    call(RDXTestY2, YP_TEST);
    call(RDXTestY2, YL_TEST);
    call(RDXTestY2, YE_TEST);
    call(RDXTestY2, YX_TEST);
    done;
}

TEST(RDXtest);
