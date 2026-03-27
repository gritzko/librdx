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

    // Test BEKeyBuild with be: scheme
    u8cs fpath = $u8str("librdx/src/main.c");
    a_cstr(sch_be, BE_SCHEME_BE);
    u8cs empty_q = {};
    u8cs empty_f = {};
    u8 kbuf[512];
    u8s key = {kbuf, kbuf + sizeof(kbuf)};
    call(BEKeyBuild, key, sch_be, fpath, empty_q, empty_f);
    u8cs got = {kbuf, key[0]};
    u8cs expected = $u8str("be:librdx/src/main.c");
    want($eq(got, expected));

    // Test stat: scheme
    a_cstr(sch_stat, BE_SCHEME_STAT);
    key[0] = kbuf;
    call(BEKeyBuild, key, sch_stat, fpath, empty_q, empty_f);
    got[0] = kbuf;
    got[1] = key[0];
    u8cs exp2 = $u8str("stat:librdx/src/main.c");
    want($eq(got, exp2));

    // Test with query
    u8cs query = $u8str("0000000123-main");
    key[0] = kbuf;
    call(BEKeyBuild, key, sch_be, fpath, query, empty_f);
    got[0] = kbuf;
    got[1] = key[0];
    u8cs exp3 = $u8str("be:librdx/src/main.c?0000000123-main");
    want($eq(got, exp3));

    done;
}

// ---- Test 2: Key builder with waypoint ----

ok64 BEtest2() {
    sane(1);
    u8cs fpath = $u8str("librdx/src/main.c");
    u8cs branch = $u8str("main");
    ron60 branch_r60 = 0;
    RONutf8sDrain(&branch_r60, branch);
    ron60 stamp = 0x123;

    // Build waypoint key with scheme
    a_cstr(sch_be, BE_SCHEME_BE);
    u8 qqbuf[128];
    u8s qq = {qqbuf, qqbuf + sizeof(qqbuf)};
    call(BEQueryBuild, qq, stamp, branch_r60);
    u8cs query = {qqbuf, qq[0]};
    u8cs empty_f = {};
    u8 kbuf[512];
    u8s key = {kbuf, kbuf + sizeof(kbuf)};
    call(BEKeyBuild, key, sch_be, fpath, query, empty_f);
    u8cs got = {kbuf, key[0]};

    // Should start with "be:librdx/src/main.c?"
    a_cstr(prefix, "be:librdx/src/main.c?");
    want($len(got) > $len(prefix));
    want(memcmp(got[0], prefix[0], $len(prefix)) == 0);
    // Should end with "-main"
    a_cstr(suffix, "-main");
    want($len(got) >= $len(suffix));
    u8cs tail = {got[1] - $len(suffix), got[1]};
    want($eq(tail, suffix));

    // Extract branch origin (URIutf8Drain handles scheme)
    ron60 extracted_branch = 0;
    call(BEKeyBranch, &extracted_branch, got);
    ron60 branch_val = 0;
    RONutf8sDrain(&branch_val, branch);
    same(extracted_branch, branch_val);

    // Extract timestamp
    ron60 extracted_stamp = 0;
    call(BEKeyStamp, &extracted_stamp, got);
    same(extracted_stamp, stamp);

    done;
}

// ---- Test 3: Key ordering (timestamps sort correctly) ----

// Helper: build waypoint URI key into buffer (with scheme prefix)
static ok64 TestWaypoint(u8s into, u8cs scheme, u8cs proj, u8cs path,
                         ron60 stamp, ron60 branch) {
    sane($ok(into));
    u8 ppbuf[256];
    u8s pp = {ppbuf, ppbuf + sizeof(ppbuf)};
    call(u8sFeed, pp, proj);
    u8sFeed1(pp, '/');
    if ($ok(path) && !$empty(path)) call(u8sFeed, pp, path);
    u8 qqbuf[128];
    u8s qq = {qqbuf, qqbuf + sizeof(qqbuf)};
    call(BEQueryBuild, qq, stamp, branch);
    u8cs up = {ppbuf, pp[0]};
    u8cs uq = {qqbuf, qq[0]};
    u8cs empty_f = {};
    call(BEKeyBuild, into, scheme, up, uq, empty_f);
    done;
}

// Helper: build base URI key (scheme:path)
static ok64 TestBase(u8s into, u8cs scheme, u8cs proj, u8cs path) {
    sane($ok(into));
    u8 ppbuf[256];
    u8s pp = {ppbuf, ppbuf + sizeof(ppbuf)};
    call(u8sFeed, pp, proj);
    u8sFeed1(pp, '/');
    if ($ok(path) && !$empty(path)) call(u8sFeed, pp, path);
    u8cs up = {ppbuf, pp[0]};
    u8cs empty_q = {};
    u8cs empty_f = {};
    call(BEKeyBuild, into, scheme, up, empty_q, empty_f);
    done;
}

// Helper: build file prefix key (scheme:path?)
static ok64 TestFilePrefix(u8s into, u8cs scheme, u8cs proj, u8cs path) {
    sane($ok(into));
    call(TestBase, into, scheme, proj, path);
    u8sFeed1(into, '?');
    done;
}

ok64 BEtest3() {
    sane(1);
    u8cs proj = $u8str("proj");
    u8cs path = $u8str("file.c");
    u8cs branch_s = $u8str("main");
    ron60 branch = 0;
    RONutf8sDrain(&branch, branch_s);
    a_cstr(sch_be, BE_SCHEME_BE);

    u8 k1buf[512], k2buf[512], k3buf[512];
    u8s k1 = {k1buf, k1buf + sizeof(k1buf)};
    u8s k2 = {k2buf, k2buf + sizeof(k2buf)};
    u8s k3 = {k3buf, k3buf + sizeof(k3buf)};

    call(TestWaypoint, k1, sch_be, proj, path, 100, branch);
    call(TestWaypoint, k2, sch_be, proj, path, 200, branch);
    call(TestWaypoint, k3, sch_be, proj, path, 300, branch);

    u8cs s1 = {k1buf, k1[0]};
    u8cs s2 = {k2buf, k2[0]};
    u8cs s3 = {k3buf, k3[0]};

    // Lexicographic order should match timestamp order
    size_t len12 = $len(s1) < $len(s2) ? $len(s1) : $len(s2);
    want(memcmp(s1[0], s2[0], len12) < 0);
    size_t len23 = $len(s2) < $len(s3) ? $len(s2) : $len(s3);
    want(memcmp(s2[0], s3[0], len23) < 0);

    // Different branches at same timestamp: both after base prefix
    u8cs b2_s = $u8str("feat");
    ron60 b2 = 0;
    RONutf8sDrain(&b2, b2_s);
    u8 k4buf[512];
    u8s k4 = {k4buf, k4buf + sizeof(k4buf)};
    call(TestWaypoint, k4, sch_be, proj, path, 200, b2);
    u8cs s4 = {k4buf, k4[0]};
    want(!$empty(s4));  // feat waypoint key exists

    // Base key sorts before all waypoint keys for same path
    u8 bkbuf[512];
    u8s bk = {bkbuf, bkbuf + sizeof(bkbuf)};
    call(TestBase, bk, sch_be, proj, path);
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
    a_dup(u8c, bason, u8bDataC(buf));
    want(!$empty(bason));

    aBpad(u8, out, 65536);
    aBpad(u64, stk, 256);
    call(BASTExport, u8bIdle(out), stk, bason);
    a_dup(u8c, result, u8bDataC(out));
    want($eq(result, source));
    done;
}

// ---- Test 5: POST+GET roundtrip ----

ok64 BEtest5() {
    sane(1);

    a_path(wpath, $cstr("/tmp"));
    a_cstr(tmpl, "BEtest5_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    // Create a source file
    a_path(fpath, u8bDataC(wpath), $cstr("test.c"));
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
    u8cp r0 = u8bDataHead(mapbuf), r1 = u8bIdleHead(mapbuf);
    u8cs restored = {r0, r1};
    want($eq(restored, source));
    call(FILEUnMap, mapbuf);

    a_path(rpath, u8bDataC(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    done;
}

// ---- Test 6: Independent waypoints from two POSTs ----

ok64 BEtest6() {
    sane(1);

    a_path(wpath, $cstr("/tmp"));
    a_cstr(tmpl, "BEtest6_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    a_path(fpath, u8bDataC(wpath), $cstr("ver.c"));
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

    // Third edit (need 3 posts: initial=base, 2nd+3rd=waypoints)
    call(FILEUnLink, path8cgIn(fpath));
    u8cs v3 = $u8str("int x = 3;\n");
    fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, v3);
    call(FILEClose, &fd);
    u8cs msg3 = $u8str("v3");
    call(BEPost, &be, 1, paths, msg3);

    // Verify waypoint keys exist with new format (be: scheme)
    a_cstr(sch_be, BE_SCHEME_BE);
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(TestFilePrefix, pfx, sch_be, be.loc.path, relpath);
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
        // Verify branch is "main"
        ron60 branch = 0;
        ok64 o = BEKeyBranch(&branch, k);
        want(o == OK);
        a_cstr(main_br, "main");
        ron60 main_val = 0;
        RONutf8sDrain(&main_val, main_br);
        same(branch, main_val);
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
    u8cp r0 = u8bDataHead(mapbuf), r1 = u8bIdleHead(mapbuf);
    u8cs restored = {r0, r1};
    want($eq(restored, v3));
    call(FILEUnMap, mapbuf);

    a_path(rpath, u8bDataC(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    done;
}

// ---- Test 7: Multi-branch isolation ----

ok64 BEtest7() {
    sane(1);

    a_path(wpath, $cstr("/tmp"));
    a_cstr(tmpl, "BEtest7_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    // Create file
    a_path(fpath, u8bDataC(wpath), $cstr("multi.c"));
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
    u8cp r0 = u8bDataHead(mapbuf), r1 = u8bIdleHead(mapbuf);
    u8cs got_main = {r0, r1};
    want($eq(got_main, src1));
    call(FILEUnMap, mapbuf);

    // Add feat branch and GET: should get feat's value (later timestamp wins)
    call(FILEUnLink, path8cgIn(fpath));
    call(BEAddBranch, &be, feat);
    call(BEGet, &be, 1, paths, empty_branch);

    mapbuf = NULL;
    call(FILEMapRO, &mapbuf, path8cgIn(fpath));
    r0 = u8bDataHead(mapbuf);
    r1 = u8bIdleHead(mapbuf);
    u8cs got_feat = {r0, r1};
    want($eq(got_feat, src2));
    call(FILEUnMap, mapbuf);

    a_path(rpath, u8bDataC(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    done;
}

// ---- Test 8: Milestone (fold main waypoints into base) ----

ok64 BEtest8() {
    sane(1);

    a_path(wpath, $cstr("/tmp"));
    a_cstr(tmpl, "BEtest8_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    a_path(fpath, u8bDataC(wpath), $cstr("ms.c"));
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

    // Third edit (need 3 posts: initial=base, 2nd+3rd=waypoints)
    call(FILEUnLink, path8cgIn(fpath));
    u8cs src3 = $u8str("int y = 3;\n");
    fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, src3);
    call(FILEClose, &fd);
    u8cs msg3 = $u8str("ms v3");
    call(BEPost, &be, 1, paths, msg3);

    // Count waypoints before milestone (be: scheme)
    a_cstr(sch_be, BE_SCHEME_BE);
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(TestFilePrefix, pfx, sch_be, be.loc.path, relpath);
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

    // Base key should exist with merged state (be: scheme)
    u8 bkbuf[512];
    u8s bkey = {bkbuf, bkbuf + sizeof(bkbuf)};
    call(TestBase, bkey, sch_be, be.loc.path, relpath);
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
    u8cp r0 = u8bDataHead(mapbuf), r1 = u8bIdleHead(mapbuf);
    u8cs restored = {r0, r1};
    want($eq(restored, src3));
    call(FILEUnMap, mapbuf);

    a_path(rpath, u8bDataC(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    done;
}

// ---- Test 9: Branch merge (PUT) ----

ok64 BEtest9() {
    sane(1);

    a_path(wpath, $cstr("/tmp"));
    a_cstr(tmpl, "BEtest9_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    a_path(fpath, u8bDataC(wpath), $cstr("merge.c"));
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
    u8cp r0 = u8bDataHead(mapbuf), r1 = u8bIdleHead(mapbuf);
    u8cs restored = {r0, r1};
    want($eq(restored, src2));
    call(FILEUnMap, mapbuf);

    a_path(rpath, u8bDataC(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    done;
}

// ---- Test 10: Checkpoint ----

ok64 BEtest10() {
    sane(1);

    a_path(wpath, $cstr("/tmp"));
    a_cstr(tmpl, "BEtestA_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    a_path(fpath, u8bDataC(wpath), $cstr("cp.c"));
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
    call(path8bPushCStr, dpath, ".be");
    call(path8bPush, dpath, new_repo);

    ROCKdb cpdb = {};
    call(ROCKOpenRO, &cpdb, path8cgIn(dpath));
    call(ROCKClose, &cpdb);

    a_path(rpath, u8bDataC(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    call(FILErmrf, path8cgIn(dpath));
    done;
}

// ---- Test 11: BEGetDeps ----

ok64 BEtest11() {
    sane(1);

    a_path(wpath, $cstr("/tmp"));
    a_cstr(tmpl, "BEtestB_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    a_path(fpath, u8bDataC(wpath), $cstr("main.c"));
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

    // Store a dep project's file directly in DB (be: + stat: keys)
    u8cs dep_proj = $u8str("/@test/deplib");
    u8cs dep_file = $u8str("util.c");
    a_cstr(sch_be, BE_SCHEME_BE);
    a_cstr(sch_stat, BE_SCHEME_STAT);

    // be: key with BASON content
    u8 dkbuf[512];
    u8s dkey = {dkbuf, dkbuf + sizeof(dkbuf)};
    call(TestBase, dkey, sch_be, dep_proj, dep_file);
    u8cs dep_key = {dkbuf, dkey[0]};

    u8cs dep_src = $u8str("int util() {}\n");
    u8cs dep_ext = $u8str(".c");
    aBpad(u8, dbuf, 65536);
    aBpad(u64, didx, 4096);
    call(BASTParse, dbuf, didx, dep_src, dep_ext);
    a_dup(u8c, dep_val, u8bDataC(dbuf));
    call(ROCKPut, &be.db, dep_key, dep_val);

    // stat: key with BASON stat cache
    u8 skbuf[512];
    u8s skey = {skbuf, skbuf + sizeof(skbuf)};
    call(TestBase, skey, sch_stat, dep_proj, dep_file);
    u8cs stat_key = {skbuf, skey[0]};
    aBpad(u8, mbuf, 256);
    aBpad(u64, midx, 32);
    BEstat dep_stat = {};
    call(BEStatFeedBason, mbuf, midx, dep_stat);
    a_dup(u8c, meta_val, u8bDataC(mbuf));
    call(ROCKPut, &be.db, stat_key, meta_val);

    // Write .beget file
    a_path(bgpath, u8bDataC(wpath), $cstr(".beget"));
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
    a_path(upath, u8bDataC(wpath), $cstr("util.c"));
    u8bp umap = NULL;
    call(FILEMapRO, &umap, path8cgIn(upath));
    u8cp u0 = u8bDataHead(umap), u1 = u8bIdleHead(umap);
    u8cs util_got = {u0, u1};
    want($eq(util_got, dep_src));
    call(FILEUnMap, umap);

    a_path(rpath, u8bDataC(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    done;
}

// ---- Test 12: Exec bit roundtrip (preserved via be: key '*' suffix) ----

ok64 BEtest12() {
    sane(1);

    a_path(wpath, $cstr("/tmp"));
    a_cstr(tmpl, "BEtestC_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    // Create file with exec mode
    a_path(fpath, u8bDataC(wpath), $cstr("meta.c"));
    u8cs source = $u8str("int meta = 1;\n");
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, source);
    call(FILEClose, &fd);

    // Set exec mode
    chmod((const char *)u8bDataHead(fpath), 0755);

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

    // Verify file restored with exec bit
    struct stat st_after;
    call(FILEStat, &st_after, path8cgIn(fpath));
    want((st_after.st_mode & 0111) != 0);  // exec bit preserved

    // Verify content
    u8bp mapbuf = NULL;
    call(FILEMapRO, &mapbuf, path8cgIn(fpath));
    u8cp r0 = u8bDataHead(mapbuf), r1 = u8bIdleHead(mapbuf);
    u8cs restored = {r0, r1};
    want($eq(restored, source));
    call(FILEUnMap, mapbuf);

    // Also test non-exec file gets 0644
    a_path(fpath2, u8bDataC(wpath), $cstr("noexec.c"));
    u8cs source2 = $u8str("int noexec = 2;\n");
    fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath2));
    call(FILEFeedall, fd, source2);
    call(FILEClose, &fd);
    chmod((const char *)u8bDataHead(fpath2), 0644);

    u8cs relpath2 = $u8str("noexec.c");
    u8cs *paths2 = &relpath2;
    u8cs msg2 = $u8str("noexec test");
    call(BEPost, &be, 1, paths2, msg2);
    call(FILEUnLink, path8cgIn(fpath2));
    call(BEGet, &be, 1, paths2, empty_branch);

    struct stat st2;
    call(FILEStat, &st2, path8cgIn(fpath2));
    want((st2.st_mode & 0111) == 0);  // no exec bit

    a_path(rpath, u8bDataC(be.repo_pp));
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

    // BEstat BASON roundtrip
    BEstat s = {.mtime = 1234567890, .hash = 0x123456789ABCULL};
    aBpad(u8, mbuf, 256);
    aBpad(u64, midx, 32);
    call(BEStatFeedBason, mbuf, midx, s);
    a_dup(u8c, mcs, u8bDataC(mbuf));
    want(!$empty(mcs));
    BEstat s2 = {};
    call(BEStatDrainBason, &s2, mcs);
    same(s.mtime, s2.mtime);
    want(s.hash == s2.hash);

    done;
}

// ---- Test 14: Trigram extraction ----

typedef struct {
    int count;
    u8 trigrams[256][3];
} TriCollect;

static ok64 TriCollectCB(voidp arg, u8cs trigram) {
    TriCollect *tc = (TriCollect *)arg;
    test($len(trigram) == 3, BEBAD);
    test(tc->count < 256, BEBAD);
    memcpy(tc->trigrams[tc->count], trigram[0], 3);
    tc->count++;
    return OK;
}

ok64 BEtest14() {
    sane(1);

    // Parse a source string into BASON, extract trigrams
    u8cs source = $u8str("int foo = bar;\n");
    u8cs ext = $u8str(".c");
    aBpad(u8, buf, 65536);
    aBpad(u64, idx, 4096);
    call(BASTParse, buf, idx, source, ext);
    a_dup(u8c, bason, u8bDataC(buf));
    want(!$empty(bason));

    TriCollect tc = {};
    call(BETriExtract, bason, TriCollectCB, &tc);

    // Should find "int", "foo", "bar" at minimum
    want(tc.count >= 3);

    // Verify "foo" is among extracted trigrams
    b8 found_foo = NO;
    for (int i = 0; i < tc.count; i++) {
        if (tc.trigrams[i][0] == 'f' &&
            tc.trigrams[i][1] == 'o' &&
            tc.trigrams[i][2] == 'o') {
            found_foo = YES;
        }
    }
    want(found_foo);

    done;
}

// ---- Test 15: Hashlet determinism ----

ok64 BEtest15() {
    sane(1);

    u8cs path1 = $u8str("src/main.c");
    u8cs path2 = $u8str("src/util.c");

    u8 h1buf[4], h2buf[4], h3buf[4];
    u8s h1 = {h1buf, h1buf + sizeof(h1buf)};
    u8s h2 = {h2buf, h2buf + sizeof(h2buf)};
    u8s h3 = {h3buf, h3buf + sizeof(h3buf)};

    call(BEHashlet, h1, path1);
    call(BEHashlet, h2, path2);
    call(BEHashlet, h3, path1);

    u8cs s1 = {h1buf, h1[0]};
    u8cs s3 = {h3buf, h3[0]};
    // Same path → same hashlet
    want($eq(s1, s3));
    // Hashlet should be 2 chars
    same((int)$len(s1), 2);

    done;
}

// ---- Test 16: Trigram search roundtrip (POST + grep) ----

typedef struct {
    int count;
    u8cs paths[16];
} GrepResult;

static ok64 GrepCollectCB(voidp arg, u8cs filepath, int lineno, u8cs line) {
    GrepResult *gr = (GrepResult *)arg;
    test(gr->count < 16, BEBAD);
    u8csMv(gr->paths[gr->count], filepath);
    gr->count++;
    return OK;
}

ok64 BEtest16() {
    sane(1);

    a_path(wpath, $cstr("/tmp"));
    a_cstr(tmpl, "BEtestG_XXXXXX");
    call(path8gAddTmp, path8gIn(wpath), tmpl);
    call(FILEMakeDir, path8cgIn(wpath));

    // Create two files with different content
    a_path(fpath1, u8bDataC(wpath), $cstr("alpha.c"));
    u8cs src1 = $u8str("int unique_xyz = 1;\n");
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath1));
    call(FILEFeedall, fd, src1);
    call(FILEClose, &fd);

    a_path(fpath2, u8bDataC(wpath), $cstr("beta.c"));
    u8cs src2 = $u8str("int other_abc = 2;\n");
    fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath2));
    call(FILEFeedall, fd, src2);
    call(FILEClose, &fd);

    // Init + POST both files
    BE be = {};
    u8cs be_uri = $u8str("be://BEtestG/@test/proj?main");
    call(BEInit, &be, be_uri, path8cgIn(wpath));
    u8cs msg = $u8str("test trigrams");
    call(BEPost, &be, 0, NULL, msg);

    // Grep for "unique_xyz" — should find alpha.c only
    u8cs f1 = $u8str("unique_xyz");
    uri gu1 = {};
    gu1.fragment[0] = f1[0]; gu1.fragment[1] = f1[1];
    GrepResult gr1 = {};
    call(BEGrep, &be, &gu1, GrepCollectCB, &gr1);
    same(gr1.count, 1);

    // Grep for "other_abc" — should find beta.c only
    u8cs f2 = $u8str("other_abc");
    uri gu2 = {};
    gu2.fragment[0] = f2[0]; gu2.fragment[1] = f2[1];
    GrepResult gr2 = {};
    call(BEGrep, &be, &gu2, GrepCollectCB, &gr2);
    same(gr2.count, 1);

    // Grep for "int" — should find both files
    u8cs f3 = $u8str("int");
    uri gu3 = {};
    gu3.fragment[0] = f3[0]; gu3.fragment[1] = f3[1];
    GrepResult gr3 = {};
    call(BEGrep, &be, &gu3, GrepCollectCB, &gr3);
    want(gr3.count >= 2);

    // Grep for "nonexistent_term" — should find nothing
    u8cs f4 = $u8str("nonexistent_term");
    uri gu4 = {};
    gu4.fragment[0] = f4[0]; gu4.fragment[1] = f4[1];
    GrepResult gr4 = {};
    call(BEGrep, &be, &gu4, GrepCollectCB, &gr4);
    same(gr4.count, 0);

    a_path(rpath, u8bDataC(be.repo_pp));
    call(BEClose, &be);
    call(FILErmrf, path8cgIn(wpath));
    call(FILErmrf, path8cgIn(rpath));
    done;
}

// ---- Test 17: BASTGrepNodes (AST-aware line selection) ----

// Callback: select function definition nodes (type letter 'E')
static b8 SelectFunctions(const bason *node, void *ctx) {
    (void)ctx;
    return (node->type & ~0x20) == 'E' ? YES : NO;
}

ok64 BEtest17() {
    sane(1);

    // Multi-function source with blank lines between
    const char *src_str =
        "#include <stdio.h>\n"
        "\n"
        "int add(int a, int b) {\n"
        "    return a + b;\n"
        "}\n"
        "\n"
        "int mul(int a, int b) {\n"
        "    return a * b;\n"
        "}\n"
        "\n"
        "int main() {\n"
        "    return 0;\n"
        "}\n";
    u8cs source = $u8str(src_str);
    u8cs ext = $u8str(".c");

    // Parse to BASON
    aBpad(u8, pbuf, 65536);
    aBpad(u64, pidx, 4096);
    call(BASTParse, pbuf, pidx, source, ext);
    a_dup(u8c, bason_data, u8bDataC(pbuf));
    want(!$empty(bason_data));

    // Test 1: Select functions with k=0 (no context)
    aBpad(u8, obuf, 65536);
    call(BASTGrepNodes, u8bIdle(obuf), bason_data, 0,
         SelectFunctions, NULL);
    a_dup(u8c, result, u8bDataC(obuf));
    want(!$empty(result));

    // Should contain "int add" and "int mul" and "int main"
    a_cstr(add_str, "int add");
    a_cstr(mul_str, "int mul");
    a_cstr(main_str, "int main");
    // Check substrings present
    b8 has_add = NO, has_mul = NO, has_main = NO;
    for (u32 i = 0; i + $len(add_str) <= (u32)$len(result); i++) {
        if (memcmp(result[0] + i, add_str[0], $len(add_str)) == 0)
            has_add = YES;
        if (memcmp(result[0] + i, mul_str[0], $len(mul_str)) == 0)
            has_mul = YES;
        if (memcmp(result[0] + i, main_str[0], $len(main_str)) == 0)
            has_main = YES;
    }
    want(has_add);
    want(has_mul);
    want(has_main);

    // Should have "--\n" separators between non-contiguous groups
    a_cstr(sep, "--\n");
    int sep_count = 0;
    for (u32 i = 0; i + $len(sep) <= (u32)$len(result); i++) {
        if (memcmp(result[0] + i, sep[0], $len(sep)) == 0)
            sep_count++;
    }
    want(sep_count >= 2);  // 3 functions → at least 2 separators

    // Should NOT contain "#include" (no context, it's not a function)
    a_cstr(inc_str, "#include");
    b8 has_inc = NO;
    for (u32 i = 0; i + $len(inc_str) <= (u32)$len(result); i++) {
        if (memcmp(result[0] + i, inc_str[0], $len(inc_str)) == 0)
            has_inc = YES;
    }
    want(!has_inc);

    // Test 2: With k=1 context, functions separated by 1 blank line
    // should merge adjacent groups
    u8bReset(obuf);
    call(BASTGrepNodes, u8bIdle(obuf), bason_data, 1,
         SelectFunctions, NULL);
    a_dup(u8c, result2, u8bDataC(obuf));
    want(!$empty(result2));
    // With k=1, the blank line between add and mul gets included as
    // context from both sides, so they merge into one group
    // Count separators: should be fewer than k=0 case
    int sep_count2 = 0;
    for (u32 i = 0; i + $len(sep) <= (u32)$len(result2); i++) {
        if (memcmp(result2[0] + i, sep[0], $len(sep)) == 0)
            sep_count2++;
    }
    want(sep_count2 < sep_count);

    done;
}

// ---- Test 18: BASTDiffBuild + BASTGrepNodes (diff with context) ----

// Callback: select nodes with '-' or '+' status prefix in key
static b8 SelectChanged(const bason *node, void *ctx) {
    (void)ctx;
    if ($empty(node->key)) return NO;
    u8 status = node->key[0][0];
    return status == '-' || status == '+';
}

ok64 BEtest18() {
    sane(1);

    u8cs old_src = $u8str(
        "int x = 1;\n"
        "int y = 2;\n"
        "int z = 3;\n");
    u8cs new_src = $u8str(
        "int x = 1;\n"
        "int y = 99;\n"
        "int z = 3;\n");
    u8cs ext = $u8str(".c");

    // Parse both to BASON
    aBpad(u8, obuf, 65536);
    aBpad(u64, oidx, 4096);
    call(BASTParse, obuf, oidx, old_src, ext);
    u8cp o0 = u8bDataHead(obuf), o1 = u8bIdleHead(obuf);
    u8cs old_bason = {o0, o1};
    want(!$empty(old_bason));

    aBpad(u8, nbuf, 65536);
    aBpad(u64, nidx, 4096);
    call(BASTParse, nbuf, nidx, new_src, ext);
    u8cp n0 = u8bDataHead(nbuf), n1 = u8bIdleHead(nbuf);
    u8cs new_bason = {n0, n1};
    want(!$empty(new_bason));

    // Compute diff (patch)
    aBpad(u8, dbuf, 65536);
    aBpad(u64, dstk1, 256);
    aBpad(u64, dstk2, 256);
    call(BASONDiff, dbuf, NULL, dstk1, old_bason, dstk2, new_bason, NULL);
    u8cp d0 = u8bDataHead(dbuf), d1 = u8bIdleHead(dbuf);
    u8cs patch = {d0, d1};
    want(!$empty(patch));

    // Build unified diff BASON
    aBpad(u8, ubuf, 65536);
    aBpad(u64, ustk1, 256);
    aBpad(u64, ustk2, 256);
    call(BASTDiffBuild, ubuf, NULL, ustk1, old_bason, ustk2, patch);
    u8cp u0 = u8bDataHead(ubuf), u1 = u8bIdleHead(ubuf);
    u8cs unified = {u0, u1};
    want(!$empty(unified));

    // Walk unified BASON, verify status prefixes exist
    aBpad(u64, vstk, 256);
    call(BASONOpen, vstk, unified);
    int n_eq = 0, n_del = 0, n_add = 0, n_chg = 0;
    int depth = 0;
    for (;;) {
        u8 type = 0;
        u8cs key = {}, val = {};
        ok64 o = BASONDrain(vstk, unified, &type, key, val);
        if (o != OK) {
            if (depth <= 0) break;
            call(BASONOuto, vstk);
            depth--;
            continue;
        }
        if (!$empty(key)) {
            u8 st = key[0][0];
            if (st == '=') n_eq++;
            else if (st == '-') n_del++;
            else if (st == '+') n_add++;
            else if (st == '~') n_chg++;
        }
        if (BASONCollection(type)) {
            call(BASONInto, vstk, unified, val);
            depth++;
        }
    }
    // Must have equal, deleted, and added nodes
    want(n_eq > 0);
    want(n_del > 0);
    want(n_add > 0);

    // Select changed nodes with BASTGrepNodes, k=0
    aBpad(u8, rbuf, 65536);
    call(BASTGrepNodes, u8bIdle(rbuf), unified, 0, SelectChanged, NULL);
    u8cp r0 = u8bDataHead(rbuf), r1 = u8bIdleHead(rbuf);
    u8cs result = {r0, r1};
    want(!$empty(result));
    // Changed line has "y" (old "2" and new "99" concatenated)
    a_cstr(y_str, "int y");
    b8 has_y = NO;
    for (u32 i = 0; i + $len(y_str) <= (u32)$len(result); i++) {
        if (memcmp(result[0] + i, y_str[0], $len(y_str)) == 0)
            has_y = YES;
    }
    want(has_y);
    // Unchanged lines should NOT appear with k=0
    a_cstr(x_str, "x = 1");
    a_cstr(z_str, "z = 3");
    b8 has_x = NO, has_z = NO;
    for (u32 i = 0; i + $len(x_str) <= (u32)$len(result); i++) {
        if (memcmp(result[0] + i, x_str[0], $len(x_str)) == 0)
            has_x = YES;
    }
    for (u32 i = 0; i + $len(z_str) <= (u32)$len(result); i++) {
        if (memcmp(result[0] + i, z_str[0], $len(z_str)) == 0)
            has_z = YES;
    }
    want(!has_x);
    want(!has_z);

    // With k=1, context should include adjacent lines
    u8bReset(rbuf);
    call(BASTGrepNodes, u8bIdle(rbuf), unified, 1, SelectChanged, NULL);
    r0 = u8bDataHead(rbuf); r1 = u8bIdleHead(rbuf);
    u8cs result2 = {r0, r1};
    want(!$empty(result2));
    want($len(result2) > $len(result));

    done;
}

// ---- Test 19: Symbol extraction from BASON ----

typedef struct {
    int count;
    u8cs syms[64];
} SymCollect;

static ok64 SymCollectCB(voidp arg, u8cs symbol) {
    SymCollect *sc = (SymCollect *)arg;
    test(sc->count < 64, BEBAD);
    sc->syms[sc->count][0] = symbol[0];
    sc->syms[sc->count][1] = symbol[1];
    sc->count++;
    return OK;
}

ok64 BEtest19() {
    sane(1);

    // Parse C source with function + struct into BASON
    u8cs source = $u8str(
        "int foo(int x) { return x; }\n"
        "struct MyStruct { int a; };\n");
    u8cs ext = $u8str(".c");
    aBpad(u8, buf, 65536);
    aBpad(u64, idx, 4096);
    call(BASTParse, buf, idx, source, ext);
    a_dup(u8c, bason, u8bDataC(buf));
    want(!$empty(bason));

    // Extract symbols
    SymCollect sc = {};
    call(BESymExtract, bason, SymCollectCB, &sc);

    // Should find at least "foo" and "MyStruct"
    want(sc.count >= 2);

    b8 found_foo = NO;
    b8 found_MyStruct = NO;
    for (int i = 0; i < sc.count; i++) {
        size_t slen = (size_t)$len(sc.syms[i]);
        if (slen == 3 && memcmp(sc.syms[i][0], "foo", 3) == 0)
            found_foo = YES;
        if (slen == 8 && memcmp(sc.syms[i][0], "MyStruct", 8) == 0)
            found_MyStruct = YES;
    }
    want(found_foo);
    want(found_MyStruct);

    // Verify BASTExport roundtrip still works with name nodes
    a_pad(u8, out, 65536);
    aBpad(u64, stk, 256);
    call(BASTExport, out_idle, stk, bason);
    a_dup(u8c, result, u8bDataC(out));
    want($len(result) == $len(source));
    want(memcmp(result[0], source[0], $len(source)) == 0);

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
    call(BEtest14);
    call(BEtest15);
    call(BEtest16);
    call(BEtest17);
    call(BEtest18);
    call(BEtest19);
    done;
}

TEST(maintest)
