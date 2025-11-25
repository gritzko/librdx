#include "JDR.h"

#include "RDX.h"
#include "abc/PRO.h"

const u8 CLASSES[] = {
    [0] = RDX_JDR_CLASS_OPEN,    ['['] = RDX_JDR_CLASS_OPEN,
    [']'] = RDX_JDR_CLASS_CLOSE, ['('] = RDX_JDR_CLASS_OPEN,
    [')'] = RDX_JDR_CLASS_CLOSE, ['<'] = RDX_JDR_CLASS_OPEN,
    ['>'] = RDX_JDR_CLASS_CLOSE, ['{'] = RDX_JDR_CLASS_OPEN,
    ['}'] = RDX_JDR_CLASS_CLOSE, [':'] = RDX_JDR_CLASS_INTER,
    [','] = RDX_JDR_CLASS_INTER, ['F'] = RDX_JDR_CLASS_FIRST,
    ['I'] = RDX_JDR_CLASS_FIRST, ['R'] = RDX_JDR_CLASS_FIRST,
    ['S'] = RDX_JDR_CLASS_FIRST, ['T'] = RDX_JDR_CLASS_FIRST,
    ['.'] = RDX_JDR_CLASS_CLOSE,
};

ok64 rdxSeekJDR(rdxp x) { return NOTIMPLYET; }

ok64 rdxWriteSeekJDR(rdxp x) { return NOTIMPLYET; }

ok64 RDXSkipBracketsJDR(rdxp x) {
    // recur
    return NOTIMPLYET;
}
ok64 RDXSkipInlineTupleJDR(rdxp x) {
    RDXSkipBracketsJDR(x);  // recur
    return NOTIMPLYET;
}

static const rdx emptyP = {
    .format = RDX_FORMAT_JDR,
    .type = RDX_TYPE_TUPLE,
    .prnt = ':',
};
static const rdx openP = {
    .format = RDX_FORMAT_JDR,
    .type = RDX_TYPE_TUPLE,
    .prnt = '(',
};
static const rdx closeP = {
    .format = RDX_FORMAT_JDR,
    .type = RDX_TYPE_TUPLE,
    .prnt = ')',
};

ok64 RDXCheckNestingJDR(u8 parent_prnt, rdxp at, rdxp next) {
    sane(1);
    if (parent_prnt == ':') {
        if (at->prnt != ':') {
            *at = closeP;
            fail(NODATA);
        }
    } else if (at->prnt != ',') {
        fail(JDRbad);
    }
    done;
}

u8 skipCommaColon(u8cs data) {
    if ($empty(data)) return 0;
    u8 p = **data;
    if (p == ':' || p == ',') {
        ++*data;
        return p;
    }
    return 0;
}

ok64 rdxNextJDR(rdxp at) {
    sane(at);
    test(!$empty(at->data), END);
    u8 inlineP = at->prnt == RDX_TYPE_TUPLE && (at - 1)->enc == ':';
    if (at->type && at->type < RDX_TYPE_PLEX_LEN) {
        $mv(at->data, at->plex)
    }
    if ($empty(at->data)) {
        at->type = 0;
        return END;
    }
    if (inlineP) {
        if (**at->data != ':') {
            at->type = 0;
            return END;
        }
        ++*at->data;  // inline tuple continues
    } else {
        if (**at->data == ',') ++*at->data;
    }
    u8 pre_enc = at->enc;
    u8cs pre_data;
    $mv(pre_data, at->data);

    call(JDRlexer, at);

    switch (at->enc) {
        case '{':
            $mv(at->plex, at->data);
            done;
        case '}':
            test(at->type == at->prnt, RDXBADNEST);
            return END;
        case ':':
            at->type = RDX_TYPE_TUPLE;
            $null(at->plex);
            done;
        case ',':
            at->type = RDX_TYPE_TUPLE;
            $null(at->plex);
            done;
        case '1': {
            if (!inlineP && !$empty(at->data) && ':' == **at->data) {
                at->type = RDX_TYPE_TUPLE;
                at->enc = ':';
                $mv(at->plex, pre_data);
            }  // todo enc
            done;
        }
    }

    done;
}

ok64 rdxWriteNextJDR(rdxp x) {
    sane(x);
    if (x->len != 0 && x->prnt) {
        u8 sep = (x->prnt == RDX_TYPE_TUPLE && (x - 1)->enc == ':') ? ':' : ',';
        call(u8sFeed1, x->into, sep);
    }
    // todo indent
    switch (x->type) {
        case 0:
            if (x->prnt) {
                call(u8sFeed1, x->into, RDX_PLEX_CLOSE_LIT[x->prnt]);
            }
            break;
        case RDX_TYPE_TUPLE:
        case RDX_TYPE_LINEAR:
        case RDX_TYPE_EULER:
        case RDX_TYPE_MULTIX:
            call(u8sFeed1, x->into, RDX_PLEX_OPEN_LIT[x->prnt]);
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
            test(x->enc != RDX_UTF_ENC_UTF8, NOTIMPLYET); // todo
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
ok64 RDXWriteIntoJDR(rdxb x) {
    sane(rdxbOK(x));
    call(rdxbFed1, x);
    if (rdxbDataLen(x) > 1) {
        rdxp last = rdxbLast(x);
        $mv(last->data, (last - 1)->data);
    }
    done;
}
ok64 RDXWriteOutoJDR(rdxb x) {
    sane(rdxbOK(x) && rdxbDataLen(x));
    if (rdxbDataLen(x) > 1) {
        rdxp last = rdxbLast(x);
        $mv((last - 1)->data, last->data);
    }
    call(rdxbPop, x);
    done;
}
ok64 RDXWriteSeekJDR(rdxb x) { return NOTIMPLYET; }

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
