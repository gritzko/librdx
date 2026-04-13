#include "dog/FRAG.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

typedef struct {
    const char *input;
    u8 type;
    const char *body;   // NULL = empty
    u32 line;
    u32 line_end;
    const char *exts[FRAG_MAX_EXTS]; // NULL-terminated
} FRAGCase;

static const FRAGCase FRAG_CASES[] = {

    // --- Identifiers (symbol/grep) ---
    {"MSETOpen",          FRAG_IDENT, "MSETOpen", 0, 0, {NULL}},
    {"FILEFeedAll",       FRAG_IDENT, "FILEFeedAll", 0, 0, {NULL}},
    {"x",                 FRAG_IDENT, "x", 0, 0, {NULL}},
    {"_foo_bar",          FRAG_IDENT, "_foo_bar", 0, 0, {NULL}},

    // --- Identifier + line ---
    {"MSETOpen:42",       FRAG_IDENT, "MSETOpen", 42, 0, {NULL}},
    {"foo:1",             FRAG_IDENT, "foo", 1, 0, {NULL}},

    // --- Identifier + range ---
    {"MSETOpen:10-20",    FRAG_IDENT, "MSETOpen", 10, 20, {NULL}},

    // --- Identifier + ext ---
    {"FILEFeedAll.c",     FRAG_IDENT, "FILEFeedAll", 0, 0, {"c", NULL}},
    {"FILEFeedAll.c.cpp", FRAG_IDENT, "FILEFeedAll", 0, 0, {"c", "cpp", NULL}},
    {"TODO.c",            FRAG_IDENT, "TODO", 0, 0, {"c", NULL}},
    {"MSETOpen.c.h",      FRAG_IDENT, "MSETOpen", 0, 0, {"c", "h", NULL}},

    // --- Identifier + line + ext ---
    {"MSETOpen:42.c",     FRAG_IDENT, "MSETOpen", 42, 0, {"c", NULL}},
    {"foo:10-20.h",       FRAG_IDENT, "foo", 10, 20, {"h", NULL}},

    // --- Line number ---
    {"42",                FRAG_LINE,  NULL, 42, 0, {NULL}},
    {"1",                 FRAG_LINE,  NULL, 1, 0, {NULL}},
    {"999",               FRAG_LINE,  NULL, 999, 0, {NULL}},

    // --- Line range ---
    {"10-20",             FRAG_LINE,  NULL, 10, 20, {NULL}},
    {"1-1000",            FRAG_LINE,  NULL, 1, 1000, {NULL}},

    // --- Structural search (spot) ---
    {"'ok64 o = OK'",     FRAG_SPOT,  "ok64 o = OK", 0, 0, {NULL}},
    {"'f(x,y)'",          FRAG_SPOT,  "f(x,y)", 0, 0, {NULL}},
    {"'ok64 o = OK'.c",   FRAG_SPOT,  "ok64 o = OK", 0, 0, {"c", NULL}},

    // --- Structural search, unclosed ---
    {"'ok64 o = OK",      FRAG_SPOT,  "ok64 o = OK", 0, 0, {NULL}},
    {"'f(x)",             FRAG_SPOT,  "f(x)", 0, 0, {NULL}},

    // --- Regex ---
    {"/u8sFeed/",         FRAG_PCRE,  "u8sFeed", 0, 0, {NULL}},
    {"/u\\d+sFeed/",      FRAG_PCRE,  "u\\d+sFeed", 0, 0, {NULL}},
    {"/u8sFeed/.h",       FRAG_PCRE,  "u8sFeed", 0, 0, {"h", NULL}},
    {"/u8sFeed/.c.h",     FRAG_PCRE,  "u8sFeed", 0, 0, {"c", "h", NULL}},

    // --- Regex with escaped slash ---
    {"/u8s\\/Feed/",      FRAG_PCRE,  "u8s\\/Feed", 0, 0, {NULL}},
    {"/a\\/b\\/c/.c",     FRAG_PCRE,  "a\\/b\\/c", 0, 0, {"c", NULL}},
};

#define FRAG_NCASES (sizeof(FRAG_CASES) / sizeof(FRAG_CASES[0]))

static b8 frag_eq(u8cs s, const char *expect) {
    if (expect == NULL) return $empty(s);
    size_t elen = strlen(expect);
    if ((size_t)$len(s) != elen) return NO;
    if (elen == 0) return YES;
    return memcmp(s[0], expect, elen) == 0;
}

ok64 FRAGTestTable() {
    sane(1);
    for (size_t i = 0; i < FRAG_NCASES; i++) {
        const FRAGCase *tc = &FRAG_CASES[i];
        u8cs input = {(u8cp)tc->input, (u8cp)tc->input + strlen(tc->input)};

        frag f = {};
        ok64 o = FRAGu8sDrain(input, &f);
        if (o != OK) {
            fprintf(stderr, "FAIL [%zu] '%s': parse error %s\n",
                    i, tc->input, ok64str(o));
            fail(TESTFAIL);
        }

        if (f.type != tc->type) {
            fprintf(stderr, "FAIL [%zu] '%s': type got %d want %d\n",
                    i, tc->input, f.type, tc->type);
            fail(TESTFAIL);
        }

        if (!frag_eq(f.body, tc->body)) {
            fprintf(stderr, "FAIL [%zu] '%s': body got '%.*s' want '%s'\n",
                    i, tc->input,
                    (int)$len(f.body), $empty(f.body) ? "" : (char *)f.body[0],
                    tc->body ? tc->body : "(null)");
            fail(TESTFAIL);
        }

        if (f.line != tc->line) {
            fprintf(stderr, "FAIL [%zu] '%s': line got %u want %u\n",
                    i, tc->input, f.line, tc->line);
            fail(TESTFAIL);
        }

        if (f.line_end != tc->line_end) {
            fprintf(stderr, "FAIL [%zu] '%s': line_end got %u want %u\n",
                    i, tc->input, f.line_end, tc->line_end);
            fail(TESTFAIL);
        }

        // Check ext filters
        for (int e = 0; e < FRAG_MAX_EXTS; e++) {
            if (tc->exts[e] == NULL) {
                if (e != f.nexts) {
                    fprintf(stderr, "FAIL [%zu] '%s': nexts got %d want %d\n",
                            i, tc->input, f.nexts, e);
                    fail(TESTFAIL);
                }
                break;
            }
            if (e >= f.nexts) {
                fprintf(stderr, "FAIL [%zu] '%s': missing ext[%d]='%s'\n",
                        i, tc->input, e, tc->exts[e]);
                fail(TESTFAIL);
            }
            if (!frag_eq(f.exts[e], tc->exts[e])) {
                fprintf(stderr, "FAIL [%zu] '%s': ext[%d] got '%.*s' want '%s'\n",
                        i, tc->input, e,
                        (int)$len(f.exts[e]),
                        $empty(f.exts[e]) ? "" : (char *)f.exts[e][0],
                        tc->exts[e]);
                fail(TESTFAIL);
            }
        }
    }
    done;
}

ok64 FRAGtest() {
    sane(1);
    call(FRAGTestTable);
    done;
}

TEST(FRAGtest);
