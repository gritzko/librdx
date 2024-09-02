#include "MARQ.h"

#include <unistd.h>

#include "$.h"
#include "ANSI.h"

pro(openesc, $u8 $into, u8 mask) {
    sane($ok($into));
    while (mask != 0) {
        u8 low = ctz32(mask);
        call(escfeed, $into, MARQesc[low]);
        mask -= 1 << low;
    }
    done;
}

pro(closeesc, $u8 $into) {
    sane($ok($into));
    call(escfeed, $into, 0);
    done;
}

pro(MARQANSI, $u8 $into, $u8c const $txt, $u8c const $fmt) {
    sane($ok($into) && $len($txt) <= $len($fmt));
    u8 prev = 0;
    u8cp fp = $fmt[0];
    $for(u8c, p, $txt) {
        if (*fp != prev) {
            if (prev != 0) call(closeesc, $into);
            call(openesc, $into, *fp);
            prev = *fp;
        }
        call($u8feed1, $into, *p);  // todo segments
        ++fp;
    }
    if (prev != 0) call(closeesc, $into);
    done;
}

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

pro(MARQHTML, $u8 $into, $u8c $txt, $u8c $fmt) {
    sane($ok($into) && $len($txt) <= $len($fmt));
    u8 prev = 0xff;
    u8cp fp = $fmt[0];
    $for(u8c, p, $txt) {
        if (*fp != prev) {
            if (prev != 0xff) call(closespan, $into);
            call(openspan, $into, *fp);
            prev = *fp;
        }
        call($u8feed1, $into, *p);  // todo segments
        ++fp;
    }
    if (prev != 0xff) call(closespan, $into);
    done;
}

pro(MARQrange, MARQfmt fmt, $cu8c tok, MARQstate* state) {
    sane(state != nil && $ok(tok) && $within(state->text, tok));
    size_t f = tok[0] - state->text[0];
    size_t t = tok[1] - state->text[0];
    for (size_t i = f; i < t; ++i) $at(state->fmt, i) |= 1 << fmt;
    done;
}

pro(MARQopenbracket, MARQfmt fmt, $cu8c tok, MARQstate* state) {
    sane(state != nil && $ok(tok) && $within(state->text, tok));
    //$u8c tmpl = $u8str("MARQopenbracket '$s' at $u\n");
    // FILEfeedf(STDOUT_FILENO, tmpl, tok, tok[0] - state->text[0]);
    u8c$ text = state->text;
    $u64 $b = {};
    $u64str0((u64c**)$b, state->brackets);
    test($len($b) < MARQ_MAX_OPEN_BRACKETS, OK);
    size_t pos = tok[0] - text[0];
    *$term($b) = O1join32(pos, fmt);
    done;
}

pro(MARQclosebracket, MARQfmt fmt, $cu8c tok, MARQstate* state) {
    sane(state != nil && $ok(tok) && $within(state->text, tok));
    u8c$ text = state->text;
    $u64 $b = {};
    $u64str0((u64c**)$b, state->brackets);
    a$findif(u64, p, $b, *p != 0 && O1high32(*p) == fmt);
    test(*p != 0, OK);
    size_t f = O1low32(*p) + 1;
    size_t t = tok[1] - text[0];
    for (size_t i = f; i < t; ++i) $at(state->fmt, i) |= 1 << fmt;
    $at(state->fmt, f) |= 1 << MARQ_MARKUP;
    $at(state->fmt, t - 1) |= 1 << MARQ_MARKUP;
    $rm1p($b, p);    // FIXME this is a minefield
    *$term($b) = 0;  // FIXME this one is too (term is p!!!)
    done;
}

ok64 MARQonRef0($cu8c tok, MARQstate* state) {
    return MARQopenbracket(MARQ_LINK, tok, state);
}

pro(MARQonRef1, $cu8c tok, MARQstate* state) {
    sane(state != nil);
    call(MARQclosebracket, MARQ_LINK, tok, state);
    size_t off = tok[1] - state->text[0];
    $at(state->fmt, off - 2) |= 1 << MARQ_MARKUP;
    $at(state->fmt, off - 3) |= 1 << MARQ_MARKUP;
    $at(state->fmt, off - 4) |= 1 << MARQ_MARKUP;
    done;
}

ok64 MARQonEm0($cu8c tok, MARQstate* state) {
    return MARQopenbracket(MARQ_EMPH, tok, state);
}

ok64 MARQonEm1($cu8c tok, MARQstate* state) {
    return MARQclosebracket(MARQ_EMPH, tok, state);
}

ok64 MARQonEm($cu8c tok, MARQstate* state) {
    return MARQrange(MARQ_EMPH, tok, state);
}

pro(MARQonCode01, $cu8c tok, MARQstate* state) {
    sane(state != nil && $ok(tok) && $within(state->text, tok));
    u8c$ text = state->text;
    printf("CODE %lu\n", tok[1] - text[0]);
    $u64 $b = {};
    $u64str0((u64c**)$b, state->brackets);
    a$findif(u64, p, $b, *p != 0 && O1high32(*p) == MARQ_CODE);
    if (*p == 0) {
        size_t pos = tok[0] - text[0];
        *$term($b) = O1join32(pos, MARQ_CODE);
        skip;
    }
    size_t f = O1low32(*p);
    size_t t = tok[1] - text[0];
    for (size_t i = f; i < t; ++i) $at(state->fmt, i) |= 1 << MARQ_CODE;
    $at(state->fmt, f) |= 1 << MARQ_MARKUP;
    $at(state->fmt, t - 1) |= 1 << MARQ_MARKUP;
    $rm1p($b, p);    // FIXME this is a minefield
    *$term($b) = 0;  // FIXME this one is too (term is p!!!)
    done;
}

ok64 MARQonSt0($cu8c tok, MARQstate* state) {
    return MARQopenbracket(MARQ_STRONG, tok, state);
}

ok64 MARQonSt1($cu8c tok, MARQstate* state) {
    return MARQclosebracket(MARQ_STRONG, tok, state);
}

ok64 MARQonSt($cu8c tok, MARQstate* state) {
    return MARQrange(MARQ_STRONG, tok, state);
}
ok64 MARQonRoot($cu8c tok, MARQstate* state) { return OK; }
