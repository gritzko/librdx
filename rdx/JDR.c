
#include "JDR.h"

#include "LSM.h"
#include "RDX.h"
#include "RDXY.h"
#include "RDXZ.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/BUF.h"
#include "abc/INT.h"
#include "abc/OK.h"
#include "abc/PRO.h"
#include "abc/TLV.h"
#include "abc/UTF8.h"
#include "abc/ZINT.h"

#define RDX_TUPLE_INLINE 'U'

// . . . . . . . p a r s e r . . . . . . .

#define JDRaa (0x20UL << B_MAX_LEN_BITS)

a$u8c(TLV_EMPTY_TUPLE, 'p', 1, 0);

fun ok64 JDRfeedempty(JDRstate* state) {
    state->pre = RDX_TUPLE;
    return $u8feedall(state->tlv, TLV_EMPTY_TUPLE);
}

fun ok64 JDRcloseinline(JDRstate* state) {
    sane(1);
    if (state->pre == ':') call(JDRfeedempty, state);
    **Bu8ptop(state->stack) = RDX_TUPLE;
    state->pre = RDX_TUPLE_INLINE;
    call(TLVcloseany, state->tlv, RDX_TUPLE, state->stack);
    done;
}

ok64 JDRonFIRST0($cu8c tok, JDRstate* state, u8 lit) {
    sane(state != nil);
    if (!Bempty(state->stack) && **Bu8ptop(state->stack) == RDX_TUPLE_INLINE &&
        state->pre != ':') {
        call(JDRcloseinline, state);
    }
    call(TLVopenshort, state->tlv, lit, state->stack);
    $mv(state->val, tok);
    done;
}

ok64 JDRonFloat($cu8c tok, JDRstate* state) {
    return JDRonFIRST0(tok, state, RDX_FLOAT);
}

ok64 JDRonInt($cu8c tok, JDRstate* state) {
    return JDRonFIRST0(tok, state, RDX_INT);
}

ok64 JDRonRef($cu8c tok, JDRstate* state) {
    return JDRonFIRST0(tok, state, RDX_REF);
}

ok64 JDRonString($cu8c tok, JDRstate* state) {
    $u8c str = {tok[0] + 1, tok[1] - 1};
    return JDRonFIRST0(str, state, RDX_STRING);
}

ok64 JDRonMLString($cu8c tok, JDRstate* state) {
    $u8c str = {tok[0] + 1, tok[1] - 1};
    return JDRonFIRST0(str, state, RDX_STRING);
}

ok64 JDRonTerm($cu8c tok, JDRstate* state) {
    return JDRonFIRST0(tok, state, RDX_TERM);
}

ok64 JDRonNoStamp($cu8c tok, JDRstate* state) {
    sane($ok(tok) && state != nil);
    return $u8feed1(state->tlv, 0);
}

ok64 JDRonStamp($cu8c tok, JDRstate* state) {
    sane($ok(tok) && $len(tok) >= 2 && **tok == '@' && state != nil);
    $u8c idstr = {tok[0] + 1, tok[1]};
    id128 id = {};
    call(RDXid128drain, &id, idstr);
    u8* p = $head(state->tlv);
    call($u8feed1, state->tlv, 0);
    call(ZINTu128feed, state->tlv, id);
    *p = $head(state->tlv) - p - 1;
    done;
}

ok64 JDRonFIRST($cu8c tok, JDRstate* state) {
    sane(state != nil && $ok(tok) && !Bempty(state->stack));
    u8 lit = **Bu8ptop(state->stack) & ~TLVaa;
    switch (lit) {
        case RDX_FLOAT: {
            u8c* e = 0;
            double d = strtod((const char*)state->val[0], (char**)&e);
            call(ZINTf64feed, state->tlv, &d);
            break;
        }
        case RDX_INT: {
            i64 i;
            call(i64decdrain, &i, state->val);
            call(ZINTi64feed, state->tlv, &i);
            break;
        }
        case RDX_REF: {
            id128 bits = {};
            call(RDXid128drain, &bits, state->val);
            call(ZINTu128feed, state->tlv, bits);
            break;
        }
        case RDX_STRING: {
            if (**tok == '"') {
                call(JDRfeedSesc, state->tlv, state->val);
            } else if (**tok == '`') {
                call($u8feed, state->tlv, state->val);
            } else {
                fail(FAILsanity);
            }
            break;
        }
        case RDX_TERM: {
            call($u8feed, state->tlv, state->val);
            break;
        }
        default:
            fail(FAILsanity);
    }
    call(TLVcloseany, state->tlv, lit, state->stack);
    state->pre = '1';
    done;
}

fun u8 JDRtop(Bu8p stack) {
    if (Bempty(stack)) return 0;
    return **Bu8ptop(stack);
}

ok64 JDRonPLEX0(u8 lit, JDRstate* state) {
    sane(state != nil);
    if (JDRtop(state->stack) == RDX_TUPLE_INLINE && state->pre != ':')
        call(JDRcloseinline, state);
    state->pre = 0;
    call(TLVopenlong, state->tlv, lit, state->stack);
    done;
}

ok64 JDRonPLEX1(u8 lit, JDRstate* state) {
    sane(state != nil && RDXisPLEX(lit));
    if (**Bu8ptop(state->stack) == RDX_TUPLE_INLINE) {
        call(JDRcloseinline, state);
    } else if (state->pre == ',' || (lit != RDX_TUPLE && state->pre == 0)) {
        call(JDRfeedempty, state);
    }
    state->pre = lit;
    done;
}

ok64 JDRonOpenP($cu8c tok, JDRstate* state) {
    return JDRonPLEX0(RDX_TUPLE, state);
}

ok64 JDRonCloseP($cu8c tok, JDRstate* state) {
    return JDRonPLEX1(RDX_TUPLE, state);
}

ok64 JDRonOpenL($cu8c tok, JDRstate* state) {
    return JDRonPLEX0(RDX_LINEAR, state);
}

ok64 JDRonCloseL($cu8c tok, JDRstate* state) {
    return JDRonPLEX1(RDX_LINEAR, state);
}

ok64 JDRonOpenE($cu8c tok, JDRstate* state) {
    return JDRonPLEX0(RDX_EULER, state);
}

ok64 JDRonCloseE($cu8c tok, JDRstate* state) {
    return JDRonPLEX1(RDX_EULER, state);
}

ok64 JDRonOpenX($cu8c tok, JDRstate* state) {
    return JDRonPLEX0(RDX_MULTIX, state);
}

ok64 JDRonCloseX($cu8c tok, JDRstate* state) {
    return JDRonPLEX1(RDX_MULTIX, state);
}

ok64 JDRonOpen($cu8c tok, JDRstate* state) { return OK; }

fun b8 JDRisP1(JDRstate* state) {
    u8p start = *Bu8ptop(state->stack);
    if (*start != RDX_TUPLE) return NO;
    $u8c inner = {start + 1 + 4 + 1 + start[5], state->tlv[0]}, rec;
    if ($empty(inner)) return NO;
    TLVdrain$(rec, inner);
    return $empty(inner);
}

ok64 JDRsort(JDRstate* state, $u8cZfn cmp, $u8cYfn mrg) {
    sane(state != nil);
    u8p start = *Bu8ptop(state->stack) + 1 + 4;
    start += *start + 1;
    $u8c body = {start, state->tlv[0]};
    u8p mark = state->tlv[0];
    call(LSMsort, state->tlv, body, cmp, mrg);
    $u8c from = {mark, state->tlv[0]};
    $u8c into = {start, start + $len(from)};
    $u8move((u8$)into, from);
    state->tlv[0] = start + $len(from);
    done;
}

ok64 JDRonClose($cu8c tok, JDRstate* state) {
    sane(state != nil);
    u8 lit = state->pre;
    test(JDRtop(state->stack) == lit, FAILsanity);
    if (!RDXisPLEX(lit)) fail(FAILsanity);
    if (lit == RDX_TUPLE && JDRisP1(state)) {
        u8p start = *Bu8ptop(state->stack);
        $u8c from = {start + 1 + 4 + 1, state->tlv[0]};
        $u8 into = {start, state->tlv[0] - 4 - 1 - 1};
        $u8move(into, from);
        state->tlv[0] -= 4 + 1 + 1;
        Bu8ppop(state->stack);
    } else if (lit == RDX_EULER) {
        call(JDRsort, state, RDXZvalue, RDXY);
        call(TLVcloseany, state->tlv, lit, state->stack);
    } else if (lit == RDX_MULTIX) {
        call(JDRsort, state, RDXZauthor, RDXY);
        call(TLVcloseany, state->tlv, lit, state->stack);
    } else {
        call(TLVcloseany, state->tlv, lit, state->stack);
    }
    done;
}

ok64 JDRinsertU(JDRstate* state) {
    sane(state != nil);
    if (state->pre == 0 || state->pre == ',') {
        call(TLVopenlong, state->tlv, RDX_TUPLE_INLINE, state->stack);
        call($u8feed1, state->tlv, 0);
        call(JDRfeedempty, state);
    } else {
        u8p start = *$term(Bu8pdata(state->stack));
        $u8c rec = {start, state->tlv[0]}, key, body;
        test($len(state->tlv) > 1 + 4 + 1 + 1 + $len(rec), JDRnoroom);
        u8 lit;
        call(TLVdrainkv, &lit, key, body, rec);
        test($empty(rec), FAILsanity);
        u8p safe0 = state->tlv[0] + 1 + 4 + 1 + 1;
        $u8 safekey = {safe0, safe0 + $len(key)};
        $u8 safebody = {safe0 + $len(key), safe0 + $len(key) + $len(body)};
        $u8move(safekey, key);
        $u8move(safebody, body);
        state->tlv[0] = start;
        call(TLVopenlong, state->tlv, RDX_TUPLE_INLINE, state->stack);
        call($u8feed1, state->tlv, $len(key));
        call($u8feedall, state->tlv, (u8c$)safekey);
        --safebody[0];
        **safebody = 0;
        call(TLVfeed, state->tlv, lit, (u8c$)safebody);
    }
    done;
}

ok64 JDRonColon($cu8c tok, JDRstate* state) {
    // FIXME  <1:2>
    sane($ok(tok) && state != nil);
    if (JDRtop(state->stack) != RDX_TUPLE_INLINE) {
        call(JDRinsertU, state);
    } else if (state->pre == 0 || state->pre == ':') {
        call(JDRfeedempty, state);
    }
    state->pre = ':';
    done;
}

ok64 JDRonComma($cu8c tok, JDRstate* state) {
    sane($ok(tok) && state != nil);
    if (!Bempty(state->stack) && **Bu8ptop(state->stack) == RDX_TUPLE_INLINE) {
        call(JDRcloseinline, state);
    }
    if (state->pre == 0 || state->pre == ',') {
        call(JDRfeedempty, state);
    }
    state->pre = ',';
    done;
}

pro(JDRonRoot, $cu8c tok, JDRstate* state) {
    sane($ok(tok) && state != nil);
    if (!Bempty(state->stack) && **Bu8ptop(state->stack) == RDX_TUPLE_INLINE)
        call(JDRcloseinline, state);
    done;
}

ok64 JDRonInter($cu8c tok, JDRstate* state) { return OK; }

ok64 JDRonUtf8cp1($cu8c tok, JDRstate* state) { return OK; }

ok64 JDRonUtf8cp2($cu8c tok, JDRstate* state) {
    u32 cp = $at(tok, 0) & 0x1f;
    cp = (cp << 6) | ($at(tok, 1) & 0x3f);
    if (unlikely(cp >= 0xd800 || cp < 0xe000)) return UTF8bad;
    return OK;
}

ok64 JDRonUtf8cp3($cu8c tok, JDRstate* state) {
    u32 cp = $at(tok, 0) & 0x1f;
    cp = (cp << 6) | ($at(tok, 1) & 0x3f);
    cp = (cp << 6) | ($at(tok, 2) & 0x3f);
    if (unlikely(cp >= 0xd800 || cp < 0xe000)) return UTF8bad;
    return OK;
}

ok64 JDRonUtf8cp4($cu8c tok, JDRstate* state) {
    // TODO check unicode range
    return OK;
}

ok64 JDRonNL($cu8c tok, JDRstate* state) {
    state->line++;
    state->col = tok[0] - state->text[0];
    return OK;
}

ok64 JDRfeedSesc($u8 tlv, $u8c txt) {
    while (!$empty(txt) && !$empty(tlv)) {
        if (**txt != '\\') {
            **tlv = **txt;
            ++*tlv;
            ++*txt;
            continue;
        }
        ++*txt;
        if ($empty(txt)) return JDRbad;
        switch (**txt) {
            case 't':
                **tlv = '\t';
                break;
            case 'r':
                **tlv = '\r';
                break;
            case 'n':
                **tlv = '\n';
                break;
            case 'b':
                **tlv = '\b';
                break;
            case 'f':
                **tlv = '\f';
                break;
            case '0':
                **tlv = 0;
                break;
            case '\\':
                **tlv = '\\';
                break;
            case '/':
                **tlv = '/';
                break;
            case '"':
                **tlv = '"';
                break;
            case 'u': {
                if ($len(txt) < 5) return HEXnodata;
                $u8c hex = {*txt + 1, *txt + 5};
                *txt += 4;
                u64 cp = 0;
                ok64 o = u64hexdrain(&cp, hex);
                if (o == OK) o = UTF8feed1(tlv, cp);
                --*tlv;
                if (o != OK) return o;
                break;
            }
            default:
                return JDRbad;
        }
        ++*tlv;
        ++*txt;
    }
    if (!$empty(txt)) return RDXnospace;
    return OK;
}

ok64 JDRdrainSesc($u8 txt, $u8c tlv);
// . . . . . . . w r i t e r . . . . . . .

fun ok64 JDRfeedstamp($u8 rdxj, id128 stamp) {
    if (id128empty(stamp)) return OK;
    // call($u8feed2, rdxj, ' ', '@');
    if ($len(rdxj) < 2) return RDXnospace;
    $u8feed1(rdxj, '@');
    return RDXid128feed(rdxj, stamp);
}

fun ok64 JDRindent($u8 rdxj, u64 style) {
    if (style & StyleIndentTab) {
        return $u8feedcn(rdxj, '\t', style & 0xff);
    } else if (style & StyleIndentSpace4) {
        return $u8feedcn(rdxj, ' ', (style & 0xff) << 2);
    } else {
        return OK;
    }
}

fun ok64 JDRfeedlist($u8 rdxj, $u8c tlv, u64 style) {
    sane($ok(rdxj) && $ok(tlv));
    a$dup(u8c, rest, tlv);
    u64 commanl = (style & StyleCommaNL) != 0 ||
                  ((style & StyleTopCommaNL) != 0 && (style & 0xff) == 0);
    ok64 err = OK;
    while ($len(rest) > 0 && err == OK) {
        $u8c rec = {};
        call(TLVdrain$, rec, rest);
        call(JDRfeed1, rdxj, rec, style);
        if (!$empty(rest) || (style & StyleTrailingComma) != 0) {
            call($u8feed1, rdxj, ',');
            if (commanl) {
                call($u8feed1, rdxj, '\n');
                call(JDRindent, rdxj, style);
            }
        }
    }
    if (commanl) {
        call($u8feed1, rdxj, '\n');
    }
    done;
}

ok64 JDRfeed1($u8 rdxj, $u8c tlv, u64 style) {
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
            call(JDRfeedlist, rdxj, value, style + 1);
            call($u8feed1, rdxj, ']');
            call(JDRfeedstamp, rdxj, id);
            break;
        case RDX_TUPLE: {
            b8 brackets = 1;  //(parent == RDX_TUPLE || !id128empty(id));
            if (brackets) {
                call($u8feed1, rdxj, '<');
                call(JDRfeedlist, rdxj, value, style + 1);
                call($u8feed1, rdxj, '>');
                call(JDRfeedstamp, rdxj, id);
            }
            /*while (!$empty(value)) {
                call(_JDRfeed, rdxj, value, RDX_TUPLE);
                if (!$empty(value)) call($u8feed1, rdxj, RDX_TUPLE_INLINE);
            }*/
            break;
        }
        case RDX_EULER:
            call($u8feed1, rdxj, '{');
            call(JDRfeedlist, rdxj, value, style + 1);
            call($u8feed1, rdxj, '}');
            call(JDRfeedstamp, rdxj, id);
            break;
        case RDX_MULTIX:
            call($u8feed1, rdxj, '(');
            call(JDRfeedlist, rdxj, value, style + 1);
            call($u8feed1, rdxj, ')');
            call(JDRfeedstamp, rdxj, id);
            break;
    }
    done;
}

pro(JDRdrainSesc, $u8 txt, $u8c tlv) {
    sane($ok(txt) && $ok(tlv));
    if ($len(txt) < $len(tlv)) return RDXnospace;
    u8 t = 0;
    $u8c key = {};
    $u8c val = {};
    call(TLVdrainkv, &t, key, val, tlv);
    while (!$empty(val) && !$empty(txt)) {
        switch (**val) {
            case '\t':
                call($u8feed2, txt, '\\', 't');
                break;
            case '\r':
                call($u8feed2, txt, '\\', 'r');
                break;
            case '\n':
                call($u8feed2, txt, '\\', 'n');
                break;
            case '\b':
                call($u8feed2, txt, '\\', 'b');
                break;
            case '\f':
                call($u8feed2, txt, '\\', 'f');
                break;
            case '\\':
                call($u8feed2, txt, '\\', '\\');
                break;
            case '/':
                call($u8feed2, txt, '\\', '/');
                break;
            case '"':
                call($u8feed2, txt, '\\', '"');
                break;
            case 0:
                call($u8feed2, txt, '\\', '0');
                break;
                // TODO \u etc
            default:
                $u8feed1(txt, **val);
        }
        ++*val;
    }
    done;
}
