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

ok64 RDXSkipBracketsJDR(rdxb x) {
    // recur
}
ok64 RDXSkipInlineTupleJDR(rdxb x) {
    RDXSkipBracketsJDR(x);  // recur
}

static const rdx emptyP = {
    .format = RDX_FORMAT_JDR,
    .type = RDX_TYPE_TUPLE,
    .mark = ':',
};
static const rdx openP = {
    .format = RDX_FORMAT_JDR,
    .type = RDX_TYPE_TUPLE,
    .mark = '(',
};
static const rdx closeP = {
    .format = RDX_FORMAT_JDR,
    .type = RDX_TYPE_TUPLE,
    .mark = ')',
};

ok64 RDXCheckNestingJDR(u8 parent_mark, rdxp at, rdxp next) {
    sane(1);
    if (parent_mark == ':') {
        if (at->mark != ':') {
            *at = closeP;
            fail(NOdata);
        }
    } else if (at->mark != ',') {
        fail(JDRbad);
    }
    done;
}

ok64 RDXNextJDR(rdxb x) {
    sane(rdxbOK(x));
    rdxp at = rdxbLast(x);
    test(!$empty(at->data), NOdata);
    switch (at->mark) {
        case RDX_JDR_CLASS_OPEN:
            return RDXSkipBracketsJDR(x);
        case RDX_JDR_CLASS_INTER:  // FIXME
            return RDXSkipInlineTupleJDR(x);
        case RDX_JDR_CLASS_CLOSE:
            fail(NOdata);
    }
    u8 parent_mark = '(';
    u8 parent_type = RDX_TYPE_ROOT;
    if (rdxbDataLen(x) > 1) {
        rdxp parent = at - 1;
        parent_type = parent->type;
        parent_mark = parent->mark;
    }
    rdxp next = $term(rdxbData(x));
    rdxMove(at, next);
    call(JDRlexer, next);
    u8 state = (CLASSES[at->mark] << 4) | CLASSES[next->mark];
    // todo implied brackets 4x4  mark: returns {[(<FIRST non-returns  >)]}:,x
    switch (state) {  // todo move tricks
        case RDX_JDR_CLASS_OPEN_OPEN:
        case RDX_JDR_CLASS_OPEN_INTER:
        case RDX_JDR_CLASS_OPEN_FIRST:
        case RDX_JDR_CLASS_OPEN_CLOSE:
            done;  // :)
        case RDX_JDR_CLASS_INTER_OPEN:
            call(RDXCheckNestingJDR, parent_mark, at, next);
            *at = *next;
            return JDRlexer(next);
        case RDX_JDR_CLASS_INTER_INTER:
            call(RDXCheckNestingJDR, parent_mark, at, next);
            *at = emptyP;
            done;
        case RDX_JDR_CLASS_INTER_FIRST:
            call(RDXCheckNestingJDR, parent_mark, at, next);
            *at = *next;
            return JDRlexer(next);
        case RDX_JDR_CLASS_INTER_CLOSE:
            call(RDXCheckNestingJDR, parent_mark, at, next);
            *at = emptyP;
            done;
        case RDX_JDR_CLASS_FIRST_OPEN:  // 1:2 (  // bust input
            if (parent_type == RDX_TYPE_TUPLE && parent_mark == ':') {
                *next = closeP;
                $mv(next->data, at->data);
            }
            done;
        case RDX_JDR_CLASS_FIRST_INTER:
            if (parent_mark == next->mark) {
                call(JDRlexer, next);
            } else if (next->mark == ':') {
                *next = *at;
                *at = openP;
            } else if (next->mark == ',') {
                *next = closeP;
                $mv(next->data, at->data);
            }
            done;
        case RDX_JDR_CLASS_FIRST_FIRST:  // 1:2 3  bust input
            if (parent_type == RDX_TYPE_TUPLE && parent_mark == ':') {
                *next = closeP;
                $mv(next->data, at->data);
            }
            done;
        case RDX_JDR_CLASS_FIRST_CLOSE:
            done;
        case RDX_JDR_CLASS_CLOSE_OPEN:
        case RDX_JDR_CLASS_CLOSE_INTER:
        case RDX_JDR_CLASS_CLOSE_FIRST:
        case RDX_JDR_CLASS_CLOSE_CLOSE:  // 1:() (  bust input twice?
            if (parent_type != at->type) fail(JDRbadnest);
            if (parent_type == RDX_TYPE_TUPLE && parent_mark == ':')
                fail(JDRbadnest);
            fail(NOdata);  // bust input
    }
    done;
}

ok64 RDXIntoJDR(rdxb x) {
    sane(rdxbOK(x));
    if (rdxbDataLen(x)) {
        $mv(rdxbLast(x)->data, (*rdxbIdle(x))->data);
    }
    call(rdxbFed1, x);
    done;
}

ok64 RDXOutoJDR(rdxb x) {
    sane(rdxbOK(x));
    call(rdxbPop, x);
    if (rdxbDataLen(x)) {
        rdxp last = rdxbLast(x);
        rdxp inner = last + 1;  // fixme :(
        if (inner->data[0]) last->data[0] = inner->data[0];
    }
    done;
}

ok64 RDXSeekJDR(rdxb x) { return NOTimplyet; }

static const char* BRACKET[2] = {" ([{<", " )]}>"};

ok64 RDXWriteNextJDRIndent(rdxb x) {
    sane(rdxbOK(x) && !rdxbEmpty(x));
    rdxp last = rdxbLast(x);
    u64 style = last->pos;
    if (style & RDX_JDR_FORMAT_INDENT_LINES)
        if ((style & RDX_JDR_FORMAT_NL_FIRST) ||
            (rdxTypePlex(last) && (style & RDX_JDR_FORMAT_NL_PLEX))) {
            call(u8sFeed1, last->into, '\n');
            call(u8sFeed1xN, last->into, ' ', (rdxbDataLen(x) - 1) * 4);
        }
    done;
}

ok64 RDXWriteNextJDRSeparator(rdxb x) {
    sane(rdxbOK(x) && !rdxbEmpty(x));
    rdxp last = rdxbLast(x);
    u64 style = last->pos;
    if (rdxbDataLen(x) && (last - 1)->type == RDX_TYPE_TUPLE &&
        (last - 1)->mark == ':') {
        call(u8sFeed1, last->into, ':');
        done;
    }
    // if (style & RDX_JDR_FORMAT_COMMA_FIRST) todo make it a shift
    // ',' ', ' ',\n' ':'
    done;
}

ok64 RDXWriteNextJDR(rdxb x) {
    sane(rdxbOK(x) && !rdxbEmpty(x));
    rdxp last = rdxbLast(x);
    if (last->mark) {  // maybe close bracket
        call(RDXWriteNextJDRIndent, x);
        call(u8sFeed1, last->into, last->mark);
        call(RDXWriteNextJDRSeparator, x);
    }
    call(RDXWriteNextJDRIndent, x);
    switch (last->type) {
        case RDX_TYPE_FLOAT:
            call(utf8sFeedFloat, last->into, &last->f);
            break;
        case RDX_TYPE_INT:
            call(utf8sFeedInt, last->into, &last->i);
            break;
        case RDX_TYPE_REF:
            call(RDXutf8sFeedID, last->into, &last->r);
            break;
        case RDX_TYPE_STRING:
            test(last->enc == RDX_UTF_ENC_UTF8,
                 NOTimplyet);  // may need 2 recodings
            call(utf8sFeed1, last->into, '"');
            call(UTABLE[RDX_UTF_ENC_UTF8_ESC][UTF8_ENCODER_ALL], last->into,
                 last->s);
            call(utf8sFeed1, last->into, '"');
            break;
        case RDX_TYPE_TERM:
            call(u8sFeed, last->into, last->t);
            break;
        case RDX_TYPE_TUPLE:
        case RDX_TYPE_LINEAR:
        case RDX_TYPE_EULER:
        case RDX_TYPE_MULTIX:
            call(u8sFeed1, last->into, BRACKET[0][last->type]);
            last->mark = BRACKET[1][last->type];
            break;
    }
    call(RDXWriteNextJDRSeparator, x);
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
ok64 RDXWriteSeekJDR(rdxb x) { return NOTimplyet; }

ok64 RDXutf8sFeedID(utf8s into, id128cp ref) {
    if (unlikely($len(into) < 24)) return NOroom;
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
