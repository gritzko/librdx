//  REFADV — refs advertisement build + emit + tip lookup.
//
//  Coverage:
//    1. Empty REFS: REFADVOpen on a fresh keeper → count == 0;
//                   REFADVEmit → just a flush packet (`0000`).
//    2. Single trunk ref: heads/main → count == 1, refname/dir match,
//                   pkt-line carries the capability suffix.
//    3. Multiple refs: heads/main + tags/v1.0 → both advertised; only
//                   the first line carries capabilities.
//    4. Tip lookup: REFADVTipDirs returns the right dir for a known
//                   sha; 0 for an unknown sha.
//    5. Round-trip: advertise → drain pkt-lines → split on SP/NUL/LF
//                   → verify (sha, refname) pairs match input.

#include "keeper/REFADV.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PRO.h"
#include "abc/TEST.h"
#include "keeper/PKT.h"
#include "keeper/REFS.h"

// --- helpers ---

//  Make a tmp dir under /tmp; feed the resolved path into `out` as a
//  NUL-terminated cstr.  Caller frees with rm -rf via tmp_rm.
static ok64 tmp_make(char *tmpl) {
    if (mkdtemp(tmpl) == NULL) return FAIL;
    return OK;
}

static void tmp_rm(char const *path) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", path);
    int _ = system(cmd);
    (void)_;
}

//  Append one canonical "?heads/<name>" → "?<40-hex>" entry to the
//  trunk REFS via REFSAppend.  `hex_sha` must be 40 hex chars.
static ok64 push_head(u8csc keepdir, char const *name, char const *hex_sha) {
    sane(keepdir);
    a_pad(u8, kbuf, 256);
    u8bFeed1(kbuf, '?');
    a_cstr(heads, "heads/");
    u8bFeed(kbuf, heads);
    a_cstr(name_s, name);
    u8bFeed(kbuf, name_s);
    a_dup(u8c, key, u8bData(kbuf));

    a_pad(u8, vbuf, 64);
    u8bFeed1(vbuf, '?');
    a_cstr(sha_s, hex_sha);
    u8bFeed(vbuf, sha_s);
    a_dup(u8c, val, u8bData(vbuf));

    call(REFSAppend, keepdir, key, val);
    done;
}

static ok64 push_tag(u8csc keepdir, char const *name, char const *hex_sha) {
    sane(keepdir);
    a_pad(u8, kbuf, 256);
    u8bFeed1(kbuf, '?');
    a_cstr(tags, "tags/");
    u8bFeed(kbuf, tags);
    a_cstr(name_s, name);
    u8bFeed(kbuf, name_s);
    a_dup(u8c, key, u8bData(kbuf));

    a_pad(u8, vbuf, 64);
    u8bFeed1(vbuf, '?');
    a_cstr(sha_s, hex_sha);
    u8bFeed(vbuf, sha_s);
    a_dup(u8c, val, u8bData(vbuf));

    call(REFSAppend, keepdir, key, val);
    done;
}

static void hex_to_sha(sha1 *out, char const *hex_40) {
    a_cstr(hex_s, hex_40);
    a_dup(u8c, hex_dup, hex_s);
    u8s bin = {out->data, out->data + 20};
    HEXu8sDrainSome(bin, hex_dup);
}

//  Capture REFADVEmit output to a tempfile fd, mmap the result, hand
//  back the bytes via `*out`.  Caller frees the buffer via free().
static ok64 emit_to_buf(refadv const *adv, u8 **out_data, size_t *out_len) {
    sane(adv && out_data && out_len);
    char tmpl[] = "/tmp/refadv-emit-XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) fail(REFADVFAIL);
    unlink(tmpl);

    call(REFADVEmit, fd, adv);

    off_t end = lseek(fd, 0, SEEK_END);
    if (end <= 0) { close(fd); fail(REFADVFAIL); }
    lseek(fd, 0, SEEK_SET);
    u8 *buf = (u8 *)malloc((size_t)end);
    if (!buf) { close(fd); fail(REFADVFAIL); }
    ssize_t r = read(fd, buf, (size_t)end);
    close(fd);
    if (r != end) { free(buf); fail(REFADVFAIL); }
    *out_data = buf;
    *out_len  = (size_t)end;
    done;
}

// ---- Test 1: empty REFS ----

ok64 REFADVtest_empty() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/refadv-empty-XXXXXX";
    want(tmp_make(tmpdir) == OK);
    a_cstr(root, tmpdir);
    home h = {};
    call(HOMEOpen, &h, root, YES);
    call(KEEPOpen, &h, YES);

    refadv adv = {};
    call(REFADVOpen, &adv, &KEEP);
    want(adv.count == 0);

    u8 *bytes = NULL;
    size_t blen = 0;
    call(emit_to_buf, &adv, &bytes, &blen);

    //  Expect a single flush packet "0000".
    want(blen == 4);
    want(memcmp(bytes, "0000", 4) == 0);
    free(bytes);

    REFADVClose(&adv);
    call(KEEPClose);
    HOMEClose(&h);
    tmp_rm(tmpdir);
    done;
}

// ---- Test 2: single trunk ref (heads/main) ----

ok64 REFADVtest_single_trunk() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/refadv-single-XXXXXX";
    want(tmp_make(tmpdir) == OK);
    a_cstr(root, tmpdir);
    home h = {};
    call(HOMEOpen, &h, root, YES);
    call(KEEPOpen, &h, YES);

    //  REFS lives at <root>/.dogs.
    a_path(keepdir, u8bDataC(h.root), KEEP_DIR_S);
    char const *hex = "0123456789abcdef0123456789abcdef01234567";
    call(push_head, $path(keepdir), "main", hex);

    refadv adv = {};
    call(REFADVOpen, &adv, &KEEP);
    want(adv.count == 1);

    sha1 expect = {};
    hex_to_sha(&expect, hex);
    want(sha1eq(&adv.ents[0].tip, &expect));
    a_cstr(want_refname, "refs/heads/main");
    want(u8csLen(adv.ents[0].refname) == u8csLen(want_refname));
    want(memcmp(adv.ents[0].refname[0], want_refname[0],
                u8csLen(want_refname)) == 0);
    //  heads/main → trunk → empty dir.
    want(u8csLen(adv.ents[0].dir) == 0);

    u8 *bytes = NULL;
    size_t blen = 0;
    call(emit_to_buf, &adv, &bytes, &blen);

    //  Drain the first pkt-line, check it carries capabilities.
    u8cs scan = {bytes, bytes + blen};
    u8cs line = {};
    ok64 d = PKTu8sDrain(scan, line);
    want(d == OK);
    //  Layout: 40 sha + ' ' + 'refs/heads/main' + '\0' + caps + '\n'.
    want(u8csLen(line) > 40 + 1 + 15 + 1);
    want(line[0][40] == ' ');
    //  Find NUL after refname.
    b8 saw_nul = NO;
    for (u8c *p = line[0]; p < line[1]; p++) {
        if (*p == 0) { saw_nul = YES; break; }
    }
    want(saw_nul == YES);
    //  Last byte must be '\n'.
    want(line[1][-1] == '\n');

    //  Next packet is flush.
    d = PKTu8sDrain(scan, line);
    want(d == PKTFLUSH);

    free(bytes);
    REFADVClose(&adv);
    call(KEEPClose);
    HOMEClose(&h);
    tmp_rm(tmpdir);
    done;
}

// ---- Test 3: multiple refs (heads/main + tags/v1.0) ----

ok64 REFADVtest_multi() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/refadv-multi-XXXXXX";
    want(tmp_make(tmpdir) == OK);
    a_cstr(root, tmpdir);
    home h = {};
    call(HOMEOpen, &h, root, YES);
    call(KEEPOpen, &h, YES);

    a_path(keepdir, u8bDataC(h.root), KEEP_DIR_S);
    char const *hex_main = "1111111111111111111111111111111111111111";
    char const *hex_tag  = "2222222222222222222222222222222222222222";
    call(push_head, $path(keepdir), "main",  hex_main);
    call(push_tag,  $path(keepdir), "v1.0", hex_tag);

    refadv adv = {};
    call(REFADVOpen, &adv, &KEEP);
    want(adv.count == 2);

    //  Find which entry is which (REFS order is undefined).
    refadv_entry const *main_ent = NULL;
    refadv_entry const *tag_ent  = NULL;
    for (u32 i = 0; i < adv.count; i++) {
        a_cstr(rh, "refs/heads/main");
        a_cstr(rt, "refs/tags/v1.0");
        if (u8csLen(adv.ents[i].refname) == u8csLen(rh) &&
            memcmp(adv.ents[i].refname[0], rh[0], u8csLen(rh)) == 0)
            main_ent = &adv.ents[i];
        if (u8csLen(adv.ents[i].refname) == u8csLen(rt) &&
            memcmp(adv.ents[i].refname[0], rt[0], u8csLen(rt)) == 0)
            tag_ent  = &adv.ents[i];
    }
    want(main_ent != NULL);
    want(tag_ent  != NULL);
    //  heads/main → trunk dir = empty; tags/v1.0 → "tags/v1.0".
    want(u8csLen(main_ent->dir) == 0);
    a_cstr(want_tag_dir, "tags/v1.0");
    want(u8csLen(tag_ent->dir) == u8csLen(want_tag_dir));
    want(memcmp(tag_ent->dir[0], want_tag_dir[0],
                u8csLen(want_tag_dir)) == 0);

    u8 *bytes = NULL;
    size_t blen = 0;
    call(emit_to_buf, &adv, &bytes, &blen);

    //  First line carries '\0caps'; second does NOT.
    u8cs scan = {bytes, bytes + blen};
    u8cs line1 = {}, line2 = {};
    want(PKTu8sDrain(scan, line1) == OK);
    want(PKTu8sDrain(scan, line2) == OK);
    b8 first_has_nul = NO;
    for (u8c *p = line1[0]; p < line1[1]; p++)
        if (*p == 0) { first_has_nul = YES; break; }
    b8 second_has_nul = NO;
    for (u8c *p = line2[0]; p < line2[1]; p++)
        if (*p == 0) { second_has_nul = YES; break; }
    want(first_has_nul == YES);
    want(second_has_nul == NO);

    want(PKTu8sDrain(scan, line1) == PKTFLUSH);

    free(bytes);
    REFADVClose(&adv);
    call(KEEPClose);
    HOMEClose(&h);
    tmp_rm(tmpdir);
    done;
}

// ---- Test 4: tip lookup ----

ok64 REFADVtest_tip_lookup() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/refadv-tip-XXXXXX";
    want(tmp_make(tmpdir) == OK);
    a_cstr(root, tmpdir);
    home h = {};
    call(HOMEOpen, &h, root, YES);
    call(KEEPOpen, &h, YES);

    a_path(keepdir, u8bDataC(h.root), KEEP_DIR_S);
    char const *hex_main = "deadbeef00000000000000000000000000000000";
    char const *hex_tag  = "00000000000000000000000000000000beadeed0";
    call(push_head, $path(keepdir), "main",  hex_main);
    call(push_tag,  $path(keepdir), "v0.9", hex_tag);

    refadv adv = {};
    call(REFADVOpen, &adv, &KEEP);
    want(adv.count == 2);

    sha1 sha_main = {};
    hex_to_sha(&sha_main, hex_main);
    u8cs dirs[4] = {};
    u32 m = REFADVTipDirs(&adv, &sha_main, dirs, 4);
    want(m == 1);
    //  main → trunk → empty dir.
    want(u8csLen(dirs[0]) == 0);

    sha1 sha_tag = {};
    hex_to_sha(&sha_tag, hex_tag);
    m = REFADVTipDirs(&adv, &sha_tag, dirs, 4);
    want(m == 1);
    a_cstr(want_dir, "tags/v0.9");
    want(u8csLen(dirs[0]) == u8csLen(want_dir));
    want(memcmp(dirs[0][0], want_dir[0], u8csLen(want_dir)) == 0);

    sha1 sha_unknown = {};
    char const *hex_unk = "abababababababababababababababababababab";
    hex_to_sha(&sha_unknown, hex_unk);
    m = REFADVTipDirs(&adv, &sha_unknown, dirs, 4);
    want(m == 0);

    REFADVClose(&adv);
    call(KEEPClose);
    HOMEClose(&h);
    tmp_rm(tmpdir);
    done;
}

// ---- Test 5: round-trip pkt-line parsing ----

ok64 REFADVtest_round_trip() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/refadv-round-XXXXXX";
    want(tmp_make(tmpdir) == OK);
    a_cstr(root, tmpdir);
    home h = {};
    call(HOMEOpen, &h, root, YES);
    call(KEEPOpen, &h, YES);

    a_path(keepdir, u8bDataC(h.root), KEEP_DIR_S);
    char const *hex_a = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    char const *hex_b = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    char const *hex_c = "cccccccccccccccccccccccccccccccccccccccc";
    call(push_head, $path(keepdir), "main",  hex_a);
    call(push_head, $path(keepdir), "feat",  hex_b);
    call(push_tag,  $path(keepdir), "v2.0", hex_c);

    refadv adv = {};
    call(REFADVOpen, &adv, &KEEP);
    want(adv.count == 3);

    u8 *bytes = NULL;
    size_t blen = 0;
    call(emit_to_buf, &adv, &bytes, &blen);

    //  Drain each pkt-line, parse "<sha> <refname>" (drop NUL+caps from
    //  the first line), verify it round-trips against the in-memory adv.
    u8cs scan = {bytes, bytes + blen};
    u32 seen = 0;
    for (;;) {
        u8cs line = {};
        ok64 d = PKTu8sDrain(scan, line);
        if (d == PKTFLUSH) break;
        want(d == OK);

        //  trim trailing '\n'
        if (line[1] > line[0] && line[1][-1] == '\n') line[1]--;

        //  truncate at first NUL (caps separator)
        for (u8c *p = line[0]; p < line[1]; p++) {
            if (*p == 0) { line[1] = p; break; }
        }

        want(u8csLen(line) >= 41);
        want(line[0][40] == ' ');

        //  decode sha
        sha1 got_sha = {};
        u8cs hex_in = {line[0], line[0] + 40};
        a_dup(u8c, hex_dup, hex_in);
        u8s bin = {got_sha.data, got_sha.data + 20};
        want(HEXu8sDrainSome(bin, hex_dup) == OK);

        u8cs got_refname = {line[0] + 41, line[1]};

        //  match against any entry
        b8 matched = NO;
        for (u32 i = 0; i < adv.count; i++) {
            if (!sha1eq(&adv.ents[i].tip, &got_sha)) continue;
            if (u8csLen(got_refname) != u8csLen(adv.ents[i].refname))
                continue;
            if (memcmp(got_refname[0], adv.ents[i].refname[0],
                       u8csLen(got_refname)) == 0) {
                matched = YES;
                break;
            }
        }
        want(matched == YES);
        seen++;
    }
    want(seen == adv.count);

    free(bytes);
    REFADVClose(&adv);
    call(KEEPClose);
    HOMEClose(&h);
    tmp_rm(tmpdir);
    done;
}

ok64 maintest() {
    sane(1);
    fprintf(stderr, "REFADVtest_empty...\n");
    call(REFADVtest_empty);
    fprintf(stderr, "REFADVtest_single_trunk...\n");
    call(REFADVtest_single_trunk);
    fprintf(stderr, "REFADVtest_multi...\n");
    call(REFADVtest_multi);
    fprintf(stderr, "REFADVtest_tip_lookup...\n");
    call(REFADVtest_tip_lookup);
    fprintf(stderr, "REFADVtest_round_trip...\n");
    call(REFADVtest_round_trip);
    fprintf(stderr, "all passed\n");
    done;
}

TEST(maintest)
