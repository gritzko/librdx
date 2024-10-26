#ifndef ABC_LINE_H
#define ABC_LINE_H
#include "INT.h"
#include "OK.h"
#include "PRO.h"
#include "TLV.h"

#define LSM_MAX_INPUTS 64

con ok64 LSMeof = 0xab3a56715;
con ok64 LSMbad = 0xa25996715;
con ok64 LSMnodata = 0x25e25a33c96715;
con ok64 LSMnoroom = 0x31cf3db3c96715;

typedef ok64 (*LSMeater)(u8c$ bite, $u8c from);
typedef int (*$u8cmpfn)($cu8c* a, $cu8c* b);
typedef ok64 (*LSMmerger)($u8 into, $$u8c from);

#define X(M, name) M##$u8c##name
#include "HEAPx.h"
#undef X

typedef B$u8c LSM;

fun pro(LSMmore, B$u8c lsm, $u8c x, $u8cmpfn cmp) {
    sane(Bok(lsm) && $ok(x) && cmp != nil);
    // call($$u8cfeed1, B$u8cidle(lsm), x);
    memcpy(lsm[2], x, sizeof($u8c));
    B$u8cidle(lsm)[0]++;
    HEAP$u8cupf(lsm, cmp);
    done;
}

fun pro(LSMnext, $u8 into, $$u8c lsm, $u8cmpfn cmp, LSMmerger mrg) {
    sane($ok(into) && Bok(lsm) && cmp != nil && mrg != nil);
    $u8c next = {};
    aBpad2($u8c, in, LSM_MAX_INPUTS);

    do {
        call(TLVdrain$, next, **lsm);
        call($$u8cfeedp, inidle, &next);
        if ($empty(**lsm)) {
            $u8cswap($head(lsm), $last(lsm));
            --$term(lsm);
            if ($empty(lsm)) break;
        }
        HEAP$u8cdownf(lsm, cmp);
    } while (0 == cmp($head(lsm), &next));

    if ($len(indata) == 1) {
        call($u8feed, into, next);
    } else {
        call(mrg, into, indata);
    }
    done;
}

fun ok64 LSMsort($$u8c lsm, $u8cmpfn cmp) {
    $sort(lsm, cmp);
    return OK;
}

fun ok64 LSMmerge($u8 into, $$u8c lsm, $u8cmpfn cmp, LSMmerger mrg) {
    ok64 o = LSMsort(lsm, cmp);
    if (o != OK) return o;
    while (o == OK && !$empty(lsm)) {
        o = LSMnext(into, lsm, cmp, mrg);
    }
    return o;
}

#endif
