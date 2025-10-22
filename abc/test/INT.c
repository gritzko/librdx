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

pro(print, int c) {
    sane(1);
    printf("%c", c);
    done;
}

/** A simple true/false checker function */
pro(check, int a, int b) {
    sane(1);
    if (a != b) fail(FAIL);
    // nedo(fprintf(stderr, "res: %lx\n", __));
    done;
}

pro(Utest1) {
    sane(1);
    u8 array[] = {1, 2, 3, 4};
    a$(u8, slice, array);
    $eat(slice) { printf("%i\n", **slice); }
    Bu8 buf = {};
    u8bAllocate(buf, 32);
    u8 i = 0;
    $eat(u8bIdle(buf)) { Bi(buf) = i++; }
    assert(Bd(buf) == 0);
    $reverse(u8bData(buf));
    assert(Bd(buf) == 31);
    $sort(u8bData(buf), &u8cmp);
    assert(Bd(buf) == 0);
    check(Bd(buf), 0);
    check(Bd(buf), 31);
    u8 **data = u8bData(buf);
    $eat(data) printf("%c", (int)(**data + 'A'));
    printf("\n");
    $u8 abc = $cut(u8bPast(buf), 0, 3);
    assert($len(abc) == 3);
    u8 three = 3;
    u8c *c = $u8find(u8cbPast(buf), &three);
    assert(c - buf[0] == 3);
    u8bFree(buf);
    done;
}

pro(Utest2) {
    sane(1);
    a$str(dec, "-123");
    i64 i;
    call(i64decdrain, &i, dec);
    testeq(i, -123);
    done;
}

pro(OKdec) {
    sane(YES);
    aBpad(u8, into, 64);
    call(u64decfeed, u8bIdle(into), 12345UL);
    $print(u8cbData(into));
    a$str(str, "12345");
    testeq(YES, $eq(u8bData(into), str));
    done;
}

pro(Utest) {
    sane(1);
    call(Utest1);
    call(Utest2);
    call(OKdec);
    done;
}

TEST(Utest);
