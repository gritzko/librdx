#include "MARK.h"

#include "01.h"
#include "FILE.h"
#include "MARK2.h"
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

fun void addp($u64 divs) {
    for (u64* p = divs[0]; p < divs[1]; ++p) {
        u8 bl = u64bytelen(*p);
        if (bl == 0 || u64byte(*p, bl - 1) < MARK_P)
            *p |= ((u64)MARK_P) << (bl << 3);
    }
}

pro(MARKparse, MARKstate* state) {
    sane(state != nil && $ok(state->text));
    call(MARKlexer, state);
    $mv(state->text, state->doc);
    call(MARK2lexer, state);
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

fun ok64 openp($u8 $into) { return tagopen($into, MARK_P); }
fun ok64 closep($u8 $into) { return tagclose($into, MARK_P); }

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

pro(spanchain, $u8 $into, u64 lndx, u8 depth, MARKstate const* state) {
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

ok64 MARKonDiv($cu8c tok, MARKstate* state) { return OK; }

ok64 MARKonLink($cu8c tok, MARK2state* state) { return OK; }

ok64 MARKonRoot($cu8c tok, MARK2state* state) { return OK; }

ok64 MARKonLine($cu8c tok, MARKstate* state) {
    Bu64feed1(state->divs, state->div);
    state->div = 0;
    return Bu8cpfeed1(state->lines, tok[1]);
}
ok64 MARKonH1($cu8c tok, MARKstate* state) { return pushdiv(state, MARK_H1); }
ok64 MARKonH2($cu8c tok, MARKstate* state) { return pushdiv(state, MARK_H2); }
ok64 MARKonH3($cu8c tok, MARKstate* state) { return pushdiv(state, MARK_H3); }
ok64 MARKonH4($cu8c tok, MARKstate* state) { return pushdiv(state, MARK_H4); }
ok64 MARKonIndent($cu8c tok, MARKstate* state) {
    return pushdiv(state, MARK_INDENT);
}
