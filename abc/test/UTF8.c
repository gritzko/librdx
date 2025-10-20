
#include "UTF8.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "S.h"
#include "INT.h"
#include "TEST.h"

pro(UTF8test1) {
    sane(1);
    u8cs abc = $u8str("abc");
    u32 a, b, c;
    call(utf8sDrain32, &a, abc);
    call(utf8sDrain32, &b, abc);
    call(utf8sDrain32, &c, abc);
    same(a, 'a');
    same(b, 'b');
    same(c, 'c');
    done;
}

pro(UTF8test) {
    sane(1);
    call(UTF8test1);
    done;
}

TEST(UTF8test);
