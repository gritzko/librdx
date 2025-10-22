#include "RDX2.h"

#include <stddef.h>

#include "JDR2.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/HEX.h"
#include "abc/PRO.h"
#include "abc/TLV.h"
#include "abc/ZINT.h"
#include "abc/ryu/ryu.h"

ok64 RDXutf8sFeedID(utf8s into, ref128cp ref) {
    if (unlikely($len(into) < 24)) return RDXnoroom;
    RONutf8sFeed64(into, ron60Max & ref->src);
    utf8sFeed1(into, '-');
    RONutf8sFeed64(into, ron60Max & ref->seq);
    return OK;
}

ok64 RDXutf8sDrainID(utf8cs from, ref128p ref) {
    a$dup(u8c, t, from);
    u8 DELIM = '-';
    u8c* p = $u8find(t, &DELIM);
    ok64 o = OK;
    if (p == NULL) {
        test($len(t) <= 10, RONbad);
        ref->src = 0;
        o = RONdrain64(&ref->seq, t);
        if (o == OK) from[0] = t[0];
    } else {
        u8cs src = {t[0], p};
        u8cs time = {p + 1, t[1]};
        test($len(src) <= 10 && $len(time) <= 10, RONbad);
        o = RONdrain64(&ref->src, src);
        if (o == OK) o = RONdrain64(&ref->seq, time);
        if (o == OK) from[0] = t[1];
    }
    return o;
}

ok64 RDXutf8sFeedStamp(utf8s into, rdxcp rp) {
    sane($ok(into));
    if (rp->id.src != 0 || rp->id.seq != 0) {
        call(utf8sFeed1, into, '@');
        if (rp->id.src != 0) {
            call(RDXutf8sFeedID, into, &rp->id);
        } else {
            call(RONutf8sFeed64, into, rp->id.seq & ron60Max);
        }
    }
    done;
}

ok64 RDXutf8sDrainStamp(utf8cs from, rdxp rp) {
    sane($ok(from) && rp != NULL);
    while (!$empty(from) && RDXisWS(**from)) (*from)++;
    if (!$empty(from)) {
        test(**from == '@', RDXbad);
        (*from)++;
        call(RDXutf8sDrainID, from, &rp->id);
    }
    done;
}

ok64 RDXu8sFeed1(u8s into, rdxcp rp) {
    switch (rp->type) {
        case RDX_FLOAT:
            return RDXu8sFeedF(into, rp);
        case RDX_INT:
            return RDXu8sFeedI(into, rp);
        case RDX_REF:
            return RDXu8sFeedR(into, rp);
        case RDX_STRING:
            return RDXu8sFeedS(into, rp);
        case RDX_TERM:
            return RDXu8sFeedT(into, rp);
        default:
            return RDXbad;
    }
}

ok64 RDXutf8sFeed1(utf8s into, rdxcp rp) {
    switch (rp->type) {
        case RDX_FLOAT:
            return RDXutf8sFeedF(into, rp);
        case RDX_INT:
            return RDXutf8sFeedI(into, rp);
        case RDX_REF:
            return RDXutf8sFeedR(into, rp);
        case RDX_STRING:
            return RDXutf8sFeedS(into, rp);
        case RDX_TERM:
            return RDXutf8sFeedT(into, rp);
        default:
            return RDXbad;
    }
}

// . . . . . . . . FLOAT . . . . . . . .

ok64 RDXutf8sFeedF(utf8s into, rdxcp rp) {
    sane($ok(into) && rp != NULL && rp->type == RDX_FLOAT);
    call(utf8sFeedFloat, into, &rp->f);
    call(RDXutf8sFeedStamp, into, rp);
    done;
}

ok64 RDXutf8sDrainF(utf8csc from, rdxp rp) {
    sane(!$empty(from) && rp != NULL);
    utf8cs txt;
    utf8csDup(txt, from);
    call(utf8sDrainFloat, txt, &rp->f);
    rp->type = RDX_FLOAT;
    call(RDXutf8sDrainStamp, txt, rp);
    test($empty(txt), RDXbadF);
    done;
}

ok64 RDXu8sFeedF(u8s into, rdxcp rcp) {
    sane($ok(into) && rcp != NULL && rcp->type == RDX_FLOAT);
    a_pad(u8, fpad, 16);
    a_pad(u8, idpad, 16);
    ZINTu8sFeedFloat(fpad_idle, &rcp->f);
    ZINTu8sFeed128(idpad_idle, rcp->id.src, rcp->id.seq);
    call(TLVFeedKeyVal, into, RDX_FLOAT, idpad_datac, fpad_datac);
    done;
}

ok64 RDXu8sDrainF(u8csc from, rdxp rp) {
    sane($ok(from) && rp != NULL);
    u8cs key, val, fro;
    u8csDup(fro, from);
    call(TLVDrainKeyVal, &rp->type, key, val, fro);
    test(rp->type == RDX_FLOAT, RDXbad);
    call(ZINTu8sDrain128, key, &rp->id.src, &rp->id.seq);
    call(ZINTu8sDrainFloat, &rp->f, val);
    done;
}

// . . . . . . . . INTEGER . . . . . . . .

ok64 RDXutf8sFeedI(utf8s into, rdxcp rp) {
    sane($ok(into) && rp != NULL && rp->type == RDX_INT);
    call(utf8sFeedInt, into, &rp->i);
    call(RDXutf8sFeedStamp, into, rp);
    done;
}

ok64 RDXutf8sDrainI(utf8csc from, rdxp rp) {
    sane(!$empty(from) && rp != NULL);
    utf8cs txt;
    utf8csDup(txt, from);
    call(utf8sDrainInt, txt, &rp->i);
    rp->type = RDX_INT;
    call(RDXutf8sDrainStamp, txt, rp);
    test($empty(txt), RDXbadF);
    done;
}

ok64 RDXu8sFeedI(u8s into, rdxcp rcp) {
    sane($ok(into) && rcp != NULL && rcp->type == RDX_INT);
    a_pad(u8, vpad, 16);
    a_pad(u8, idpad, 16);
    ZINTu8sFeedInt(vpad_idle, &rcp->i);
    ZINTu8sFeed128(idpad_idle, rcp->id.src, rcp->id.seq);
    call(TLVFeedKeyVal, into, RDX_INT, idpad_datac, vpad_datac);
    done;
}

ok64 RDXu8sDrainI(u8csc from, rdxp rp) {
    sane($ok(from) && rp != NULL);
    u8cs key, val, fro;
    u8csDup(fro, from);
    call(TLVDrainKeyVal, &rp->type, key, val, fro);
    test(rp->type == RDX_INT, RDXbad);
    call(ZINTu8sDrain128, key, &rp->id.src, &rp->id.seq);
    call(ZINTu8sDrainInt, &rp->i, val);
    done;
}

// . . . . . . . . REFERENCE . . . . . . . .

ok64 RDXutf8sFeedR(utf8s into, rdxcp rp) {
    sane($ok(into) && rp != NULL && rp->type == RDX_REF);
    call(RDXutf8sFeedID, into, &rp->r);
    call(RDXutf8sFeedStamp, into, rp);
    done;
}

ok64 RDXutf8sDrainR(utf8csc from, rdxp rp) {
    sane(!$empty(from) && rp != NULL);
    utf8cs txt;
    utf8csDup(txt, from);
    call(RDXutf8sDrainID, txt, &rp->r);
    rp->type = RDX_REF;
    call(RDXutf8sDrainStamp, txt, rp);
    test($empty(txt), RDXbadF);
    done;
}

ok64 RDXu8sFeedR(u8s into, rdxcp rcp) {
    sane($ok(into) && rcp != NULL && rcp->type == RDX_REF);
    a_pad(u8, vpad, 16);
    a_pad(u8, idpad, 16);
    ZINTu8sFeed128(vpad_idle, rcp->r.src, rcp->r.seq);
    ZINTu8sFeed128(idpad_idle, rcp->id.src, rcp->id.seq);
    call(TLVFeedKeyVal, into, RDX_REF, idpad_datac, vpad_datac);
    done;
}

ok64 RDXu8sDrainR(u8csc from, rdxp rp) {
    sane($ok(from) && rp != NULL);
    u8cs key, val, fro;
    u8csDup(fro, from);
    call(TLVDrainKeyVal, &rp->type, key, val, fro);
    test(rp->type == RDX_REF, RDXbad);
    call(ZINTu8sDrain128, key, &rp->id.src, &rp->id.seq);
    call(ZINTu8sDrain128, val, &rp->r.src, &rp->r.seq);
    done;
}

// . . . . . . . . STRING . . . . . . . .

pro(RDXutf8sFeedEscape, utf8s txt, u8cs val) {
    sane($ok(txt) && $ok(val));
    if ($len(txt) < $len(val)) return RDXnoroom;
    while (!$empty(val) && !$empty(txt)) {
        switch (**val) {
            case '\t':
                call(u8sFeed2, txt, '\\', 't');
                break;
            case '\r':
                call(u8sFeed2, txt, '\\', 'r');
                break;
            case '\n':
                call(u8sFeed2, txt, '\\', 'n');
                break;
            case '\b':
                call(u8sFeed2, txt, '\\', 'b');
                break;
            case '\f':
                call(u8sFeed2, txt, '\\', 'f');
                break;
            case '\\':
                call(u8sFeed2, txt, '\\', '\\');
                break;
            case '/':
                call(u8sFeed2, txt, '\\', '/');
                break;
            case '"':
                call(u8sFeed2, txt, '\\', '"');
                break;
            case 0:
                call(u8sFeed2, txt, '\\', '0');
                break;
                // TODO \u etc
            default:
                u8sFeed1(txt, **val);
        }
        ++*val;
    }
    test($empty(val), RDXnoroom);
    done;
}

ok64 RDXutf8sDrainEscaped(utf8cs txt, u8s tlv) {
    while (!$empty(txt) && !$empty(tlv)) {
        if (**txt != '\\') {
            **tlv = **txt;
            ++*tlv;
            ++*txt;
            continue;
        }
        ++*txt;
        if ($empty(txt)) return RDXbadS;
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
                u8cs hex = {*txt + 1, *txt + 5};
                *txt += 4;
                u64 cp = 0;
                ok64 o = u64hexdrain(&cp, hex);
                if (o == OK) o = utf8sFeed32(tlv, cp);
                --*tlv;
                if (o != OK) return o;
                break;
            }
            default:
                return RDXbadS;
        }
        ++*tlv;
        ++*txt;
    }
    if (!$empty(txt)) return RDXnoroom;
    return OK;
}

ok64 RDXutf8sFeedS(utf8s into, rdxcp rp) {
    sane($ok(into) && rp != NULL && rp->type == RDX_STRING);
    u8cs dup;
    u8csDup(dup, rp->s);
    call(utf8sFeed1, into, '"');
    if (rp->enc == RDX_ENC_UTF8 || rp->enc == 0) {
        call(utf8sFeed, into, dup);
    } else if (rp->enc == RDX_ENC_UTF8_ESC) {
        call(RDXutf8sFeedEscape, into, dup);
    } else {
        return notimplyet;
    }
    call(utf8sFeed1, into, '"');
    call(RDXutf8sFeedStamp, into, rp);
    done;
}

ok64 RDXutf8sDrainS(utf8csc from, rdxp rp) {
    sane($len(from) >= 2 && rp != NULL);
    u8csp t = rp->t;
    u8 quote = *from[0];
    test(quote == '"' || quote == '`', RDXbadS);
    t[0] = t[1] = from[0] + 1;
    while (t[1] < from[1]) {
        if (*t[1] == quote) break;
        if (*t[1] == '\\' && quote == '"') t[1]++;
        t[1]++;
    }
    test(t[1] < from[1], RDXbadS);
    utf8cs txt = {t[1] + 1, from[1]};
    rp->type = RDX_STRING;
    rp->enc = RDX_ENC_UTF8_ESC;
    call(RDXutf8sDrainStamp, txt, rp);
    test($empty(txt), RDXbadF);
    done;
}

ok64 RDXu8sFeedS(u8s into, rdxcp rcp) {
    sane($ok(into) && rcp != NULL && rcp->type == RDX_STRING);
    a_pad(u8, idpad, 16);
    ZINTu8sFeed128(idpad_idle, rcp->id.src, rcp->id.seq);
    a_pad(u8p, stack, 1);
    if ($len(rcp->s) < 0xff - 16) {
        call(TLVinitshort, into, RDX_STRING, (u8p**)stack);
    } else {
        call(TLVinitlong, into, RDX_STRING, (u8p**)stack);  // todo
    }
    call(u8sFeed1, into, $len(idpad_datac));
    call(u8sFeed, into, idpad_datac);
    if (rcp->enc == RDX_ENC_UTF8_ESC) {
        u8cs dup;
        u8csDup(dup, rcp->s);
        call(RDXutf8sDrainEscaped, dup, into);
    } else if (rcp->enc == RDX_ENC_UTF8 || rcp->enc == 0) {
        call(u8sFeed, into, rcp->s);
    } else {
        return notimplyet;
    }
    call(TLVendany, into, RDX_STRING, (u8p**)stack);
    done;
}

ok64 RDXu8sDrainS(u8csc from, rdxp rp) {
    sane($ok(from) && rp != NULL);
    u8cs key, val, fro;
    u8csDup(fro, from);
    call(TLVDrainKeyVal, &rp->type, key, val, fro);
    test(rp->type == RDX_STRING, RDXbad);
    rp->enc = RDX_ENC_UTF8;
    call(ZINTu8sDrain128, key, &rp->id.src, &rp->id.seq);
    u8csDup(rp->t, val);
    done;
}

// . . . . . . . . TERM . . . . . . . .

ok64 RDXutf8sFeedT(utf8s into, rdxcp rp) {
    sane($ok(into) && rp != NULL && rp->type == RDX_TERM);
    call(utf8sFeed, into, rp->t);
    call(RDXutf8sFeedStamp, into, rp);
    done;
}

ok64 RDXutf8sDrainT(utf8csc from, rdxp rp) {
    sane(!$empty(from) && rp != NULL);
    u8csp t = rp->t;
    t[0] = from[0];
    t[1] = from[0];
    while (t[1] < from[1] && BASEron64rev[*t[1]] != 0xff) t[1]++;
    utf8cs txt = {t[1], from[1]};
    rp->type = RDX_TERM;
    call(RDXutf8sDrainStamp, txt, rp);
    test($empty(txt), RDXbadF);
    done;
}

ok64 RDXu8sFeedT(u8s into, rdxcp rcp) {
    sane($ok(into) && rcp != NULL && rcp->type == RDX_TERM);
    a_pad(u8, idpad, 16);
    ZINTu8sFeed128(idpad_idle, rcp->id.src, rcp->id.seq);
    call(TLVFeedKeyVal, into, RDX_TERM, idpad_datac, rcp->t);
    done;
}

ok64 RDXu8sDrainT(u8csc from, rdxp rp) {
    sane($ok(from) && rp != NULL);
    u8cs key, val, fro;
    u8csDup(fro, from);
    call(TLVDrainKeyVal, &rp->type, key, val, fro);
    test(rp->type == RDX_TERM, RDXbad);
    call(ZINTu8sDrain128, key, &rp->id.src, &rp->id.seq);
    u8csDup(rp->t, val);  // TODO len, chars
    done;
}

// . . . . . . . . READER . . . . . . . .

a_cstr(RDX_ROOT_REC, " \x01\x00");

ok64 rdxbInit(rdxb reader, u8cs data) {
    sane(Bok(reader) && $ok(data));
    Breset(reader);
    rdx r;
    call(rdxInit, &r, data);
    call(rdxsFeedP, rdxbIdle(reader), &r);
    done;
}

ok64 rdxbNext(rdxb reader) {
    sane(Bok(reader) && Bdatalen(reader) != 0);
    rdxp p = Blastp(reader);
    test(!$empty(p->rest), RDXnodata);
    u8 type;
    u8cs id, val;
    call(TLVDrainKeyVal, &type, id, val, p->rest);
    call(ZINTu8sDrain128, id, &p->id.src, &p->id.seq);
    switch (type) {
        case RDX_FLOAT:
            call(ZINTu8sDrainFloat, &p->f, val);
            break;
        case RDX_INT:
            call(ZINTu8sDrainInt, &p->i, val);
            break;
        case RDX_REF:
            call(ZINTu8sDrain128, val, &p->r.src, &p->r.seq);
            break;
        case RDX_STRING:
            u8csDup(p->s, val);
            break;
        case RDX_TERM:
            u8csDup(p->t, val);
            break;
        default:
            u8csDup(p->plex, val);
            break;
    }
    done;
}

ok64 rdxbInto(rdxb reader) {
    sane(Bok(reader) && Bdatalen(reader) > 0 && Bidlelen(reader) > 0 &&
         RDXisPLEX(rdxbType(reader)));
    rdxp up = Blastp(reader);
    call(rdxsFed1, rdxbIdle(reader));
    rdxp p = Blastp(reader);
    zerop(p);
    u8csDup(p->rest, up->plex);
    done;
}

ok64 rdxbOuto(rdxb reader) {
    sane(Bok(reader) && Bdatalen(reader) > 0);
    --((rdx**)reader)[2];
    done;
}

ok64 rdxpsUpAt(rdxps heap, size_t ndx, rdxz z) {
    sane(ndx < rdxpsLen(heap));
    size_t a = ndx;
    while (a) {
        size_t b = (a - 1) / 2;  // parent
        if (!z($at(heap, a), $at(heap, b))) break;
        rdxpsSwap(heap, a, b);
        a = b;
    }
    done;
}

ok64 rdxpsDownAt(rdxps heap, size_t ndx, rdxz z) {
    sane(rdxpsOK(heap) && ndx < rdxpsLen(heap) && z != NULL);
    size_t i = ndx;
    size_t n = rdxpsLen(heap);
    do {
        size_t left = 2 * i + 1;
        if (left >= n || left < i) break;
        size_t j = left;
        size_t right = left + 1;
        if (right < n && ($at(heap, right), $at(heap, j))) j = right;
        if (!z($at(heap, j), $at(heap, i))) break;
        rdxpsSwap(heap, i, j);
        i = j;
    } while (1);
    done;
}

ok64 rdxpsEqs(rdxps heap, u32p eqs, rdxz z) {
    sane($ok(heap) && eqs != NULL && z != NULL);
    if ($len(heap) <= 1) {
        *eqs = $len(heap);
        done;
    }
    *eqs = 1;
    a_pad(u8, q, RDX_MAX_INPUTS);
    u8Bfeed2(q, 1, 2);
    eats(u8, n, q_data) {
        if (!z($at(heap, 0), $at(heap, *n))) {
            u8 j1 = 2 * *n + 1;
            if (j1 < $len(heap)) {
                call(u8bFeed1, q, j1);
                if (j1 + 1 < $len(heap)) call(u8bFeed1, q, j1 + 1);
            }
            rdxpSwap($atp(heap, *eqs), $atp(heap, *n));
            ++*eqs;
        }
    }
    done;
}

ok64 rdxpsNexts(rdxps heap, u32 eqs, rdxz z) {  // ejects
    sane($ok(heap) && z != NULL);
    u8 i = eqs;
    while (i > 0) {
        --i;
        rdxp p = $at(heap, i);
        if (!$empty(p->rest)) {
            call(rdxNext, p);
        } else {
            rdxpsSwap(heap, i, $len(heap) - 1);
            --heap[1];
        }
        if (i < $len(heap)) rdxpsDownAt(heap, i, z);
    }
    done;
}

//
ok64 RDXu8ssMonoFeed(u8css spans, u8cs input, rdxz z) {
    sane(!$empty(spans) && input != NULL && z != NULL);
    u8cs span;
    u8csDup(span, input);
    rdx fit, old;  // todo it[2]
    rdxInit(&fit, input);
    call(rdxNext, &fit);
    rdxMove(&old, &fit);
    ok64 o;
    scan(rdxNext, &fit) {
        if (!z(&old, &fit)) {  // order break
            span[1] = old.rest[0];
            call(u8cssFeed1, spans, span);
            u8csDup(span, old.rest);
        }
        rdxMove(&old, &fit);
    }
    seen($nodata);
    span[1] = old.rest[0];
    call(u8cssFeed1, spans, span);
    done;
}

ok64 RDXu8sFeed(u8s rdx, rdxcp fit) {
    a_pad(u8, val_pad, 16);
    a_pad(u8, key_pad, 16);
    u8cs val = {};
    u8 lit = fit->type;
    switch (lit) {
        case 0:
            break;
        case RDX_FLOAT:
            ZINTu8sFeedFloat(val_pad_idle, &fit->f);
            $mv(val, val_pad_data);
            break;
        case RDX_INT:
            ZINTu8sFeedInt(val_pad_idle, &fit->i);
            $mv(val, val_pad_data);
            break;
        case RDX_REF:
            ZINTu8sFeed128(val_pad_idle, fit->r.src, fit->r.seq);
            $mv(val, val_pad_data);
            break;
        case RDX_STRING:
            $mv(val, fit->s);
            break;
        case RDX_TERM:
            $mv(val, fit->t);
            break;
        case RDX_TUPLE:
        case RDX_LINEAR:
        case RDX_EULER:
        case RDX_MULTIX:
            $mv(val, fit->plex);
            break;
        default:
            return RDXbadrec;
    }
    ZINTu8sFeed128(key_pad_idle, fit->id.src, fit->id.seq);
    return TLVFeedKeyVal(rdx, lit, key_pad_datac, val);
}

ok64 RDXu8bInto(u8b builder, rdxcp what) {
    sane(Bok(builder) && what != NULL);
    u8sp idle = u8bIdle(builder);
    call(u8sFeed1, idle, what->type);
    size_t dlen = Bdatalen(builder);
    test(dlen <= u32max, Bnoroom);
    call(u8sFeed32, idle, (u32*)&dlen);
    size_t ol = $len(idle);
    call(ZINTu8sFeed128, idle, what->id.src, what->id.seq);
    call(u8sFeed1, idle, ol - $len(idle));
    ((u8**)builder)[1] = builder[2];  // FIXME TLVu8bInto()
    done;
}

ok64 RDXu8bOuto(u8b builder, rdxcp what) {
    sane(Bok(builder) && $len(Bpast(builder)) >= 6 && what != NULL);
    u8csp past = u8cbPast(builder);
    u8cs oldpast;
    u8csDup(oldpast, past);
    u8sp data = u8bData(builder);
    a_pad(u8, oldid, 16);
    u8 idlen = 0;
    u32 prevlen = 0;
    call(u8sPop1, past, &idlen);
    call(u8sPop, past, oldid_idle);
    call(u8sPop32, past, &prevlen);
    size_t newlen = $len(data);
    if (newlen < 0xff) {
        u8sFeed1(data, (u8)newlen);
    } else {
        u8sFeed32(data, (u32*)&newlen);
    }
    u8sFeed1(data, idlen);
    u8sFeed(data, oldid_datac);
    ptrdiff_t d = $len(past);
    if (d != $len(oldpast)) {
        past[1] = oldpast[1];
        call(u8bShift, builder, d);
    }
    // todo test(what==NULL || what->type==type, RDXbadnest);
    ((u8**)builder)[1] = builder[0] + prevlen;  // FIXME TLVu8bOuto
    done;
}

ok64 RDXu8bFeedDeep(u8b builder, rdxb reader) {
    sane(Bok(builder) && $ok(reader));
    scan(rdxbNext, reader) {
        if (RDXisFIRST(rdxbType(reader))) {
            call(RDXu8bFeed, builder, rdxbLast(reader));
        } else {
            rdxcp top = rdxbLast(reader);
            call(RDXu8bInto, builder, top);
            call(rdxbInto, reader);
            call(RDXu8bFeedDeep, builder, reader);
            call(rdxbOuto, reader);
            call(RDXu8bOuto, builder, top);
        }
    }
    seen($nodata);
    done;
}

ok64 RDXu8bFeedAll(u8bp into, u8cs from) {
    sane(Bok(into) && $ok(from));
    a_pad(rdx, reader, RDX_MAX_NESTING);
    call(rdxbInit, reader, from);
    u8b builder = {into[0], into[0], into[0], into[1]};
    call(RDXu8bFeedDeep, builder, reader);
    ((u8**)into)[0] = builder[2];  // FIXME
    done;
}

ok64 rdxNext(rdxp it) {
    sane($ok(it->rest) && it->reclen <= $len(it->rest));
    u8 lit;
    u8cs key = {}, val = {};
    it->rest[0] += it->reclen;
    size_t ol = $len(it->rest);
    call(TLVDrainKeyVal, &lit, key, val, it->rest);
    it->reclen = ol - $len(it->rest);
    switch (lit) {
        case 0:
            break;
        case RDX_FLOAT:
            call(ZINTu8sDrainFloat, &it->f, val);
            break;
        case RDX_INT:
            call(ZINTu8sDrainInt, &it->i, val);
            break;
        case RDX_REF:
            call(ZINTu8sDrain128, val, &it->r.src, &it->r.seq);
            break;
        case RDX_STRING:
            $mv(it->s, val);
            break;
        case RDX_TERM:
            $mv(it->t, val);
            break;
        case RDX_TUPLE:
        case RDX_LINEAR:
        case RDX_EULER:
        case RDX_MULTIX:
            $mv(it->plex, val);
            break;
        default:
            return RDXbadrec;
    }
    call(ZINTu8sDrain128, key, &it->id.src, &it->id.seq);
    it->type = lit;
    done;
}

ok64 RDXu8bMergeLWW(u8bp merged, rdxpsc eqs) {
    sane(Bok(merged) && !$empty(eqs));
    int eqlen = 1;
    for (rdxp* p = *eqs + 1; p < $term(eqs); ++p) {
        if (rdxLastWriteWinsZ(**eqs, *p)) { // e < p
            rdxpSwap(*eqs, p);
            eqlen = 1;
        } else if (!rdxLastWriteWinsZ(*p, **eqs)) {  // e>=p && p<=e
            rdxpSwap(*eqs + eqlen, p);
            ++eqlen;
        } else {
            ;  // skip
        }
    }
    rdxp toprev = **eqs;
    if (eqlen == 1) return RDXu8bFeed(merged, toprev);
    a_head(rdxp, wins, eqs, eqlen);
    b8 plex = NO;
    rdxz z = NULL;
    switch ((**wins)->type) {
        case RDX_FLOAT: {
            eats(rdxp, p, wins) if ((*p)->f > toprev->f) toprev = *p;
            break;
        }
        case RDX_INT: {
            eats(rdxp, p, wins) if ((*p)->i > toprev->i) toprev = *p;
            break;
        }
        case RDX_REF: {
            eats(rdxp, p, wins) if (ref128Z(&(*p)->r, &toprev->r)) toprev = *p;
            break;
        }
        case RDX_STRING: {
            eats(rdxp, p, wins) if (u8csZ((*p)->s, toprev->s)) toprev = *p;
            break;
        }
        case RDX_TERM: {
            eats(rdxp, p, wins) if (u8csZ((*p)->t, toprev->t)) toprev = *p;
            break;
        }
        case RDX_TUPLE: {
            plex = YES;
            z = rdxTupleZ;
            break;
        }
        case RDX_LINEAR: {
            plex = YES;
            z = rdxLinearZ;
            break;
        }
        case RDX_EULER: {
            plex = YES;
            z = rdxEulerZ;
            break;
        }
        case RDX_MULTIX: {
            plex = YES;
            z = rdxMultixZ;
            break;
        }
    }
    if (!plex) {
        call(RDXu8bFeed, merged, toprev);
    } else {
        a_pad(u8cs, inners, RDX_MAX_INPUTS);
        eats(rdxp, p, wins) u8cssFeed1(inners_idle, (**p).plex);
        call(RDXu8bInto, merged, toprev);
        call(RDXu8bMergeZ, merged, inners_data, z);
        call(RDXu8bOuto, merged, toprev);
    }
    done;
}

ok64 RDXu8bMerge(u8bp merged, rdxps inputs, rdxz z) {
    sane($ok(merged) && $ok(inputs) && z != NULL);
    while (!$empty(inputs)) {
        u32 eqs = 0;
        rdxpsEqs(inputs, &eqs, z);
        a_head(rdxp, eq, inputs, eqs);
        call(RDXu8bMergeLWW, merged, eq);
        call(rdxpsNexts, inputs, eqs, z);
    }
    done;
}

ok64 RDXu8bMergeZ(u8bp merged, u8css inputs, rdxz less) {
    sane($ok(merged) && $ok(inputs));
    a_pad(rdx, its, RDX_MAX_INPUTS);
    a_pad(rdxp, ins, RDX_MAX_INPUTS);

    eats(u8cs, i, inputs) {
        rdx r = {};
        rdxInit(&r, (u8cspc)i);

        call(rdxsFeedP, its_idle, &r);
        call(rdxpsFeed1, ins_idle, $last(its_data));
        rdxpsUp(ins_data, rdxTupleZ);
    }

    call(RDXu8bMerge, merged, ins_data, rdxTupleZ);

    done;
}

ok64 rdx1Z(rdxcp a, rdxcp b) {
    u8 at = a->type;
    u8 bt = b->type;
    if (at != bt) return rdxTypeZ(a, b);
    switch (at) {
        case RDX_FLOAT:
            return a->f < b->f;
        case RDX_INT:
            return a->i < b->i;
        case RDX_REF:
            return ref128Z(&a->r, &b->r);
        case RDX_STRING:
            return u8csZ(a->s, b->s);
        case RDX_TERM:
            return u8csZ(a->t, b->t);
        default:
            return ref128Z(&a->id, &b->id);
    }
}

ok64 rdxTypeZ(rdxcp a, rdxcp b) {
    u8 at = a->type;
    u8 bt = b->type;
    if (at == bt) return GREQ;
    b8 aplex = rdxIsPLEX(a);
    b8 bplex = rdxIsPLEX(b);
    return aplex < bplex || (aplex == bplex && at < bt);
}

ok64 rdxTupleZ(rdxcp a, rdxcp b) { return GREQ; }

ok64 rdxLinearZ(rdxcp a, rdxcp b) { return ref128Z(&a->id, &b->id); }

ok64 rdxEulerZ(rdxcp a, rdxcp b) {
    rdx aa;
    if (a->type == RDX_TUPLE) {
        rdxInit(&aa, a->plex);
        rdxNext(&aa);
        a = &aa;
    }
    rdx bb;
    if (b->type == RDX_TUPLE) {
        rdxInit(&bb, b->plex);
        rdxNext(&bb);
        b = &bb;
    }
    return rdx1Z(a, b);
}

ok64 rdxMultixZ(rdxcp a, rdxcp b) { return u64Z(&a->id.src, &b->id.src); }

ok64 rdxLastWriteWinsZ(rdxcp a, rdxcp b) {
    if (!ref128Eq(&a->id, &b->id)) return ref128Z(&a->id, &b->id);
    u8 at = a->type;
    u8 bt = b->type;
    return (at == bt) ? rdx1Z(a, b) : rdxTypeZ(a, b);
}
