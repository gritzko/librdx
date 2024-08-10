#include "MARK.h"

#include "PRO.h"

pro(MARKparse, MARKstate* state) {
    sane(state != nil && $ok(state->text));
    call(MARKlexer, state);
    done;
}

ok64 _open($u8 into, u32 tag) { return OK; }
ok64 _close($u8 into, u32 tag) { return OK; }

pro(MARKlinehtml, $u8 into, u32 line, MARKstate const* state) {
    sane($ok(into));
    done;
}

a$str(TAG_H1, "h1");
a$str(TAG_H2, "h2");
a$str(TAG_H3, "h3");
a$str(TAG_OL, "ol");
a$str(TAG_UL, "ul");
a$str(TAG_LI, "li");
a$str(TAG_HR, "hr");
a$str(TAG_P, "p");
a$str(TAG_DIV, "div");

fun u8$cc tag4mark(u8 mark) {
    switch (mark) {
        case '1':
            return TAG_H1;
        case '2':
            return TAG_H2;
        case '3':
            return TAG_H3;
        case '-':
            return TAG_UL;
        case '.':
            return TAG_OL;
        case '*':
            return TAG_LI;
        case '~':
            return TAG_HR;
        case ' ':
            return TAG_P;
        default:
            return nil;
    }
}

pro(opentag, $u8 into, $u8 tags, u8 mark) {
    u8$cc tag = tag4mark(mark);
    sane(tag != nil && $len(into) >= $len(tag) + 2);
    $feed1(into, '<');
    $feed(into, tag);
    $feed1(into, '>');
    done;
}
pro(closetag, $u8 into, $u8 tags) {
    sane(!$empty(tags) && $len(into) >= 6);
    --tags[1];
    u8 mark = *tags[1];
    u8$cc tag = tag4mark(mark);
    $feed1(into, '<');
    $feed1(into, '/');
    $feed(into, tag);
    $feed1(into, '>');
    done;
}

pro(closetags, $u8 into, $u8 tags) {
    sane($len(into) >= $len(tags) * 6);
    while (!$empty(tags)) call(closetag, into, tags);
    done;
}

pro(MARKhtml, $u8 into, MARKstate const* state) {
    sane($ok(into));
    aBpad(u8, tagpad, 32);
    u8$ tags = Bu8data(tagpad);
    $u8c stack;
    if ($len(stack) == 0) {  // TODO p gap
    }
    $for(u8c, p, stack) {
        *tags = *tagpad;
        switch (*p) {
            case '1':
                call(closetags, into, tags);
                call(opentag, into, tags, '1');
                break;
            case '2':
                call(closetags, into, tags);
                call(opentag, into, tags, '2');
                break;
            case '.':
                if ($empty(tags)) {
                    call(opentag, into, tags, '.');
                } else if (**tags == '.') {
                    ++*tags;
                    call(closetags, into, tags);
                } else {
                    call(closetags, into, tags);
                    call(opentag, into, tags, '.');
                }
                call(opentag, into, tags, '*');
                call(opentag, into, tags, ' ');
                break;
            case '-':
                // TODO same
                break;
            case '~':
                call(closetags, into, tags);
                call(opentag, into, tags, '~');
                break;
            case ' ':
                if ($empty(tags)) {
                    break;
                }
                switch (**tags) {
                    default:
                        ++*tags;
                }
                // TODO p breaks
                break;
        }
    }
    done;
}
/*
pro(MARKhtml, $u8 into, MARKstate const* state) {
    sane($ok(into));
    con u32 MASK = (1 << MARK_DIV_BITS) - 1;
    u32 pfmt = 0;
    u32 fmt = 0;
    for (u32 line = 0; line < $len(Bdata(state->linefmts)); ++line) {
        fmt = *Bat(state->linefmts, line);
        u8 pr = MARKprefix(pfmt, fmt);
        u8 pl = MARKdivlen(pfmt);
        while (pl > pr) {
            call(_close, into, pfmt & MASK);
            pfmt >>= MARK_DIV_BITS;
            --pl;
        }
        u8 l = MARKdivlen(fmt);
        while (l > pr) {
            u8 w = (l - pr - 1) * MARK_DIV_BITS;
            call(_open, into, (fmt >> w) & MASK);
            ++l;
        }
        call(MARKlinehtml, into, line, state);
        pfmt = fmt;
    }
    while (pfmt > 0) {
        call(_close, into, pfmt & MASK);
        pfmt >>= MARK_DIV_BITS;
    }
    done;
}
*/
