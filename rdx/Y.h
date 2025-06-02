#ifndef ABC_Y_H
#define ABC_Y_H
#include "RDX.h"
#include "Z.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/LSM.h"
#include "abc/OK.h"
#include "abc/TLV.h"
#include "abc/ZINT.h"

#define Y_MAX_INPUTS LSM_MAX_INPUTS

static const ok64 Ybadlit = 0x229a5a30b78;

fun ok64 Yone($u8 into, $$u8c inputs);

fun ok64 YmergeF($u8 into, $$u8c from) {
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

fun ok64 YmergeI($u8 into, $$u8c from) {
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
        fprintf(stderr, "YmergeI %li %li\n", val, max);
    }
    call($u8feed, into, res);
    done;
}

fun ok64 YmergeR($u8 into, $$u8c from) {
    sane($ok(into) && $ok(from) && !$empty(from));
    RDXref max = {};
    $u8c res = {};
    call(ZINTu128drain, &max, **from);
    $mv(res, **from);
    ++*from;
    $eat(from) {
        RDXref val;
        call(ZINTu128drain, &val, **from);
        if (id128z(&max, &val) < 0) {
            max = val;
            $mv(res, **from);
        }
    }
    call($u8feed, into, res);
    done;
}

fun ok64 YmergeS($u8 into, $$u8c from) {
    sane($ok(into) && $ok(from) && !$empty(from));
    $u8c res = {};
    $mv(res, **from);
    ++*from;
    $eat(from) {
        if ($u8cz(&res, &**from) < 0) {
            $mv(res, **from);
        }
    }
    call($u8feed, into, res);
    done;
}

fun ok64 YmergeT($u8 into, $$u8c from) { return YmergeS(into, from); }

fun ok64 YmergeP($u8 into, $$u8c bares) {
    sane($ok(into) && $ok(bares));
    aBpad2($u8c, y, Y_MAX_INPUTS);

    do {
        Breset(ybuf);
        RDX top = {};
        a$dup($u8c, bare, bares);
        $eat(bare) {
            if ($empty(**bare)) continue;
            $u8c rec = {};
            call(TLVdrain$, rec, **bare);
            RDX next = {};
            call(RDXparse$, &next, rec);
            if ($empty(ydata)) {
                $$u8cfeed1(yidle, rec);
                top = next;
                continue;
            }
            z32 z = Zlww(&top, &next);
            if (z == z32lt) {
                Breset(ybuf);
                top = next;
                $$u8cfeed1(yidle, rec);
            } else if (z == z32eq && RDXisPLEX(top.lit)) {
                $$u8cfeed1(yidle, rec);
            }
        }
        if ($len(ydata) == 1) {
            $u8feedall(into, **ydata);
        } else if ($len(ydata) > 1) {
            call(Yone, into, ydata);
        }
    } while (!$empty(ydata));

    done;
}

fun ok64 YmergeL($u8 into, $$u8c bares) {
    return LSMmerge(into, bares, Zlinear$, Yone);
}

fun ok64 YmergeE($u8 into, $$u8c bare) {
    return LSMmerge(into, bare, Zeuler$, Yone);
}

fun ok64 YmergeX($u8 into, $$u8c bare) {
    return LSMmerge(into, bare, Zmultix$, Yone);
}

fun ok64 Yone($u8 into, $$u8c inputs) {
    sane($ok(into) && $ok(inputs));
    if ($empty(inputs)) done;
    u8 lit = TLVup(****inputs);
    $u8c key = {};  // TODO prettify
    aBpad2($u8c, bare, Y_MAX_INPUTS);
    a$dup($u8c, ins, inputs);
    $eat(ins) {
        u8 l = 0;
        $u8c bare = {};
        call(TLVdrainkv, &l, key, bare, **ins);
        if (lit != l) {
            fprintf(stderr, "bad lit\n");
        }
        test(lit == l, Ybadlit);
        call($$u8cfeed1, bareidle, bare);
    }
    u32* len = nil;
    call(TLVopen, into, lit, &len);
    call($u8feed1, into, $len(key));
    call($u8feedall, into, key);  // FIXME WRONG
    switch (lit) {
        case RDX_FLOAT:
            call(YmergeF, into, baredata);
            break;
        case RDX_INT:
            call(YmergeI, into, baredata);
            break;
        case RDX_REF:
            call(YmergeR, into, baredata);
            break;
        case RDX_STRING:
            call(YmergeS, into, baredata);
            break;
        case RDX_TERM:
            call(YmergeT, into, baredata);
            break;
        case RDX_TUPLE:
            call(YmergeP, into, baredata);
            break;
        case RDX_LINEAR:
            call(LSMmerge, into, baredata, Zlinear$, Yone);
            break;
        case RDX_EULER:
            call(LSMmerge, into, baredata, Zeuler$, Yone);
            break;
        case RDX_MULTIX:
            call(LSMmerge, into, baredata, Zmultix$, Yone);
            break;
        default:
            fail(RDXbad);
    }

    call(TLVclose, into, lit, &len);
    done;
}

#endif
