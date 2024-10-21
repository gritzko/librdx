#ifndef ABC_RDXY_H
#define ABC_RDXY_H
#include "LSM.h"
#include "OK.h"
#include "RDX.h"

fun ok64 RDXdrainid(id128* id, $u8c from) { return notimplyet; }

fun pro(RDXYmergeFIRST, $u8 into, $$u8c from) {
    sane($ok(into) && $ok(from));
    u128 max = {};
    $u8c rec = {};
    u8 t = {};
    u128 id = {};
    $u8c value = {};
    $eat(from) {
        u8c$ p = **from;
        call(RDXdrain, &t, &id, value, p);
        if (u128cmp(&max, &id) < 0) {
            max = id;
            $mv(rec, p);
        }
    }
    if (*rec != nil) call($u8feed, into, rec);
    done;
}

typedef ok64 (*LSMeater)(u8c$ bite, $u8c from);

fun int valC($cu8c* a, $cu8c* b) { return 0; }

fun int srcC($cu8c* a, $cu8c* b) { return 0; }

fun ok64 maxidY($u8 into, $$u8c from) {
    // FIXME this may recur for LIVE
    return OK;
}

fun ok64 RDXYmergeP($u8 into, $$u8c from) { return notimplyet; }

// { @a1ece-0 1, 2, 3, 4 }
fun ok64 RDXYmergeE($u8 into, $$u8c from) {
    return LSMmerge(into, from, valC, maxidY);
}

fun ok64 RDXYmergeN($u8 into, $$u8c from) {
    return LSMmerge(into, from, srcC, maxidY);
}

// [ @b0b-12 1, 2, 3, 4, "5" ]
fun pro(RDXYmergeL, $u8 into, $$u8c from) {
    sane($ok(into) && $ok(from));
    // TODO injects
    // TODO step wise
    fail(notimplyet);
    done;
}

#endif
