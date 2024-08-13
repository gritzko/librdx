#include "MARK.h"

#include "01.h"
#include "PRO.h"

pro(MARKstatealloc, MARKstate* state, $u8c text) {
    sane(state != nil);
    u64 lines = roundup(1 + $len(text) / 8, PAGESIZE);
    $mv(state->text, text);
    $mv(state->doc, text);
    call(Bu8cpalloc, state->lines, lines);
    call(Bu64alloc, state->links, lines);
    call(Bu64alloc, state->divs, lines);
    call(Bu8alloc, state->fmt, $len(text));
    done;
}

pro(MARKstatereset, MARKstate* state) {
    sane(state != nil);
    Bu8cpreset(state->lines);
    Bu64reset(state->divs);
    Bu8reset(state->fmt);
    Bu64reset(state->links);
    done;
}

pro(MARKstatefree, MARKstate* state) {
    sane(state != nil);
    call(Bu8cpfree, state->lines);
    call(Bu64free, state->links);
    call(Bu64free, state->divs);
    call(Bu8free, state->fmt);
    done;
}

fun void w64trim(w64* p, u8 l) {
    for (u8 i = l; i < 8; ++i) p->_8[i] = 0;
}

fun b8 iscomas(w64 w, u8 l) {
    for (u8 i = 0; i < l; ++i)
        if (w._8[i] != MARK_COMA) return NO;
    return YES;
}

fun void nestany($u64c divs, u8 indent) {
    w64 w = {._64 = {**divs}};
    w64* p = (w64*)divs[0];
    w64 const* e = (w64 const*)divs[1];
    switch (w._8[indent]) {
        case MARK_H1:
            w64trim(p, indent + 1);
            while (p < e && p->_8[indent] == MARK_INDENT) {
                p->_8[indent] = ',';
                ++p;
            }
            break;
        case MARK_OLIST:

        default:
    }
}

pro(MARKnester, MARKstate* state) {
    sane(state != nil);
    nestany(Bu64cdata(state->divs), 0);
    done;
}

pro(MARKilexer, MARKstate* state) {
    sane(state != nil);
    w64** divs = (w64**)Bu64data(state->divs);
    u64 l = 0;
    u64 e = Bdatalen(state->lines);
    while (l < e) {
        u8 dl = w64bytelen($at(divs, l));
        do {
            w64 div = $at(divs, l);
            u8c$ line = state->lines[0] + l;
            a$tail(u8c, iline, line, dl * 4);
            $mv(state->text, iline);
            call(MARK2lexer, state);
            ++l;
        } while (l < e && iscomas($at(divs, l), dl));
    }
    done;
}

pro(MARKparse, MARKstate* state) {
    sane(state != nil && $ok(state->text));
    call(MARKlexer, state);
    call(MARKnester, state);
    call(MARKilexer, state);
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

pro(opentag, $u8 into, u8 mark) {
    u8$cc tag = tag4mark(mark);
    sane(tag != nil && $len(into) >= $len(tag) + 2);
    $feed1(into, '<');
    $feed(into, tag);
    $feed1(into, '>');
    done;
}
pro(closetag, $u8 into, u8 mark) {
    sane($len(into) >= 6);
    u8$cc tag = tag4mark(mark);
    $feed1(into, '<');
    $feed1(into, '/');
    $feed(into, tag);
    $feed1(into, '>');
    done;
}

/*pro(closetags, $u8 into, $u8 tags) {
    sane($len(into) >= $len(tags) * 6);
    while (!$empty(tags)) call(closetag, into, tags);
    done;
}*/

pro(linehtml, $u8 into, MARKstate const* state, u8 tab, u64 from, u64 till) {
    sane($ok(into));
    // just feed the text
    // acc to the line limits
    // FIXME here we don't see divs
    done;
}
/*
pro(h1html, $u8 into, MARKstate const* state, u8 tab, u64 from, u64* till) {
    sane($ok(into));
    u8cp$cc lines = Bu8cpcdata(state->lines);
    w64 divs = {._64 = {Bat(state->divs, from)}};
    call(opentag, into, '1');
    $u8c line;
    call(MARK2lex, state, line);
    u64 l = from + 1;
    while (l < *till) {
        w64 divs = {._64 = {Bat(state->divs, l)}};
        if (divs._8[tab] != MARK_INDENT) break;
        $mv(line, line);
        call(MARK2lex, state, line);
    }
    call(linehtml, into, state, tab, from, l);
    call(closetag, into, '1');
    *till = l;
    done;
}

pro(MARKlihtml, $u8 into, MARKstate const* state, u64 line) {
    sane($ok(into));
    // check nesteds
    // if nothing, p
    // scan forward
    done;
}

pro(MARKolisthtml, $u8 into, MARKstate const* state, u64 line) {
    sane($ok(into));
    done;
}
*/
pro(MARKhtml, $u8 into, MARKstate const* state) {
    sane($ok(into));
    done;
}

pro(html, $u8 into, MARKstate const* state, u64 line, u8 indent) {
    sane(YES);
    // scan, open/refresh/close
    // zero(mark2), parse?...

    done;
}

/*
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
    con u32 MASK = (1 << MARK_BITS) - 1;
    u32 pfmt = 0;
    u32 fmt = 0;
    for (u32 line = 0; line < $len(Bdata(state->linefmt)); ++line) {
        fmt = Bat(state->linefmt, line);
        u8 pr = MARKprefix(pfmt, fmt);
        u8 pl = MARKdivlen(pfmt);
        while (pl > pr) {
            call(_close, into, pfmt & MASK);
            pfmt >>= MARK_BITS;
            --pl;
        }
        u8 l = MARKdivlen(fmt);
        while (l > pr) {
            u8 w = (l - pr - 1) * MARK_BITS;
            call(_open, into, (fmt >> w) & MASK);
            ++l;
        }
        call(MARKlinehtml, into, line, state);
        pfmt = fmt;
    }
    while (pfmt > 0) {
        call(_close, into, pfmt & MASK);
        pfmt >>= MARK_BITS;
    }
    done;
}
*/
