#include "PATH.h"

#include <stdio.h>
#include <string.h>

#include "PRO.h"
#include "TEST.h"

// Helper: declare const gauge from C string
#define a_path8cg(name, cstr) u8cg name = path8cgOf(cstr)

// Table-driven tests for path operations

ok64 PATHTestVerify() {
    sane(1);

    // Valid paths
    static char const* valid[] = {
        "/",
        "/usr/bin",
        "relative/path",
        ".",
        "..",
        "../foo",
        "a/b/c",
        "/a/b/c/",
        "файл",  // UTF-8
        "/путь/к/файлу",
    };

    for (size_t i = 0; i < sizeof(valid) / sizeof(valid[0]); i++) {
        a_path8cg(path, valid[i]);
        ok64 o = path8gVerify(path);
        if (o != OK) {
            fprintf(stderr, "FAIL verify valid[%zu]: %s\n", i, valid[i]);
            return PATHBAD;
        }
    }

    // Invalid paths (contain \r, \t, \n, \0)
    static struct { char const* path; size_t len; } invalid[] = {
        {"path\twith\ttab", 13},
        {"path\nwith\nnewline", 17},
        {"path\rwith\rreturn", 16},
        {"path\0with\0null", 14},
    };

    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        u8cp pstart = (u8cp)invalid[i].path;
        u8cp pend = pstart + invalid[i].len;
        u8cg path = {pstart, pend, pend};
        ok64 o = path8gVerify(path);
        if (o != PATHBAD) {
            fprintf(stderr, "FAIL verify invalid[%zu] should fail\n", i);
            return PATHFAIL;
        }
    }

    done;
}

ok64 PATHTestIsAbsolute() {
    sane(1);

    static struct { char const* path; b8 expected; } cases[] = {
        {"/", YES},
        {"/usr/bin", YES},
        {"relative", NO},
        {"./local", NO},
        {"../up", NO},
        {"", NO},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        a_path8cg(path, cases[i].path);
        b8 result = path8gIsAbsolute(path);
        if (result != cases[i].expected) {
            fprintf(stderr, "FAIL isAbsolute[%zu]: %s expected %d got %d\n",
                    i, cases[i].path, cases[i].expected, result);
            return PATHFAIL;
        }
    }

    done;
}

ok64 PATHTestNext() {
    sane(1);

    // Test segment iteration
    static struct { char const* path; char const* segments[8]; } cases[] = {
        {"/a/b/c", {"a", "b", "c", NULL}},
        {"a/b/c", {"a", "b", "c", NULL}},
        {"/", {NULL}},
        {"", {NULL}},
        {"single", {"single", NULL}},
        {"/one", {"one", NULL}},
        {"//double//slash", {"", "double", "", "slash", NULL}},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        a_path8cg(path, cases[i].path);
        // Create a working copy of the gauge for iteration
        u8cg rem = {path[0], path[1], path[2]};
        size_t seg_idx = 0;

        while (cases[i].segments[seg_idx] != NULL) {
            u8cs seg = {};
            ok64 o = path8gNext(rem, seg);
            if (o != OK) {
                fprintf(stderr, "FAIL next[%zu] seg %zu: unexpected END\n", i, seg_idx);
                return PATHFAIL;
            }
            char const* expected = cases[i].segments[seg_idx];
            u8cs exp_slice = {(u8cp)expected, (u8cp)(expected + strlen(expected))};
            if (!$eq(seg, exp_slice)) {
                fprintf(stderr, "FAIL next[%zu] seg %zu: expected '%s' got '%.*s'\n",
                        i, seg_idx, expected, (int)$len(seg), *seg);
                return PATHFAIL;
            }
            seg_idx++;
        }

        // Verify END
        u8cs seg = {};
        ok64 o = path8gNext(rem, seg);
        if (o != END) {
            fprintf(stderr, "FAIL next[%zu]: expected END after segments\n", i);
            return PATHFAIL;
        }
    }

    done;
}

ok64 PATHTestNorm() {
    sane(1);

    static struct { char const* input; char const* expected; } cases[] = {
        // Basic normalization
        {"/a/b/c", "/a/b/c"},
        {"a/b/c", "a/b/c"},

        // Dot removal
        {"/a/./b/./c", "/a/b/c"},
        {"./a/b", "a/b"},
        {"a/./b/.", "a/b"},

        // Double dot handling
        {"/a/b/../c", "/a/c"},
        {"/a/b/c/../..", "/a"},
        {"a/b/../c", "a/c"},
        {"../a/b", "../a/b"},
        {"../../a", "../../a"},

        // Absolute paths: .. at root is ignored
        {"/a/../..", "/"},
        {"/../../../a", "/a"},

        // Double slashes
        {"/a//b///c", "/a/b/c"},
        {"a//b", "a/b"},

        // Combined
        {"/a/b/../c/./d/../e", "/a/c/e"},
        {"./a/../b/./c/..", "b"},

        // Edge cases
        {"/", "/"},
        {".", "."},
        {"..", ".."},
        {"", "."},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        a_path8cg(input, cases[i].input);
        u8cs expected = {(u8cp)cases[i].expected, (u8cp)(cases[i].expected + strlen(cases[i].expected))};

        a_pad(u8, normbuf, 256);
        u8gp norm = u8bDataIdle(normbuf);
        ok64 o = path8gNorm(norm, input);
        if (o != OK) {
            fprintf(stderr, "FAIL norm[%zu]: %s returned %s\n", i, cases[i].input, ok64str(o));
            return o;
        }

        u8cs result = {normbuf[1], norm[1]};
        if (!$eq(result, expected)) {
            fprintf(stderr, "FAIL norm[%zu]: '%s' expected '%s' got '%.*s'\n",
                    i, cases[i].input, cases[i].expected, (int)$len(result), *result);
            return PATHFAIL;
        }
    }

    done;
}

ok64 PATHTestRelative() {
    sane(1);

    // Triplets: base, target, expected_relative
    // Using Python os.path.relpath semantics (all paths as directories)
    static struct { char const* base; char const* target; char const* expected; } cases[] = {
        // Same path
        {"/a/b/c", "/a/b/c", "."},

        // Siblings (directory semantics: go up then across)
        {"/a/b/c", "/a/b/d", "../d"},
        {"/dir/file1", "/dir/file2", "../file2"},

        // Go up
        {"/a/b/c", "/a/x", "../../x"},
        {"/a/b/c", "/x", "../../../x"},
        {"/a/b/c/d", "/a/e", "../../../e"},

        // Go down
        {"/a", "/a/b/c", "b/c"},
        {"/a/b", "/a/b/c/d", "c/d"},

        // Root
        {"/", "/a/b", "a/b"},
        {"/a/b", "/", "../.."},

        // Complex
        {"/a/b/c/d", "/a/x/y/z", "../../../x/y/z"},
        {"/home/user/projects", "/home/user/documents/file", "../documents/file"},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        a_path8cg(base, cases[i].base);
        a_path8cg(target, cases[i].target);
        u8cs expected = {(u8cp)cases[i].expected, (u8cp)(cases[i].expected + strlen(cases[i].expected))};

        a_pad(u8, relbuf, 256);
        u8gp rel = u8bDataIdle(relbuf);
        ok64 o = path8gRelative(rel, base, target);
        if (o != OK) {
            fprintf(stderr, "FAIL relative[%zu]: returned %s\n", i, ok64str(o));
            return o;
        }

        u8cs result = {relbuf[1], rel[1]};
        if (!$eq(result, expected)) {
            fprintf(stderr, "FAIL relative[%zu]: base='%s' target='%s' expected='%s' got='%.*s'\n",
                    i, cases[i].base, cases[i].target, cases[i].expected,
                    (int)$len(result), *result);
            return PATHFAIL;
        }
    }

    done;
}

ok64 PATHTestAbsolute() {
    sane(1);

    // Triplets: base, relative, expected_absolute
    // Using directory semantics (base is a directory, rel is resolved within it)
    static struct { char const* base; char const* rel; char const* expected; } cases[] = {
        // Relative path resolution
        {"/a/b/c", "d", "/a/b/c/d"},
        {"/a/b/c", "../d", "/a/b/d"},
        {"/a/b/c", "../../d", "/a/d"},
        {"/a/b/c", "./d", "/a/b/c/d"},

        // Absolute relative (just copy)
        {"/a/b/c", "/x/y", "/x/y"},

        // Go up to root
        {"/a/b/c", "../../..", "/"},
        {"/a/b/c", "../../../..", "/"},

        // Current and parent
        {"/a/b/c", ".", "/a/b/c"},
        {"/a/b/c", "..", "/a/b"},

        // Complex
        {"/home/user/file", "../other/./new/../final", "/home/user/other/final"},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        a_path8cg(base, cases[i].base);
        a_path8cg(relpath, cases[i].rel);
        u8cs expected = {(u8cp)cases[i].expected, (u8cp)(cases[i].expected + strlen(cases[i].expected))};

        a_pad(u8, absbuf, 256);
        u8gp abs = u8bDataIdle(absbuf);
        ok64 o = path8gAbsolute(abs, base, relpath);
        if (o != OK) {
            fprintf(stderr, "FAIL absolute[%zu]: returned %s\n", i, ok64str(o));
            return o;
        }

        u8cs result = {absbuf[1], abs[1]};
        if (!$eq(result, expected)) {
            fprintf(stderr, "FAIL absolute[%zu]: base='%s' rel='%s' expected='%s' got='%.*s'\n",
                    i, cases[i].base, cases[i].rel, cases[i].expected,
                    (int)$len(result), *result);
            return PATHFAIL;
        }
    }

    done;
}

ok64 PATHTestPush() {
    sane(1);

    a_pad(u8, pathbuf, 256);
    u8gp path = u8bDataIdle(pathbuf);

    // Start with empty
    u8cs seg1 = {(u8cp)"first", (u8cp)"first" + 5};
    call(path8gPush, path, seg1);
    u8cs expected1 = {(u8cp)"first", (u8cp)"first" + 5};
    u8cs result1 = {pathbuf[1], path[1]};
    if (!$eq(result1, expected1)) {
        fprintf(stderr, "FAIL push[0]: expected 'first' got '%.*s'\n", (int)$len(result1), *result1);
        return PATHFAIL;
    }

    // Push second
    u8cs seg2 = {(u8cp)"second", (u8cp)"second" + 6};
    call(path8gPush, path, seg2);
    u8cs expected2 = {(u8cp)"first/second", (u8cp)"first/second" + 12};
    u8cs result2 = {pathbuf[1], path[1]};
    if (!$eq(result2, expected2)) {
        fprintf(stderr, "FAIL push[1]: expected 'first/second' got '%.*s'\n", (int)$len(result2), *result2);
        return PATHFAIL;
    }

    // Test invalid segments are rejected
    a_pad(u8, badbuf, 256);
    u8gp badpath = u8bDataIdle(badbuf);

    // Segment with slash should fail
    u8cs bad_slash = {(u8cp)"a/b", (u8cp)"a/b" + 3};
    ok64 o1 = path8gPush(badpath, bad_slash);
    if (o1 != PATHBAD) {
        fprintf(stderr, "FAIL push: segment with slash should fail\n");
        return PATHFAIL;
    }

    // Segment with tab should fail
    u8cs bad_tab = {(u8cp)"a\tb", (u8cp)"a\tb" + 3};
    ok64 o2 = path8gPush(badpath, bad_tab);
    if (o2 != PATHBAD) {
        fprintf(stderr, "FAIL push: segment with tab should fail\n");
        return PATHFAIL;
    }

    done;
}

ok64 PATHTestBase() {
    sane(1);

    static struct { char const* path; char const* expected; } cases[] = {
        {"/usr/bin/ls", "ls"},
        {"/usr/bin/", ""},
        {"/", ""},
        {"filename", "filename"},
        {"dir/file", "file"},
        {"./local", "local"},
        {"../up", "up"},
        {"/a/b/c", "c"},
        {"", ""},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        a_path8cg(path, cases[i].path);
        u8cs expected = {(u8cp)cases[i].expected, (u8cp)(cases[i].expected + strlen(cases[i].expected))};

        u8cs base = {};
        path8gBase(base, path);

        if ($len(base) != $len(expected) || ($len(base) > 0 && 0 != $cmp(base, expected))) {
            fprintf(stderr, "FAIL base[%zu]: '%s' expected '%s' got '%.*s'\n",
                    i, cases[i].path, cases[i].expected, (int)$len(base), *base);
            return PATHFAIL;
        }
    }

    done;
}

ok64 PATHTestDir() {
    sane(1);

    static struct { char const* path; char const* expected; } cases[] = {
        {"/usr/bin/ls", "/usr/bin"},
        {"/usr/bin/", "/usr/bin"},
        {"/", "/"},
        {"filename", "."},
        {"dir/file", "dir"},
        {"./local", "."},
        {"../up", ".."},
        {"/a/b/c", "/a/b"},
        {"a/b/c", "a/b"},
        {"", ""},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        a_path8cg(path, cases[i].path);
        u8cs expected = {(u8cp)cases[i].expected, (u8cp)(cases[i].expected + strlen(cases[i].expected))};

        u8cs dir = {};
        path8gDir(dir, path);

        if ($len(dir) != $len(expected) || ($len(dir) > 0 && 0 != $cmp(dir, expected))) {
            fprintf(stderr, "FAIL dir[%zu]: '%s' expected '%s' got '%.*s'\n",
                    i, cases[i].path, cases[i].expected, (int)$len(dir), *dir);
            return PATHFAIL;
        }
    }

    done;
}

ok64 PATHTestRoundTrip() {
    sane(1);

    // Verify: path8gRelative + path8gAbsolute round-trips
    static struct { char const* base; char const* target; } cases[] = {
        {"/a/b/c", "/x/y/z"},
        {"/home/user", "/var/log/syslog"},
        {"/a/b/c/d/e", "/a/b/x"},
        {"/", "/a/b"},
        {"/a/b", "/a/b/c/d"},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        a_path8cg(base, cases[i].base);
        a_path8cg(target, cases[i].target);

        // Compute relative
        a_pad(u8, relbuf, 256);
        u8gp relg = u8bDataIdle(relbuf);
        call(path8gRelative, relg, base, target);
        u8cg rel = {relbuf[1], relg[1], relg[2]};

        // Resolve back
        a_pad(u8, absbuf, 256);
        u8gp absg = u8bDataIdle(absbuf);
        call(path8gAbsolute, absg, base, rel);
        u8cs resolved = {absbuf[1], absg[1]};

        // Normalize target for comparison
        a_pad(u8, normbuf, 256);
        u8gp normg = u8bDataIdle(normbuf);
        call(path8gNorm, normg, target);
        u8cs norm_target = {normbuf[1], normg[1]};

        if (!$eq(resolved, norm_target)) {
            fprintf(stderr, "FAIL roundtrip[%zu]: base='%s' target='%s' rel='%.*s' resolved='%.*s'\n",
                    i, cases[i].base, cases[i].target,
                    (int)$len(rel), *rel, (int)$len(resolved), *resolved);
            return PATHFAIL;
        }
    }

    done;
}

ok64 PATHtest() {
    sane(1);
    call(PATHTestVerify);
    call(PATHTestIsAbsolute);
    call(PATHTestNext);
    call(PATHTestNorm);
    call(PATHTestBase);
    call(PATHTestDir);
    call(PATHTestRelative);
    call(PATHTestAbsolute);
    call(PATHTestPush);
    call(PATHTestRoundTrip);
    done;
}

TEST(PATHtest);
