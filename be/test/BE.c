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
        const char *branch;
        ron60 stamp;
        const char *expected;
        int which;  // 0=head, 1=ver, 2=branch, 3=commit, 4=branchptr
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
        u8cs branch = {};
        if (cases[i].branch) {
            branch[0] = (u8cp)cases[i].branch;
            branch[1] = (u8cp)cases[i].branch + strlen(cases[i].branch);
        }

        ok64 o = OK;
        switch (cases[i].which) {
            case 0:
                o = BEKeyHead(key, proj, path);
                break;
            case 2:
                o = BEKeyBranch(key, proj, path, branch);
                break;
            case 4:
                o = BEKeyBranchPtr(key, proj, branch);
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
    u8cs empty_branch = {};
    call(BEGet, &be, 1, paths, empty_branch);

    // Verify file restored
    u8bp mapbuf = NULL;
    call(FILEMapRO, &mapbuf, path8cgIn(fpath));
    u8cp r0 = mapbuf[1], r1 = mapbuf[2];
    u8cs restored = {r0, r1};
    want($eq(restored, source));
    call(FILEUnMap, mapbuf);

    // Cleanup — save repo path before close zeroes it
    a_path(rpath, "");
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

    a_path(rpath5, "");
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

    a_path(rpath6, "");
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

    // Checkpoint to new repo
    u8cs new_repo = $u8str("BEtest7cp");
    call(BECheckpoint, &be, new_repo);

    // Verify new repo DB can be opened read-only
    u8 dpbuf[FILE_PATH_MAX_LEN];
    path8 dpath = {dpbuf, dpbuf, dpbuf, dpbuf + FILE_PATH_MAX_LEN};
    const char *home = getenv("HOME");
    want(home != NULL);
    a_cstr(homecs, home);
    call(u8sFeed, u8bIdle(dpath), homecs);
    call(path8gTerm, path8gIn(dpath));
    a_cstr(dotbe, ".be");
    call(path8gPush, path8gIn(dpath), dotbe);
    call(path8gPush, path8gIn(dpath), new_repo);

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
    a_path(rpath7, "");
    call(path8gDup, path8gIn(rpath7), path8cgIn(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath7));
    call(FILErmrf, path8cgIn(dpath));
    done;
}

// ---- Test 8: BEGetDeps ----

ok64 BEtest8() {
    sane(1);

    // Create worktree with two "projects": proj and dep
    a_path(wpath, "/tmp");
    a_cstr(tmpl, "BEtest8_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    // Write a source file for main project
    a_path(fpath, "");
    call(path8gDup, path8gIn(fpath), path8cgIn(wpath));
    a_cstr(fname, "main.c");
    call(path8gPush, path8gIn(fpath), fname);
    u8cs main_src = $u8str("int main() { return 0; }\n");
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, main_src);
    call(FILEClose, &fd);

    // Init BE for main project
    BE be = {};
    u8cs uri = $u8str("be://BEtest8/@test/proj");
    call(BEInit, &be, uri, path8cgIn(wpath));

    // Post main project
    u8cs relpath = $u8str("main.c");
    u8cs *paths = &relpath;
    u8cs msg = $u8str("init");
    call(BEPost, &be, 1, paths, msg);

    // Manually store a dep project's file in the same repo DB
    // Simulate: project @test/deplib has a file "util.c"
    u8cs dep_proj = $u8str("/@test/deplib");
    u8cs dep_file = $u8str("util.c");
    u8 dkbuf[512];
    u8s dkey = {dkbuf, dkbuf + sizeof(dkbuf)};
    call(BEKeyHead, dkey, dep_proj, dep_file);
    u8cs dep_key = {dkbuf, dkey[0]};

    // Parse "int util() {}\n" to BASON so BEGetFile can export it
    u8cs dep_src = $u8str("int util() {}\n");
    u8cs dep_ext = $u8str(".c");
    aBpad(u8, dbuf, 65536);
    aBpad(u64, didx, 4096);
    call(BASTParse, dbuf, didx, dep_src, dep_ext);
    u8cs dep_bason = {dbuf[1], dbuf[2]};
    call(ROCKPut, &be.db, dep_key, dep_bason);

    // Also store an optional dep: @test/optlib/extra.c
    u8cs opt_proj = $u8str("/@test/optlib");
    u8cs opt_file = $u8str("extra.c");
    u8 okbuf[512];
    u8s okey = {okbuf, okbuf + sizeof(okbuf)};
    call(BEKeyHead, okey, opt_proj, opt_file);
    u8cs opt_key = {okbuf, okey[0]};

    u8cs opt_src = $u8str("void extra() {}\n");
    aBpad(u8, obuf2, 65536);
    aBpad(u64, oidx, 4096);
    call(BASTParse, obuf2, oidx, opt_src, dep_ext);
    u8cs opt_bason = {obuf2[1], obuf2[2]};
    call(ROCKPut, &be.db, opt_key, opt_bason);

    // Write .beget file
    a_path(bgpath, "");
    call(path8gDup, path8gIn(bgpath), path8cgIn(wpath));
    a_cstr(bgname, ".beget");
    call(path8gPush, path8gIn(bgpath), bgname);
    u8cs beget_content = $u8str(
        "# test deps\n"
        "[deps]\n"
        "/@test/deplib\n"
        "\n"
        "[opt]\n"
        "/@test/optlib\n");
    fd = 0;
    call(FILECreate, &fd, path8cgIn(bgpath));
    call(FILEFeedall, fd, beget_content);
    call(FILEClose, &fd);

    // --- Test: get required deps only ---
    call(BEGetDeps, &be, NO);

    // util.c should exist
    a_path(upath, "");
    call(path8gDup, path8gIn(upath), path8cgIn(wpath));
    a_cstr(uname, "util.c");
    call(path8gPush, path8gIn(upath), uname);
    u8bp umap = NULL;
    call(FILEMapRO, &umap, path8cgIn(upath));
    u8cp u0 = umap[1], u1 = umap[2];
    u8cs util_got = {u0, u1};
    want($eq(util_got, dep_src));
    call(FILEUnMap, umap);

    // extra.c should NOT exist (opt not requested)
    a_path(epath, "");
    call(path8gDup, path8gIn(epath), path8cgIn(wpath));
    a_cstr(ename, "extra.c");
    call(path8gPush, path8gIn(epath), ename);
    struct stat est;
    ok64 eo = FILEStat(&est, path8cgIn(epath));
    want(eo != OK);

    // --- Test: get with opt ---
    call(BEGetDeps, &be, YES);

    // Now extra.c should exist
    u8bp emap = NULL;
    call(FILEMapRO, &emap, path8cgIn(epath));
    u8cp e0 = emap[1], e1 = emap[2];
    u8cs extra_got = {e0, e1};
    want($eq(extra_got, opt_src));
    call(FILEUnMap, emap);

    // Cleanup
    a_path(rpath8, "");
    call(path8gDup, path8gIn(rpath8), path8cgIn(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath8));
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
    call(BEtest8);
    done;
}

TEST(maintest)
