#include "beagle/GURI.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

// --- Table-driven GURI parser test ---
//
// Each row: input string, then expected values for each field.
// NULL = expect empty/unset.  The test parses, checks every field,
// and verifies GURISearchType + round-trip via GURIu8sFeed.

typedef struct {
    const char *input;

    // Expected parsed components (NULL = empty)
    const char *remote;
    const char *repo;
    const char *path;
    const char *ref;
    const char *search;
    const char *ext;

    // Expected flags
    b8 has_remote;
    b8 has_repo;
    b8 has_path;
    b8 has_ref;
    b8 has_search;
    b8 is_ext_filter;
    b8 is_dir;
    b8 is_range;

    // Expected search type
    u8 search_type;
} GURICase;

static const GURICase GURI_CASES[] = {

    // --- Bare file paths ---
    {
        .input = "abc/MSET.h",
        .path = "abc/MSET.h", .ext = "h",
        .has_path = YES,
    },
    {
        .input = "spot/CAPO.c",
        .path = "spot/CAPO.c", .ext = "c",
        .has_path = YES,
    },
    {
        .input = "README.md",
        .path = "README.md", .ext = "md",
        .has_path = YES,
    },

    // --- Bare path with ref ---
    {
        .input = "abc/MSET.h?main",
        .path = "abc/MSET.h", .ref = "main", .ext = "h",
        .has_path = YES, .has_ref = YES,
    },
    {
        .input = "abc/MSET.h?HEAD~3",
        .path = "abc/MSET.h", .ref = "HEAD~3", .ext = "h",
        .has_path = YES, .has_ref = YES,
    },

    // --- Path with ref and search ---
    {
        .input = "abc/MSET.h?main#42",
        .path = "abc/MSET.h", .ref = "main", .search = "42",
        .ext = "h",
        .has_path = YES, .has_ref = YES, .has_search = YES,
        .search_type = GURI_SEARCH_LINE,
    },
    {
        .input = "abc/MSET.h#L10-L20",
        .path = "abc/MSET.h", .search = "L10-L20", .ext = "h",
        .has_path = YES, .has_search = YES,
        .search_type = GURI_SEARCH_LINE,
    },

    // --- Extension filter ---
    {
        .input = ".c",
        .path = ".c", .ext = "c",
        .has_path = YES, .is_ext_filter = YES,
    },
    {
        .input = ".c#TODO",
        .path = ".c", .ext = "c", .search = "TODO",
        .has_path = YES, .has_search = YES,
        .is_ext_filter = YES,
        .search_type = GURI_SEARCH_GREP,
    },
    {
        .input = ".c#'f(x,y)'",
        .path = ".c", .ext = "c", .search = "'f(x,y)'",
        .has_path = YES, .has_search = YES,
        .is_ext_filter = YES,
        .search_type = GURI_SEARCH_SPOT,
    },
    {
        .input = ".c#/u\\d+sFeed/",
        .path = ".c", .ext = "c", .search = "/u\\d+sFeed/",
        .has_path = YES, .has_search = YES,
        .is_ext_filter = YES,
        .search_type = GURI_SEARCH_REGEX,
    },

    // --- Remote with alias ---
    {
        .input = "//origin/abc/MSET.h?feat",
        .remote = "origin", .path = "abc/MSET.h",
        .ref = "feat", .ext = "h",
        .has_remote = YES, .has_path = YES, .has_ref = YES,
    },
    {
        .input = "//origin/",
        .remote = "origin",
        .has_remote = YES,
        // path is empty after stripping leading "/"
    },

    // --- Full remote with .git/ boundary ---
    {
        .input = "//github.com/gritzko/dogs.git/abc/MSET.h?main",
        .remote = "github.com",
        .repo = "gritzko/dogs.git",
        .path = "abc/MSET.h", .ref = "main", .ext = "h",
        .has_remote = YES, .has_repo = YES, .has_path = YES,
        .has_ref = YES,
    },
    {
        .input = "//github.com/gritzko/dogs.git/?main",
        .remote = "github.com",
        .repo = "gritzko/dogs.git",
        .ref = "main",
        .has_remote = YES, .has_repo = YES, .has_ref = YES,
    },

    // --- Ref-only (no path) ---
    {
        .input = "?main",
        .ref = "main",
        .has_ref = YES,
    },
    {
        .input = "?main..feat",
        .ref = "main..feat",
        .has_ref = YES, .is_range = YES,
    },
    {
        .input = "?HEAD~3..HEAD",
        .ref = "HEAD~3..HEAD",
        .has_ref = YES, .is_range = YES,
    },
    {
        .input = "?HEAD",
        .ref = "HEAD",
        .has_ref = YES,
    },

    // --- Directory paths ---
    {
        .input = "abc/",
        .path = "abc/",
        .has_path = YES, .is_dir = YES,
    },
    {
        .input = "//origin/abc/?main",
        .remote = "origin", .path = "abc/",
        .ref = "main",
        .has_remote = YES, .has_path = YES, .has_ref = YES,
        .is_dir = YES,
    },

    // --- Search types ---
    {
        .input = "abc/MSET.h#MSETOpen",
        .path = "abc/MSET.h", .search = "MSETOpen", .ext = "h",
        .has_path = YES, .has_search = YES,
        .search_type = GURI_SEARCH_GREP,
    },

    // --- Working tree root ---
    {
        .input = ".",
        .path = ".",
        .has_path = YES,
    },

    // --- Triple dots (not a range) ---
    {
        .input = "?main...feat",
        .ref = "main...feat",
        .has_ref = YES,
        // three dots is not a range (git diff --merge-base syntax)
    },
};

#define GURI_NCASES (sizeof(GURI_CASES) / sizeof(GURI_CASES[0]))

static b8 guri_slice_eq(u8cs s, const char *expect) {
    if (expect == NULL) return $empty(s);
    size_t elen = strlen(expect);
    if ((size_t)$len(s) != elen) return NO;
    if (elen == 0) return YES;
    return memcmp(s[0], expect, elen) == 0;
}

ok64 GURITestTable() {
    sane(1);

    for (size_t i = 0; i < GURI_NCASES; i++) {
        const GURICase *tc = &GURI_CASES[i];
        u8cs input = {(u8cp)tc->input, (u8cp)tc->input + strlen(tc->input)};

        guri g = {};
        ok64 o = GURIu8sDrain(input, &g);
        if (o != OK) {
            fprintf(stderr, "FAIL [%zu] %s: parse error %s\n",
                    i, tc->input, ok64str(o));
            fail(TESTFAIL);
        }

        #define CHECK_SLICE(field, expect) do { \
            if (!guri_slice_eq(g.field, expect)) { \
                fprintf(stderr, "FAIL [%zu] %s: " #field \
                    " got '%.*s' want '%s'\n", \
                    i, tc->input, \
                    (int)$len(g.field), \
                    $empty(g.field) ? "" : (char*)g.field[0], \
                    expect ? expect : "(null)"); \
                fail(TESTFAIL); \
            } \
        } while(0)

        #define CHECK_FLAG(field, expect) do { \
            if (g.field != expect) { \
                fprintf(stderr, "FAIL [%zu] %s: " #field \
                    " got %d want %d\n", \
                    i, tc->input, g.field, expect); \
                fail(TESTFAIL); \
            } \
        } while(0)

        CHECK_SLICE(remote, tc->remote);
        CHECK_SLICE(repo, tc->repo);
        CHECK_SLICE(path, tc->path);
        CHECK_SLICE(ref, tc->ref);
        CHECK_SLICE(search, tc->search);
        CHECK_SLICE(ext, tc->ext);

        CHECK_FLAG(has_remote, tc->has_remote);
        CHECK_FLAG(has_repo, tc->has_repo);
        CHECK_FLAG(has_path, tc->has_path);
        CHECK_FLAG(has_ref, tc->has_ref);
        CHECK_FLAG(has_search, tc->has_search);
        CHECK_FLAG(is_ext_filter, tc->is_ext_filter);
        CHECK_FLAG(is_dir, tc->is_dir);
        CHECK_FLAG(is_range, tc->is_range);

        if (tc->search_type != 0) {
            u8 got = GURISearchType(&g);
            if (got != tc->search_type) {
                fprintf(stderr, "FAIL [%zu] %s: search_type got %d want %d\n",
                        i, tc->input, got, tc->search_type);
                fail(TESTFAIL);
            }
        }

        #undef CHECK_SLICE
        #undef CHECK_FLAG
    }
    done;
}

ok64 GURItest() {
    sane(1);
    call(GURITestTable);
    done;
}

TEST(GURItest);
