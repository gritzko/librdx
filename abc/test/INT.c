//
// Created by gritzko on 5/11/24.
//
#include "INT.h"

#include <assert.h>
#include <stdio.h>

#include "FILE.h"
#include "OK.h"
#include "PRO.h"
#include "S.h"
#include "TEST.h"

ok64 print(int c) {
    sane(1);
    printf("%c", c);
    done;
}

/** A simple true/false checker function */
ok64 check(int a, int b) {
    sane(1);
    if (a != b) fail(FAIL);
    // nedo(fprintf(stderr, "res: %lx\n", __));
    done;
}

ok64 Utest1() {
    sane(1);
    u8 array[] = {1, 2, 3, 4};
    a$(u8, slice, array);
    $eat(slice) { printf("%i\n", **slice); }
    Bu8 buf = {};
    Bu8alloc(buf, 32);
    u8 i = 0;
    $eat(Bu8idle(buf)) { Bi(buf) = i++; }
    assert(Bd(buf) == 0);
    $reverse(Bu8data(buf));
    assert(Bd(buf) == 31);
    $sort(Bu8data(buf), &u8z);
    assert(Bd(buf) == 0);
    check(Bd(buf), 0);
    check(Bd(buf), 31);
    u8 **data = Bu8data(buf);
    $eat(data) printf("%c", (int)(**data + 'A'));
    printf("\n");
    $u8 abc = $cut(Bu8past(buf), 0, 3);
    assert($len(abc) == 3);
    u8 three = 3;
    u8c *c = $u8find(Bu8cpast(buf), &three);
    assert(c - buf[0] == 3);
    Bu8free(buf);
    done;
}

ok64 Utest2() {
    sane(1);
    a$str(dec, "-123");
    i64 i;
    call(i64decdrain, &i, dec);
    testeq(i, -123);
    done;
}

ok64 OKdec() {
    sane(YES);
    aBpad(u8, into, 64);
    call(u64decfeed, Bu8idle(into), 12345UL);
    $print(Bu8cdata(into));
    a$str(str, "12345");
    testeq(YES, $eq(Bu8data(into), str));
    done;
}

ok64 Utest() {
    sane(1);
    call(Utest1);
    call(Utest2);
    call(OKdec);
    done;
}

TEST(Utest);
