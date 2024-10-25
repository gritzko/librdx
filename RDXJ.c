
#include "RDXJ.h"

#include <stdint.h>

#include "01.h"
#include "B.h"
#include "OK.h"
#include "PRO.h"
#include "RDX.h"
#include "RDXJ.rl.h"
#include "ZINT.h"

pro(_RDXJfeedS, RDXJstate* state) {
    sane(state != nil);
    aBcpad(u8, id, 64);
    call(ZINTu128feed, ididle, state->id);
    u32* len = nil;
    call(TLVopen, state->tlv, RDX_STRING, &len);
    call($u8feed1, state->tlv, $len(iddata));
    call($u8feed, state->tlv, iddata);
    call(RDXJfeedSesc, state->tlv, state->val);
    call(TLVclose, state->tlv, RDX_STRING, &len);
    done;
}

ok64 RDXJonString($cu8c tok, RDXJstate* state) {
    state->lit = RDX_STRING;
    state->val[0] = tok[0] + 1;
    state->val[1] = tok[1] - 1;
    return OK;
}

pro(_RDXJfeedI, RDXJstate* state) {
    sane(state != nil);
    // TODO neg
    u64 x;
    call(u64decdrain, &x, state->val);
    i64 y = x;
    u64 bits = ZINTzigzag(y);
    aBcpad(u8, i, 8);
    aBcpad(u8, id, 16);
    ZINTu64feed(iidle, bits);
    call(ZINTu128feed, ididle, state->id);
    call(TLVfeedkv, state->tlv, RDX_INT, iddata, idata);
    done;
}

ok64 RDXJonInt($cu8c tok, RDXJstate* state) {
    state->lit = RDX_INT;
    $mv(state->val, tok);
    return OK;
}

pro(_RDXJfeedF, RDXJstate* state) {
    sane(state != nil);
    u8c* e = 0;
    double d = strtod((const char*)state->val[0], (char**)&e);
    u64 bits = flip64(*(u64*)&d);
    aBcpad(u8, i, 8);
    aBcpad(u8, id, 16);
    ZINTu64feed(iidle, bits);
    call(ZINTu128feed, ididle, state->id);
    call(TLVfeedkv, state->tlv, RDX_FLOAT, iddata, idata);
    done;
}

ok64 RDXJonFloat($cu8c tok, RDXJstate* state) {
    state->lit = RDX_FLOAT;
    $mv(state->val, tok);
    return OK;
}

pro(_RDXJfeedR, RDXJstate* state) {
    sane(state != nil && state->nest > 0);
    id128 bits = {};
    call(RDXid128drain, &bits, state->val);
    aBcpad(u8, pad, 16);
    call(ZINTu128feed, padidle, bits);
    aBcpad(u8, id, 16);
    call(ZINTu128feed, ididle, state->id);
    call(TLVfeedkv, state->tlv, RDX_REF, iddata, paddata);
    done;
}

ok64 RDXJonRef($cu8c tok, RDXJstate* state) {
    state->lit = RDX_REF;
    $mv(state->val, tok);
    return OK;
}

pro(_RDXJfeedT, RDXJstate* state) {
    sane(state != nil && state->nest > 0);
    aBcpad(u8, id, 16);
    call(ZINTu128feed, ididle, state->id);
    call(TLVfeedkv, state->tlv, RDX_TERM, iddata, state->val);
    done;
}

ok64 RDXJonTerm($cu8c tok, RDXJstate* state) {
    state->lit = RDX_TERM;
    $mv(state->val, tok);
    return OK;
}

ok64 RDXJonOpen($cu8c tok, RDXJstate* state) {
    sane($ok(tok) && state != nil && state->nest > 0);
    RDXJnest* prnt = state->stack + state->nest - 1;
    test((prnt->toks & 1) == 0 && prnt->toks <= UINT32_MAX &&
             state->nest < RDX_MAX_NEST,
         RDXJbad);
    ++prnt->toks;
    RDXJnest* child = state->stack + state->nest;
    ++state->nest;
    test(RDXisPLEX(state->lit), FAILsanity);
    child->lit = state->lit;
    call(TLVopen, state->tlv, state->lit, &child->l);
    aBcpad(u8, id, 64);
    call(ZINTu128feed, ididle, state->id);
    call($u8feed1, state->tlv, $len(iddata));
    call($u8feed, state->tlv, iddata);
    zero(state->id);
    done;
}

ok64 RDXJonClose($cu8c tok, RDXJstate* state) {
    sane($ok(tok) && state != nil && state->nest > 0);
    RDXJnest* prnt = state->stack + state->nest - 1;
    if (state->lit != RDX_TUPLE && prnt->lit == RDX_TUPLE) {  // FIXME
        u8 memo = state->lit;
        state->lit = RDX_TUPLE;
        call(RDXJonClose, tok, state);
        state->lit = memo;
        prnt = state->stack + state->nest - 1;
    }
    test(prnt->lit == state->lit, RDXJbad);
    call(TLVclose, state->tlv, prnt->lit, &prnt->l);
    zero(*prnt);
    --state->nest;
    done;
}

ok64 RDXJonFIRST($cu8c tok, RDXJstate* state) {
    sane(state != nil && state->nest > 0 && $ok(tok));
    RDXJnest* prnt = state->stack + state->nest - 1;
    test((prnt->toks & 1) == 0 && prnt->toks <= UINT32_MAX, RDXJbad);

    if (tok[1] < state->text[1] && *tok[1] == ':' && prnt->lit != RDX_TUPLE) {
        u8 memo = state->lit;
        state->lit = RDX_TUPLE;
        call(RDXJonOpen, tok, state);
        state->lit = memo;
        prnt = state->stack + state->nest - 1;
    }

    ++prnt->toks;
    ok64 o = OK;
    switch (state->lit) {
        case RDX_FLOAT:
            o = _RDXJfeedF(state);
            break;
        case RDX_INT:
            o = _RDXJfeedI(state);
            break;
        case RDX_REF:
            o = _RDXJfeedR(state);
            break;
        case RDX_STRING:
            o = _RDXJfeedS(state);
            break;
        case RDX_TERM:
            o = _RDXJfeedT(state);
            break;
    }
    zero(state->val);
    zero(state->id);
    zero(state->lit);
    done;
}

ok64 RDXJonOpenP($cu8c tok, RDXJstate* state) {
    state->lit = RDX_TUPLE;
    return OK;
}

ok64 RDXJonCloseP($cu8c tok, RDXJstate* state) {
    state->lit = RDX_TUPLE;
    return OK;
}

ok64 RDXJonOpenL($cu8c tok, RDXJstate* state) {
    state->lit = RDX_LINEAR;
    return OK;
}

ok64 RDXJonCloseL($cu8c tok, RDXJstate* state) {
    state->lit = RDX_LINEAR;
    return OK;
}

ok64 RDXJonOpenE($cu8c tok, RDXJstate* state) {
    state->lit = RDX_EULER;
    return OK;
}

ok64 RDXJonCloseE($cu8c tok, RDXJstate* state) {
    state->lit = RDX_EULER;
    return OK;
}

ok64 RDXJonOpenX($cu8c tok, RDXJstate* state) {
    state->lit = RDX_MULTIX;
    return OK;
}

ok64 RDXJonCloseX($cu8c tok, RDXJstate* state) {
    state->lit = RDX_MULTIX;
    return OK;
}

ok64 RDXJonColon($cu8c tok, RDXJstate* state) {
    sane($ok(tok) && state != nil);
    RDXJnest* prnt = state->stack + state->nest - 1;
    test((prnt->toks & 1) == 1 && prnt->toks <= UINT32_MAX, RDXJbad);
    ++prnt->toks;
    test(prnt->lit == RDX_TUPLE, RDXJbad);
    done;
}

ok64 RDXJonComma($cu8c tok, RDXJstate* state) {
    sane($ok(tok) && state != nil && state->nest > 0);
    RDXJnest* prnt = state->stack + state->nest - 1;
    if (prnt->lit == RDX_TUPLE) {
        call(TLVclose, state->tlv, prnt->lit, &prnt->l);
        zero(*prnt);
        --state->nest;
        prnt = state->stack + state->nest - 1;
        test(prnt->lit != RDX_TUPLE, RDXJbad);
    }
    test((prnt->toks & 1) == 1 && prnt->toks <= UINT32_MAX, RDXJbad);
    ++prnt->toks;
    done;
}

ok64 RDXJonStamp($cu8c tok, RDXJstate* state) {
    sane($ok(tok) && $len(tok) >= 2 && **tok == '@' && state != nil);
    $u8c id = {tok[0] + 1, tok[1]};
    call(RDXid128drain, &state->id, id);
    done;
}

pro(RDXJonRoot, $cu8c tok, RDXJstate* state) {
    sane($ok(tok) && state != nil);
    if (state->nest == 2 && state->stack[1].lit == RDX_TUPLE) {
        state->lit = RDX_TUPLE;
        call(RDXJonClose, tok, state);
    }
    test(state->nest == 1, RDXbad);
    done;
}

ok64 RDXJonInter($cu8c tok, RDXJstate* state) { return OK; }

ok64 RDXJonOpenVector($cu8c tok, RDXJstate* state) { return OK; }
ok64 RDXJonCloseVector($cu8c tok, RDXJstate* state) { return OK; }

fun b8 id128empty(id128 id) { return id._64[0] == 0 && id._64[1] == 0; }

fun ok64 RDXJfeedstamp($u8 rdxj, id128 stamp) {
    if (id128empty(stamp)) return OK;
    // call($u8feed2, rdxj, ' ', '@');
    if ($len(rdxj) < 2) return RDXnospace;
    $u8feed1(rdxj, '@');
    return RDXid128feed(rdxj, stamp);
}

pro(_RDXJfeed, $u8 rdxj, $u8c tlv, u8 parent) {
    sane($ok(rdxj) && $ok(tlv));
    u8 lit;
    $u8c value;
    $u8c idz;
    id128 id = {};
    a$dup(u8c, tlv2, tlv);
    call(TLVdrainkv, &lit, idz, value, tlv);
    call(ZINTu128drain, &id, idz);
    switch (lit) {
        case RDX_INT:
            call(RDXItlv2txt, rdxj, tlv2);
            call(RDXJfeedstamp, rdxj, id);
            break;
        case RDX_FLOAT:
            call(RDXFtlv2txt, rdxj, tlv2);
            call(RDXJfeedstamp, rdxj, id);
            break;
        case RDX_REF:
            call(RDXRtlv2txt, rdxj, tlv2);
            call(RDXJfeedstamp, rdxj, id);
            break;
        case RDX_STRING:
            call($u8feed1, rdxj, '"');
            call(RDXJdrainSesc, rdxj, tlv2);
            call($u8feed1, rdxj, '"');
            call(RDXJfeedstamp, rdxj, id);
            break;
        case RDX_TERM:
            call($u8feed, rdxj, value);
            call(RDXJfeedstamp, rdxj, id);
            break;
        case RDX_LINEAR:
            call($u8feed1, rdxj, '[');
            call(RDXJfeedstamp, rdxj, id);
            while (!$empty(value)) {
                call(_RDXJfeed, rdxj, value, RDX_LINEAR);
                if (!$empty(value)) call($u8feed1, rdxj, ',');
            }
            call($u8feed1, rdxj, ']');
            break;
        case RDX_TUPLE: {
            b8 brackets = (parent == RDX_TUPLE || !id128empty(id));
            if (brackets) {
                call($u8feed1, rdxj, '<');
                if (!id128empty(id)) {
                    call(RDXJfeedstamp, rdxj, id);
                    call($u8feed1, rdxj, ' ');
                }
            }
            while (!$empty(value)) {
                call(_RDXJfeed, rdxj, value, RDX_TUPLE);
                if (!$empty(value)) call($u8feed1, rdxj, ':');
            }
            if (brackets) call($u8feed1, rdxj, '>');
            break;
        }
        case RDX_EULER:
            call($u8feed1, rdxj, '{');
            while (!$empty(value)) {
                call(_RDXJfeed, rdxj, value, RDX_EULER);
                if (!$empty(value)) call($u8feed1, rdxj, ',');
            }
            call($u8feed1, rdxj, '}');
            break;
        case RDX_MULTIX:
            call($u8feed1, rdxj, '(');
            while (!$empty(value)) {
                call(_RDXJfeed, rdxj, value, RDX_MULTIX);
                if (!$empty(value)) call($u8feed1, rdxj, ',');
            }
            call($u8feed1, rdxj, ')');
            break;
    }
    done;
}
