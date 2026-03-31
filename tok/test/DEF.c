//
// DEF tests — symbol definition marking
//

#include "DEF.h"
#include "TOK.h"

#include "abc/PRO.h"
#include "abc/TEST.h"

// Tokenize source, run DEFMark, return tags as string
typedef struct {
    u32 toks_buf[1024];
    u32 *toks[4];  // u32b
    u8c *base;
} DEFctx;

static ok64 def_cb(u8 tag, u8cs tok, void *vctx) {
    sane(vctx != NULL);
    DEFctx *c = vctx;
    u32 end = (u32)(tok[1] - c->base);
    call(u32bFeed1, c->toks, TOK_PACK(tag, end));
    done;
}

// Tokenize + mark defs, write tags of non-whitespace tokens into out
ok64 def_test(u8csc src, u8csc ext, u8s out) {
    sane(1);
    DEFctx ctx = {};
    // init u32b manually
    ctx.toks[0] = ctx.toks_buf;
    ctx.toks[1] = ctx.toks_buf;
    ctx.toks[2] = ctx.toks_buf;
    ctx.toks[3] = ctx.toks_buf + 1024;
    ctx.base = src[0];

    TOKstate st = {.data = {src[0], src[1]}, .cb = def_cb, .ctx = &ctx};
    call(TOKLexer, &st, ext);

    // now toks[1..2) is DATA
    u32 *ts[2] = {u32bData(ctx.toks)[0], u32bData(ctx.toks)[1]};
    call(DEFMark, ts, src, ext);

    // collect tags of non-whitespace tokens
    u32 ntoks = (u32)$len(ts);
    for (u32 i = 0; i < ntoks; i++) {
        u32 tok = ts[0][i];
        u8 tag = TOK_TAG(tok);
        u8cs val;
        TOK_VAL(val, ts, src[0], i);
        // skip whitespace
        b8 ws = YES;
        for (u8c const *p = val[0]; p < val[1]; p++)
            if (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
                ws = NO;
                break;
            }
        if (ws) continue;
        call(u8sFeed1, out, tag);
    }
    done;
}

typedef struct {
    const char *src;
    const char *ext;
    const char *tags;  // expected tags (non-ws tokens)
} DEFCase;

ok64 test_c_func() {
    sane(1);
    DEFCase cases[] = {
        // simple function def
        {"int main() {}", "c", "RNPPPP"},
        // with args
        {"void foo(int x) {}", "c", "RNPRSPPP"},
        // static qualifier
        {"static int bar(void) {}", "c", "RRNPRPPP"},
        // __INLINE__ and pointer return
        {"__INLINE__ int *MyFunc() {}", "c", "SRPNPPPP"},
        // function call (not a def: has = before ()
        {"int a = foo(x);", "c", "RSPSPSPP"},
        // forward declaration
        {"void baz(int x);", "c", "RNPRSPP"},
        // struct definition
        {"struct Foo {}", "c", "RNPP"},
        // enum definition
        {"enum Color { RED }", "c", "RNPSP"},
        // typedef struct {} Name;
        {"typedef struct { int x; } Foo;", "c", "RRPRSPPNP"},
        // typedef type Name;
        {"typedef int MyInt;", "c", "RRNP"},
        // typedef void (*FuncPtr)(int);
        {"typedef void (*FuncPtr)(int);", "c", "RRPPNPPRPP"},
        // flow keyword should NOT trigger def
        {"if (fd < 0) fail(x);", "c", "RPSPLPSPSPP"},
        // return call should NOT trigger def
        {"return foo(x);", "c", "RSPSPP"},
    };
    int n = sizeof(cases) / sizeof(cases[0]);
    for (int i = 0; i < n; i++) {
        u8 tagbuf[256];
        u8s out = {tagbuf, tagbuf + 256};
        u8csc src = {(u8c *)cases[i].src,
                     (u8c *)cases[i].src + strlen(cases[i].src)};
        u8csc ext = {(u8c *)cases[i].ext,
                     (u8c *)cases[i].ext + strlen(cases[i].ext)};
        ok64 o = def_test(src, ext, out);
        if (o != OK) {
            fprintf(stderr, "FAIL C case %d: '%s' error %s\n", i,
                    cases[i].src, ok64str(o));
            fail(TESTFAIL);
        }
        u64 got_len = out[0] - tagbuf;
        u64 exp_len = strlen(cases[i].tags);
        if (got_len != exp_len ||
            memcmp(tagbuf, cases[i].tags, exp_len) != 0) {
            fprintf(stderr, "FAIL C case %d: '%s'\n  got:  %.*s\n  want: %s\n",
                    i, cases[i].src, (int)got_len, tagbuf, cases[i].tags);
            fail(TESTFAIL);
        }
    }
    done;
}

ok64 test_go() {
    sane(1);
    DEFCase cases[] = {
        {"func main() {}", "go", "RNPPPP"},
        {"type Foo struct {}", "go", "RNRPP"},
        {"var x = 5", "go", "RNPL"},
        {"const Y = 10", "go", "RNPL"},
        // method with receiver
        {"func (r *Router) Handle() {}", "go", "RPSPSPNPPPP"},
    };
    int n = sizeof(cases) / sizeof(cases[0]);
    for (int i = 0; i < n; i++) {
        u8 tagbuf[256];
        u8s out = {tagbuf, tagbuf + 256};
        u8csc src = {(u8c *)cases[i].src,
                     (u8c *)cases[i].src + strlen(cases[i].src)};
        u8csc ext = {(u8c *)cases[i].ext,
                     (u8c *)cases[i].ext + strlen(cases[i].ext)};
        ok64 o = def_test(src, ext, out);
        if (o != OK) {
            fprintf(stderr, "FAIL Go case %d: '%s' error %s\n", i,
                    cases[i].src, ok64str(o));
            fail(TESTFAIL);
        }
        u64 got_len = out[0] - tagbuf;
        u64 exp_len = strlen(cases[i].tags);
        if (got_len != exp_len ||
            memcmp(tagbuf, cases[i].tags, exp_len) != 0) {
            fprintf(stderr,
                    "FAIL Go case %d: '%s'\n  got:  %.*s\n  want: %s\n", i,
                    cases[i].src, (int)got_len, tagbuf, cases[i].tags);
            fail(TESTFAIL);
        }
    }
    done;
}

ok64 test_python() {
    sane(1);
    DEFCase cases[] = {
        {"def foo():", "py", "RNPPP"},
        {"class Bar:", "py", "RNP"},
    };
    int n = sizeof(cases) / sizeof(cases[0]);
    for (int i = 0; i < n; i++) {
        u8 tagbuf[256];
        u8s out = {tagbuf, tagbuf + 256};
        u8csc src = {(u8c *)cases[i].src,
                     (u8c *)cases[i].src + strlen(cases[i].src)};
        u8csc ext = {(u8c *)cases[i].ext,
                     (u8c *)cases[i].ext + strlen(cases[i].ext)};
        ok64 o = def_test(src, ext, out);
        if (o != OK) {
            fprintf(stderr, "FAIL Py case %d: '%s' error %s\n", i,
                    cases[i].src, ok64str(o));
            fail(TESTFAIL);
        }
        u64 got_len = out[0] - tagbuf;
        u64 exp_len = strlen(cases[i].tags);
        if (got_len != exp_len ||
            memcmp(tagbuf, cases[i].tags, exp_len) != 0) {
            fprintf(stderr,
                    "FAIL Py case %d: '%s'\n  got:  %.*s\n  want: %s\n", i,
                    cases[i].src, (int)got_len, tagbuf, cases[i].tags);
            fail(TESTFAIL);
        }
    }
    done;
}

ok64 test_kotlin() {
    sane(1);
    DEFCase cases[] = {
        {"fun greet(name: String) {}", "kt", "RNPSPSPPP"},
        {"class Foo {}", "kt", "RNPP"},
        {"val x = 5", "kt", "RNPL"},
        {"interface Bar {}", "kt", "RNPP"},
    };
    int n = sizeof(cases) / sizeof(cases[0]);
    for (int i = 0; i < n; i++) {
        u8 tagbuf[256];
        u8s out = {tagbuf, tagbuf + 256};
        u8csc src = {(u8c *)cases[i].src,
                     (u8c *)cases[i].src + strlen(cases[i].src)};
        u8csc ext = {(u8c *)cases[i].ext,
                     (u8c *)cases[i].ext + strlen(cases[i].ext)};
        ok64 o = def_test(src, ext, out);
        if (o != OK) {
            fprintf(stderr, "FAIL Kt case %d: '%s' error %s\n", i,
                    cases[i].src, ok64str(o));
            fail(TESTFAIL);
        }
        u64 got_len = out[0] - tagbuf;
        u64 exp_len = strlen(cases[i].tags);
        if (got_len != exp_len ||
            memcmp(tagbuf, cases[i].tags, exp_len) != 0) {
            fprintf(stderr,
                    "FAIL Kt case %d: '%s'\n  got:  %.*s\n  want: %s\n", i,
                    cases[i].src, (int)got_len, tagbuf, cases[i].tags);
            fail(TESTFAIL);
        }
    }
    done;
}

ok64 test_swift() {
    sane(1);
    DEFCase cases[] = {
        {"func greet(name: String) {}", "swift", "RNPSPSPPP"},
        {"class Foo {}", "swift", "RNPP"},
        {"struct Bar {}", "swift", "RNPP"},
        {"let x = 5", "swift", "RNPL"},
    };
    int n = sizeof(cases) / sizeof(cases[0]);
    for (int i = 0; i < n; i++) {
        u8 tagbuf[256];
        u8s out = {tagbuf, tagbuf + 256};
        u8csc src = {(u8c *)cases[i].src,
                     (u8c *)cases[i].src + strlen(cases[i].src)};
        u8csc ext = {(u8c *)cases[i].ext,
                     (u8c *)cases[i].ext + strlen(cases[i].ext)};
        ok64 o = def_test(src, ext, out);
        if (o != OK) {
            fprintf(stderr, "FAIL Swift case %d: '%s' error %s\n", i,
                    cases[i].src, ok64str(o));
            fail(TESTFAIL);
        }
        u64 got_len = out[0] - tagbuf;
        u64 exp_len = strlen(cases[i].tags);
        if (got_len != exp_len ||
            memcmp(tagbuf, cases[i].tags, exp_len) != 0) {
            fprintf(stderr,
                    "FAIL Swift case %d: '%s'\n  got:  %.*s\n  want: %s\n", i,
                    cases[i].src, (int)got_len, tagbuf, cases[i].tags);
            fail(TESTFAIL);
        }
    }
    done;
}

ok64 test_dart() {
    sane(1);
    DEFCase cases[] = {
        {"void main() {}", "dart", "RNPPPP"},
        {"class Foo {}", "dart", "RNPP"},
        {"class Foo { void bar() {} }", "dart", "RNPRNPPPPP"},
    };
    int n = sizeof(cases) / sizeof(cases[0]);
    for (int i = 0; i < n; i++) {
        u8 tagbuf[256];
        u8s out = {tagbuf, tagbuf + 256};
        u8csc src = {(u8c *)cases[i].src,
                     (u8c *)cases[i].src + strlen(cases[i].src)};
        u8csc ext = {(u8c *)cases[i].ext,
                     (u8c *)cases[i].ext + strlen(cases[i].ext)};
        ok64 o = def_test(src, ext, out);
        if (o != OK) {
            fprintf(stderr, "FAIL Dart case %d: '%s' error %s\n", i,
                    cases[i].src, ok64str(o));
            fail(TESTFAIL);
        }
        u64 got_len = out[0] - tagbuf;
        u64 exp_len = strlen(cases[i].tags);
        if (got_len != exp_len ||
            memcmp(tagbuf, cases[i].tags, exp_len) != 0) {
            fprintf(stderr,
                    "FAIL Dart case %d: '%s'\n  got:  %.*s\n  want: %s\n", i,
                    cases[i].src, (int)got_len, tagbuf, cases[i].tags);
            fail(TESTFAIL);
        }
    }
    done;
}

ok64 all_tests() {
    sane(1);
    call(test_c_func);
    call(test_go);
    call(test_python);
    call(test_kotlin);
    call(test_swift);
    call(test_dart);
    done;
}

TEST(all_tests)
