
#include "UTF8.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "$.h"
#include "INT.h"
#include "TEST.h"

ok64 UTF8test1() {
    sane(1);
    $u8c abc = $u8str("abc");
    u32 a, b, c;
    call(UTF8drain1, &a, abc);
    call(UTF8drain1, &b, abc);
    call(UTF8drain1, &c, abc);
    same(a, 'a');
    same(b, 'b');
    same(c, 'c');
    done;
}

ok64 UTF8test() {
    sane(1);
    call(UTF8test1);
    done;
}

TEST(UTF8test);
