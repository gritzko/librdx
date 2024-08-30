#include "MARK.h"

#include "01.h"
#include "INT.h"
#include "MARQ.h"
#include "PRO.h"

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

fun b8 MARKcontinues(u64 a, u64 b) {
    u8 al = u64bytelen(a);
    u8 bl = u64bytelen(b);
    if (al != bl) return NO;
    for (u8 i = 0; i < al; ++i) {
        switch (u64byte(b, i)) {
            case MARK_INDENT:
                break;
            case MARK_QUOTE:
                if (u64byte(a, i) != MARK_QUOTE) return NO;
                break;
            default:
                return NO;
        }
    }
    return YES;
}

pro(MARKHTML, $u8 $into, MARKstate const* state) {
    sane($ok($into) && state != nil);
    u64 stack = 0;
    u64 plen = 0;
    $u64 divs;
    $u8cp lines;
    $mv(divs, Bu64data(state->divs));
    $mv(lines, Bu8cpdata(state->lines));
    $for(u64, div, divs) {
        u64 d = *div;
        u8 depth = u64bytelen(stack);
        u8 newdepth = u64bytelen(d);
        u8 keep = 0;
        for (; keep < depth && keep < newdepth; ++keep) {
            u8 n = u64byte(d, keep);
            u8 x = u64byte(stack, keep);
            if (n == MARK_INDENT) {
                continue;
            } else if (n == MARK_QUOTE) {
                if (x == MARK_QUOTE) {
                    continue;
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        if (keep < depth) {
            $u8c ptxt = {*(*lines - plen), *(*lines + 1)};
            $u8c pfmt;
            call(MARQHTML, $into, ptxt, pfmt);
            // maybe close p
            for (u8 c = depth - 1; c >= keep; --c) {
                call(tagopen, $into, u64byte(d, c));
            }
            stack = u64lowbytes(stack, keep);
        } else if (keep == depth && u64topbyte(d) != MARK_INDENT) {
            call(tagredo, $into, u64topbyte(d));
        } else {  // maybe open things
            for (u8 o = keep; o < newdepth; ++o) {
                u8 b = u64byte(d, o);
                call(tagclose, $into, b);
                stack = u64setbyte(stack, b, o);
            }
        }
        ++*lines;
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
ok64 MARKonLink($cu8c tok, MARKstate* state) { return OK; }
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

pro(MARKonLine, $cu8c tok, MARKstate* state) {
    sane($ok(tok) && state != nil);
    if (Bempty(state->divs)) {
        state->_plen = 0;
    } else if (MARKcontinues(Blast(state->divs), state->_div)) {
        ++state->_plen;
    } else {  // a block just ended
        zero(state->marq);
        u8c* tb = state->text[0];
        u8* fb = state->fmt[0];
        size_t start = Bdatalen(state->divs) - state->_plen - 1;
        size_t boff = Bat(state->lines, start) - tb;
        size_t eoff = tok[0] - tb;
        state->marq.text[0] = tb + boff;
        state->marq.text[1] = tb + eoff;
        state->marq.fmt[0] = fb + boff;
        state->marq.fmt[1] = fb + eoff;
        call(MARQlexer, &state->marq);
        state->_plen = 0;
    }
    call(Bu64feed1, state->divs, state->_div);
    call(Bu8cpfeed1, state->lines, tok[0]);
    if (tok[1] == state->text[1]) call(Bu8cpfeed1, state->lines, tok[1]);
    state->_div = 0;
    done;
}
