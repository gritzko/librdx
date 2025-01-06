
#include "JDR.h"

#include <stdint.h>

#include "RDX.h"
#include "VLT.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/BUF.h"
#include "abc/OK.h"
#include "abc/PRO.h"
#include "abc/TLV.h"
#include "abc/UTF8.h"
#include "abc/ZINT.h"

#define RDX_TUPLE_INLINE 'U'

// . . . . . . . p a r s e r . . . . . . .

#define JDRaa (0x20UL << B_MAX_LEN_BITS)

ok64 JDRclose(u8 lit, JDRstate* state) {
    if (0 == (JDRaa & *Btop(state->stack))) $u8feed1(Bu8idle(state->vlt), 0);
    return VLTclose(state->vlt, state->stack, lit);
}

a$u8c(VLT_EMPTY_TUPLE, 0, 1, 'p');

fun ok64 JDRfeedempty(JDRstate* state) {
    state->pre = RDX_TUPLE;
    return $u8feed(Bu8idle(state->vlt), VLT_EMPTY_TUPLE);
}

fun ok64 JDRcloseinline(JDRstate* state) {
    sane(1);
    if (state->pre == ':') call(JDRfeedempty, state);
    call(VLTrename, state->vlt, state->stack, RDX_TUPLE_INLINE, RDX_TUPLE);
    state->pre = RDX_TUPLE_INLINE;
    call(JDRclose, RDX_TUPLE, state);
    done;
}

ok64 JDRonFIRST0(JDRstate* state, u8 lit) {
    if (!Bempty(state->stack) && VLTtop(state->stack) == RDX_TUPLE_INLINE &&
        state->pre != ':') {
        ok64 o = JDRcloseinline(state);
        if (o != OK) return o;
    }
    return VLTopen(state->vlt, state->stack, lit);
}

ok64 JDRonFloat($cu8c tok, JDRstate* state) {
    ok64 o = JDRonFIRST0(state, RDX_FLOAT);
    if (o == OK) {
        u8c* e = 0;
        double d = strtod((const char*)*tok, (char**)&e);
        o = ZINTf64feed(Bu8idle(state->vlt), &d);
    }
    return o;
}

#define I64_MAX INT64_MAX
#define I64_MIN INT64_MIN
#define I64_MIN_ABS 0x8000000000000000

ok64 JDRonInt($cu8c tok, JDRstate* state) {
    sane($ok(tok) && state != nil);
    call(JDRonFIRST0, state, RDX_INT);
    a$dup(u8c, dec, tok);
    if (**tok == '-' || **tok == '+') ++*dec;
    u64 x;
    call(u64decdrain, &x, dec);
    u64 lim = INT64_MAX;
    i64 y;
    if (**tok == '-') {
        test(x <= I64_MIN_ABS, JDRbadint);
        y = -x;
        lim = I64_MIN_ABS;
    } else {
        test(x <= I64_MAX, JDRbadint);
        y = x;
    }
    test(x <= lim, JDRbadint);
    return ZINTi64feed(Bu8idle(state->vlt), &y);
}

ok64 JDRonRef($cu8c tok, JDRstate* state) {
    sane($ok(tok) && state != nil);
    call(JDRonFIRST0, state, RDX_REF);
    id128 bits = {};
    call(RDXid128drain, &bits, tok);
    call(ZINTu128feed, Bu8idle(state->vlt), bits);
    done;
}

ok64 JDRonString($cu8c tok, JDRstate* state) {
    sane($ok(tok) && state != nil);
    call(JDRonFIRST0, state, RDX_STRING);
    $u8c str = {tok[0] + 1, tok[1] - 1};
    call(JDRfeedSesc, Bu8idle(state->vlt), str);
    done;
}

ok64 JDRonMLString($cu8c tok, JDRstate* state) {
    sane($ok(tok) && state != nil);
    call(JDRonFIRST0, state, RDX_STRING);
    $u8c str = {tok[0] + 1, tok[1] - 1};
    call($u8feed, Bu8idle(state->vlt), str);
    done;
}

ok64 JDRonTerm($cu8c tok, JDRstate* state) {
    sane($ok(tok) && state != nil);
    call(JDRonFIRST0, state, RDX_TERM);
    call($u8feed, Bu8idle(state->vlt), tok);
    done;
}

ok64 JDRonStamp($cu8c tok, JDRstate* state) {
    sane($ok(tok) && $len(tok) >= 2 && **tok == '@' && state != nil);
    *Btop(state->stack) |= JDRaa;
    $u8c idstr = {tok[0] + 1, tok[1]};
    id128 id = {};
    u64 l = Busylen(state->vlt);
    call(RDXid128drain, &id, idstr);
    call(ZINTu128feed, Bu8idle(state->vlt), id);
    call($u8feed1, Bu8idle(state->vlt), Busylen(state->vlt) - l);
    done;
}

ok64 JDRonFIRST($cu8c tok, JDRstate* state) {
    state->pre = '1';
    return JDRclose(0, state);
}

ok64 JDRonPLEX0(u8 lit, JDRstate* state) {
    sane(state != nil);
    if (VLTtop(state->stack) == RDX_TUPLE_INLINE && state->pre != ':')
        call(JDRcloseinline, state);
    state->pre = 0;
    return VLTopen(state->vlt, state->stack, lit);
}

ok64 JDRonPLEX1(u8 lit, JDRstate* state) {
    sane(state != nil && RDXisPLEX(lit));
    if (VLTtop(state->stack) == RDX_TUPLE_INLINE) {
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

ok64 JDRonClose($cu8c tok, JDRstate* state) {
    sane(state != nil);
    u8 lit = state->pre;
    if (!RDXisPLEX(lit)) fail(FAILsanity);
    if (VLTis1(state->stack) && VLTtop(state->stack) == RDX_TUPLE) {
        Bu64pop(state->stack);
    } else {
        call(JDRclose, lit, state);
    }
    done;
}

ok64 JDRonColon($cu8c tok, JDRstate* state) {
    sane($ok(tok) && state != nil);
    u8 top = VLTtop(state->stack);
    if (top != RDX_TUPLE_INLINE) {
        if (state->pre == 0 || state->pre == ',') {
            call(VLTopen, state->vlt, state->stack, RDX_TUPLE_INLINE);
            call(JDRfeedempty, state);
        } else {
            call(VLTreopen, state->vlt, state->stack, RDX_TUPLE_INLINE);
        }
    } else if (state->pre == 0 || state->pre == ':') {
        call(JDRfeedempty, state);
    }
    state->pre = ':';
    done;
}

ok64 JDRonComma($cu8c tok, JDRstate* state) {
    sane($ok(tok) && state != nil);
    u8 top = VLTtop(state->stack);
    if (top == RDX_TUPLE_INLINE) {
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
    if (VLTtop(state->stack) == RDX_TUPLE_INLINE) call(JDRcloseinline, state);
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

a$strc(Tabs,
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");

a$strc(Spaces,
       "                                "
       "                                "
       "                                "
       "                                "
       "                                "
       "                                "
       "                                "
       "                                "
       "                                "
       "                                "
       "                                "
       "                                "
       "                                "
       "                                "
       "                                "
       "                                ");

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
