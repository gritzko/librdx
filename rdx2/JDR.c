#include "JDR.h"

#include "RDX.h"
#include "abc/PRO.h"

ok64 rdxIntoJDR(rdxp c, rdxp p) {
    sane(c && p && p->type);
    c->format = p->cformat;
    c->data = p->plex;
    c->ptype = p->type;
    c->len = 0;
    if (!c->type) {
        c->type = 0;
        c->cformat = 0;
        zero(c->r);
    } else {
        fail(NOTIMPLYET);
    }
    done;
}

ok64 rdxOutoJDR(rdxp c, rdxp p) {
    sane(c && p);
    p->data[0] = p->plex[0];
    done;
}

ok64 rdxWriteNextJDR(rdxp x) {
    sane(x);
    if (x->len != 0) {
        u8 sep = (x->format == RDX_FORMAT_JDR_PIN) ? ':' : ',';
        call(u8sFeed1, x->into, sep);
    }
    // todo indent
    switch (x->type) {
        case 0:
            fail(RDXBAD);
        case RDX_TYPE_TUPLE:
        case RDX_TYPE_LINEAR:
        case RDX_TYPE_EULER:
        case RDX_TYPE_MULTIX:
            call(u8sFeed1, x->into, RDX_TYPE_BRACKET_OPEN[x->type]);
            $mv(x->plex, x->data);
            x->cformat = RDX_FORMAT_JDR | RDX_FORMAT_WRITE;
            break;
        case RDX_TYPE_FLOAT:
            call(utf8sFeedFloat, x->into, &x->f);
            break;
        case RDX_TYPE_INT:
            call(utf8sFeedInt, x->into, &x->i);
            break;
        case RDX_TYPE_REF:
            call(RDXutf8sFeedID, x->into, &x->r);
            break;
        case RDX_TYPE_STRING:
            test(x->cformat != RDX_UTF_ENC_UTF8, NOTIMPLYET);  // todo
            call(utf8sFeed1, x->into, '"');
            call(UTABLE[RDX_UTF_ENC_UTF8_ESC][UTF8_ENCODER_ALL], x->into, x->s);
            call(utf8sFeed1, x->into, '"');
            break;
        case RDX_TYPE_TERM:
            call(u8sFeed, x->into, x->t);
            break;
    }
    ++x->len;
    done;
}

ok64 rdxWriteIntoJDR(rdxp c, rdxp p) {
    sane(c && p && p->type);
    c->format = p->cformat;  // fixme may be unset
    c->data = p->plex;
    c->ptype = p->type;
    c->type = 0;
    c->cformat = 0;
    c->len = 0;
    zero(c->r);
    done;
}

ok64 rdxWriteOutoJDR(rdxp c, rdxp p) {
    sane(c && p);
    if (p->type) {
        call(u8sFeed1, c->into, RDX_TYPE_BRACKET_CLOSE[p->type]);
    }
    p->data[0] = p->plex[0];
    done;
}

ok64 RDXutf8sFeedID(utf8s into, id128cp ref) {
    if (unlikely($len(into) < 24)) return NOROOM;
    RONutf8sFeed64(into, ron60Max & ref->src);
    utf8sFeed1(into, '-');
    RONutf8sFeed64(into, ron60Max & ref->seq);
    return OK;
}

ok64 RDXutf8sDrainID(utf8cs from, id128p ref) {
    a_dup(u8c, t, from);
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

const u8 RDX_TYPE_LIT_REV[] = {
    ['P'] = RDX_TYPE_TUPLE,  ['L'] = RDX_TYPE_LINEAR, ['E'] = RDX_TYPE_EULER,
    ['X'] = RDX_TYPE_MULTIX, ['F'] = RDX_TYPE_FLOAT,  ['I'] = RDX_TYPE_INT,
    ['R'] = RDX_TYPE_REF,    ['S'] = RDX_TYPE_STRING, ['T'] = RDX_TYPE_TERM,
};

const u8 RDX_TYPE_BRACKET_REV[] = {
    ['('] = RDX_TYPE_TUPLE,  ['['] = RDX_TYPE_LINEAR, ['{'] = RDX_TYPE_EULER,
    ['<'] = RDX_TYPE_MULTIX, [')'] = RDX_TYPE_TUPLE,  [']'] = RDX_TYPE_LINEAR,
    ['}'] = RDX_TYPE_EULER,  ['>'] = RDX_TYPE_MULTIX,
};
