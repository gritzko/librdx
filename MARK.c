#include "MARK.h"

#include "01.h"
#include "FILE.h"
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

pro(openspan, $u8 $into, u8 mask) {
    sane($ok($into));
    $cu8c OPEN0 = $u8str("<span>");
    if (mask == 0) fwdcall($u8feed, $into, OPEN0);
    $cu8c OPEN = $u8str("<span class='");
    $cu8c END = $u8str("'>");
    $cu8c CLASSES[] = {
        $u8str("mark"),   $u8str("code"), $u8str("link"),
        $u8str("strong"), $u8str("emph"),
    };
    call($u8feed, $into, OPEN);
    b8 first = YES;
    while (mask != 0) {
        u8 low = ctz32(mask);
        if (!first) call($u8feed1, $into, ' ');
        call($u8feed, $into, CLASSES[low]);
        mask -= 1 << low;
        first = NO;
    }
    call($u8feed, $into, END);
    done;
}

fun ok64 closespan($u8 $into) {
    $cu8c CLOSE = $u8str("</span>");
    return $u8feed($into, CLOSE);
}

fun u64 alltabs(u8 depth) {
    if (depth == 0) return 0;
    con u64 taboid = 0x2020202020202020;
    return taboid >> ((8 - depth) << 3);
}

pro(spanchain, MARKstate const* state) {
    sane($ok($into));
    u32 fmt = 0xffff;
    a$dup(u8c, line, state->lines[0] + lndx);
    line[0] += 4 * depth;
    $for(u8c, p, line) {
        u8 f = Bat(state->fmt, p - state->doc[0]);
        if (f != fmt) {
            if (fmt != 0xffff) call(closespan, $into);
            call(openspan, $into, f);
            fmt = f;
        }
        call($u8feed1, $into, *p);  // todo segments
    }
    call(closespan, $into);
    done;
}

fun b8 pable(u64 stack) {
    u8 l = u64bytelen(stack);
    if (l == 0) return YES;
    u8 b = u64byte(stack, l - 1);
    return b == MARK_OLIST || b == MARK_ULIST || b == MARK_QUOTE;
}

pro(flushp, MARKstate* state) {
    sane(1);
    a$str(P0, "<p>");
    a$str(P1, "</p>");
    b8 hasp = pable(state->stack);
    if (hasp) call($u8feed, state->into, P0);
    call(spanchain, state);
    if (hasp) call($u8feed, state->into, P1);
    done;
}

pro(MARKHTML, $u8 $into, MARKstate const* state) {
    sane($ok($into));
    u64 stack = 0;
    u8 depth = 0;
    b8 hasp = NO;
    for (u64 l = 0; l < Bdatalen(state->divs); ++l) {
        u64 div = Bat(state->divs, l);
        u8 d = u64bytelen(div);
        depth = u64bytelen(stack);
        u64 nextdiv =
            l + 1 < Bdatalen(state->divs) ? Bat(state->divs, l + 1) : 0;
        u8 c = 0;
        while (c < d && c < depth &&
               (u64byte(div, c) == MARK_INDENT ||
                (u64byte(stack, c) == MARK_QUOTE &&
                 u64byte(div, c) == MARK_QUOTE)))
            ++c;
        printf("line [%lu] div %lu next %lu ?= alltabs %lu(dep %d) len %lu\n",
               Bdatalen(state->divs), div, nextdiv, alltabs(depth), depth,
               $len(state->lines[0] + l));
        if (div == 0 && nextdiv == alltabs(depth) &&
            $len(state->lines[0] + l) == 1 && hasp) {  // FIXME quotes
            call(closep, $into);
            call(openp, $into);
            continue;
        } else if (d == depth && c == d - 1 &&
                   u64byte(stack, c) == u64byte(div, c)) {
            if (hasp) {
                hasp = NO;
                call(closep, $into);
            }
            call(tagrefresh, $into, u64byte(stack, c));
        } else {
            while (depth > c) {
                --depth;
                if (hasp == YES) {
                    hasp = NO;
                    call(closep, $into);
                }
                call(tagclose, $into, u64byte(stack, depth));
                stack = u64lowbytes(stack, depth);
            }
            while (c < d) {
                u8 tag = u64byte(div, c);
                if (hasp == YES) {
                    hasp = NO;
                    call(closep, $into);
                }
                call(tagopen, $into, tag);
                stack = u64setbyte(stack, tag, depth);
                ++depth;
                ++c;
            }
        }
        if (pable(div) && !hasp) {
            hasp = YES;
            call(openp, $into);
        }
        call(spanchain, $into, l, depth, state);
    }
    if (hasp) {
        call(closep, $into);
        hasp = NO;
    }
    while (depth > 0) {
        --depth;
        call(tagclose, $into, u64byte(stack, depth));
    }
    done;
}

fun ok64 pushdiv(MARKstate* state, u8 div) {
    u8 l = u64bytelen(state->div);
    if (l < 8) {
        state->div = u64setbyte(state->div, div, l);
    }
    printf("pushdiv %c=%lu\n", div, state->div);
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

ok64 MARKonLink($cu8c tok, MARK2state* state) { return OK; }

ok64 MARKonRoot($cu8c tok, MARK2state* state) { return OK; }

pro(MARKonDiv, $cu8c tok, MARKstate* state) {
    sane(1);
    done;
}

pro(MARKonLine, $cu8c tok, MARKstate* state) {
    sane(1);
    u8 predepth = u64bytelen(state->stack);
    u8 depth = u64bytelen(state->div);
    $u8c body = {tok[0] + depth * 4, tok[1]};
    // close? refresh?
    if (depth != predepth) {
    }
    state->ptxt;
    state->plen;
    state->div = 0;
    state->div;
    state->stack;
    // here we realize p is over
    switch (state->mode) {
        case MARK_MODE_HTML:
            break;
        case MARK_MODE_ANSI:
            break;
    }
    state->plen = 0;
    done;
}
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
