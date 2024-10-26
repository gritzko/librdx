#ifndef ABC_RDXY_H
#define ABC_RDXY_H
#include "B.h"
#include "LSM.h"
#include "OK.h"
#include "RDX.h"
#include "RDXZ.h"
#include "TLV.h"
#include "ZINT.h"

#define RDXY_MAX_INPUTS LSM_MAX_INPUTS

fun ok64 RDXY($u8 into, $$u8c from);

fun pro(RDXYmergeFIRST, $u8 into, $$u8c from) {
    sane($ok(into) && $ok(from));
    u128 max = {};
    $u8c rec = {};
    u8 t = {};
    u128 id = {};
    $u8c value = {};
    $eat(from) {
        $u8c p = $dup(**from);
        call(RDXdrain, &t, &id, value, p);
        if (u128cmp(&max, &id) < 0) {
            max = id;
            $mv(rec, **from);
        }
    }
    if (*rec != nil) call($u8feed, into, rec);
    done;
}

fun pro(RDXYmergeP, $u8 into, $$u8c from) {
    sane($ok(into) && $ok(from));
    id128 maxid = {};
    aBpad2($u8c, ins, RDXY_MAX_INPUTS);
    $eat(from) {
        u8 t = 0;
        id128 id = {};
        $u8c value = {};
        call(RDXdrain, &t, &id, value, **from);
        int z = id128cmp(&maxid, &id);
        if (z < 0) {
            maxid = id;
            Breset(insbuf);
        }
        if (z <= 0) {
            call($$u8cfeed1, insidle, value);
        }
    }

    u32* len = nil;
    call(TLVopen, into, RDX_TUPLE, &len);
    aBcpad(u8, id, 16);
    ZINTu128feed(ididle, maxid);
    call($u8feed1, into, $len(iddata));
    call($u8feed, into, iddata);

    while (1) {
        aBpad2($u8c, yputs, RDXY_MAX_INPUTS);
        for (size_t i = 0; i < $len(insdata); ++i) {
            u8c$ n = $$u8catp(insdata, i);
            if ($empty(n)) continue;
            $u8c rec = {};
            call(TLVdrain$, rec, n);
            $$u8cfeed1(yputsidle, rec);
        }
        if ($empty(yputsdata)) break;
        call(RDXY, into, yputsdata);
    }
    call(TLVclose, into, RDX_TUPLE, &len);
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
    for ($u8c* p = from[0]; p < from[1]; ++p) {
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
