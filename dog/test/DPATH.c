#include "dog/DPATH.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

typedef struct {
    const char *input;
    ok64 expect;       // OK or DPATHBAD or DPATHFAIL
    const char *body;  // expected segment (NULL if should fail)
} DPATHCase;

static const DPATHCase DPATH_CASES[] = {

    // --- Good: plain ASCII filenames ---
    {"hello",         OK, "hello"},
    {"Makefile",      OK, "Makefile"},
    {"README.md",     OK, "README.md"},
    {"a",             OK, "a"},
    {"x86_64",        OK, "x86_64"},
    {"my-file",       OK, "my-file"},
    {"v1.0.0",        OK, "v1.0.0"},
    {"file.tar.gz",   OK, "file.tar.gz"},
    {"CAPS",          OK, "CAPS"},
    {"MixedCase.TXT", OK, "MixedCase.TXT"},
    {"a b c",         OK, "a b c"},
    {"hello world",   OK, "hello world"},

    // --- Good: numbers and underscores ---
    {"0",             OK, "0"},
    {"123",           OK, "123"},
    {"__init__",      OK, "__init__"},
    {"_",             OK, "_"},

    // --- Good: dotfiles (not .git or .dogs) ---
    {".bashrc",       OK, ".bashrc"},
    {".config",       OK, ".config"},
    {".gitignore",    OK, ".gitignore"},
    {".github",       OK, ".github"},
    {".gitmodules",   OK, ".gitmodules"},
    {".dogfood",      OK, ".dogfood"},
    {".gita",         OK, ".gita"},
    {".dogss",        OK, ".dogss"},
    {"...",            OK, "..."},
    {"....git",       OK, "....git"},

    // --- Good: UTF-8 filenames ---
    {"\xc3\xa9",               OK, "\xc3\xa9"},          // é
    {"\xc3\xbc\x62\x65\x72",  OK, "\xc3\xbc\x62\x65\x72"}, // über
    {"\xe4\xb8\xad\xe6\x96\x87", OK, "\xe4\xb8\xad\xe6\x96\x87"}, // 中文
    {"\xf0\x9f\x90\xb6",      OK, "\xf0\x9f\x90\xb6"},  // 🐶

    // --- Bad: directory traversal ---
    {"..",            DPATHBAD, NULL},

    // --- Bad: current dir ---
    {".",             DPATHBAD, NULL},

    // --- Bad: .git (case-insensitive) ---
    {".git",          DPATHBAD, NULL},
    {".GIT",          DPATHBAD, NULL},
    {".Git",          DPATHBAD, NULL},
    {".gIt",          DPATHBAD, NULL},
    {".giT",          DPATHBAD, NULL},
    {".GiT",          DPATHBAD, NULL},

    // --- Bad: .dogs (case-insensitive) ---
    {".dogs",         DPATHBAD, NULL},
    {".DOGS",         DPATHBAD, NULL},
    {".Dogs",         DPATHBAD, NULL},
    {".dOgS",         DPATHBAD, NULL},

    // --- Bad: slash in name ---
    {"a/b",           DPATHBAD, NULL},
    {"/etc",          DPATHBAD, NULL},

    // --- Bad: backslash ---
    {"a\\b",          DPATHBAD, NULL},

    // --- Bad: NUL byte ---
    {"\0hidden",      DPATHFAIL, NULL},

    // --- Bad: invalid UTF-8 ---
    {"\x80",          DPATHBAD, NULL},     // bare continuation
    {"\xC0\xAF",      DPATHBAD, NULL},     // overlong /
    {"\xC1\xBF",      DPATHBAD, NULL},     // overlong
    {"\xFE",          DPATHBAD, NULL},     // invalid lead
    {"\xFF",          DPATHBAD, NULL},     // invalid lead
    {"\xED\xA0\x80",  DPATHBAD, NULL},     // surrogate half

    // --- Bad: empty ---
    {"",              DPATHFAIL, NULL},
};

#define DPATH_NCASES (sizeof(DPATH_CASES) / sizeof(DPATH_CASES[0]))

static b8 dpath_eq(u8cs s, const char *expect) {
    if (expect == NULL) return $empty(s);
    size_t elen = strlen(expect);
    if ((size_t)$len(s) != elen) return NO;
    if (elen == 0) return YES;
    return memcmp(s[0], expect, elen) == 0;
}

ok64 DPATHTestTable() {
    sane(1);
    for (size_t i = 0; i < DPATH_NCASES; i++) {
        const DPATHCase *tc = &DPATH_CASES[i];
        // Use strlen for input length, except for NUL byte test
        size_t ilen = strlen(tc->input);
        // For the NUL test case, input is "" (strlen=0) which maps to DPATHFAIL
        u8cs input = {(u8cp)tc->input, (u8cp)tc->input + ilen};

        u8cs out = {};
        ok64 o = DPATHu8sDrainSeg(input, out);

        if (o != tc->expect) {
            fprintf(stderr, "FAIL [%zu] '%.*s': got %s want %s\n",
                    i, (int)ilen, tc->input,
                    ok64str(o), ok64str(tc->expect));
            fail(TESTFAIL);
        }

        if (o == OK && !dpath_eq(out, tc->body)) {
            fprintf(stderr, "FAIL [%zu] '%.*s': body got '%.*s' want '%s'\n",
                    i, (int)ilen, tc->input,
                    (int)$len(out),
                    $empty(out) ? "" : (char *)out[0],
                    tc->body);
            fail(TESTFAIL);
        }
    }
    done;
}

ok64 DPATHtest() {
    sane(1);
    call(DPATHTestTable);
    done;
}

TEST(DPATHtest);
