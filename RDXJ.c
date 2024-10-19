
#include "RDXJ.h"

#include "01.h"
#include "B.h"
#include "OK.h"
#include "PRO.h"
#include "RDX.h"
#include "RDX1.h"
#include "RDXJ.rl.h"
#include "ZINT.h"

ok64 RDXJonFIRST($cu8c tok, RDXJstate* state) {
    sane(state != nil && !Bempty(state->stack));
    rdx64* prnt = (rdx64*)Blastp(state->stack);
    switch (prnt->node + 'A') {
        case RDX_MAP:
        case RDX_LINEAR:
            test((prnt->toks & 1) == 0, RDXJbad);
            break;
        case RDX_Z:
        case RDX_NAT:
            test((prnt->toks & 1) == 0, RDXJbad);
            test(state->lit == RDX_INT, RDXJbad);
            break;
        case 0:
    }
    ++prnt->toks;
    aBcpad(u8, id, 64);
    call(ZINTu128feed, ididle, state->id);
    call(TLVfeedkv, Bu8idle(state->tlv), state->lit, iddata,
         Bu8cdata(state->pad));
    Breset(state->pad);
    zero(state->id);
    done;
}

ok64 RDXJonOpen($cu8c tok, RDXJstate* state, int node) {
    sane($ok(tok) && state != nil && !Bempty(state->stack));
    rdx64* prnt = (rdx64*)Blastp(state->stack);
    test((prnt->toks & 1) == 0, RDXJbad);
    ++prnt->toks;
    rdx64 child = {.node = node - 'A'};
    call(TLVopen, state->tlv, node, &child.tlvpos);
    call($u8feed1, Bu8idle(state->tlv), 0);  // id=0
    call(Bu64feedp, state->stack, (u64*)&child);
    done;
}

ok64 RDXJonClose($cu8c tok, RDXJstate* state, int node) {
    sane($ok(tok) && state != nil && !Bempty(state->stack));
    rdx64* prnt = (rdx64*)Blastp(state->stack);
    // test((prnt->toks & 1) == 0, RDXJbad);  // TODO
    test(prnt->node + 'A' == node, RDXJbad);
    call(TLVclose, state->tlv, prnt->node + 'A', &prnt->tlvpos);
    Bu64pop(state->stack);
    done;
}

// particularities
//
ok64 RDXJonUTF8($cu8c tok, RDXJstate* state) {
    return $u8feed(Bu8idle(state->pad), tok);
}

ok64 RDXJonString($cu8c tok, RDXJstate* state) {
    sane($ok(tok) && state != nil && !Bempty(state->stack));
    // TODO state->lit state->pad
    state->lit = RDX_STRING;
    done;
}

ok64 RDXJonInt($cu8c tok, RDXJstate* state) {
    sane($ok(tok) && state != nil && !Bempty(state->stack));
    u64 x;
    call(u64decdrain, &x, tok);
    i64 y = x;
    u64 z = ZINTzigzag(y);
    ZINTu64feed(Bu8idle(state->pad), z);
    state->lit = RDX_INT;
    done;
}

ok64 RDXJonFloat($cu8c tok, RDXJstate* state) {
    sane($ok(tok) && state != nil && !Bempty(state->stack));
    u8c* e = 0;
    double d = strtod((const char*)tok[0], (char**)&e);
    u64 bits = *(u64*)&d;
    ZINTu64feed(Bu8idle(state->pad), flip64(bits));  // FIXME nospace?!!
    state->lit = RDX_FLOAT;
    done;
}

ok64 RDXJonRef($cu8c tok, RDXJstate* state) {
    sane($ok(tok) && state != nil && !Bempty(state->stack));
    id128 id;
    call(RDXid128drain, &id, tok);
    call(ZINTu128feed, Bu8idle(state->pad), id);
    state->lit = RDX_REF;
    done;
}

ok64 RDXJonTerm($cu8c tok, RDXJstate* state) {
    sane($ok(tok) && state != nil && !Bempty(state->stack));
    call($u8feed, Bu8idle(state->pad), tok);
    state->lit = RDX_TERM;
    done;
}

ok64 RDXJonOpenObject($cu8c tok, RDXJstate* state) {
    return RDXJonOpen(tok, state, RDX_MAP);
}

ok64 RDXJonCloseObject($cu8c tok, RDXJstate* state) {
    return RDXJonClose(tok, state, RDX_MAP);
}

ok64 RDXJonOpenArray($cu8c tok, RDXJstate* state) {
    return RDXJonOpen(tok, state, RDX_LINEAR);
}

ok64 RDXJonCloseArray($cu8c tok, RDXJstate* state) {
    return RDXJonClose(tok, state, RDX_LINEAR);
}

ok64 RDXJonComma($cu8c tok, RDXJstate* state) {
    sane($ok(tok) && state != nil);
    rdx64* prnt = (rdx64*)Blastp(state->stack);
    switch (prnt->node + 'A') {
        case RDX_LINEAR:
        case RDX_SET:
        case RDX_NAT:
        case RDX_Z:
        case 'A':  // FIXME ROOT
            test((prnt->toks & 1) == 1, RDXJbad);
            break;
        case RDX_MAP:
            test((prnt->toks & 3) == 3, RDXJbad);
            break;
        default:
            fail(RDXJbad);
    }
    ++prnt->toks;
    done;
}

ok64 RDXJonColon($cu8c tok, RDXJstate* state) {
    sane($ok(tok) && state != nil);
    rdx64* prnt = (rdx64*)Blastp(state->stack);
    switch (prnt->node + 'A') {
        case RDX_MAP:
            test((prnt->toks & 3) == 1, RDXJbad);
            break;
        default:
            fail(RDXJbad);
    }
    ++prnt->toks;
    done;
}

ok64 RDXJonStamp($cu8c tok, RDXJstate* state) {
    sane($ok(tok) && $len(tok) >= 2 && **tok == '@' && state != nil);
    $u8c id = {tok[0] + 1, tok[1]};
    call(RDXid128drain, &state->id, id);
    done;
}

ok64 RDXJonRDXJ($cu8c tok, RDXJstate* state) { return OK; }

ok64 RDXJonRoot($cu8c tok, RDXJstate* state) { return OK; }

ok64 RDXJonEsc($cu8c tok, RDXJstate* state) { return OK; }
ok64 RDXJonHexEsc($cu8c tok, RDXJstate* state) { return OK; }
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

pro(RDXJfromTLV, $u8 rdxj, $u8c tlv) {
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
            call(RDXStlv2txt, rdxj, tlv2);
            call(RDXJfeedstamp, rdxj, id);
            break;
        case RDX_LINEAR:
            call($u8feed1, rdxj, '[');
            while (!$empty(value)) {
                call(RDXJfromTLV, rdxj, value);
                if (!$empty(value)) call($u8feed1, rdxj, ',');
            }
            call($u8feed1, rdxj, ']');
            break;
        case RDX_MAP:
            call($u8feed1, rdxj, '{');
            int i = 0;
            u8 sgn[2] = {':', ','};
            while (!$empty(value)) {
                call(RDXJfromTLV, rdxj, value);
                if (!$empty(value)) call($u8feed1, rdxj, sgn[i & 1]);
                ++i;
            }
            call($u8feed1, rdxj, '}');
            break;
    }
    done;
}
