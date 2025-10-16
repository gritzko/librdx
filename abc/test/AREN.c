
#include "AREN.h"

#include <stdint.h>
#include <unistd.h>

#include "INT.h"
#include "TEST.h"

typedef struct {
    u64 key, val;
} kv64;

pro(ARENtest1) {
    sane(1);
    aBcpad(u8, arena, PAGESIZE);
    u8cs abc = $u8str("abcdefg");

    afed(str, $u8feed, arenaidle, abc);  // 7
    want($eq(abc, str));
    u32 u = 0x12345678;

    afedc(uu, u8sFeed32, arenaidle, &u);  // 12
    u32 u2 = 0;
    $u8drain32(&u2, uu);

    a32(u3, 123, arenaidle);  // 16
    same(*u3, 123);
    want(arenabuf[0] < (u8*)u3 && (u8*)u3 < arenabuf[3]);

    a64(u4, UINT64_MAX, arenaidle);  // 24
    same(*u4, UINT64_MAX);
    *u4 = 0;
    same(*u3, 123);
    same(*u4, 0);

    arec(kv64, rec, arenaidle);  // 40
    same(rec->key, 0);
    same(rec->val, 0);
    rec->key = 1;
    rec->val = 2;
    same(*u3, 123);
    same(*u4, 0);
    want($size(arenadata) == 8 * 5);
    done;
}

pro(ARENtest) {
    sane(1);
    call(ARENtest1);
    done;
}

TEST(ARENtest);
