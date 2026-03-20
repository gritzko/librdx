#ifndef LIBRDX_CSS_H
#define LIBRDX_CSS_H

#include "json/BASON.h"

con ok64 CSSBAD = 0x1c1584de8d;

typedef struct {
    u8cs data;
    u8bp buf;
    u64bp idx;
    u8 after_dot;
    u8 pending_comb;
    int paren_depth;
} CSSstate;

ok64 CSSLexer(CSSstate *state);

ok64 CSSParse(u8bp qbuf, u64bp qidx, u8cs selector);

ok64 CSSMatch(u8s out, u8cs bason_data, u8cs query,
              int context_lines, b8 use_color);

#endif
