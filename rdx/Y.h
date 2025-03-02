#ifndef ABC_Y_H
#define ABC_Y_H
#include "RDX.h"
#include "RDXZ.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/LSM.h"
#include "abc/OK.h"
#include "abc/TLV.h"
#include "abc/ZINT.h"

#define Y_MAX_INPUTS LSM_MAX_INPUTS

fun ok64 Y($u8 into, $$u8c from);

fun pro(YmergeF, $u8 into, $$u8c from) {
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

fun pro(YmergeI, $u8 into, $$u8c from) {
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

fun pro(YmergeR, $u8 into, $$u8c from) {
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

fun pro(YmergeS, $u8 into, $$u8c from) {
    sane($ok(into) && $ok(from) && !$empty(from));
    $u8c res = {};
    $mv(res, **from);
    ++*from;
    $eat(from) {
        if ($u8ccmp(&res, &**from) < 0) {
            $mv(res, **from);
        }
    }
    call($u8feed, into, res);
    done;
}

fun ok64 YmergeT($u8 into, $$u8c from) { return YmergeS(into, from); }

fun pro(YmergeP, $u8 into, $$u8c bare) {
    sane($ok(into) && $ok(bare));

    while (1) {
        aBpad2($u8c, yputs, Y_MAX_INPUTS);
        for (size_t i = 0; i < $len(bare); ++i) {
            u8c$ n = (u8c$)$$u8catp(bare, i);
            if ($empty(n)) continue;
            $u8c rec = {};
            call(TLVdrain$, rec, n);
            $$u8cfeed1(yputsidle, rec);
        }
        if ($empty(yputsdata)) break;
        call(Y, into, yputsdata);
    }

    done;
}

fun pro(YmergeL, $u8 into, $$u8c from) {
    sane($ok(into) && $ok(from));
    // TODO injects
    // TODO step wise
    fail(notimplyet);
    done;
}

fun pro(YmergeE, $u8 into, $$u8c bare) {
    return LSMmerge(into, bare, RDXZvalue, Y);
}

fun ok64 YmergeX($u8 into, $$u8c bare) {
    return LSMmerge(into, bare, RDXZauthor, Y);
}

fun pro(Y, $u8 into, $$u8c inputs) {
    sane($ok(into) && $ok(inputs));
    aBpad2($u8c, bares, Y_MAX_INPUTS);
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
    ZINTu128feed(ididle, &maxid);
    call($u8feed1, into, $len(iddata));
    call($u8feed, into, iddata);

    switch (maxt) {
        case RDX_FLOAT:
            call(YmergeF, into, baresdata);
            break;
        case RDX_INT:
            call(YmergeI, into, baresdata);
            break;
        case RDX_REF:
            call(YmergeR, into, baresdata);
            break;
        case RDX_STRING:
            call(YmergeS, into, baresdata);
            break;
        case RDX_TERM:
            call(YmergeT, into, baresdata);
            break;
        case RDX_TUPLE:
            call(YmergeP, into, baresdata);
            break;
        case RDX_LINEAR:
            call(YmergeL, into, baresdata);
            break;
        case RDX_EULER:
            call(YmergeE, into, baresdata);
            break;
        case RDX_MULTIX:
            call(YmergeX, into, baresdata);
            break;
        default:
            fail(RDXbad);
    }

    call(TLVclose, into, maxt, &len);
    done;
}

#endif
