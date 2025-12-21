#include "RDX.h"
#include "abc/PRO.h"

ok64 rdxNextY(rdxp x) {
    sane(x && x->len <= rdxgLeftLen(x->ins));
    rdxsp ins = rdxgLeft(x->ins);
    rdxz Z = ZTABLE[x->ptype];
    if (x->len == 0) {
        if (rdxsEmpty(ins)) return END;
        x->len = rdxsLen(ins);
    }
    a_head(rdx, eqs, ins, x->len);
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
        x->len = 0;
        return END;
    }
    rdxsTopsZ(ins, eqs, Z);
    x->len = $len(eqs);
    if (x->len > 1) call(rdxsHeapZ, eqs, rdxWinZ);
    rdxMv(x, *eqs);
    done;
}

// multiple child iterators - g use
ok64 rdxIntoY(rdxp c, rdxp p) {
    sane(c && p && p->len && p->len <= rdxgLeftLen(p->ins) && rdxTypePlex(p));
    c->format = RDX_FMT_Y;
    rdxgDiv(c->ins, rdxgRest(p->ins));
    c->ptype = p->type;
    a_head(rdx, eqs, p->ins, p->len);
    rdxs wins = {};
    rdxsTopsZ(eqs, wins, rdxWinZ);
    $for(rdx, p, wins) {
        rdxp cc = 0;
        call(rdxgFedP, c->ins, &cc);
        rdxMv(cc, c);
        call(rdxInto, cc, p);
    }
    c->len = 0;
    done;
}

ok64 rdxOutoY(rdxp c, rdxp p) {
    sane(c && p);
    // in fact, Outo is optional for reads;
    done;
}
