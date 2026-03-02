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

    // Test base key (path-only URI)
    u8cs proj = $u8str("librdx");
    u8cs path = $u8str("src/main.c");
    u8 ppbuf[256];
    u8s pp = {ppbuf, ppbuf + sizeof(ppbuf)};
    call(u8sFeed, pp, proj);
    u8sFeed1(pp, '/');
    call(u8sFeed, pp, path);
    u8cs fpath = {ppbuf, pp[0]};

    uri u = {};
    u8csMv(u.path, fpath);
    u8 kbuf[512];
    u8s key = {kbuf, kbuf + sizeof(kbuf)};
    call(URIutf8Feed, key, &u);
    u8cs got = {kbuf, key[0]};
    u8cs expected = $u8str("librdx/src/main.c");
    want($eq(got, expected));

    // Test base key with empty path
    u8 ppbuf2[256];
    u8s pp2 = {ppbuf2, ppbuf2 + sizeof(ppbuf2)};
    call(u8sFeed, pp2, proj);
    u8sFeed1(pp2, '/');
    uri u2 = {};
    u8cs fpath2 = {ppbuf2, pp2[0]};
    u8csMv(u2.path, fpath2);
    key[0] = kbuf;
    call(URIutf8Feed, key, &u2);
    got[0] = kbuf;
    got[1] = key[0];
    u8cs exp2 = $u8str("librdx/");
    want($eq(got, exp2));

    // Test file prefix (path + '?')
    uri u3 = {};
    u8csMv(u3.path, fpath);
    key[0] = kbuf;
    call(URIutf8Feed, key, &u3);
    u8sFeed1(key, '?');
    got[0] = kbuf;
    got[1] = key[0];
    u8cs exp3 = $u8str("librdx/src/main.c?");
    want($eq(got, exp3));

    done;
}

// ---- Test 2: Key builder with waypoint ----

ok64 BEtest2() {
    sane(1);
    u8cs proj = $u8str("librdx");
    u8cs path = $u8str("src/main.c");
    u8cs branch = $u8str("main");
    ron60 stamp = 0x123;

    // Build waypoint key via URI API
    u8 ppbuf[256];
    u8s pp = {ppbuf, ppbuf + sizeof(ppbuf)};
    call(u8sFeed, pp, proj);
    u8sFeed1(pp, '/');
    call(u8sFeed, pp, path);
    u8 qqbuf[128];
    u8s qq = {qqbuf, qqbuf + sizeof(qqbuf)};
    call(BEQueryBuild, qq, stamp, branch);
    uri u = {};
    u8cs up = {ppbuf, pp[0]};
    u8cs uq = {qqbuf, qq[0]};
    u8csMv(u.path, up);
    u8csMv(u.query, uq);
    u8 kbuf[512];
    u8s key = {kbuf, kbuf + sizeof(kbuf)};
    call(URIutf8Feed, key, &u);
    u8cs got = {kbuf, key[0]};

    // Should contain "librdx/src/main.c?" + 10-char timestamp + "-main"
    a_cstr(prefix, "librdx/src/main.c?");
    want($len(got) > $len(prefix));
    want(memcmp(got[0], prefix[0], $len(prefix)) == 0);
    // Should end with "-main"
    a_cstr(suffix, "-main");
    want($len(got) >= $len(suffix));
    u8cs tail = {got[1] - $len(suffix), got[1]};
    want($eq(tail, suffix));

    // Extract branch suffix
    u8cs extracted_branch = {};
    call(BEKeyBranchSuffix, extracted_branch, got);
    want($eq(extracted_branch, branch));

    // Extract timestamp
    ron60 extracted_stamp = 0;
    call(BEKeyStamp, &extracted_stamp, got);
    same(extracted_stamp, stamp);

    done;
}

// ---- Test 3: Key ordering (timestamps sort correctly) ----

// Helper: build waypoint URI key into buffer
static ok64 TestWaypoint(u8s into, u8cs proj, u8cs path,
                         ron60 stamp, u8cs branch) {
    sane($ok(into));
    u8 ppbuf[256];
    u8s pp = {ppbuf, ppbuf + sizeof(ppbuf)};
    call(u8sFeed, pp, proj);
    u8sFeed1(pp, '/');
    if ($ok(path) && !$empty(path)) call(u8sFeed, pp, path);
    u8 qqbuf[128];
    u8s qq = {qqbuf, qqbuf + sizeof(qqbuf)};
    call(BEQueryBuild, qq, stamp, branch);
    uri u = {};
    u8cs up = {ppbuf, pp[0]};
    u8cs uq = {qqbuf, qq[0]};
    u8csMv(u.path, up);
    u8csMv(u.query, uq);
    call(URIutf8Feed, into, &u);
    done;
}

// Helper: build base URI key (path only)
static ok64 TestBase(u8s into, u8cs proj, u8cs path) {
    sane($ok(into));
    u8 ppbuf[256];
    u8s pp = {ppbuf, ppbuf + sizeof(ppbuf)};
    call(u8sFeed, pp, proj);
    u8sFeed1(pp, '/');
    if ($ok(path) && !$empty(path)) call(u8sFeed, pp, path);
    uri u = {};
    u8cs up = {ppbuf, pp[0]};
    u8csMv(u.path, up);
    call(URIutf8Feed, into, &u);
    done;
}

// Helper: build file prefix key (path + '?')
static ok64 TestFilePrefix(u8s into, u8cs proj, u8cs path) {
    sane($ok(into));
    call(TestBase, into, proj, path);
    u8sFeed1(into, '?');
    done;
}

ok64 BEtest3() {
    sane(1);
    u8cs proj = $u8str("proj");
    u8cs path = $u8str("file.c");
    u8cs branch = $u8str("main");

    u8 k1buf[512], k2buf[512], k3buf[512];
    u8s k1 = {k1buf, k1buf + sizeof(k1buf)};
    u8s k2 = {k2buf, k2buf + sizeof(k2buf)};
    u8s k3 = {k3buf, k3buf + sizeof(k3buf)};

    call(TestWaypoint, k1, proj, path, 100, branch);
    call(TestWaypoint, k2, proj, path, 200, branch);
    call(TestWaypoint, k3, proj, path, 300, branch);

    u8cs s1 = {k1buf, k1[0]};
    u8cs s2 = {k2buf, k2[0]};
    u8cs s3 = {k3buf, k3[0]};

    // Lexicographic order should match timestamp order
    size_t len12 = $len(s1) < $len(s2) ? $len(s1) : $len(s2);
    want(memcmp(s1[0], s2[0], len12) < 0);
    size_t len23 = $len(s2) < $len(s3) ? $len(s2) : $len(s3);
    want(memcmp(s2[0], s3[0], len23) < 0);

    // Different branches at same timestamp: both after base prefix
    u8cs b2 = $u8str("feat");
    u8 k4buf[512];
    u8s k4 = {k4buf, k4buf + sizeof(k4buf)};
    call(TestWaypoint, k4, proj, path, 200, b2);
    u8cs s4 = {k4buf, k4[0]};

    // Base key sorts before all waypoint keys for same path
    u8 bkbuf[512];
    u8s bk = {bkbuf, bkbuf + sizeof(bkbuf)};
    call(TestBase, bk, proj, path);
    u8cs sbase = {bkbuf, bk[0]};
    // base is shorter prefix, so base < waypoint in lexicographic order
    want($len(sbase) < $len(s2));
    want(memcmp(sbase[0], s2[0], $len(sbase)) == 0);
    // Waypoint key has '?' after base path, which comes after any path char

    done;
}

// ---- Test 4: BASTExport roundtrip ----

ok64 BEtest4() {
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

// ---- Test 5: POST+GET roundtrip ----

ok64 BEtest5() {
    sane(1);

    a_path(wpath, "/tmp");
    a_cstr(tmpl, "BEtest5_XXXXXX");
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

    // Init BE with main branch
    BE be = {};
    u8cs uri = $u8str("be://BEtest5/@test/proj?main");
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

    a_path(rpath, "");
    call(path8gDup, path8gIn(rpath), path8cgIn(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    done;
}

// ---- Test 6: Independent waypoints from two POSTs ----

ok64 BEtest6() {
    sane(1);

    a_path(wpath, "/tmp");
    a_cstr(tmpl, "BEtest6_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

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
    u8cs uri = $u8str("be://BEtest6/@test/proj?main");
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

    // Verify waypoint keys exist with new format
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(TestFilePrefix, pfx, be.loc.path, relpath);
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
        // Verify branch suffix is "main"
        u8cs branch = {};
        ok64 o = BEKeyBranchSuffix(branch, k);
        want(o == OK);
        a_cstr(main_br, "main");
        want($eq(branch, main_br));
        wp_count++;
        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);
    want(wp_count >= 2);  // Both POSTs create waypoints

    // GET should reconstruct latest state
    call(FILEUnLink, path8cgIn(fpath));
    u8cs empty_branch = {};
    call(BEGet, &be, 1, paths, empty_branch);

    u8bp mapbuf = NULL;
    call(FILEMapRO, &mapbuf, path8cgIn(fpath));
    u8cp r0 = mapbuf[1], r1 = mapbuf[2];
    u8cs restored = {r0, r1};
    want($eq(restored, v2));
    call(FILEUnMap, mapbuf);

    a_path(rpath, "");
    call(path8gDup, path8gIn(rpath), path8cgIn(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    done;
}

// ---- Test 7: Multi-branch isolation ----

ok64 BEtest7() {
    sane(1);

    a_path(wpath, "/tmp");
    a_cstr(tmpl, "BEtest7_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    // Create file
    a_path(fpath, "");
    call(path8gDup, path8gIn(fpath), path8cgIn(wpath));
    a_cstr(fname, "multi.c");
    call(path8gPush, path8gIn(fpath), fname);
    u8cs src1 = $u8str("int x = 1;\n");
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, src1);
    call(FILEClose, &fd);

    // Init with main branch
    BE be = {};
    u8cs uri = $u8str("be://BEtest7/@test/proj?main");
    call(BEInit, &be, uri, path8cgIn(wpath));

    u8cs relpath = $u8str("multi.c");
    u8cs *paths = &relpath;
    u8cs msg = $u8str("main v1");
    call(BEPost, &be, 1, paths, msg);

    // Switch to "feat" branch and make a different edit
    a_cstr(feat, "feat");
    call(BESetActive, &be, feat);

    call(FILEUnLink, path8cgIn(fpath));
    u8cs src2 = $u8str("int x = 99;\n");
    fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, src2);
    call(FILEClose, &fd);

    u8cs msg2 = $u8str("feat change");
    call(BEPost, &be, 1, paths, msg2);

    // GET with only "main" visible: should get main's value
    call(FILEUnLink, path8cgIn(fpath));
    call(BERemoveBranch, &be, feat);
    u8cs empty_branch = {};
    call(BEGet, &be, 1, paths, empty_branch);

    u8bp mapbuf = NULL;
    call(FILEMapRO, &mapbuf, path8cgIn(fpath));
    u8cp r0 = mapbuf[1], r1 = mapbuf[2];
    u8cs got_main = {r0, r1};
    want($eq(got_main, src1));
    call(FILEUnMap, mapbuf);

    // Add feat branch and GET: should get feat's value (later timestamp wins)
    call(FILEUnLink, path8cgIn(fpath));
    call(BEAddBranch, &be, feat);
    call(BEGet, &be, 1, paths, empty_branch);

    mapbuf = NULL;
    call(FILEMapRO, &mapbuf, path8cgIn(fpath));
    r0 = mapbuf[1];
    r1 = mapbuf[2];
    u8cs got_feat = {r0, r1};
    want($eq(got_feat, src2));
    call(FILEUnMap, mapbuf);

    a_path(rpath, "");
    call(path8gDup, path8gIn(rpath), path8cgIn(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    done;
}

// ---- Test 8: Milestone (fold main waypoints into base) ----

ok64 BEtest8() {
    sane(1);

    a_path(wpath, "/tmp");
    a_cstr(tmpl, "BEtest8_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    a_path(fpath, "");
    call(path8gDup, path8gIn(fpath), path8cgIn(wpath));
    a_cstr(fname, "ms.c");
    call(path8gPush, path8gIn(fpath), fname);
    u8cs src1 = $u8str("int y = 1;\n");
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, src1);
    call(FILEClose, &fd);

    BE be = {};
    u8cs uri = $u8str("be://BEtest8/@test/proj?main");
    call(BEInit, &be, uri, path8cgIn(wpath));

    u8cs relpath = $u8str("ms.c");
    u8cs *paths = &relpath;
    u8cs msg1 = $u8str("ms v1");
    call(BEPost, &be, 1, paths, msg1);

    // Second edit
    call(FILEUnLink, path8cgIn(fpath));
    u8cs src2 = $u8str("int y = 2;\n");
    fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, src2);
    call(FILEClose, &fd);
    u8cs msg2 = $u8str("ms v2");
    call(BEPost, &be, 1, paths, msg2);

    // Count waypoints before milestone
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(TestFilePrefix, pfx, be.loc.path, relpath);
    u8cs prefix = {pfxbuf, pfx[0]};

    ROCKiter it = {};
    call(ROCKIterOpen, &it, &be.db);
    call(ROCKIterSeek, &it, prefix);
    int wp_before = 0;
    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(prefix) ||
            memcmp(k[0], prefix[0], $len(prefix)) != 0)
            break;
        wp_before++;
        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);
    want(wp_before >= 2);

    // Create milestone
    u8cs ms_name = $u8str("v1.0");
    call(BEMilestone, &be, ms_name);

    // Count waypoints after milestone: should be 0 (all folded)
    call(ROCKIterOpen, &it, &be.db);
    call(ROCKIterSeek, &it, prefix);
    int wp_after = 0;
    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(prefix) ||
            memcmp(k[0], prefix[0], $len(prefix)) != 0)
            break;
        wp_after++;
        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);
    same(wp_after, 0);

    // Base key should exist with merged state
    u8 bkbuf[512];
    u8s bkey = {bkbuf, bkbuf + sizeof(bkbuf)};
    call(TestBase, bkey, be.loc.path, relpath);
    u8cs base_key = {bkbuf, bkey[0]};
    aBpad(u8, vbuf, 65536);
    ok64 go = ROCKGet(&be.db, vbuf, base_key);
    same(go, OK);

    // GET should still produce latest content
    call(FILEUnLink, path8cgIn(fpath));
    u8cs empty_branch = {};
    call(BEGet, &be, 1, paths, empty_branch);

    u8bp mapbuf = NULL;
    call(FILEMapRO, &mapbuf, path8cgIn(fpath));
    u8cp r0 = mapbuf[1], r1 = mapbuf[2];
    u8cs restored = {r0, r1};
    want($eq(restored, src2));
    call(FILEUnMap, mapbuf);

    a_path(rpath, "");
    call(path8gDup, path8gIn(rpath), path8cgIn(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    done;
}

// ---- Test 9: Branch merge (PUT) ----

ok64 BEtest9() {
    sane(1);

    a_path(wpath, "/tmp");
    a_cstr(tmpl, "BEtest9_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    a_path(fpath, "");
    call(path8gDup, path8gIn(fpath), path8cgIn(wpath));
    a_cstr(fname, "merge.c");
    call(path8gPush, path8gIn(fpath), fname);
    u8cs src1 = $u8str("int z = 0;\n");
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, src1);
    call(FILEClose, &fd);

    BE be = {};
    u8cs uri = $u8str("be://BEtest9/@test/proj?main");
    call(BEInit, &be, uri, path8cgIn(wpath));

    u8cs relpath = $u8str("merge.c");
    u8cs *paths = &relpath;
    u8cs msg1 = $u8str("main init");
    call(BEPost, &be, 1, paths, msg1);

    // Create change on "dev" branch
    a_cstr(dev, "dev");
    call(BESetActive, &be, dev);

    call(FILEUnLink, path8cgIn(fpath));
    u8cs src2 = $u8str("int z = 42;\n");
    fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, src2);
    call(FILEClose, &fd);
    u8cs msg2 = $u8str("dev change");
    call(BEPost, &be, 1, paths, msg2);

    // Merge dev into main
    a_cstr(main_br, "main");
    call(BESetActive, &be, main_br);
    u8cs empty = {};
    call(BEPut, &be, dev, empty);

    // After merge, dev waypoints should be gone, main should have them
    // Verify: GET with only main should produce dev's content
    call(BERemoveBranch, &be, dev);
    call(FILEUnLink, path8cgIn(fpath));
    u8cs empty_branch = {};
    call(BEGet, &be, 1, paths, empty_branch);

    u8bp mapbuf = NULL;
    call(FILEMapRO, &mapbuf, path8cgIn(fpath));
    u8cp r0 = mapbuf[1], r1 = mapbuf[2];
    u8cs restored = {r0, r1};
    want($eq(restored, src2));
    call(FILEUnMap, mapbuf);

    a_path(rpath, "");
    call(path8gDup, path8gIn(rpath), path8cgIn(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    done;
}

// ---- Test 10: Checkpoint ----

ok64 BEtest10() {
    sane(1);

    a_path(wpath, "/tmp");
    a_cstr(tmpl, "BEtestA_XXXXXX");
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
    u8cs uri = $u8str("be://BEtestA/@test/proj?main");
    call(BEInit, &be, uri, path8cgIn(wpath));

    u8cs relpath = $u8str("cp.c");
    u8cs *paths = &relpath;
    u8cs msg = $u8str("before checkpoint");
    call(BEPost, &be, 1, paths, msg);

    u8cs new_repo = $u8str("BEtestAcp");
    call(BECheckpoint, &be, new_repo);

    // Verify new repo DB can be opened
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
    call(ROCKClose, &cpdb);

    a_path(rpath, "");
    call(path8gDup, path8gIn(rpath), path8cgIn(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    call(FILErmrf, path8cgIn(dpath));
    done;
}

// ---- Test 11: BEGetDeps ----

ok64 BEtest11() {
    sane(1);

    a_path(wpath, "/tmp");
    a_cstr(tmpl, "BEtestB_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    a_path(fpath, "");
    call(path8gDup, path8gIn(fpath), path8cgIn(wpath));
    a_cstr(fname, "main.c");
    call(path8gPush, path8gIn(fpath), fname);
    u8cs main_src = $u8str("int main() { return 0; }\n");
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, main_src);
    call(FILEClose, &fd);

    BE be = {};
    u8cs uri = $u8str("be://BEtestB/@test/proj?main");
    call(BEInit, &be, uri, path8cgIn(wpath));

    u8cs relpath = $u8str("main.c");
    u8cs *paths = &relpath;
    u8cs msg = $u8str("init");
    call(BEPost, &be, 1, paths, msg);

    // Store a dep project's file directly in DB (base key)
    u8cs dep_proj = $u8str("/@test/deplib");
    u8cs dep_file = $u8str("util.c");
    u8 dkbuf[512];
    u8s dkey = {dkbuf, dkbuf + sizeof(dkbuf)};
    call(TestBase, dkey, dep_proj, dep_file);
    u8cs dep_key = {dkbuf, dkey[0]};

    u8cs dep_src = $u8str("int util() {}\n");
    u8cs dep_ext = $u8str(".c");
    aBpad(u8, dbuf, 65536);
    aBpad(u64, didx, 4096);
    // Prepend zero metadata to the base value
    BEmeta dep_meta = {};
    call(BEMetaPack, u8bIdle(dbuf), dep_meta);
    call(BASTParse, dbuf, didx, dep_src, dep_ext);
    u8cs dep_val = {dbuf[1], dbuf[2]};
    call(ROCKPut, &be.db, dep_key, dep_val);

    // Write .beget file
    a_path(bgpath, "");
    call(path8gDup, path8gIn(bgpath), path8cgIn(wpath));
    a_cstr(bgname, ".beget");
    call(path8gPush, path8gIn(bgpath), bgname);
    u8cs beget_content = $u8str(
        "# test deps\n"
        "[deps]\n"
        "/@test/deplib\n");
    fd = 0;
    call(FILECreate, &fd, path8cgIn(bgpath));
    call(FILEFeedall, fd, beget_content);
    call(FILEClose, &fd);

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

    a_path(rpath, "");
    call(path8gDup, path8gIn(rpath), path8cgIn(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    done;
}

// ---- Test 12: Metadata roundtrip (mtime + mode preserved) ----

ok64 BEtest12() {
    sane(1);

    a_path(wpath, "/tmp");
    a_cstr(tmpl, "BEtestC_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    // Create file with specific mode
    a_path(fpath, "");
    call(path8gDup, path8gIn(fpath), path8cgIn(wpath));
    a_cstr(fname, "meta.c");
    call(path8gPush, path8gIn(fpath), fname);
    u8cs source = $u8str("int meta = 1;\n");
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, source);
    call(FILEClose, &fd);

    // Set specific mode
    chmod((const char *)fpath[1], 0755);

    // Stat before POST
    struct stat st_before;
    call(FILEStat, &st_before, path8cgIn(fpath));

    BE be = {};
    u8cs uri = $u8str("be://BEtestC/@test/proj?main");
    call(BEInit, &be, uri, path8cgIn(wpath));

    u8cs relpath = $u8str("meta.c");
    u8cs *paths = &relpath;
    u8cs msg = $u8str("meta test");
    call(BEPost, &be, 1, paths, msg);

    // Delete file
    call(FILEUnLink, path8cgIn(fpath));

    // GET to restore
    u8cs empty_branch = {};
    call(BEGet, &be, 1, paths, empty_branch);

    // Verify file restored with correct metadata
    struct stat st_after;
    call(FILEStat, &st_after, path8cgIn(fpath));
    same((int)(st_before.st_mode & 07777), (int)(st_after.st_mode & 07777));
    same((int)st_before.st_mtime, (int)st_after.st_mtime);

    // Verify content
    u8bp mapbuf = NULL;
    call(FILEMapRO, &mapbuf, path8cgIn(fpath));
    u8cp r0 = mapbuf[1], r1 = mapbuf[2];
    u8cs restored = {r0, r1};
    want($eq(restored, source));
    call(FILEUnMap, mapbuf);

    a_path(rpath, "");
    call(path8gDup, path8gIn(rpath), path8cgIn(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    done;
}

// ---- Test 13: BASTFtype roundtrip ----

ok64 BEtest13() {
    sane(1);
    // Test known extensions
    u8cs ext_c = $u8str(".c");
    u32 ft_c = BASTFtype(ext_c);
    want(ft_c != 0);

    u8cs ext_py = $u8str(".py");
    u32 ft_py = BASTFtype(ext_py);
    want(ft_py != 0);
    want(ft_c != ft_py);

    // Roundtrip: ftype → extension
    u8 ebuf[8];
    u8s eout = {ebuf, ebuf + sizeof(ebuf)};
    call(BASTFtypeExt, eout, ft_c);
    u8cs got_ext = {ebuf, eout[0]};
    want($eq(got_ext, ext_c));

    // Unknown extension → 0
    u8cs ext_unk = $u8str(".???");
    u32 ft_unk = BASTFtype(ext_unk);
    same(ft_unk, 0);

    // Empty/null → 0
    u8cs ext_empty = {};
    u32 ft_empty = BASTFtype(ext_empty);
    same(ft_empty, 0);

    // BEmeta pack/unpack roundtrip
    BEmeta m = {.mtime = 1234567890, .modeftype = (0755 << 20) | (ft_c << 2)};
    u8 mbuf[BE_META_SIZE];
    u8s ms = {mbuf, mbuf + BE_META_SIZE};
    call(BEMetaPack, ms, m);
    BEmeta m2 = {};
    u8cs mcs = {mbuf, mbuf + BE_META_SIZE};
    call(BEMetaUnpack, &m2, mcs);
    same(m.mtime, m2.mtime);
    same(m.modeftype, m2.modeftype);
    // Verify field extraction
    u32 mode = m2.modeftype >> 20;
    same(mode, 0755);
    u32 ftype = (m2.modeftype >> 2) & 0x3FFFF;
    same(ftype, ft_c);

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
    call(BEtest9);
    call(BEtest10);
    call(BEtest11);
    call(BEtest12);
    call(BEtest13);
    done;
}

TEST(maintest)
