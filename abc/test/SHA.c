#include "SHA.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "HEX.h"
#include "TEST.h"

pro(SHAtest1) {
    sane(1);
    u8cs value1 = $u8str("Good morning!\n");
    u8cs value2 = $u8str("Hello world!\n");
    u8cs hex1 = $u8str(
        "98411c31e3c6c0d7de624d2b234eb8f688264cd6c6a77dc52297741bae62e7d7");
    u8cs hex2 = $u8str(
        "0ba904eae8773b70c75333db4de2f3ac45a8ad4ddba1b242f0b3cfc199391dd8");

    sha256 hash1 = {};
    SHAsum(&hash1, value1);
    aBpad(u8, hex1b, 64);
    call(HEXsha256put, u8bIdle(hex1b), &hash1);
    testeq(YES, $eq(hex1,Bu8cdata(hex1b)));

    sha256 hash2;
    aBpad(u8, hex2b, 64);
    SHAstate state;
    SHAopen(&state);
    SHAfeed(&state, value2);
    SHAclose(&state, &hash2);
    a$(u8c, hs2, hash2.data);
    HEXput(u8bIdle(hex2b), hs2);
    testeq(YES, $eq(u8bData(hex2b), hex2));

    done;
}

pro(SHAtest) {
    sane(1);
    call(SHAtest1);
    done;
}

TEST(SHAtest);
