#ifndef ABC_RDXY_H
#define ABC_RDXY_H
#include "01.h"
#include "B.h"
#include "LSM.h"
#include "OK.h"
#include "RDX.h"
#include "RDXZ.h"
#include "TLV.h"
#include "ZINT.h"

#define RDXY_MAX_INPUTS LSM_MAX_INPUTS

fun ok64 RDXY($u8 into, $$u8c from);

fun pro(RDXopenPLEX, $u8 into, u8 lit, u128 id, u32** len) {
    sane($ok(into));
    call(TLVopen, into, lit, len);
    aBcpad(u8, id, 16);
    ZINTu128feed(ididle, id);
    call($u8feed1, into, $len(iddata));
    call($u8feed, into, iddata);
    done;
}

fun ok64 RDXclosePLEX($u8 into, u8 lit, u32** len) {
    return TLVclose(into, lit, len);
}

fun pro(RDXprepPLEX, $$u8c bare, id128* topid, $$u8c inputs) {
    sane($ok(bare) && $ok(inputs));
    a$dup($u8c, from, inputs);
    a$dup($u8c, out, bare);
    u128 maxid = {};
    $eat(from) {
        u8 t = 0;
        id128 id = {};
        $u8c value = {};
        call(RDXdrain, &t, &id, value, **from);
        int z = id128cmp(&maxid, &id);
        if (z < 0) {
            maxid = id;
            $mv(out, bare);
        }
        if (z <= 0) {
            call($$u8cfeed1, out, value);
        }
    }
    $mv(bare, out);
    *topid = maxid;
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
        if (*rec == nil || u128cmp(&max, &id) < 0) {
            max = id;
            $mv(rec, **from);
        }
    }
    if (*rec != nil) call($u8feed, into, rec);
    done;
}

fun pro(RDXYmergeP, $u8 into, $$u8c bare) {
    sane($ok(into) && $ok(bare));

    while (1) {
        aBpad2($u8c, yputs, RDXY_MAX_INPUTS);
        for (size_t i = 0; i < $len(bare); ++i) {
            u8c$ n = $$u8catp(bare, i);
            if ($empty(n)) continue;
            $u8c rec = {};
            call(TLVdrain$, rec, n);
            $$u8cfeed1(yputsidle, rec);
        }
        if ($empty(yputsdata)) break;
        call(RDXY, into, yputsdata);
    }

    done;
}

fun pro(RDXYmergeE, $u8 into, $$u8c bare) {
    return LSMmerge(into, bare, RDXZvalue, RDXY);
}

fun ok64 RDXYmergeX($u8 into, $$u8c bare) {
    return LSMmerge(into, bare, RDXZauthor, RDXY);
}

fun pro(RDXYmergePLEX, $u8 into, $$u8c from) {
    sane($ok(into) && $ok(from));
    u8 t = ****from & ~TLVaa;  // TODO

    id128 maxid = {};
    aBpad2($u8c, bare, RDXY_MAX_INPUTS);
    call(RDXprepPLEX, bareidle, &maxid, from);

    u32* len = nil;
    call(RDXopenPLEX, into, t, maxid, &len);

    switch (t) {
        case 'P':
            call(RDXYmergeP, into, baredata);
            break;
        case 'L':
            call(RDXYmergeL, into, baredata);
            break;
        case 'E':
            call(RDXYmergeE, into, baredata);
            break;
        case 'X':
            call(RDXYmergeX, into, baredata);
            break;
        default:
            fail(RDXbad);
    }

    call(RDXclosePLEX, into, t, &len);
    done;
}

fun ok64 RDXY($u8 into, $$u8c from) {
    u8 t = ****from & ~TLVaa;
    b8 same = YES;
    for ($u8c* p = from[0]; same && p < from[1]; ++p) {
        u8 n = ***p & ~TLVaa;
        same &= (n == t);
    }
    if (same == NO || RDXisFIRST(t)) {
        return RDXYmergeFIRST(into, from);
    } else {
        return RDXYmergePLEX(into, from);
    }
}

#endif
