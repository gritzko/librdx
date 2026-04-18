#include "sniff/SNIFF.h"
#include "sniff/DEL.h"
#include "sniff/GET.h"
#include "sniff/POST.h"
#include "sniff/PUT.h"

#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/TEST.h"
#include "keeper/REFS.h"
#include "keeper/SHA1.h"

// --- Helpers ---

static char g_tmpdir[256];

static ok64 make_tmpdir(void) {
    sane(1);
    snprintf(g_tmpdir, sizeof(g_tmpdir), "/tmp/sniff-test-XXXXXX");
    want(mkdtemp(g_tmpdir) != NULL);
    done;
}

static void rm_tmpdir(void) {
    char cmd[300];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", g_tmpdir);
    system(cmd);
}

// --- Test: intern + path round-trip ---

ok64 SNIFFInternPath() {
    sane(1);
    call(FILEInit);
    call(make_tmpdir);

    a_cstr(root, g_tmpdir);
    home h = {};
    call(HOMEOpen, &h, root, YES);
    sniff s = {};
    call(SNIFFOpen, &s, &h, YES);

    a_cstr(p1, "src/foo.c");
    a_cstr(p2, "src/bar.c");
    a_cstr(p3, "README.md");

    u32 i1 = SNIFFIntern(&s, p1);
    u32 i2 = SNIFFIntern(&s, p2);
    u32 i3 = SNIFFIntern(&s, p3);

    // Distinct indices
    want(i1 != i2);
    want(i1 != i3);
    want(i2 != i3);

    // Re-intern returns same index
    want(SNIFFIntern(&s, p1) == i1);
    want(SNIFFIntern(&s, p2) == i2);

    // Path round-trip
    u8cs out = {};
    call(SNIFFPath, out, &s, i1);
    want($len(out) == $len(p1));
    want(memcmp(out[0], p1[0], $len(p1)) == 0);

    call(SNIFFPath, out, &s, i3);
    want($len(out) == $len(p3));
    want(memcmp(out[0], p3[0], $len(p3)) == 0);

    // Count includes reserved root-dir index "/".
    want(SNIFFCount(&s) == 4);

    call(SNIFFClose, &s);
    HOMEClose(&h);
    rm_tmpdir();
    done;
}

// --- Test: record + get round-trip ---

ok64 SNIFFRecordGet() {
    sane(1);
    call(FILEInit);
    call(make_tmpdir);

    a_cstr(root, g_tmpdir);
    home h = {};
    call(HOMEOpen, &h, root, YES);
    sniff s = {};
    call(SNIFFOpen, &s, &h, YES);

    a_cstr(p, "test.c");
    u32 idx = SNIFFIntern(&s, p);

    SNIFFRecord(&s, SNIFF_BLOB, idx, 0x123456789AULL);
    SNIFFRecord(&s, SNIFF_CHECKOUT, idx, 1700000000ULL);
    SNIFFRecord(&s, SNIFF_CHANGED, idx, 1700000010ULL);

    want(SNIFFGet(&s, SNIFF_BLOB, idx) == 0x123456789AULL);
    want(SNIFFGet(&s, SNIFF_CHECKOUT, idx) == 1700000000ULL);
    want(SNIFFGet(&s, SNIFF_CHANGED, idx) == 1700000010ULL);

    // Later record overwrites
    SNIFFRecord(&s, SNIFF_CHANGED, idx, 1700000020ULL);
    want(SNIFFGet(&s, SNIFF_CHANGED, idx) == 1700000020ULL);

    call(SNIFFClose, &s);
    HOMEClose(&h);
    rm_tmpdir();
    done;
}

// --- Test: state persists across close/reopen ---

ok64 SNIFFPersist() {
    sane(1);
    call(FILEInit);
    call(make_tmpdir);

    a_cstr(root, g_tmpdir);
    home h = {};
    call(HOMEOpen, &h, root, YES);

    // Write
    {
        sniff s = {};
        call(SNIFFOpen, &s, &h, YES);
        a_cstr(p, "persist.c");
        u32 idx = SNIFFIntern(&s, p);
        SNIFFRecord(&s, SNIFF_BLOB, idx, 0xABCDEF0123ULL);
        SNIFFRecord(&s, SNIFF_CHECKOUT, idx, 1700000000ULL);
        call(SNIFFClose, &s);
    }

    // Reopen and verify.  Index 0 is the reserved root-dir "/".
    {
        sniff s = {};
        call(SNIFFOpen, &s, &h, NO);
        want(SNIFFCount(&s) == 2);

        u8cs root_p = {};
        call(SNIFFPath, root_p, &s, 0);
        want($len(root_p) == 1);
        want(root_p[0][0] == '/');

        u8cs out = {};
        call(SNIFFPath, out, &s, 1);
        want($len(out) == 9);
        want(memcmp(out[0], "persist.c", 9) == 0);

        want(SNIFFGet(&s, SNIFF_BLOB, 1) == 0xABCDEF0123ULL);
        want(SNIFFGet(&s, SNIFF_CHECKOUT, 1) == 1700000000ULL);

        call(SNIFFClose, &s);
    }

    HOMEClose(&h);
    rm_tmpdir();
    done;
}

// --- Test: checkout + commit round-trip ---

ok64 SNIFFCheckoutCommit() {
    sane(1);
    call(FILEInit);
    call(make_tmpdir);

    a_cstr(root, g_tmpdir);
    home h = {};
    call(HOMEOpen, &h, root, YES);

    // Open keeper, create a blob + tree + commit manually
    keeper k = {};
    call(KEEPOpen, &k, &h, YES);

    keep_pack p = {};
    call(KEEPPackOpen, &k, &p);

    // Blob: "hello\n"
    a_cstr(blob_data, "hello\n");
    sha1 blob_sha = {};
    call(KEEPPackFeed, &k, &p, DOG_OBJ_BLOB, blob_data, &blob_sha);

    // Tree: one entry "100644 test.txt\0<sha>"
    a_pad(u8, tree_buf, 256);
    a_cstr(tm, "100644 test.txt");
    u8bFeed(tree_buf, tm);
    u8bFeed1(tree_buf, 0);
    a_rawc(sha_s, blob_sha);
    u8bFeed(tree_buf, sha_s);
    a_dup(u8c, tree_content, u8bData(tree_buf));

    sha1 tree_sha = {};
    call(KEEPPackFeed, &k, &p, DOG_OBJ_TREE, tree_content, &tree_sha);

    // Commit
    a_pad(u8, cbuf, 512);
    a_cstr(c1, "tree ");
    u8bFeed(cbuf, c1);
    a_pad(u8, thex, 40);
    a_rawc(ts, tree_sha);
    HEXu8sFeedSome(thex_idle, ts);
    u8bFeed(cbuf, u8bDataC(thex));
    a_cstr(c2, "\nauthor Test <t@t> 1700000000 +0000\n"
               "committer Test <t@t> 1700000000 +0000\n\ninitial\n");
    u8bFeed(cbuf, c2);
    a_dup(u8c, commit_content, u8bData(cbuf));

    sha1 commit_sha = {};
    call(KEEPPackFeed, &k, &p, DOG_OBJ_COMMIT, commit_content, &commit_sha);
    call(KEEPPackClose, &k, &p);

    // Hex of commit SHA for CLI
    a_pad(u8, commit_hex, 40);
    a_rawc(csha_s, commit_sha);
    HEXu8sFeedSome(commit_hex_idle, csha_s);

    // Now checkout via sniff
    sniff s = {};
    call(SNIFFOpen, &s, &h, YES);
    u8cs hex = {u8bDataHead(commit_hex), u8bIdleHead(commit_hex)};
    u8cs no_src_ = {}; call(GETCheckout, &s, &k, root, hex, no_src_);

    // Verify file exists
    a_path(fp, root);
    a_cstr(fn, "/test.txt");
    call(u8bFeed, fp, fn);
    call(PATHu8bTerm, fp);
    struct stat sb = {};
    want(FILEStat(&sb, $path(fp)) == OK);

    // Verify sniff state.  Root "/" is idx 0, test.txt is idx 1.
    want(SNIFFCount(&s) == 2);
    want(SNIFFGet(&s, SNIFF_TREE, 0) != 0);   // root tree base
    want(SNIFFGet(&s, SNIFF_BLOB, 1) != 0);   // test.txt blob
    want(SNIFFGet(&s, SNIFF_CHECKOUT, 1) != 0);

    // Modify file, update, commit
    {
        int fd = -1;
        call(FILECreate, &fd, $path(fp));
        a_cstr(newdata, "modified\n");
        FILEFeedAll(fd, newdata);
        FILEClose(&fd);
    }
    // Need mtime to differ
    sleep(1);

    // Record changed mtime (test.txt is idx 1; idx 0 is root "/").
    struct stat sb2 = {};
    call(FILEStat, &sb2, $path(fp));
    SNIFFRecord(&s, SNIFF_CHANGED, 1, (u64)sb2.st_mtim.tv_sec);

    // Commit (HEAD is already set to the initial commit from GETCheckout).
    a_cstr(msg, "second commit");
    a_cstr(author, "Test <t@t>");
    sha1 new_sha = {};
    call(POSTCommit, &s, &k, root, msg, author, &new_sha);

    // Verify new commit exists
    u64 new_hashlet = WHIFFHashlet60(&new_sha);
    want(KEEPHas(&k, new_hashlet, 15) == OK);

    // Verify via KEEPGet
    Bu8 out = {};
    call(u8bAllocate, out, 1UL << 20);
    u8 otype = 0;
    call(KEEPGet, &k, new_hashlet, 10, out, &otype);
    want(otype == DOG_OBJ_COMMIT);

    // Verify commit content mentions parent
    u8cs body = {u8bDataHead(out), u8bIdleHead(out)};
    u8cs scan = {body[0], body[1]};
    want(u8csFind(scan, 'p') == OK);  // "parent ..."

    u8bFree(out);
    call(SNIFFClose, &s);
    call(KEEPClose, &k);
    HOMEClose(&h);
    rm_tmpdir();
    done;
}

// --- Helper: sha1 to hex string ---

static void sha2hex(u8bp buf, sha1 const *sha) {
    u8bReset(buf);
    a_rawc(s, *sha);
    u8s idle = {buf[2], buf[3]};
    HEXu8sFeedSome(idle, s);
    ((u8p *)buf)[2] = idle[0];  // advance DATA end
}

// --- Helper: write a file under root ---

static ok64 write_file(u8cs root, char const *rel, char const *content) {
    sane($ok(root));
    a_path(fp, root);
    a_cstr(sep, "/");
    call(u8bFeed, fp, sep);
    a_cstr(r, rel);
    call(u8bFeed, fp, r);
    call(PATHu8bTerm, fp);

    // Ensure parent dir exists
    a_path(dp, root);
    u8bFeed(dp, sep);
    u8bFeed(dp, r);
    PATHu8bTerm(dp);
    // Walk back to last /
    u8cs dpath = {u8bDataHead(dp), u8bIdleHead(dp)};
    u8cs dscan = {dpath[0], dpath[1]};
    u8cp last_slash = NULL;
    a_dup(u8c, dfind, dscan);
    while (u8csFind(dfind, '/') == OK) {
        last_slash = dfind[0];
        ++dfind[0];
    }
    if (last_slash && last_slash > dpath[0]) {
        u8 save = *last_slash;
        *(u8p)last_slash = 0;
        a_cstr(dp2, (char *)dpath[0]);
        a_path(dirp, dp2);
        FILEMakeDirP($path(dirp));
        *(u8p)last_slash = save;
    }

    int fd = -1;
    call(FILECreate, &fd, $path(fp));
    a_cstr(data, content);
    call(FILEFeedAll, fd, data);
    close(fd);
    done;
}

// --- Helper: check file exists and has expected content ---

static ok64 check_file(u8cs root, char const *rel, char const *expected) {
    sane($ok(root));
    a_path(fp, root);
    a_cstr(sep, "/");
    call(u8bFeed, fp, sep);
    a_cstr(r, rel);
    call(u8bFeed, fp, r);
    call(PATHu8bTerm, fp);

    struct stat sb = {};
    want(FILEStat(&sb, $path(fp)) == OK);

    Bu8 content = {};
    call(u8bAllocate, content, 1UL << 20);
    int fd = -1;
    call(FILEOpen, &fd, $path(fp), O_RDONLY);
    FILEdrainall(u8bIdle(content), fd);
    close(fd);

    size_t elen = strlen(expected);
    want(u8bDataLen(content) == elen);
    want(memcmp(u8bDataHead(content), expected, elen) == 0);
    u8bFree(content);
    done;
}

// --- Helper: check file does NOT exist ---

static b8 file_gone(u8cs root, char const *rel) {
    a_path(fp, root);
    a_cstr(sep, "/");
    u8bFeed(fp, sep);
    a_cstr(r, rel);
    u8bFeed(fp, r);
    PATHu8bTerm(fp);
    struct stat sb = {};
    return (FILEStat(&sb, $path(fp)) != OK);
}

// --- Helper: create initial commit with N files in keeper ---

typedef struct { char const *name; char const *data; } testfile;

static ok64 make_commit(sha1 *commit_out, keeper *k,
                        testfile const *files, u32 nfiles,
                        sha1 const *parent) {
    sane(k && commit_out);
    keep_pack p = {};
    call(KEEPPackOpen, k, &p);

    // Create blobs + tree entries
    a_pad(u8, tree_buf, 4096);
    for (u32 i = 0; i < nfiles; i++) {
        a_cstr(blob, files[i].data);
        sha1 bsha = {};
        call(KEEPPackFeed, k, &p, DOG_OBJ_BLOB, blob, &bsha);
        a_cstr(mode, "100644 ");
        u8bFeed(tree_buf, mode);
        a_cstr(name, files[i].name);
        u8bFeed(tree_buf, name);
        u8bFeed1(tree_buf, 0);
        a_rawc(sraw, bsha);
        u8bFeed(tree_buf, sraw);
    }

    sha1 tree_sha = {};
    a_dup(u8c, tc, u8bData(tree_buf));
    call(KEEPPackFeed, k, &p, DOG_OBJ_TREE, tc, &tree_sha);

    // Commit object
    a_pad(u8, cbuf, 1024);
    a_cstr(cl1, "tree ");
    u8bFeed(cbuf, cl1);
    a_pad(u8, thex, 40);
    a_rawc(ts, tree_sha);
    HEXu8sFeedSome(thex_idle, ts);
    u8bFeed(cbuf, u8bDataC(thex));
    u8bFeed1(cbuf, '\n');

    if (parent) {
        a_cstr(pl, "parent ");
        u8bFeed(cbuf, pl);
        a_pad(u8, phex, 40);
        a_rawc(ps, *parent);
        HEXu8sFeedSome(phex_idle, ps);
        u8bFeed(cbuf, u8bDataC(phex));
        u8bFeed1(cbuf, '\n');
    }

    a_cstr(hdr, "author Test <t@t> 1700000000 +0000\n"
                "committer Test <t@t> 1700000000 +0000\n\ninitial\n");
    u8bFeed(cbuf, hdr);

    a_dup(u8c, cc, u8bData(cbuf));
    call(KEEPPackFeed, k, &p, DOG_OBJ_COMMIT, cc, commit_out);
    call(KEEPPackClose, k, &p);
    done;
}

// --- Test: full round-trip: get, modify, post, delete, get ---

ok64 SNIFFRoundTrip() {
    sane(1);
    call(FILEInit);
    call(make_tmpdir);

    a_cstr(root, g_tmpdir);
    home h = {};
    call(HOMEOpen, &h, root, YES);

    // 1. Create initial commit: a.txt, b.txt, c.txt
    keeper k = {};
    call(KEEPOpen, &k, &h, YES);

    testfile init_files[] = {
        {"a.txt", "alpha\n"},
        {"b.txt", "bravo\n"},
        {"c.txt", "charlie\n"},
    };
    sha1 c1_sha = {};
    call(make_commit, &c1_sha, &k, init_files, 3, NULL);

    a_pad(u8, c1_hex, 40);
    sha2hex(c1_hex, &c1_sha);
    u8cs c1h = {u8bDataHead(c1_hex), u8bIdleHead(c1_hex)};

    // 2. GET: checkout initial commit
    sniff s = {};
    call(SNIFFOpen, &s, &h, YES);
    u8cs no_src1 = {}; call(GETCheckout, &s, &k, root, c1h, no_src1);

    call(check_file, root, "a.txt", "alpha\n");
    call(check_file, root, "b.txt", "bravo\n");
    call(check_file, root, "c.txt", "charlie\n");
    fprintf(stderr, "  get: initial checkout OK\n");

    // Worktree's commit recorded in keeper refs.
    {
        a_path(keepdir, u8bDataC(h.root), KEEP_DIR_S);
        a_pad(u8, wtbuf, 1280);
        a_cstr(scheme, "file://");
        u8bFeed(wtbuf, scheme);
        u8bFeed(wtbuf, root);
        a_dup(u8c, wt_key, u8bData(wtbuf));
        a_pad(u8, arena, 256);
        uri resolved = {};
        call(REFSResolve, &resolved, arena, $path(keepdir), wt_key);
        want($len(resolved.query) == 40);
        want(memcmp(resolved.query[0], c1h[0], 40) == 0);
    }

    // 3. Modify a.txt, add d.txt
    sleep(1);  // ensure mtime differs
    call(write_file, root, "a.txt", "ALPHA MODIFIED\n");
    call(write_file, root, "d.txt", "delta new\n");

    // Intern d.txt and record changes
    a_cstr(dpath, "d.txt");
    u32 didx = SNIFFIntern(&s, dpath);
    {
        a_path(afp, root);
        a_cstr(asep, "/a.txt");
        u8bFeed(afp, asep);
        PATHu8bTerm(afp);
        struct stat asb = {};
        call(FILEStat, &asb, $path(afp));
        // a.txt is idx 0 after checkout
        a_cstr(apath, "a.txt");
        u32 aidx = SNIFFIntern(&s, apath);
        SNIFFRecord(&s, SNIFF_CHANGED, aidx, (u64)asb.st_mtim.tv_sec);
    }
    {
        a_path(dfp, root);
        a_cstr(dsep, "/d.txt");
        u8bFeed(dfp, dsep);
        PATHu8bTerm(dfp);
        struct stat dsb = {};
        call(FILEStat, &dsb, $path(dfp));
        SNIFFRecord(&s, SNIFF_CHANGED, didx, (u64)dsb.st_mtim.tv_sec);
    }

    // 4. POST: commit changes.  HEAD is set to c1 from GETCheckout;
    //    POSTCommit auto-stages everything dirty, then commits.
    sha1 c2_sha = {};
    a_cstr(msg2, "modify a, add d");
    a_cstr(author, "Test <t@t>");
    call(POSTCommit, &s, &k, root, msg2, author, &c2_sha);

    a_pad(u8, c2_hex, 40);
    sha2hex(c2_hex, &c2_sha);
    u8cs c2h = {u8bDataHead(c2_hex), u8bIdleHead(c2_hex)};
    fprintf(stderr, "  put: commit 2 OK\n");

    // Worktree's commit advanced to commit 2.
    {
        a_path(keepdir, u8bDataC(h.root), KEEP_DIR_S);
        a_pad(u8, wtbuf, 1280);
        a_cstr(scheme, "file://");
        u8bFeed(wtbuf, scheme);
        u8bFeed(wtbuf, root);
        a_dup(u8c, wt_key, u8bData(wtbuf));
        a_pad(u8, arena, 256);
        uri resolved = {};
        call(REFSResolve, &resolved, arena, $path(keepdir), wt_key);
        want($len(resolved.query) == 40);
        want(memcmp(resolved.query[0], c2h[0], 40) == 0);
    }

    // 5. Wipe sniff state + worktree, re-checkout commit 2 cleanly
    call(SNIFFClose, &s);
    {
        char cmd[300];
        snprintf(cmd, sizeof(cmd),
                 "rm -rf %s/.dogs/sniff %s/a.txt %s/b.txt %s/c.txt %s/d.txt",
                 g_tmpdir, g_tmpdir, g_tmpdir, g_tmpdir, g_tmpdir);
        system(cmd);
    }

    call(SNIFFOpen, &s, &h, YES);
    u8cs no_src2 = {}; call(GETCheckout, &s, &k, root, c2h, no_src2);

    call(check_file, root, "a.txt", "ALPHA MODIFIED\n");
    call(check_file, root, "b.txt", "bravo\n");
    call(check_file, root, "c.txt", "charlie\n");
    call(check_file, root, "d.txt", "delta new\n");
    fprintf(stderr, "  get: commit 2 checkout OK\n");

    // 6. DEL + POST: remove b.txt, create new commit
    {
        keep_pack dp = {};
        call(KEEPPackOpen, &k, &dp);

        u32 npaths = SNIFFCount(&s);
        Bu8 dsbuf = {};
        call(u8bAllocate, dsbuf, npaths);
        memset(u8bDataHead(dsbuf), 0, npaths);
        u8p dset = u8bDataHead(dsbuf);

        a_cstr(bpath, "b.txt");
        u32 bidx = SNIFFIntern(&s, bpath);
        dset[bidx] = 1;

        sha1 del_tree = {};
        call(DELStage, &del_tree, &s, &k, &dp, root, dset);
        u8bFree(dsbuf);
        KEEPPackClose(&k, &dp);

        // DELStage updated the base tree.  Now commit it via POST.
        // (We manually wrap here instead of POSTCommit to verify the
        // DELStage's staged tree SHA propagates.)
        keep_pack dp2 = {};
        call(KEEPPackOpen, &k, &dp2);
        a_pad(u8, com, 1024);
        a_cstr(tl, "tree ");
        u8bFeed(com, tl);
        a_pad(u8, dthex, 40);
        sha2hex(dthex, &del_tree);
        u8bFeed(com, u8bDataC(dthex));
        u8bFeed1(com, '\n');
        a_cstr(par, "parent ");
        u8bFeed(com, par);
        u8bFeed(com, u8bDataC(c2_hex));
        u8bFeed1(com, '\n');
        a_cstr(rest, "author Test <t@t> 1700000000 +0000\n"
                     "committer Test <t@t> 1700000000 +0000\n\ndelete b\n");
        u8bFeed(com, rest);

        sha1 c3_sha = {};
        a_dup(u8c, cdata, u8bData(com));
        call(KEEPPackFeed, &k, &dp2, DOG_OBJ_COMMIT, cdata, &c3_sha);
        call(KEEPPackClose, &k, &dp2);

        a_pad(u8, c3_hex, 40);
        sha2hex(c3_hex, &c3_sha);
        u8cs c3h = {u8bDataHead(c3_hex), u8bIdleHead(c3_hex)};

        // 7. GET the delete commit
        u8cs no_src3 = {}; call(GETCheckout, &s, &k, root, c3h, no_src3);

        call(check_file, root, "a.txt", "ALPHA MODIFIED\n");
        want(file_gone(root, "b.txt"));
        call(check_file, root, "c.txt", "charlie\n");
        call(check_file, root, "d.txt", "delta new\n");
        fprintf(stderr, "  get: commit 3 (delete b) OK\n");
    }

    call(SNIFFClose, &s);
    call(KEEPClose, &k);
    HOMEClose(&h);
    rm_tmpdir();
    done;
}

// --- Main ---

ok64 maintest() {
    sane(1);
    fprintf(stderr, "SNIFFInternPath...\n");
    call(SNIFFInternPath);
    fprintf(stderr, "SNIFFRecordGet...\n");
    call(SNIFFRecordGet);
    fprintf(stderr, "SNIFFPersist...\n");
    call(SNIFFPersist);
    fprintf(stderr, "SNIFFCheckoutCommit...\n");
    call(SNIFFCheckoutCommit);
    fprintf(stderr, "SNIFFRoundTrip...\n");
    call(SNIFFRoundTrip);
    fprintf(stderr, "all passed\n");
    done;
}

TEST(maintest);
