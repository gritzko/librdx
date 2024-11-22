#include "TLV.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "FILE.h"
#include "INT.h"
#include "TEST.h"

pro(TLVtest1) {
    sane(1);
    aBpad(u8, pad, 64);
    $u8c str1 = $u8str("Hello");
    $u8c str2 = $u8str(" ");
    $u8c str3 = $u8str("world!");
    u8 **into = Bu8idle(pad);
    call(TLVfeed, into, 'S', str1);
    call(TLVtinyfeed, into, 'S', str2);
    call(TLVfeed, into, 'S', str3);
    u8c **from = Bu8cdata(pad);
    //$print(from);
    $u8c str1b, str2b, str3b;
    call(TLVtake, 'S', str1b, from);
    testeq(0, $cmp(str1, str1b));
    call(TLVtake, 'S', str2b, from);
    testeq(0, $cmp(str2, str2b));
    call(TLVtake, 'S', str3b, from);
    testeq(0, $cmp(str3, str3b));
    done;
}

pro(TLVtest2) {
    sane(1);
    aBpad(u8, pad, 256);
    u8 **init = Bu8idle(pad);
    aBpad(u8, tlv, 5000);
    for (u32 v = 0; v < 256; v++) {
        **init = v;
        ++*init;
    }
    u8c **block = Bu8cdata(pad);
    testeq($len(block), 256);
    u8 **into = Bu8idle(tlv);
    for (int j = 0; j < 2; j++) {
        call(TLVfeed, into, 'B', block);
    }
    u8c **from = Bu8cdata(tlv);
    $print(from);
    for (int i = 0; i < 2; i++) {
        $u8c take;
        call(TLVtake, 'B', take, from);
        sane(0 == $cmp(block, take));
    }
    done;
}

fun int u32pcmp(u32p const *a, u32p const *b) {
    if (*a == *b) return 0;
    return *a < *b ? -1 : 1;
}

#define X(M, name) M##u32p##name
#include "Bx.h"
#undef X

pro(TLVtest3) {
    sane(1);
    aBcpad(u8, pad, 1024);
    aBpad(u32p, stack, 8);

    call(TLVopen, padidle, 'A', Bu32ppush(stack));
    $u8c aaa = $u8str("aaa");

    call(TLVopen, padidle, 'B', Bu32ppush(stack));
    $u8c bbb = $u8str("bbbb");

    call(TLVopen, padidle, 'C', Bu32ppush(stack));
    $u8c ccc = $u8str("ccccc");

    $u8feed(padidle, ccc);
    call(TLVclose, padidle, 'C', Bu32ppop(stack));

    $u8feed(padidle, bbb);
    call(TLVclose, padidle, 'B', Bu32ppop(stack));

    $u8feed(padidle, aaa);
    call(TLVclose, padidle, 'A', Bu32ppop(stack));

    $println(paddata);

    $u8c ina = {}, inb = {}, inc = {};
    call(TLVtake, 'A', ina, paddata);
    call(TLVtake, 'B', inb, ina);
    call(TLVtake, 'C', inc, inb);

    testeq(0, $cmp(ccc, inc));
    testeq(0, $cmp(bbb, inb));
    testeq(0, $cmp(aaa, ina));

    done;
}

pro(TLVtest) {
    sane(1);
    call(TLVtest1);
    call(TLVtest2);
    call(TLVtest3);
    done;
}

TEST(TLVtest);