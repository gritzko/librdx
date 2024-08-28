#include "MARQ.h"

#include <unistd.h>

#include "$.h"

pro(MARQrange, MARQfmt fmt, $cu8c tok, MARQstate* state) {
    sane(state != nil && $ok(tok) && $within(tok, state->text));
    size_t f = tok[0] - state->text[0];
    size_t t = tok[1] - state->text[0];
    for (size_t i = f; i < t; ++i) $at(state->fmt, i) |= 1 << fmt;
    done;
}

pro(MARQopenbracket, MARQfmt fmt, $cu8c tok, MARQstate* state) {
    sane(state != nil && $ok(tok) && $within(tok, state->text));
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
    sane(state != nil && $ok(tok) && $within(tok, state->text));
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
    $rm1p($b, p);
    $term($b) = 0;
    done;
}

ok64 MARQonRef0($cu8c tok, MARQstate* state) {
    return MARQopenbracket(MARQ_LINK, tok, state);
}

ok64 MARQonRef1($cu8c tok, MARQstate* state) {
    return MARQclosebracket(MARQ_LINK, tok, state);
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
