#include "BE.h"

#include <stdlib.h>
#include <string.h>

#include "abc/FILE.h"
#include "abc/POL.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

// ---- Test 1: Key builders ----

ok64 BEtest1() {
    sane(1);

    typedef struct {
        const char *project;
        const char *path;
        const char *twig;
        ron60 stamp;
        const char *expected;
        int which;  // 0=head, 1=ver, 2=twig, 3=commit, 4=twigptr
    } keycase;

    keycase cases[] = {
        {"librdx", "src/main.c", NULL, 0, "librdx/src/main.c", 0},
        {"librdx", "", NULL, 0, "librdx/", 0},
        {"librdx", "README.md", "feat", 0, "librdx/README.md?y=feat", 2},
        {"librdx", "", "feat", 0, "librdx/?y=feat", 4},
    };
    int n = sizeof(cases) / sizeof(cases[0]);

    for (int i = 0; i < n; i++) {
        u8 kbuf[512];
        u8s key = {kbuf, kbuf + sizeof(kbuf)};
        u8cs proj = $u8str(cases[i].project);
        u8cs path = {};
        if (cases[i].path) {
            path[0] = (u8cp)cases[i].path;
            path[1] = (u8cp)cases[i].path + strlen(cases[i].path);
        }
        u8cs twig = {};
        if (cases[i].twig) {
            twig[0] = (u8cp)cases[i].twig;
            twig[1] = (u8cp)cases[i].twig + strlen(cases[i].twig);
        }

        ok64 o = OK;
        switch (cases[i].which) {
            case 0:
                o = BEKeyHead(key, proj, path);
                break;
            case 2:
                o = BEKeyTwig(key, proj, path, twig);
                break;
            case 4:
                o = BEKeyTwigPtr(key, proj, twig);
                break;
        }
        want(o == OK);

        u8cs got = {kbuf, key[0]};
        u8cs expected = $u8str(cases[i].expected);
        want($eq(got, expected));
    }
    done;
}

// ---- Test 2: Key builder with version stamp ----

ok64 BEtest2() {
    sane(1);
    u8 kbuf[512];
    u8s key = {kbuf, kbuf + sizeof(kbuf)};
    u8cs proj = $u8str("librdx");
    u8cs path = $u8str("src/main.c");
    ron60 stamp = 0x123;  // small test stamp
    call(BEKeyVer, key, proj, path, stamp);
    u8cs got = {kbuf, key[0]};
    // Should start with "librdx/src/main.c?v="
    a_cstr(prefix, "librdx/src/main.c?v=");
    want($len(got) > $len(prefix));
    want(memcmp(got[0], prefix[0], $len(prefix)) == 0);

    // Commit key
    u8s ckey = {kbuf, kbuf + sizeof(kbuf)};
    call(BEKeyCommit, ckey, proj, stamp);
    u8cs cgot = {kbuf, ckey[0]};
    a_cstr(cprefix, "librdx/?v=");
    want($len(cgot) > $len(cprefix));
    want(memcmp(cgot[0], cprefix[0], $len(cprefix)) == 0);
    done;
}

// ---- Test 3: BASTExport roundtrip ----

ok64 BEtest3() {
    sane(1);
    const char *source_str = "int main() { return 0; }\n";
    u8cs source = $u8str(source_str);
    u8cs ext = $u8str(".c");

    aBpad(u8, buf, 65536);
    aBpad(u64, idx, 4096);
    call(BASTParse, buf, idx, source, ext);
    u8cs bason = {buf[1], buf[2]};
    want(!$empty(bason));

    aBpad(u8, out, 65536);
    aBpad(u64, stk, 256);
    call(BASTExport, u8bIdle(out), stk, bason);
    u8cs result = {out[1], out[2]};
    want($eq(result, source));
    done;
}

// ---- Test 4: POST+GET roundtrip ----

ok64 BEtest4() {
    sane(1);

    // Create temp worktree
    a_path(wpath, "/tmp");
    a_cstr(tmpl, "BEtest4_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    // Create a source file
    a_path(fpath, "");
    call(path8gDup, path8gIn(fpath), path8cgIn(wpath));
    a_cstr(fname, "test.c");
    call(path8gPush, path8gIn(fpath), fname);
    u8cs source = $u8str("int x = 42;\n");
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, source);
    call(FILEClose, &fd);

    // Init BE
    BE be = {};
    u8cs uri = $u8str("be://BEtest4/@test/proj");
    call(BEInit, &be, uri, path8cgIn(wpath));

    // POST
    u8cs relpath = $u8str("test.c");
    u8cs *paths = &relpath;
    u8cs msg = $u8str("initial commit");
    call(BEPost, &be, 1, paths, msg);

    // Delete source file
    call(FILEUnLink, path8cgIn(fpath));

    // GET
    u8cs empty_twig = {};
    call(BEGet, &be, 1, paths, empty_twig);

    // Verify file restored
    u8bp mapbuf = NULL;
    call(FILEMapRO, &mapbuf, path8cgIn(fpath));
    u8cp r0 = mapbuf[1], r1 = mapbuf[2];
    u8cs restored = {r0, r1};
    want($eq(restored, source));
    call(FILEUnMap, mapbuf);

    // Cleanup — save repo path before close zeroes it
    u8 rpbuf[FILE_PATH_MAX_LEN];
    path8 rpath = {rpbuf, rpbuf, rpbuf, rpbuf + FILE_PATH_MAX_LEN};
    call(path8gDup, path8gIn(rpath), path8cgIn(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    done;
}

// ---- Test 5: Waypoint stored after two POSTs ----

ok64 BEtest5() {
    sane(1);

    a_path(wpath, "/tmp");
    a_cstr(tmpl, "BEtest5_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    // Write initial file
    a_path(fpath, "");
    call(path8gDup, path8gIn(fpath), path8cgIn(wpath));
    a_cstr(fname, "ver.c");
    call(path8gPush, path8gIn(fpath), fname);
    u8cs v1 = $u8str("int x = 1;\n");
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, v1);
    call(FILEClose, &fd);

    BE be = {};
    u8cs uri = $u8str("be://BEtest5/@test/proj");
    call(BEInit, &be, uri, path8cgIn(wpath));

    u8cs relpath = $u8str("ver.c");
    u8cs *paths = &relpath;
    u8cs msg1 = $u8str("v1");
    call(BEPost, &be, 1, paths, msg1);

    // Update file
    call(FILEUnLink, path8cgIn(fpath));
    u8cs v2 = $u8str("int x = 2;\n");
    fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, v2);
    call(FILEClose, &fd);

    u8cs msg2 = $u8str("v2");
    call(BEPost, &be, 1, paths, msg2);

    // Verify waypoint keys exist: scan for ?v= keys
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    a_cstr(proj, "/@test/proj/ver.c?v=");
    call(u8sFeed, pfx, proj);
    u8cs prefix = {pfxbuf, pfx[0]};

    ROCKiter it = {};
    call(ROCKIterOpen, &it, &be.db);
    call(ROCKIterSeek, &it, prefix);
    int wp_count = 0;
    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(prefix) ||
            memcmp(k[0], prefix[0], $len(prefix)) != 0)
            break;
        wp_count++;
        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);
    // Should have at least 1 waypoint (second POST generates delta)
    want(wp_count >= 1);

    u8 rpbuf5[FILE_PATH_MAX_LEN];
    path8 rpath5 = {rpbuf5, rpbuf5, rpbuf5, rpbuf5 + FILE_PATH_MAX_LEN};
    call(path8gDup, path8gIn(rpath5), path8cgIn(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath5));
    done;
}

// ---- Test 6: DELETE file ----

ok64 BEtest6() {
    sane(1);

    a_path(wpath, "/tmp");
    a_cstr(tmpl, "BEtest6_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    a_path(fpath, "");
    call(path8gDup, path8gIn(fpath), path8cgIn(wpath));
    a_cstr(fname, "del.c");
    call(path8gPush, path8gIn(fpath), fname);
    u8cs source = $u8str("void f() {}\n");
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, source);
    call(FILEClose, &fd);

    BE be = {};
    u8cs uri = $u8str("be://BEtest6/@test/proj");
    call(BEInit, &be, uri, path8cgIn(wpath));

    u8cs relpath = $u8str("del.c");
    u8cs *paths = &relpath;
    u8cs msg = $u8str("add");
    call(BEPost, &be, 1, paths, msg);

    // Verify it's stored
    u8 kbuf[512];
    u8s key = {kbuf, kbuf + sizeof(kbuf)};
    call(BEKeyHead, key, be.loc.path, relpath);
    u8cs head_key = {kbuf, key[0]};
    aBpad(u8, vbuf, 65536);
    ok64 go = ROCKGet(&be.db, vbuf, head_key);
    same(go, OK);

    // DELETE
    call(BEDelete, &be, relpath);

    // Verify gone — rebuild key since BEDelete uses loc.path
    u8 kbuf2[512];
    u8s key2 = {kbuf2, kbuf2 + sizeof(kbuf2)};
    call(BEKeyHead, key2, be.loc.path, relpath);
    u8cs head_key2 = {kbuf2, key2[0]};
    aBpad(u8, vbuf2, 256);
    go = ROCKGet(&be.db, vbuf2, head_key2);
    same(go, ROCKnone);

    u8 rpbuf6[FILE_PATH_MAX_LEN];
    path8 rpath6 = {rpbuf6, rpbuf6, rpbuf6, rpbuf6 + FILE_PATH_MAX_LEN};
    call(path8gDup, path8gIn(rpath6), path8cgIn(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath6));
    done;
}

// ---- Test 7: Checkpoint ----

ok64 BEtest7() {
    sane(1);

    a_path(wpath, "/tmp");
    a_cstr(tmpl, "BEtest7_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    a_path(fpath, "");
    call(path8gDup, path8gIn(fpath), path8cgIn(wpath));
    a_cstr(fname, "cp.c");
    call(path8gPush, path8gIn(fpath), fname);
    u8cs source = $u8str("int y = 7;\n");
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, source);
    call(FILEClose, &fd);

    BE be = {};
    u8cs uri = $u8str("be://BEtest7/@test/proj");
    call(BEInit, &be, uri, path8cgIn(wpath));

    u8cs relpath = $u8str("cp.c");
    u8cs *paths = &relpath;
    u8cs msg = $u8str("before checkpoint");
    call(BEPost, &be, 1, paths, msg);

    // Checkpoint to new branch
    u8cs new_branch = $u8str("BEtest7cp");
    call(BECheckpoint, &be, new_branch);

    // Verify new branch DB can be opened read-only
    u8 dpbuf[FILE_PATH_MAX_LEN];
    path8 dpath = {dpbuf, dpbuf, dpbuf, dpbuf + FILE_PATH_MAX_LEN};
    const char *home = getenv("HOME");
    want(home != NULL);
    a_cstr(homecs, home);
    call(u8sFeed, u8bIdle(dpath), homecs);
    call(path8gTerm, path8gIn(dpath));
    a_cstr(dotbe, ".be");
    call(path8gPush, path8gIn(dpath), dotbe);
    call(path8gPush, path8gIn(dpath), new_branch);

    ROCKdb cpdb = {};
    call(ROCKOpenRO, &cpdb, path8cgIn(dpath));

    // Verify data exists in checkpoint
    u8 kbuf[512];
    u8s key = {kbuf, kbuf + sizeof(kbuf)};
    call(BEKeyHead, key, be.loc.path, relpath);
    u8cs head_key = {kbuf, key[0]};
    aBpad(u8, vbuf, 65536);
    ok64 go = ROCKGet(&cpdb, vbuf, head_key);
    same(go, OK);

    call(ROCKClose, &cpdb);
    u8 rpbuf7[FILE_PATH_MAX_LEN];
    path8 rpath7 = {rpbuf7, rpbuf7, rpbuf7, rpbuf7 + FILE_PATH_MAX_LEN};
    call(path8gDup, path8gIn(rpath7), path8cgIn(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath7));
    call(FILErmrf, path8cgIn(dpath));
    done;
}

ok64 maintest() {
    sane(1);
    call(BEtest1);
    call(BEtest2);
    call(BEtest3);
    call(BEtest4);
    call(BEtest5);
    call(BEtest6);
    call(BEtest7);
    done;
}

TEST(maintest)
