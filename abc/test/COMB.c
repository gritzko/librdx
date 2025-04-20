
#include "COMB.h"

#include <unistd.h>

#include "BUF.h"
#include "PRO.h"
#include "TEST.h"

ok64 COMBtest1() {
    sane(1);
    aBcpad(u8, pad, PAGESIZE);
    COMBinit(padbuf);
    $feed1(padidle, 'A');
    COMBsave(padbuf);
    Breset(padbuf);
    COMBload(padbuf);
    testeq($len(paddata), 1);
    a$str(str1, "A");
    want($eq(paddata, str1));
    done;
}

// #define X(M, name) M##u8##name
// #include "COMBx.h"
// #undef X

ok64 COMBtest2() {
    sane(1);
    aBcpad(u8, pad, PAGESIZE);
    // call(COMBu8init, padbuf);
    done;
}

ok64 COMBtest() {
    sane(1);
    call(COMBtest1);
    // call(COMBtest2);
    done;
}

TEST(COMBtest);
