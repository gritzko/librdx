#include "Y.h"

fun ok64 _X(push)(u8csB lsm, $u8c x) {
    ok64 o = u8css_feed1(Bu8csidle(lsm), x);
    if (o == OK) HEAPu8csupf(lsm, &_X(z));
    return o;
}

fun ok64 _X(next)($u8 into, u8css lsm) {
    $u8c next = {};
    aBpad2(u8cs, in, Y_MAX_INPUTS);
    ok64 o = OK;

    do {
        o = _X(x)(next, **lsm);
        if (o != OK) return o;
        u8css_feedp(inidle, &next);
        if ($empty(**lsm)) {
            u8csswap($head(lsm), $last(lsm));
            --$term(lsm);
            if ($empty(lsm)) break;
        }
        HEAPu8csdownf(lsm, _X(z));
    } while (0 == _X(z)($head(lsm), &next));

    if ($len(indata) == 1) {
        o = $u8feed(into, next);
    } else {
        o = _X(y)(into, indata);
    }
    return o;
}

fun b8 _$u8cempty($u8c const* s) { return $empty(*s); }

fun ok64 _X(sort)(u8css lsm) {
    u8css_purge(lsm, &_$u8cempty);
    $sort(lsm, _X(z));
    return OK;  // ?
}

fun ok64 _X(merge)($u8 into, u8css lsm) {
    ok64 o = _X(sort)(lsm);
    while (o == OK && !$empty(lsm)) {
        o = _X(next)(into, lsm);
    }
    return o;
}
