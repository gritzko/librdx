#include "CLI.h"

#include <unistd.h>

#include "abc/01.h"
#include "abc/TEST.h"
#include "rdx/RDXC.h"

$u8c _STD_ARGS[64] = {};
$u8c* STD_ARGS[4] = {};

ok64 CLItest() {
    sane(1);
    aBcpad(u8, pad, PAGESIZE);
    char* args1[] = {"cmd", "12", "do:", "./something"};
    _parse_args(4, args1);
    call(JDRdrainargs, padidle);
    u8 t = 0;
    $u8c val = {};
    id128 _ = {};
    // call(RDXCdrainT, val, &_, paddata);
    //$u8c p1 = $u8str("cmd");
    //$testeq(p1, val);
    i64 i;
    call(RDXCdrainI, &i, &_, paddata);
    testeq(i, 12);
    $u8c pc = {};
    call(RDXdrain, &t, &_, pc, paddata);
    testeq($len(paddata), 0);

    call(RDXCdrainT, val, &_, pc);
    $u8c p2 = $u8str("do");
    $testeq(p2, val);
    call(RDXCdrainS, val, &_, pc);
    $u8c s3 = $u8str("./something");
    $testeq(s3, val);
    done;
}

TEST(CLItest);
