#include "MARK.h"

#include "abc/01.h"
#include "abc/B.h"
#include "abc/INT.h"
#include "MARQ.h"
#include "abc/OK.h"
#include "abc/PRO.h"

char MARKdivascii[] = " 1234~`_.-[>";

$u8c DIVTAGS[][3] = {{},
                     {
                         $u8str("<h1>"),
                         $u8str("</h1><h1>"),
                         $u8str("</h1>\n"),
                     },
                     {
                         $u8str("<h2>"),
                         $u8str("</h2><h2>"),
                         $u8str("</h2>\n"),
                     },
                     {
                         $u8str("<h3>"),
                         $u8str("</h3><h3>"),
                         $u8str("</h3>\n"),
                     },
                     {
                         $u8str("<h4>"),
                         $u8str("</h4><h4>"),
                         $u8str("</h4>\n"),
                     },
                     {
                         $u8str("<hr>"),
                         $u8str("</hr><hr>"),
                         $u8str("</hr>\n"),
                     },
                     {
                         $u8str("<pre>"),
                         $u8str(""),
                         $u8str("</pre>\n"),
                     },
                     {},
                     {
                         $u8str("<ol><li>"),
                         $u8str("</li><li>"),
                         $u8str("</li></ol>\n"),
                     },
                     {
                         $u8str("<ul><li>"),
                         $u8str("</li><li>"),
                         $u8str("</li></ul>\n"),
                     },
                     {
                         $u8str("<a>"),
                         $u8str(""),
                         $u8str("</a>\n"),
                     },
                     {
                         $u8str("<blockquote>"),
                         $u8str(""),
                         $u8str("</blockquote>\n"),
                     }};

fun ok64 tagopen($u8 $into, u8 tag) { return $u8feed($into, DIVTAGS[tag][0]); }
fun ok64 tagredo($u8 $into, u8 tag) { return $u8feed($into, DIVTAGS[tag][1]); }
fun ok64 tagclose($u8 $into, u8 tag) { return $u8feed($into, DIVTAGS[tag][2]); }

fun b8 pable(u64 stack) {
    u8 l = u64bytelen(stack);
    if (l == 0) return YES;
    u8 b = u64byte(stack, l - 1);
    return b == MARK_OLIST || b == MARK_ULIST || b == MARK_QUOTE;
}

a$strc(P0, "<p>");
a$strc(P1, "</p>\n");

fun b8 samediv(u64 stack, u64 div) {
    u8 al = u64bytelen(stack);
    u8 bl = u64bytelen(div);
    if (al != bl) return NO;
    for (u8 i = 0; i < al; ++i) {
        switch (u64byte(div, i)) {
            case MARK_INDENT:
                break;
            case MARK_QUOTE:
                if (u64byte(stack, i) != MARK_QUOTE) return NO;
                break;
            default:
                return NO;
        }
    }
    return YES;
}

fun b8 u8ws(u8 c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

pro(eatline, $u8c line, $u8c dline, u64* room) {
    sane($ok(line));
    b8 wasws = NO;
    u64 len = 0;
    u8c* p = *line;
    u8c* last = p;
    u64 lastroom = *room;
    while (p < line[1] && len < *room) {
        ++len;
        b8 isws = u8ws(*p);
        if (!wasws && isws) {
            last = p;
            lastroom = *room - len;
        }
        wasws = isws;
        ++p;
    }
    // if (p == line[1] && !wasws && len < *room) {
    //    last = p;
    //    lastroom = *room - len;
    // }
    if (last == *line) {
        last = line[1];  // screw it
    }
    dline[0] = line[0];
    dline[1] = last;
    line[0] = last;
    while (line[0] < line[1] && u8ws(**line)) ++line[0];
    *room = lastroom;
    done;
}

/**
    MARK_H1 = 1,
    MARK_H2 = 2,
    MARK_H3 = 3,
    MARK_H4 = 4,
    MARK_HLINE = 5,
    MARK_CODE = 6,
    MARK_INDENT = 7,
    MARK_OLIST = 8,
    MARK_ULIST = 9,
    MARK_LINK = 10,
    MARK_QUOTE = 11,
*/
$u8c MARKdivcanon[] = {
    $u8str("    "), $u8str("  # "), $u8str(" ## "), $u8str("### "),
    $u8str("####"), $u8str("~~~~"), $u8str("````"), $u8str("    "),
    $u8str("00. "), $u8str("  - "), $u8str("[ ]:"), $u8str("  > "),
};

pro(feedlistbullet, $u8 $into, u16 list) {
    sane(1);
    test($len($into) >= 4, MARKnoroom);
    if (list < 10) {
        $u8feed1($into, ' ');
        u64decfeed($into, list);
        $u8feed2($into, '.', ' ');
    } else if (list < 100) {
        u64decfeed($into, list);
        $u8feed2($into, '.', ' ');
    } else if (list < 1000) {
        u64decfeed($into, list);
        $u8feed1($into, '.');
    } else {
        u64decfeed($into, 999);
        $u8feed1($into, '.');
    }
    done;
}

pro(feedbullet, $u8 $into, u64 stack, b8 head, u16 list) {
    sane($ok($into));
    u8 depth = u64bytelen(stack);
    test($len($into) >= depth * 4, MARKnoroom);
    for (u8 d = 0; d < depth; ++d) {
        u8 b = u64byte(stack, d);
        test(b < MARK_END, MARKbadrec);
        b8 bullet = head && d + 1 == depth || b == MARK_QUOTE;
        if (bullet && b != MARK_OLIST) {
            call($u8feed, $into, MARKdivcanon[b]);
        } else if (bullet && b == MARK_OLIST) {
            call(feedlistbullet, $into, list);
        } else {
            call($u8feed, $into, MARKdivcanon[0]);
        }
    }
    done;
}

fun pro(MARKlinetext, $u8c text, u64 lno, MARKstate const* state) {
    sane(state != nil);
    $mv(text, state->lineB[0] + lno);
    u64 div = Bat(state->divB, lno);
    u8 depth = u64bytelen(div);
    test($len(text) >= depth * 4, MARKmiss);
    text[0] += depth * 4;
    done;
}

pro(MARKANSIdiv, $u8 $into, u64 lfrom, u64 ltill, u64 stack, u32 width,
    u16 list, MARKstate const* state) {
    sane($ok($into) && state != nil);
    u64 depth = u64bytelen(stack);
    test(width > depth * 4, MARKnoroom);
    u64 lno = lfrom;
    $u8c line = {};
    b8 nled = NO;
    call(MARKlinetext, line, lno, state);
    u64 room = width - depth * 4;
    call(feedbullet, $into, stack, YES, list);
    while (!$empty(line)) {
        $u8c wrap = {};
        call(eatline, line, wrap, &room);
        $u8c wfmt = {};
        range64 mark = {};
        call($u8mark, state->text, wrap, &mark);
        call($u8rewind, (u8c**)state->fmt, wfmt, mark);
        call(MARQANSI, $into, wrap, wfmt);
        nled = NO;
        if (!$empty(line)) {
            call($u8feed1, $into, '\n');
            room = width - depth * 4;
            call(feedbullet, $into, stack, NO, list);
            nled = YES;
        } else if (lno + 1 < ltill) {
            ++lno;
            call(MARKlinetext, line, lno, state);
            if ($len(line) == 1) {
                call($u8feed1, $into, '\n');
                room = width - depth * 4;
                call(feedbullet, $into, stack, NO, list);
                nled = YES;

            } else {
                nled = NO;
                if (room) {
                    --room;
                    call($u8feed1, $into, ' ');
                }
            }
        }
    }
    if (!nled) call($u8feed1, $into, '\n');
    done;
}

pro(MARKANSI, $u8 $into, u32 width, MARKstate const* state) {
    sane($ok($into) && state != nil && !Bempty(state->divB));
    u64$ divs = Bu64data(state->divB);
    u8cp$ lines = Bu8cpdata(state->lineB);
    u64 lists = 0;
    u64 divlen = 0;
    b8 hadgap = NO;
    size_t llen = $len(divs) - 1;
    u64 olddiv = 0xff;
    u64$ ps = Bu64data(state->pB);
    for (u64 p = 0; p + 1 < $len(ps); ++p) {
        u64 from = Bat(ps, p);
        u64 till = Bat(ps, p + 1);
        u64 div = $at(divs, from);
        u8 depth = u64bytelen(div);
        u16 li = 0;
        if (depth > 0 && u64getbyte(div, depth - 1) == MARK_OLIST) {
        }
        if (olddiv != div && olddiv != 0xff) call($u8feed1, $into, '\n');
        call(MARKANSIdiv, $into, from, till, div, width, li, state);
        olddiv = div;
    }
    done;
}

pro(MARKMARQdiv, u64 from, u64 till, MARKstate* state) {
    sane(1);
    MARQstate marq = {};
    u8c* tb = state->text[0];
    u8* fb = state->fmt[0];
    size_t boff = Bat(state->lineB, from) - tb;
    size_t eoff = Bat(state->lineB, till) - tb;
    marq.text[0] = tb + boff;
    marq.text[1] = tb + eoff;
    marq.fmt[0] = fb + boff;
    marq.fmt[1] = fb + eoff;
    call(MARQlexer, &marq);
    done;
}

pro(MARKMARQ, MARKstate* state) {
    sane(state != nil && !Bempty(state->divB));
    u64$ divs = Bu64data(state->divB);
    u8cp$ lines = Bu8cpdata(state->lineB);
    u64$ blocks = Bu64data(state->pB);
    for (u64 b = 0; b + 1 < $len(blocks); ++b) {
        call(MARKMARQdiv, $at(blocks, b), $at(blocks, b + 1), state);
    }
    done;
}

pro(MARKHTMLp, $u8 $into, u64 from, u64 till, u64 stack,
    MARKstate const* state) {
    sane($ok($into) && state != nil && till <= Bdatalen(state->lineB));
    u8 depth = u64bytelen(stack);
    u8c* text0 = state->text[0];
    u8c* fmt0 = state->fmt[0];
    if (pable(stack)) call($u8feed, $into, P0);
    for (u64 l = from; l < till; ++l) {
        $u8c line = {};
        call(MARKlinetext, line, l, state);
        if ($len(line) == 1) {
            call($u8feed, $into, P1);
            call($u8feed, $into, P0);
            continue;
        }
        size_t f = line[0] - text0;  // TODO mark
        size_t t = line[1] - text0;
        $u8c fmt = {fmt0 + f, fmt0 + t};
        call(MARQHTML, $into, line, fmt);
    }
    if (pable(stack)) call($u8feed, $into, P1);
    done;
}

fun u8 samedepth(u64 stack, u64 div) {
    u8 sd = u64bytelen(stack);
    u8 depth = u64bytelen(div);
    u8 d = sd > depth ? depth : sd;
    while (d > 0 && !samediv(u64lowbytes(stack, d), u64lowbytes(div, d))) --d;
    return d;
}

pro(MARKHTML, $u8 $into, MARKstate const* state) {
    sane($ok($into) && state != nil && !Bempty(state->divB));
    u64$ divs = Bu64data(state->divB);
    u8cp$ lines = Bu8cpdata(state->lineB);
    test($len(divs) == $len(lines), FAILsanity);
    u64 stack = 0;
    u64$ ps = Bu64data(state->pB);
    for (u64 p = 0; p + 1 < $len(ps); ++p) {
        u64 from = Bat(ps, p);
        u64 till = Bat(ps, p + 1);
        u64 div = $at(divs, from);
        u8 sd = u64bytelen(stack);
        u8 depth = u64bytelen(div);
        u8 keep = samedepth(stack, div);

        while (sd > keep + 1) {
            --sd;
            call(tagclose, $into, u64byte(stack, sd));
            stack = u64setbyte(stack, 0, sd);
        }

        if (sd == keep + 1 && depth == keep + 1 &&
            u64byte(div, keep) == u64byte(stack, keep)) {
            call(tagredo, $into, u64byte(stack, keep));
        } else if (sd == keep + 1) {
            --sd;
            call(tagclose, $into, u64byte(stack, sd));
            stack = u64setbyte(stack, 0, sd);
        }

        while (depth > sd) {
            u8 b = u64byte(div, sd);
            call(tagopen, $into, b);
            stack = u64setbyte(stack, b, sd);
            ++sd;
        }

        call(MARKHTMLp, $into, from, till, stack, state);
    }

    u8 sd = u64bytelen(stack);
    while (sd > 0) {
        --sd;
        call(tagclose, $into, u64byte(stack, sd));
    }
    done;
}

fun ok64 pushdiv(MARKstate* state, u8 div) {
    u8 l = u64bytelen(state->_div);
    if (l < 8) {
        state->_div = u64setbyte(state->_div, div, l);
    }
    printf("pushdiv %c=%lu\n", div, state->_div);
    return OK;
}

ok64 MARKonHLine($cu8c tok, MARKstate* state) {
    return pushdiv(state, MARK_HLINE);
}
ok64 MARKonOList($cu8c tok, MARKstate* state) {
    return pushdiv(state, MARK_OLIST);
}
ok64 MARKonUList($cu8c tok, MARKstate* state) {
    return pushdiv(state, MARK_ULIST);
}
ok64 MARKonLink($cu8c tok, MARKstate* state) {
    return pushdiv(state, MARK_LINK);
}
ok64 MARKonRoot($cu8c tok, MARKstate* state) { return OK; }
ok64 MARKonDiv($cu8c tok, MARKstate* state) { return OK; }
ok64 MARKonH1($cu8c tok, MARKstate* state) { return pushdiv(state, MARK_H1); }
ok64 MARKonH2($cu8c tok, MARKstate* state) { return pushdiv(state, MARK_H2); }
ok64 MARKonH3($cu8c tok, MARKstate* state) { return pushdiv(state, MARK_H3); }
ok64 MARKonH4($cu8c tok, MARKstate* state) { return pushdiv(state, MARK_H4); }
ok64 MARKonIndent($cu8c tok, MARKstate* state) {
    return pushdiv(state, MARK_INDENT);
}
ok64 MARKonQuote($cu8c tok, MARKstate* state) {
    return pushdiv(state, MARK_QUOTE);
}

ok64 MARKonCode($cu8c tok, MARKstate* state) {
    return pushdiv(state, MARK_CODE);
}

pro(MARKonLine, $cu8c tok, MARKstate* state) {
    sane($ok(tok) && state != nil);
    b8 end = tok[1] == state->text[1];
    if (Bempty(state->pB) ||
        !samediv(Blast(state->divB), state->_div)) {  // FIXME gaps
        call(Bu64feed1, state->pB, Bdatalen(state->lineB));
    }
    call(Bu64feed1, state->divB, state->_div);
    call(Bu8cpfeed1, state->lineB, tok[0]);
    if (tok[1] == state->text[1]) {
        call(Bu64feed1, state->pB, Bdatalen(state->lineB));
        call(Bu8cpfeed1, state->lineB, tok[1]);
        call(Bu64feed1, state->divB, 0);
    }
    state->_div = 0;
    done;
}
