#include "dog/DOG.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

typedef struct {
    const char *input;
    const char *scheme;
    const char *authority;
    const char *path;
    const char *query;
    const char *fragment;
} ParseCase;

static const ParseCase CASES[] = {

    // --- Normalized: bare host:path (no //) ---
    {"localhost:src/git/protocol.h",
        NULL, "localhost", "src/git/protocol.h", NULL, NULL},
    {"origin:docs/README.md",
        NULL, "origin", "docs/README.md", NULL, NULL},
    {"host:path?ref#frag",
        NULL, "host", "path", "ref", "frag"},
    {"localhost:src/git/protocol.h?v2.8.6#protocol_version",
        NULL, "localhost", "src/git/protocol.h",
        "v2.8.6", "protocol_version"},

    // --- Unchanged: proper URIs with // ---
    {"https://example.com/path",
        "https", "//example.com", "/path", NULL, NULL},
    {"file:///etc/passwd",
        "file", "//", "/etc/passwd", NULL, NULL},
    {"//localhost/src/file",
        NULL, "//localhost", "/src/file", NULL, NULL},

    // --- Unchanged: file:/path (scheme with rooted path) ---
    {"file:/etc/passwd",
        "file", NULL, "/etc/passwd", NULL, NULL},

    // --- No scheme: just path ---
    {"src/git/protocol.h",
        NULL, NULL, "src/git/protocol.h", NULL, NULL},
    {"/absolute/path",
        NULL, NULL, "/absolute/path", NULL, NULL},

    // --- Query/fragment only ---
    {"?v2.8.6",
        NULL, NULL, NULL, "v2.8.6", NULL},
    {"#symbol",
        NULL, NULL, NULL, NULL, "symbol"},

    // --- Non-numeric "port" glued back into path ---
    // RFC 3986 eats `src` as the port; we fix it up.
    {"ssh://localhost:src/dogs-sniff",
        "ssh", "//localhost", "src/dogs-sniff", NULL, NULL},
    {"ssh://host:repo",
        "ssh", "//host", "repo", NULL, NULL},
    // Numeric port left alone.
    {"ssh://host:22/repo",
        "ssh", "//host:22", "/repo", NULL, NULL},
};

#define NCASES (sizeof(CASES) / sizeof(CASES[0]))

static b8 s_eq(u8cs s, const char *expect) {
    if (expect == NULL) return $empty(s);
    size_t elen = strlen(expect);
    if ((size_t)$len(s) != elen) return NO;
    if (elen == 0) return YES;
    return memcmp(s[0], expect, elen) == 0;
}

static b8 check(size_t i, const char *field, u8cs got, const char *expect) {
    if (s_eq(got, expect)) return YES;
    fprintf(stderr, "FAIL [%zu] '%s': %s got '%.*s' want '%s'\n",
            i, CASES[i].input, field,
            (int)$len(got),
            $empty(got) ? "" : (char *)got[0],
            expect ? expect : "(empty)");
    return NO;
}

ok64 DOGTestDOGParseURI() {
    sane(1);
    for (size_t i = 0; i < NCASES; i++) {
        const ParseCase *tc = &CASES[i];
        u8csc text = {(u8cp)tc->input, (u8cp)tc->input + strlen(tc->input)};

        uri u = {};
        ok64 o = DOGParseURI(&u, text);
        if (o != OK) {
            fprintf(stderr, "FAIL [%zu] '%s': parse error %s\n",
                    i, tc->input, ok64str(o));
            fail(TESTFAIL);
        }

        if (!check(i, "scheme",    u.scheme,    tc->scheme))    fail(TESTFAIL);
        if (!check(i, "authority", u.authority, tc->authority)) fail(TESTFAIL);
        if (!check(i, "path",      u.path,      tc->path))      fail(TESTFAIL);
        if (!check(i, "query",     u.query,     tc->query))     fail(TESTFAIL);
        if (!check(i, "fragment",  u.fragment,  tc->fragment))  fail(TESTFAIL);
    }
    done;
}

ok64 DOGtest() {
    sane(1);
    call(DOGTestDOGParseURI);
    done;
}

TEST(DOGtest);
