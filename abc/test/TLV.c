#include "TLV.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "FILE.h"
#include "INT.h"
#include "PRO.h"
#include "TEST.h"

pro(TLVtest1) {
    sane(1);
    aBpad(u8, pad, 64);
    u8cs str1 = $u8str("Hello");
    u8cs str2 = $u8str(" ");
    u8cs str3 = $u8str("world!");
    u8 **into = u8bIdle(pad);
    call(TLVu8sFeed, into, 'S', str1);
    call(TLVu8sFeed, into, 'S', str2);
    call(TLVu8sFeed, into, 'S', str3);
    u8c **from = u8cbData(pad);
    u8 t1, t2, t3;
    u8cs str1b, str2b, str3b;
    call(TLVu8sDrain, from, &t1, str1b);
    testeq(0, $cmp(str1, str1b));
    call(TLVu8sDrain, from, &t2, str2b);
    testeq(0, $cmp(str2, str2b));
    call(TLVu8sDrain, from, &t3, str3b);
    testeq(0, $cmp(str3, str3b));
    done;
}

pro(TLVtest2) {
    sane(1);
    aBpad(u8, pad, 256);
    u8 **init = u8bIdle(pad);
    aBpad(u8, tlv, 5000);
    for (u32 v = 0; v < 256; v++) {
        **init = v;
        ++*init;
    }
    u8c **block = u8cbData(pad);
    testeq($len(block), 256);
    u8 **into = u8bIdle(tlv);
    for (int j = 0; j < 2; j++) {
        call(TLVu8sFeed, into, 'B', block);
    }
    u8c **from = u8cbData(tlv);
    $print(from);
    for (int i = 0; i < 2; i++) {
        u8cs take;
        u8 t;
        call(TLVu8sDrain, from, &t, take);
        must(0 == $cmp(block, take));
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
    a_pad(u8, pad, 1024);

    u8cs aaa = $u8str("aaa");
    u8cs bbb = $u8str("bbbb");
    u8cs ccc = $u8str("ccccc");

    call(TLVu8bInto, pad, 'A');
    call(u8bFeed, pad, aaa);
    call(TLVu8bInto, pad, 'B');
    call(TLVu8bInto, pad, 'C');
    call(u8bFeed, pad, ccc);
    call(TLVu8bOuto, pad, 'C');
    call(u8bFeed, pad, bbb);
    call(TLVu8bOuto, pad, 'B');
    call(TLVu8bOuto, pad, 'A');

    $println(pad_datac);

    u8cs ina = {}, inb = {}, inc = {};
    u8 at, bt, ct;
    call(TLVu8sDrain,  pad_datac, &at, ina);
    a_head(u8c, head, ina, $len(aaa));
    a_rest(u8c, rest, ina, $len(aaa));
    call(TLVu8sDrain,  rest, &bt, inb);
    call(TLVu8sDrain,  inb, &ct, inc);

    testeq(at, 'A');
    testeq(bt, 'B');
    testeq(ct, 'C');

    testeq(0, $cmp(ccc, inc));
    testeq(0, $cmp(bbb, inb));
    testeq(0, $cmp(aaa, head));

    done;
}

ok64 TLVtest4() {
    sane(1);
    aBpad2(u8, pad, PAGESIZE);
    aBpad(u8p, stack, 16);
    u8 correct[] = {'a', 4, 't', 'e', 's', 't'};
    a$(u8c, cor, correct);
    u8cs text = {cor[0] + 2, cor[1]};
    call(TLVinitlong, padidle, 'A', stack);
    call(u8sFeed, padidle, text);
    call(TLVendany, padidle, 'A', stack);
    $testeq(cor, paddata);
    done;
}

pro(TLVtest) {
    sane(1);
    call(TLVtest1);
    call(TLVtest2);
    call(TLVtest3);
    call(TLVtest4);
    done;
}

TEST(TLVtest);
