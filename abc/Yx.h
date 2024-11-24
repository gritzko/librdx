#include "Y.h"

fun ok64 _X(push)(B$u8c lsm, $u8c x) {
    ok64 o = $$u8cfeed1(B$u8cidle(lsm), x);
    if (o == OK) HEAP$u8cupf(lsm, &_X(z));
    return o;
}

fun ok64 _X(next)($u8 into, $$u8c lsm) {
    $u8c next = {};
    aBpad2($u8c, in, Y_MAX_INPUTS);
    ok64 o = OK;

    do {
        o = _X(x)(next, **lsm);
        if (o != OK) return o;
        $$u8cfeedp(inidle, &next);
        if ($empty(**lsm)) {
            $u8cswap($head(lsm), $last(lsm));
            --$term(lsm);
            if ($empty(lsm)) break;
        }
        HEAP$u8cdownf(lsm, _X(z));
    } while (0 == _X(z)($head(lsm), &next));

    if ($len(indata) == 1) {
        o = $u8feed(into, next);
    } else {
        o = _X(y)(into, indata);
    }
    return o;
}

fun b8 $u8cempty($u8c const* s) { return $empty(*s); }

fun ok64 _X(sort)($$u8c lsm) {
    $$u8cpurge(lsm, &$u8cempty);
    $sort(lsm, _X(z));
    return OK;  // ?
}

fun ok64 _X(merge)($u8 into, $$u8c lsm) {
    ok64 o = _X(sort)(lsm);
    while (o == OK && !$empty(lsm)) {
        o = _X(next)(into, lsm);
    }
    return o;
}
