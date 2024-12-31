
#include "JDR.h"

#include <stdint.h>

#include "JDR.rl.h"
#include "RDX.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/OK.h"
#include "abc/PRO.h"
#include "abc/UTF8.h"
#include "abc/ZINT.h"

pro(_JDRfeedS, JDRstate* state) {
    sane(state != nil);
    aBcpad(u8, id, 64);
    call(ZINTu128feed, ididle, state->id);
    u32* len = nil;
    call(TLVopen, state->tlv, RDX_STRING, &len);
    call($u8feed1, state->tlv, $len(iddata));
    call($u8feed, state->tlv, iddata);
    call(JDRfeedSesc, state->tlv, state->val);
    call(TLVclose, state->tlv, RDX_STRING, &len);
    done;
}

ok64 JDRonString($cu8c tok, JDRstate* state) {
    state->lit = RDX_STRING;
    state->val[0] = tok[0] + 1;
    state->val[1] = tok[1] - 1;
    return OK;
}

ok64 JDRonMLString($cu8c tok, JDRstate* state) {
    state->lit = RDX_STRING;
    state->val[0] = tok[0] + 3;
    state->val[1] = tok[1] - 3;
    return OK;
}

#define I64_MAX INT64_MAX
#define I64_MIN INT64_MIN
#define I64_MIN_ABS (1 + 0x7fffffffffffffffUL)

pro(_JDRfeedI, JDRstate* state) {
    sane(state != nil);
    a$dup(u8c, dec, state->val);
    u64 x;
    i64 y;
    if (**dec == '-') {
        ++*dec;
        call(u64decdrain, &x, dec);
        test(x <= I64_MIN_ABS, JDRbad);
        y = -x;
    } else {
        call(u64decdrain, &x, dec);
        test(x <= I64_MAX, JDRbad);
        y = x;
    }
    u64 bits = ZINTzigzag(y);
    aBcpad(u8, i, 8);
    aBcpad(u8, id, 16);
    ZINTu64feed(iidle, bits);
    call(ZINTu128feed, ididle, state->id);
    call(TLVfeedkv, state->tlv, RDX_INT, iddata, idata);
    done;
}

ok64 JDRonInt($cu8c tok, JDRstate* state) {
    state->lit = RDX_INT;
    $mv(state->val, tok);
    return OK;
}

pro(_JDRfeedF, JDRstate* state) {
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

ok64 JDRonFloat($cu8c tok, JDRstate* state) {
    state->lit = RDX_FLOAT;
    $mv(state->val, tok);
    return OK;
}

pro(_JDRfeedR, JDRstate* state) {
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

ok64 JDRonRef($cu8c tok, JDRstate* state) {
    state->lit = RDX_REF;
    $mv(state->val, tok);
    return OK;
}

pro(_JDRfeedT, JDRstate* state) {
    sane(state != nil && state->nest > 0);
    aBcpad(u8, id, 16);
    call(ZINTu128feed, ididle, state->id);
    call(TLVfeedkv, state->tlv, RDX_TERM, iddata, state->val);
    done;
}

ok64 JDRonTerm($cu8c tok, JDRstate* state) {
    state->lit = RDX_TERM;
    $mv(state->val, tok);
    return OK;
}

ok64 JDRonOpen($cu8c tok, JDRstate* state) {
    sane($ok(tok) && state != nil && state->nest > 0);
    JDRnest* prnt = state->stack + state->nest - 1;
    test((prnt->toks & 1) == 0 && prnt->toks <= UINT32_MAX &&
             state->nest < RDX_MAX_NEST,
         JDRbad);
    ++prnt->toks;
    JDRnest* child = state->stack + state->nest;
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

ok64 JDRonClose($cu8c tok, JDRstate* state) {
    sane($ok(tok) && state != nil && state->nest > 0);
    JDRnest* prnt = state->stack + state->nest - 1;
    if (state->lit != RDX_TUPLE && prnt->lit == RDX_TUPLE) {  // FIXME
        u8 memo = state->lit;
        state->lit = RDX_TUPLE;
        call(JDRonClose, tok, state);
        state->lit = memo;
        prnt = state->stack + state->nest - 1;
    }
    test(prnt->lit == state->lit, JDRbad);
    call(TLVclose, state->tlv, prnt->lit, &prnt->l);
    zero(*prnt);
    --state->nest;
    done;
}

ok64 JDRonFIRST($cu8c tok, JDRstate* state) {
    sane(state != nil && state->nest > 0 && $ok(tok));
    JDRnest* prnt = state->stack + state->nest - 1;
    test((prnt->toks & 1) == 0 && prnt->toks <= UINT32_MAX, JDRbad);

    if (tok[1] < state->text[1] && *tok[1] == ':' && prnt->lit != RDX_TUPLE) {
        u8 memo = state->lit;
        state->lit = RDX_TUPLE;
        call(JDRonOpen, tok, state);
        state->lit = memo;
        prnt = state->stack + state->nest - 1;
    }

    ++prnt->toks;
    switch (state->lit) {
        case RDX_FLOAT:
            call(_JDRfeedF, state);
            break;
        case RDX_INT:
            call(_JDRfeedI, state);
            break;
        case RDX_REF:
            call(_JDRfeedR, state);
            break;
        case RDX_STRING:
            call(_JDRfeedS, state);
            break;
        case RDX_TERM:
            call(_JDRfeedT, state);
            break;
    }
    zero(state->val);
    zero(state->id);
    zero(state->lit);
    done;
}

ok64 JDRonOpenP($cu8c tok, JDRstate* state) {
    state->lit = RDX_TUPLE;
    return OK;
}

ok64 JDRonCloseP($cu8c tok, JDRstate* state) {
    state->lit = RDX_TUPLE;
    return OK;
}

ok64 JDRonOpenL($cu8c tok, JDRstate* state) {
    state->lit = RDX_LINEAR;
    return OK;
}

ok64 JDRonCloseL($cu8c tok, JDRstate* state) {
    state->lit = RDX_LINEAR;
    return OK;
}

ok64 JDRonOpenE($cu8c tok, JDRstate* state) {
    state->lit = RDX_EULER;
    return OK;
}

ok64 JDRonCloseE($cu8c tok, JDRstate* state) {
    state->lit = RDX_EULER;
    return OK;
}

ok64 JDRonOpenX($cu8c tok, JDRstate* state) {
    state->lit = RDX_MULTIX;
    return OK;
}

ok64 JDRonCloseX($cu8c tok, JDRstate* state) {
    state->lit = RDX_MULTIX;
    return OK;
}

ok64 JDRonColon($cu8c tok, JDRstate* state) {
    sane($ok(tok) && state != nil);
    JDRnest* prnt = state->stack + state->nest - 1;
    test((prnt->toks & 1) == 1 && prnt->toks <= UINT32_MAX, JDRbad);
    ++prnt->toks;
    test(prnt->lit == RDX_TUPLE, JDRbad);
    done;
}

ok64 JDRonComma($cu8c tok, JDRstate* state) {
    sane($ok(tok) && state != nil && state->nest > 0);
    JDRnest* prnt = state->stack + state->nest - 1;
    if (prnt->lit == RDX_TUPLE) {
        call(TLVclose, state->tlv, prnt->lit, &prnt->l);
        zero(*prnt);
        --state->nest;
        prnt = state->stack + state->nest - 1;
        test(prnt->lit != RDX_TUPLE, JDRbad);
    }
    test((prnt->toks & 1) == 1 && prnt->toks <= UINT32_MAX, JDRbad);
    ++prnt->toks;
    done;
}

ok64 JDRonStamp($cu8c tok, JDRstate* state) {
    sane($ok(tok) && $len(tok) >= 2 && **tok == '@' && state != nil);
    $u8c id = {tok[0] + 1, tok[1]};
    call(RDXid128drain, &state->id, id);
    done;
}

pro(JDRonRoot, $cu8c tok, JDRstate* state) {
    sane($ok(tok) && state != nil);
    if (state->nest == 2 && state->stack[1].lit == RDX_TUPLE) {
        state->lit = RDX_TUPLE;
        call(JDRonClose, tok, state);
    }
    test(state->nest == 1, RDXbad);
    done;
}

ok64 JDRonInter($cu8c tok, JDRstate* state) { return OK; }

ok64 JDRonOpenVector($cu8c tok, JDRstate* state) { return OK; }
ok64 JDRonCloseVector($cu8c tok, JDRstate* state) { return OK; }

fun b8 id128empty(id128 id) { return id._64[0] == 0 && id._64[1] == 0; }

fun ok64 JDRfeedstamp($u8 rdxj, id128 stamp) {
    if (id128empty(stamp)) return OK;
    // call($u8feed2, rdxj, ' ', '@');
    if ($len(rdxj) < 2) return RDXnospace;
    $u8feed1(rdxj, '@');
    return RDXid128feed(rdxj, stamp);
}

pro(_JDRfeed, $u8 rdxj, $u8c tlv, u8 parent) {
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
            call(JDRfeedstamp, rdxj, id);
            break;
        case RDX_FLOAT:
            call(RDXFtlv2txt, rdxj, tlv2);
            call(JDRfeedstamp, rdxj, id);
            break;
        case RDX_REF:
            call(RDXRtlv2txt, rdxj, tlv2);
            call(JDRfeedstamp, rdxj, id);
            break;
        case RDX_STRING:
            call($u8feed1, rdxj, '"');
            call(JDRdrainSesc, rdxj, tlv2);
            call($u8feed1, rdxj, '"');
            call(JDRfeedstamp, rdxj, id);
            break;
        case RDX_TERM:
            call($u8feed, rdxj, value);
            call(JDRfeedstamp, rdxj, id);
            break;
        case RDX_LINEAR:
            call($u8feed1, rdxj, '[');
            call(JDRfeedstamp, rdxj, id);
            while (!$empty(value)) {
                call(_JDRfeed, rdxj, value, RDX_LINEAR);
                if (!$empty(value)) call($u8feed1, rdxj, ',');
            }
            call($u8feed1, rdxj, ']');
            break;
        case RDX_TUPLE: {
            b8 brackets = (parent == RDX_TUPLE || !id128empty(id));
            if (brackets) {
                call($u8feed1, rdxj, '<');
                if (!id128empty(id)) {
                    call(JDRfeedstamp, rdxj, id);
                    call($u8feed1, rdxj, ' ');
                }
            }
            while (!$empty(value)) {
                call(_JDRfeed, rdxj, value, RDX_TUPLE);
                if (!$empty(value)) call($u8feed1, rdxj, ':');
            }
            if (brackets) call($u8feed1, rdxj, '>');
            break;
        }
        case RDX_EULER:
            call($u8feed1, rdxj, '{');
            while (!$empty(value)) {
                call(_JDRfeed, rdxj, value, RDX_EULER);
                if (!$empty(value)) call($u8feed1, rdxj, ',');
            }
            call($u8feed1, rdxj, '}');
            break;
        case RDX_MULTIX:
            call($u8feed1, rdxj, '(');
            while (!$empty(value)) {
                call(_JDRfeed, rdxj, value, RDX_MULTIX);
                if (!$empty(value)) call($u8feed1, rdxj, ',');
            }
            call($u8feed1, rdxj, ')');
            break;
    }
    done;
}
