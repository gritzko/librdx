#include "MARK.h"

#include "01.h"
#include "FILE.h"
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
    call(Bu8cpfeed1, state->lines, text[0]);
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
    $mvnil(state->text);
    $mvnil(state->doc);
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

fun void nestany($u64 divs, u8 indent) {
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

fun void addp($u64 divs) {
    for (u64* p = divs[0]; p < divs[1]; ++p) {
        u8 bl = u64bytelen(*p);
        if (bl == 0 || u64byte(*p, bl - 1) < MARK_P)
            *p |= ((u64)MARK_P) << (bl << 3);
    }
}

pro(MARKnester, MARKstate* state) {
    sane(state != nil);
    u64$ divs = Bu64data(state->divs);
    nestany(divs, 0);
    addp(divs);
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
            // call(MARK2lexer, state);
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

ok64 _open($u8 $into, u32 tag) { return OK; }
ok64 _close($u8 $into, u32 tag) { return OK; }

pro(MARKlinehtml, $u8 $into, u32 line, MARKstate const* state) {
    sane($ok($into));
    done;
}

$u8c H1TRIO[3] = {
    $u8str("<h1>"),
    $u8str("</h1><h1>"),
    $u8str("</h1>\n"),
};
$u8c H2TRIO[3] = {
    $u8str("<h2>"),
    $u8str("</h2><h2>"),
    $u8str("</h2>\n"),
};
$u8c H3TRIO[3] = {
    $u8str("<h3>"),
    $u8str("</h3><h3>"),
    $u8str("</h3>\n"),
};
$u8c H4TRIO[3] = {
    $u8str("<h4>"),
    $u8str("</h4><h4>"),
    $u8str("</h4>\n"),
};
$u8c OLTRIO[3] = {
    $u8str("<ol><li>"),
    $u8str("</li><li>"),
    $u8str("</li></ol>\n"),
};
$u8c ULTRIO[3] = {
    $u8str("<ul><li>"),
    $u8str("</li><li>"),
    $u8str("</li></ul>\n"),
};
$u8c HRTRIO[3] = {
    $u8str("<hr>"),
    $u8str("</hr><hr>"),
    $u8str("</hr>\n"),
};
$u8c PTRIO[3] = {
    $u8str("<p>"),
    $u8str(""),
    $u8str("</p>\n"),
};
$cu8c QUOTETRIO[3] = {
    $u8str("<blockquote>"),
    $u8str(""),
    $u8str("</blockquote>\n"),
};

fun $cu8c* tag4mark(u8 mark) {
    switch (mark) {
        case MARK_H1:
            return H1TRIO;
        case MARK_H2:
            return H2TRIO;
        case MARK_H3:
            return H3TRIO;
        case MARK_H4:
            return H4TRIO;
        case MARK_ULIST:
            return ULTRIO;
        case MARK_OLIST:
            return OLTRIO;
        case MARK_HLINE:
            return HRTRIO;
        case MARK_P:
            return PTRIO;
        case MARK_QUOTE:
            return QUOTETRIO;
        default:
            return nil;
    }
}

pro(tagopen, $u8 $into, u8 tag) {
    sane($into != nil);
    $cu8c* trio = tag4mark(tag);
    call($u8feed, $into, trio[0]);
    done;
}

pro(tagrefresh, $u8 $into, u8 tag) {
    sane($into != nil);
    $cu8c* trio = tag4mark(tag);
    call($u8feed, $into, trio[1]);
    done;
}

pro(tagclose, $u8 $into, u8 tag) {
    sane($into != nil);
    $cu8c* trio = tag4mark(tag);
    call($u8feed, $into, trio[2]);
    done;
}

pro(linehtml, $u8 $into, MARKstate const* state, u64 from, u64 till, u8 tab) {
    sane($ok($into));
    a$dup(u8c, line, state->lines[0] + from);
    $print(line);
    call($u8feed, $into, line);
    // just feed the text
    // acc to the line limits
    // FIXME here we don't see divs
    done;
}

pro(eatblock, u64* len, MARKstate const* state, u64 from, u64 till, u8 tab) {
    sane(len != nil);
    for (u64 l = from + 1; l < till; ++l) {
        w64 div = *(w64*)Batp(state->divs, l);
        for (u8 d = 0; d < tab; d++)
            if (div._8[d] != MARK_INDENT) {
                *len = l;
                skip;
            }
    }
    *len = till;
    done;
}

pro(html, $u8 $into, MARKstate const* state, u64 from, u64 till, u8 tab) {
    sane(1);
    u8 pd = 0;
    u64 l = from;
    while (l < till) {
        w64 div = *(w64*)Batp(state->divs, l);
        u64 btill = l + 1;
        u8 tag = 0;
        if (w64bytelen(div) <= tab) {
            printf(".1\n");
            if (pd != 0) call(tagclose, $into, pd);
            call(eatblock, &btill, state, l, till, tab);
            call(linehtml, $into, state, l, btill, tab);
        } else {
            printf(".2\n");
            tag = div._8[tab];
            if (pd == tag) {
                call(tagrefresh, $into, pd);
            } else if (pd != 0) {
                call(tagclose, $into, pd);
                call(tagopen, $into, tag);
            } else {
                call(tagopen, $into, tag);
            }
            call(eatblock, &btill, state, l, till, tab + 1);
            call(html, $into, state, l, btill, tab + 1);
        }
        pd = tag;
        l = btill;
    }
    printf(".3\n");
    if (pd != 0) call(tagclose, $into, pd);
    done;
}

pro(MARKhtml, $u8 $into, MARKstate const* state) {
    sane($ok($into) && state != nil && Bok(state->lines) &&
         Bdatalen(state->lines) > 0);
    call(html, $into, state, 0, Bdatalen(state->lines) - 1, 0);
    done;
}

/*
pro(h1html, $u8 $into, MARKstate const* state, u8 tab, u64 from, u64* till) {
    sane($ok($into));
    u8cp$cc lines = Bu8cpcdata(state->lines);
    w64 divs = {._64 = {Bat(state->divs, from)}};
    call(opentag, $into, '1');
    $u8c line;
    call(MARK2lex, state, line);
    u64 l = from + 1;
    while (l < *till) {
        w64 divs = {._64 = {Bat(state->divs, l)}};
        if (divs._8[tab] != MARK_INDENT) break;
        $mv(line, line);
        call(MARK2lex, state, line);
    }
    call(linehtml, $into, state, tab, from, l);
    call(closetag, $into, '1');
    *till = l;
    done;
}

pro(MARKlihtml, $u8 $into, MARKstate const* state, u64 line) {
    sane($ok($into));
    // check nesteds
    // if nothing, p
    // scan forward
    done;
}

pro(MARKolisthtml, $u8 $into, MARKstate const* state, u64 line) {
    sane($ok($into));
    done;
}
*/

/*
pro(MARKhtml, $u8 $into, MARKstate const* state) {
    sane($ok($into));
    aBpad(u8, tagpad, 32);
    u8$ tags = Bu8data(tagpad);
    $u8c stack;
    if ($len(stack) == 0) {  // TODO p gap
    }
    $for(u8c, p, stack) {
        *tags = *tagpad;
        switch (*p) {
            case '1':
                call(closetags, $into, tags);
                call(opentag, $into, tags, '1');
                break;
            case '2':
                call(closetags, $into, tags);
                call(opentag, $into, tags, '2');
                break;
            case '.':
                if ($empty(tags)) {
                    call(opentag, $into, tags, '.');
                } else if (**tags == '.') {
                    ++*tags;
                    call(closetags, $into, tags);
                } else {
                    call(closetags, $into, tags);
                    call(opentag, $into, tags, '.');
                }
                call(opentag, $into, tags, '*');
                call(opentag, $into, tags, ' ');
                break;
            case '-':
                // TODO same
                break;
            case '~':
                call(closetags, $into, tags);
                call(opentag, $into, tags, '~');
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
pro(MARKhtml, $u8 $into, MARKstate const* state) {
    sane($ok($into));
    con u32 MASK = (1 << MARK_BITS) - 1;
    u32 pfmt = 0;
    u32 fmt = 0;
    for (u32 line = 0; line < $len(Bdata(state->linefmt)); ++line) {
        fmt = Bat(state->linefmt, line);
        u8 pr = MARKprefix(pfmt, fmt);
        u8 pl = MARKdivlen(pfmt);
        while (pl > pr) {
            call(_close, $into, pfmt & MASK);
            pfmt >>= MARK_BITS;
            --pl;
        }
        u8 l = MARKdivlen(fmt);
        while (l > pr) {
            u8 w = (l - pr - 1) * MARK_BITS;
            call(_open, $into, (fmt >> w) & MASK);
            ++l;
        }
        call(MARKlinehtml, $into, line, state);
        pfmt = fmt;
    }
    while (pfmt > 0) {
        call(_close, $into, pfmt & MASK);
        pfmt >>= MARK_BITS;
    }
    done;
}
*/
