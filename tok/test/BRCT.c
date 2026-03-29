#include "BRCT.h"
#include "CT.h"

#include "abc/BUF.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

// --- Tokenize helper ---

typedef struct {
    u32 **toks;
    u8cp base;
} brct_ctx;

static ok64 brct_cb(u8 tag, u8cs tok, void *vctx) {
    brct_ctx *ctx = vctx;
    u32 end = (u32)(tok[1] - ctx->base);
    return u32bFeed1(ctx->toks, TOK_PACK(tag, end));
}

// Tokenize a C string, filling toks buffer.
static ok64 brct_tokenize(u32bp toks, u8csc data) {
    sane(1);
    brct_ctx ctx = {.toks = (u32 **)toks, .base = data[0]};
    CTstate st = {.data = {data[0], data[1]}, .cb = brct_cb, .ctx = &ctx};
    call(CTLexer, &st);
    done;
}

#define SRC(s) {(u8cp)(s), (u8cp)(s) + sizeof(s) - 1}

// --- Tests ---

ok64 BRCTtest_match() {
    sane(1);
    Bu32 buf = {};
    u32bAllocate(buf, 256);
    u8csc src = SRC("int f() { return 0; }");
    call(brct_tokenize, buf, src);
    u32cs toks = {buf[1], buf[2]};

    // find the ( and ) tokens, the { and } tokens
    u64 n = $len(toks);
    i64 open_paren = -1, close_paren = -1;
    i64 open_brace = -1, close_brace = -1;
    for (u64 i = 0; i < n; i++) {
        if (TOK_TAG(toks[0][i]) != 'P') continue;
        u32 lo = (i > 0) ? TOK_OFF(toks[0][i - 1]) : 0;
        u8 ch = src[0][lo];
        if (ch == '(' && open_paren < 0) open_paren = (i64)i;
        if (ch == ')' && close_paren < 0) close_paren = (i64)i;
        if (ch == '{' && open_brace < 0) open_brace = (i64)i;
        if (ch == '}' && close_brace < 0) close_brace = (i64)i;
    }

    // match forward
    want(open_paren >= 0 && close_paren >= 0);
    testeq(BRCTMatch(toks, src, (u64)open_paren), close_paren);
    testeq(BRCTMatch(toks, src, (u64)open_brace), close_brace);

    // match backward
    testeq(BRCTMatch(toks, src, (u64)close_paren), open_paren);
    testeq(BRCTMatch(toks, src, (u64)close_brace), open_brace);

    // non-bracket token returns -1
    testeq(BRCTMatch(toks, src, 0), -1);

    u32bFree(buf);
    done;
}

ok64 BRCTtest_nested() {
    sane(1);
    Bu32 buf = {};
    u32bAllocate(buf, 256);
    u8csc src = SRC("{ { x } }");
    call(brct_tokenize, buf, src);
    u32cs toks = {buf[1], buf[2]};

    // tokens: { SP { SP x SP } SP }
    // find positions of the braces
    u64 n = $len(toks);
    u64 braces[4];
    u64 bc = 0;
    for (u64 i = 0; i < n && bc < 4; i++) {
        if (TOK_TAG(toks[0][i]) != 'P') continue;
        u32 lo = (i > 0) ? TOK_OFF(toks[0][i - 1]) : 0;
        u8 ch = src[0][lo];
        if (ch == '{' || ch == '}') braces[bc++] = i;
    }
    want(bc == 4);  // outer{, inner{, inner}, outer}

    // outer { matches outer }
    testeq(BRCTMatch(toks, src, braces[0]), (i64)braces[3]);
    // inner { matches inner }
    testeq(BRCTMatch(toks, src, braces[1]), (i64)braces[2]);

    // find 'x' token index
    u64 x_idx = 0;
    for (u64 i = 0; i < n; i++) {
        if (TOK_TAG(toks[0][i]) == 'S') {
            u32 lo = (i > 0) ? TOK_OFF(toks[0][i - 1]) : 0;
            if (src[0][lo] == 'x') { x_idx = i; break; }
        }
    }

    // inner region around x
    u64 from, till;
    call(BRCTInner, &from, &till, toks, src, x_idx);
    testeq(from, braces[1]);
    testeq(till, braces[2]);

    // outer region around x
    call(BRCTOuter, &from, &till, toks, src, x_idx);
    testeq(from, braces[0]);
    testeq(till, braces[3]);

    u32bFree(buf);
    done;
}

ok64 BRCTtest_check() {
    sane(1);
    Bu32 buf = {};
    u32bAllocate(buf, 256);

    // balanced
    {
        u8csc src = SRC("f({ a[0] })");
        Breset(buf);
        call(brct_tokenize, buf, src);
        u32cs toks = {buf[1], buf[2]};
        call(BRCTCheck, toks, src);
    }

    // unbalanced: missing close
    {
        u8csc src = SRC("f({ a[0] )");
        Breset(buf);
        call(brct_tokenize, buf, src);
        u32cs toks = {buf[1], buf[2]};
        ok64 o = BRCTCheck(toks, src);
        testeq(o, BRCTBAD);
    }

    // unbalanced: extra close
    {
        u8csc src = SRC("f() }");
        Breset(buf);
        call(brct_tokenize, buf, src);
        u32cs toks = {buf[1], buf[2]};
        ok64 o = BRCTCheck(toks, src);
        testeq(o, BRCTBAD);
    }

    u32bFree(buf);
    done;
}

ok64 BRCTtest_depth() {
    sane(1);
    Bu32 buf = {};
    u32bAllocate(buf, 256);
    u8csc src = SRC("a { b { c } d } e");
    call(brct_tokenize, buf, src);
    u32cs toks = {buf[1], buf[2]};

    // find token positions by first byte
    u64 n = $len(toks);
    u64 a_i = 0, b_i = 0, c_i = 0, d_i = 0, e_i = 0;
    for (u64 i = 0; i < n; i++) {
        if (TOK_TAG(toks[0][i]) != 'S') continue;
        u32 lo = (i > 0) ? TOK_OFF(toks[0][i - 1]) : 0;
        u8 ch = src[0][lo];
        if (ch == 'a') a_i = i;
        if (ch == 'b') b_i = i;
        if (ch == 'c') c_i = i;
        if (ch == 'd') d_i = i;
        if (ch == 'e') e_i = i;
    }

    testeq(BRCTDepth(toks, src, a_i), 0);
    testeq(BRCTDepth(toks, src, b_i), 1);
    testeq(BRCTDepth(toks, src, c_i), 2);
    testeq(BRCTDepth(toks, src, d_i), 1);
    testeq(BRCTDepth(toks, src, e_i), 0);

    u32bFree(buf);
    done;
}

ok64 BRCTtest_none() {
    sane(1);
    Bu32 buf = {};
    u32bAllocate(buf, 256);
    u8csc src = SRC("int x;");
    call(brct_tokenize, buf, src);
    u32cs toks = {buf[1], buf[2]};

    u64 from, till;
    ok64 o = BRCTInner(&from, &till, toks, src, 1);
    testeq(o, BRCTNONE);

    u32bFree(buf);
    done;
}

ok64 BRCTtest_string_brackets() {
    sane(1);
    Bu32 buf = {};
    u32bAllocate(buf, 256);
    // brackets inside string should be ignored
    u8csc src = SRC("f(\"{[\")");
    call(brct_tokenize, buf, src);
    u32cs toks = {buf[1], buf[2]};

    // should be balanced (string brackets don't count)
    call(BRCTCheck, toks, src);

    u32bFree(buf);
    done;
}

ok64 BRCTtest() {
    sane(1);
    call(BRCTtest_match);
    call(BRCTtest_nested);
    call(BRCTtest_check);
    call(BRCTtest_depth);
    call(BRCTtest_none);
    call(BRCTtest_string_brackets);
    done;
}

TEST(BRCTtest)
