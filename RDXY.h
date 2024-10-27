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

fun pro(RDXYmergeF, $u8 into, $$u8c from) {
    sane($ok(into) && $ok(from) && !$empty(from));
    double max = 0;
    $u8c res = {};
    call(ZINTf64drain, &max, **from);
    $mv(res, **from);
    ++*from;
    $eat(from) {
        double val;
        call(ZINTf64drain, &val, **from);
        if (val > max) {
            max = val;
            $mv(res, **from);
        }
    }
    call($u8feed, into, res);
    done;
}

fun pro(RDXYmergeI, $u8 into, $$u8c from) {
    sane($ok(into) && $ok(from) && !$empty(from));
    RDXint max = 0;
    $u8c res = {};
    call(ZINTi64drain, &max, **from);
    $mv(res, **from);
    ++*from;
    $eat(from) {
        RDXint val;
        call(ZINTi64drain, &val, **from);
        if (val > max) {
            max = val;
            $mv(res, **from);
        }
    }
    call($u8feed, into, res);
    done;
}

fun pro(RDXYmergeR, $u8 into, $$u8c from) {
    sane($ok(into) && $ok(from) && !$empty(from));
    RDXref max = {};
    $u8c res = {};
    call(ZINTu128drain, &max, **from);
    $mv(res, **from);
    ++*from;
    $eat(from) {
        RDXref val;
        call(ZINTu128drain, &val, **from);
        if (id128cmp(&max, &val) < 0) {
            max = val;
            $mv(res, **from);
        }
    }
    call($u8feed, into, res);
    done;
}

fun pro(RDXYmergeS, $u8 into, $$u8c from) {
    sane($ok(into) && $ok(from) && !$empty(from));
    $u8c res = {};
    $mv(res, **from);
    ++*from;
    $eat(from) {
        if ($cmp(res, from) < 0) {
            $mv(res, **from);
        }
    }
    call($u8feed, into, res);
    done;
}

fun ok64 RDXYmergeT($u8 into, $$u8c from) { return RDXYmergeS(into, from); }

fun pro(RDXYmergeP, $u8 into, $$u8c bare) {
    sane($ok(into) && $ok(bare));

    while (1) {
        aBpad2($u8c, yputs, RDXY_MAX_INPUTS);
        for (size_t i = 0; i < $len(bare); ++i) {
            u8c$ n = (u8c$)$$u8catp(bare, i);
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

fun pro(RDXYmergeL, $u8 into, $$u8c from) {
    sane($ok(into) && $ok(from));
    // TODO injects
    // TODO step wise
    fail(notimplyet);
    done;
}

fun pro(RDXYmergeE, $u8 into, $$u8c bare) {
    return LSMmerge(into, bare, RDXZvalue, RDXY);
}

fun ok64 RDXYmergeX($u8 into, $$u8c bare) {
    return LSMmerge(into, bare, RDXZauthor, RDXY);
}

fun pro(RDXY, $u8 into, $$u8c inputs) {
    sane($ok(into) && $ok(inputs));
    aBpad2($u8c, bares, RDXY_MAX_INPUTS);
    a$dup($u8c, ins, inputs);
    u8 maxt = 0;
    u128 maxid = {};
    $eat(ins) {
        u8 t = 0;
        id128 id = {};
        $u8c bare = {};
        call(RDXdrain, &t, &id, bare, **ins);
        int z = id128cmp(&maxid, &id);
        if (z == 0) z = RDXZlit(&maxt, &t);
        if (z < 0) {
            maxid = id;
            maxt = t;
            Breset(baresbuf);
        }
        if (z <= 0) {
            call($$u8cfeed1, baresidle, bare);
        }
    }

    u32* len = nil;
    call(TLVopen, into, maxt, &len);
    aBcpad(u8, id, 16);
    ZINTu128feed(ididle, maxid);
    call($u8feed1, into, $len(iddata));
    call($u8feed, into, iddata);

    switch (maxt) {
        case 'F':
            call(RDXYmergeF, into, baresdata);
            break;
        case 'I':
            call(RDXYmergeI, into, baresdata);
            break;
        case 'R':
            call(RDXYmergeR, into, baresdata);
            break;
        case 'S':
            call(RDXYmergeS, into, baresdata);
            break;
        case 'T':
            call(RDXYmergeT, into, baresdata);
            break;
        case 'P':
            call(RDXYmergeP, into, baresdata);
            break;
        case 'L':
            call(RDXYmergeL, into, baresdata);
            break;
        case 'E':
            call(RDXYmergeE, into, baresdata);
            break;
        case 'X':
            call(RDXYmergeX, into, baresdata);
            break;
        default:
            fail(RDXbad);
    }

    call(TLVclose, into, maxt, &len);
    done;
}

#endif
