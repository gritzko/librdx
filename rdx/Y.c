#include "RDX.h"
#include "abc/PRO.h"

// Y stores tops count in upper 6 bits of flags (lower 2 bits reserved for UTF)
#define Y_TOPS_SHIFT 2
#define YTops(x) ((x)->flags >> Y_TOPS_SHIFT)
#define YTopsSet(x, n) \
    ((x)->flags = (u8)(((n) << Y_TOPS_SHIFT) | ((x)->flags & RDX_UTF_ENC_BITS)))

ok64 rdxNextY(rdxp x) {
    u8 tops = YTops(x);
    sane(x && tops <= rdxgLeftLen(x->ins));
    rdxsp ins = rdxgLeft(x->ins);
    rdxz Z = ZTABLE[x->ptype];
    if (tops == 0) {
        if (rdxsEmpty(ins)) return END;
        tops = rdxsLen(ins);
    }
    a_head(rdx, eqs, ins, tops);
    $rof(rdx, r, eqs) {
        ok64 o = rdxNext(r);
        if (o == OK) {
            rdxsDownAtZ(ins, r - *ins, Z);
        } else if (o == END) {
            rdxSwap(r, rdxsLast(x->ins));
            rdxsShed1(ins);
            if (r < rdxsTerm(ins)) rdxsDownAtZ(ins, r - *ins, Z);
        } else {
            fail(o);
        }
    }
    if ($empty(ins)) {
        YTopsSet(x, 0);
        return END;
    }
    rdxsTopsZ(ins, eqs, Z);
    tops = $len(eqs);
    if (tops > 1) call(rdxsHeapZ, eqs, rdxWinZ);
    rdxMv(x, *eqs);
    YTopsSet(x, tops);  // restore after rdxMv overwrites flags
    done;
}

// multiple child iterators - g use
ok64 rdxIntoY(rdxp c, rdxp p) {
    u8 ptops = YTops(p);
    sane(c && p && ptops && ptops <= rdxgLeftLen(p->ins) && rdxTypePlex(p));
    c->format = RDX_FMT_Y;
    rdxsGauge(rdxgRest(p->ins), c->ins);
    c->ptype = p->type;
    a_head(rdx, eqs, p->ins, ptops);
    rdxs wins = {};
    rdxsTopsZ(eqs, wins, rdxWinZ);
    p->loc = rdxsLen(wins);  // save wins count for rdxOutoY
    u8 seek_type = c->type;
    $for(rdx, w, wins) {
        rdxp cc = 0;
        call(rdxgFedP, c->ins, &cc);
        rdxMv(cc, c);
        ok64 io = rdxInto(cc, w);
        if (io != OK) {
            // Element not found in this input - remove from gauge
            rdxgShed1(c->ins);
        }
    }

    rdxsp ins = rdxgLeft(c->ins);
    if (rdxsEmpty(ins)) {
        YTopsSet(c, 0);
        return NONE;
    }

    if (!seek_type) {
        // No search key: leave children "before first", let rdxNextY do initial
        // merge
        YTopsSet(c, 0);
        c->type = 0;
        done;
    }

    // Merge current positions of children (they're already positioned by
    // rdxInto)
    rdxz Z = ZTABLE[c->ptype];
    rdxs tops = {};
    rdxsTopsZ(ins, tops, Z);
    u8 topslen = rdxsLen(tops);
    if (topslen > 1) call(rdxsHeapZ, tops, rdxWinZ);
    rdxMv(c, *tops);
    YTopsSet(c, topslen);  // restore after rdxMv overwrites flags
    done;
}

ok64 rdxOutoY(rdxp c, rdxp p) {
    (void)c;
    sane(p);
    // Pop skip stacks pushed by rdxInto during rdxIntoY
    // Children may have been shed by rdxNextY, so walk parent's wins
    u8 nwins = (u8)p->loc;
    a_head(rdx, wins, p->ins, nwins);
    $for(rdx, w, wins) {
        rdxOuto(NULL, w);
    }
    done;
}
