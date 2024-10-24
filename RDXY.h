#ifndef ABC_RDXY_H
#define ABC_RDXY_H
#include "LSM.h"
#include "OK.h"
#include "RDX.h"
#include "RDXZ.h"
#include "TLV.h"

fun ok64 RDXY($u8 into, $$u8c from);

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

fun pro(RDXYmergeP, $u8 into, $$u8c from) {
    sane($ok(into) && $ok(from));
    for (;;) {
        $$u8c chunks;
        call(RDXY, into, chunks);
    }
    done;
}

// [ @b0b-12 1, 2, 3, 4, "5" ]
fun pro(RDXYmergeL, $u8 into, $$u8c from) {
    sane($ok(into) && $ok(from));
    // TODO injects
    // TODO step wise
    fail(notimplyet);
    done;
}

// { @a1ece-0 1, 2, 3, 4 }
fun ok64 RDXYmergeE($u8 into, $$u8c from) {
    return LSMmerge(into, from, RDXZvalue, RDXY);
}

fun ok64 RDXYmergeX($u8 into, $$u8c from) {
    return LSMmerge(into, from, RDXZauthor, RDXY);
}

fun ok64 RDXY($u8 into, $$u8c from) {
    u8 t = ****from & ~TLVaa;
    for ($u8c *p = from[0]; p < from[1]; ++p) {
        u8 n = ***p & ~TLVaa;
        if (n != t) {
            t = ' ';
            break;
        }
    }
    switch (t) {
        case ' ':
        case 'F':
        case 'I':
        case 'R':
        case 'S':
        case 'T':
            return RDXYmergeFIRST(into, from);
        case 'P':
            return RDXYmergeP(into, from);
        case 'L':
            return RDXYmergeL(into, from);
        case 'E':
            return RDXYmergeE(into, from);
        case 'X':
            return RDXYmergeX(into, from);
    }
    return RDXbad;
}

#endif
