#!/bin/bash
# Generator for tok/ language tokenizer boilerplate
# Usage: bash gen_tok.sh PREFIX BADCODE FAILCODE "keyword1 keyword2 ..."
#        with scanner rules piped to stdin or in a heredoc

PREFIX=$1
BAD=$2
FAIL=$3
shift 3
KEYWORDS="$*"

cat > ${PREFIX}.h << HEOF
#ifndef TOK_${PREFIX}_H
#define TOK_${PREFIX}_H

#include "TOK.h"

con ok64 ${PREFIX}BAD = ${BAD};
con ok64 ${PREFIX}FAIL = ${FAIL};

typedef struct {
    u8cs data;
    TOKcb cb;
    void *ctx;
} ${PREFIX}state;

ok64 ${PREFIX}Lexer(${PREFIX}state *state);

#endif
HEOF

# Generate .c file with keyword table
{
cat << CEOF
#include "${PREFIX}.h"

#include "abc/PRO.h"

CEOF

if [ -n "$KEYWORDS" ]; then
cat << 'KWSTART'
static const char *KW_TABLE[] = {
KWSTART
# Format keywords, 5 per line
echo "$KEYWORDS" | tr ' ' '\n' | awk '{printf "    \"%s\",", $1; if (NR%5==0) printf "\n"; else printf " "}'
echo ""
echo "    NULL,"
echo "};"
echo ""
cat << KWFUNC
static b8 ${PREFIX}IsKeyword(u8cs tok) {
    u64 len = u8csLen(tok);
    for (const char **kw = KW_TABLE; *kw; ++kw) {
        u64 kwlen = 0;
        const char *k = *kw;
        while (k[kwlen]) ++kwlen;
        if (kwlen != len) continue;
        if (__builtin_memcmp(tok[0], k, len) == 0) return YES;
    }
    return NO;
}
KWFUNC
fi

cat << CBEOF

ok64 ${PREFIX}onComment(u8cs tok, ${PREFIX}state *state) {
    sane(\$ok(tok) && state != NULL);
    if (state->cb) return state->cb('D', tok, state->ctx);
    done;
}

ok64 ${PREFIX}onString(u8cs tok, ${PREFIX}state *state) {
    sane(\$ok(tok) && state != NULL);
    if (state->cb) return state->cb('G', tok, state->ctx);
    done;
}

ok64 ${PREFIX}onNumber(u8cs tok, ${PREFIX}state *state) {
    sane(\$ok(tok) && state != NULL);
    if (state->cb) return state->cb('L', tok, state->ctx);
    done;
}

ok64 ${PREFIX}onPreproc(u8cs tok, ${PREFIX}state *state) {
    sane(\$ok(tok) && state != NULL);
    if (state->cb) return state->cb('H', tok, state->ctx);
    done;
}

ok64 ${PREFIX}onWord(u8cs tok, ${PREFIX}state *state) {
    sane(\$ok(tok) && state != NULL);
    if (state->cb) {
CBEOF

if [ -n "$KEYWORDS" ]; then
cat << KWCB
        u8 tag = ${PREFIX}IsKeyword(tok) ? 'R' : 'S';
        return state->cb(tag, tok, state->ctx);
KWCB
else
cat << NOKWCB
        return state->cb('S', tok, state->ctx);
NOKWCB
fi

cat << CBEOF2
    }
    done;
}

ok64 ${PREFIX}onPunct(u8cs tok, ${PREFIX}state *state) {
    sane(\$ok(tok) && state != NULL);
    if (state->cb) return state->cb('P', tok, state->ctx);
    done;
}

ok64 ${PREFIX}onSpace(u8cs tok, ${PREFIX}state *state) {
    sane(\$ok(tok) && state != NULL);
    if (state->cb) return state->cb('S', tok, state->ctx);
    done;
}
CBEOF2
} > ${PREFIX}.c

echo "Generated ${PREFIX}.h ${PREFIX}.c"
