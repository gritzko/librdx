//
// Created by gritzko on 5/11/24.
//
#include <assert.h>
#include <stdio.h>

#include "INT.h"
#include "PRO.h"
#include "TEST.h"

pro(print, int c) {
    printf("%c", c);
    done;
}

/** A simple true/false checker function */
pro(check, int a, int b) {
    if (a != b) fail(FAIL);
    nedo(fprintf(stderr, "res: %lx\n", __));
}

pro(Utest1) {
    u8 array[] = {1, 2, 3, 4};
    $u8 slice = $sliced(array);
    $eat(slice) { printf("%i\n", **slice); }
    Bu8 buf = {};
    Bu8alloc(buf, 32);
    u8 i = 0;
    $eat(Bu8idle(buf)) { Bi(buf) = i++; }
    assert(Bd(buf) == 0);
    $reverse(Bu8data(buf));
    assert(Bd(buf) == 31);
    $sort(Bu8data(buf), &u8cmp);
    assert(Bd(buf) == 0);
    check(Bd(buf), 0);
    check(Bd(buf), 31);
    u8 **data = Bu8data(buf);
    $eat(data) printf("%c", (int)(**data + 'A'));
    printf("\n");
    $u8 abc = $cut(Bu8past(buf), 0, 3);
    assert($len(abc) == 3);
    u8 three = 3;
    u8 *c = $u8find(Bu8past(buf), &three);
    assert(c - buf[0] == 3);
    Bu8free(buf);
    done;
}

pro(Utest2) { done; }

pro(Utest) {
    call(Utest1);
    call(Utest2);
    done;
}

TEST(Utest);
