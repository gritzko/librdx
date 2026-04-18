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

// --- Canonical-key round-trip: input → DOGNormalizeArg → DOGCanonURIKey ---
//
// `expect` is what the canonical byte stream should be.  Covers the full
// pipeline: classification (query/fragment/path), dog normalisations
// (scheme→authority, port-fixup, @host split), and canonicalisation
// (scheme-stripping for transports, `file:` preservation, `//`+path).
typedef struct {
    const char *input;
    const char *expect;   // DOGCanonURIKey(norm_arg, with_query=YES)
} CanonCase;

static const CanonCase CANON_CASES[] = {
    // Pass-through URIs — scheme stripped (transports fungible).
    {"ssh://localhost/src/repo?master",    "//localhost/src/repo?master"},
    {"https://localhost/src/repo?master",  "//localhost/src/repo?master"},
    {"git://localhost/src/repo?master",    "//localhost/src/repo?master"},
    // `file:` preserved — local absolute paths need the prefix.
    {"file:///etc/passwd",                 "file:///etc/passwd"},
    // Scp-like form: `host:path`, non-numeric port glued back.
    {"ssh://localhost:src/repo?master",    "//localhost/src/repo?master"},
    {"localhost:src/repo?master",          "//localhost/src/repo?master"},
    // User@host form — no `//`, promoted via the @-split rule.
    {"gritzko@pm.me/proj?main",            "//gritzko@pm.me/proj?main"},
    {"gritzko@pm.me?main",                 "//gritzko@pm.me?main"},
    // Already canonical.
    {"//localhost/src/repo?master",        "//localhost/src/repo?master"},
    // Path-only.
    {"/absolute/path",                     "/absolute/path"},
    // Bare ref name — classified as query.
    {"master",                             "?master"},
    // 40-hex SHA — classified as query.
    {"0123456789abcdef0123456789abcdef01234567",
        "?0123456789abcdef0123456789abcdef01234567"},
    // Whitespace — classified as fragment (commit msg).  DOGCanonURIKey
    // itself strips fragments; the test helper re-appends them so the
    // full representation is observable.
    {"fix the typo",                       "#fix the typo"},
    // Bare fragment.
    {"#symbol",                            "#symbol"},
    // Bare query.
    {"?v2.8.6",                            "?v2.8.6"},
    // Numeric port preserved (real port, not a glued path).
    {"ssh://host:22/repo",                 "//host:22/repo"},
};

#define NCANON (sizeof(CANON_CASES) / sizeof(CANON_CASES[0]))

// Canonical form for the test table: include query AND fragment so
// a fragment-only input still produces something observable.
static ok64 canon_for_test(u8bp out, urip u) {
    sane(out && u);
    call(DOGCanonURIKey, out, u, YES);
    if (!u8csEmpty(u->fragment)) {
        u8bFeed1(out, '#');
        u8bFeed(out, u->fragment);
    }
    done;
}

ok64 DOGTestCanonical() {
    sane(1);
    for (size_t i = 0; i < NCANON; i++) {
        const CanonCase *tc = &CANON_CASES[i];
        u8csc text = {(u8cp)tc->input, (u8cp)tc->input + strlen(tc->input)};

        uri u = {};
        ok64 o = DOGNormalizeArg(&u, text);
        if (o != OK) {
            fprintf(stderr, "FAIL [%zu] '%s': normalize error %s\n",
                    i, tc->input, ok64str(o));
            fail(TESTFAIL);
        }

        a_pad(u8, canbuf, 1024);
        call(canon_for_test, canbuf, &u);

        a_dup(u8c, got, u8bData(canbuf));
        a_cstr(want, tc->expect);
        if (!$eq(got, want)) {
            fprintf(stderr, "FAIL [%zu] '%s':\n  got    '%.*s'\n  expect '%s'\n",
                    i, tc->input,
                    (int)$len(got), (char *)got[0], tc->expect);
            fail(TESTFAIL);
        }
    }
    done;
}

ok64 DOGtest() {
    sane(1);
    call(DOGTestDOGParseURI);
    call(DOGTestCanonical);
    done;
}

TEST(DOGtest);
