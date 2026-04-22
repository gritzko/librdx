#include "sniff/SNIFF.h"
#include "sniff/AT.h"
#include "sniff/DEL.h"
#include "sniff/GET.h"
#include "sniff/POST.h"
#include "sniff/PUT.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

//  Bump a file's atime/mtime forward by `bump_sec` seconds.  Lets
//  tests observe a distinct mtime without a real sleep.
static void bump_mtime(char const *abs_path, int bump_sec) {
    struct stat sb = {};
    if (stat(abs_path, &sb) != 0) return;
    struct timeval tv[2] = {
        { .tv_sec = sb.st_atim.tv_sec + bump_sec, .tv_usec = 0 },
        { .tv_sec = sb.st_mtim.tv_sec + bump_sec, .tv_usec = 0 },
    };
    utimes(abs_path, tv);
}

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
    
    call(KEEPOpen, &h, YES);
    sniff s = {};
    call(SNIFFOpen, &h, YES);

    a_cstr(p1, "src/foo.c");
    a_cstr(p2, "src/bar.c");
    a_cstr(p3, "README.md");

    u32 i1 = SNIFFIntern(p1);
    u32 i2 = SNIFFIntern(p2);
    u32 i3 = SNIFFIntern(p3);

    // Distinct indices
    want(i1 != i2);
    want(i1 != i3);
    want(i2 != i3);

    // Re-intern returns same index
    want(SNIFFIntern(p1) == i1);
    want(SNIFFIntern(p2) == i2);

    // Path round-trip
    u8cs out = {};
    call(SNIFFPath, out, i1);
    want($len(out) == $len(p1));
    want(memcmp(out[0], p1[0], $len(p1)) == 0);

    call(SNIFFPath, out, i3);
    want($len(out) == $len(p3));
    want(memcmp(out[0], p3[0], $len(p3)) == 0);

    // Count includes reserved root-dir index "/".
    want(SNIFFCount() == 4);

    call(SNIFFClose);
    KEEPClose();
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
    
    call(KEEPOpen, &h, YES);
    sniff s = {};
    call(SNIFFOpen, &h, YES);

    a_cstr(p, "test.c");
    u32 idx = SNIFFIntern(p);

    SNIFFRecord(SNIFF_BLOB, idx, 0x123456789AULL);
    SNIFFRecord(SNIFF_CHECKOUT, idx, 1700000000ULL);
    SNIFFRecord(SNIFF_CHANGED, idx, 1700000010ULL);

    want(SNIFFGet(SNIFF_BLOB, idx) == 0x123456789AULL);
    want(SNIFFGet(SNIFF_CHECKOUT, idx) == 1700000000ULL);
    want(SNIFFGet(SNIFF_CHANGED, idx) == 1700000010ULL);

    // Later record overwrites
    SNIFFRecord(SNIFF_CHANGED, idx, 1700000020ULL);
    want(SNIFFGet(SNIFF_CHANGED, idx) == 1700000020ULL);

    call(SNIFFClose);
    KEEPClose();
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
        
        call(KEEPOpen, &h, YES);
        sniff s = {};
        call(SNIFFOpen, &h, YES);
        a_cstr(p, "persist.c");
        u32 idx = SNIFFIntern(p);
        SNIFFRecord(SNIFF_BLOB, idx, 0xABCDEF0123ULL);
        SNIFFRecord(SNIFF_CHECKOUT, idx, 1700000000ULL);
        call(SNIFFClose);
        KEEPClose();
    }

    // Reopen and verify.  Index 0 is the reserved root-dir "/".
    {
        
        call(KEEPOpen, &h, NO);
        sniff s = {};
        call(SNIFFOpen, &h, NO);
        want(SNIFFCount() == 2);

        u8cs root_p = {};
        call(SNIFFPath, root_p, 0);
        want($len(root_p) == 1);
        want(root_p[0][0] == '/');

        u8cs out = {};
        call(SNIFFPath, out, 1);
        want($len(out) == 9);
        want(memcmp(out[0], "persist.c", 9) == 0);

        want(SNIFFGet(SNIFF_BLOB, 1) == 0xABCDEF0123ULL);
        want(SNIFFGet(SNIFF_CHECKOUT, 1) == 1700000000ULL);

        call(SNIFFClose);
        KEEPClose();
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
    
    call(KEEPOpen, &h, YES);

    keep_pack p = {};
    call(KEEPPackOpen, &KEEP, &p);
    //  Hand-rolled objects for test — fed in non-canonical order
    //  (blob before tree).  Sniff's real POST path repacks canonically.
    p.strict_order = NO;

    // Blob: "hello\n"
    a_cstr(blob_data, "hello\n");
    sha1 blob_sha = {};
    u8csc nopath_b = {NULL,NULL}; call(KEEPPackFeed, &KEEP, &p, DOG_OBJ_BLOB, blob_data, nopath_b, 0, &blob_sha);

    // Tree: one entry "100644 test.txt\0<sha>"
    a_pad(u8, tree_buf, 256);
    a_cstr(tm, "100644 test.txt");
    u8bFeed(tree_buf, tm);
    u8bFeed1(tree_buf, 0);
    a_rawc(sha_s, blob_sha);
    u8bFeed(tree_buf, sha_s);
    a_dup(u8c, tree_content, u8bData(tree_buf));

    sha1 tree_sha = {};
    u8csc nopath_t = {NULL,NULL}; call(KEEPPackFeed, &KEEP, &p, DOG_OBJ_TREE, tree_content, nopath_t, 0, &tree_sha);

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
    u8csc nopath_c = {NULL,NULL}; call(KEEPPackFeed, &KEEP, &p, DOG_OBJ_COMMIT, commit_content, nopath_c, 0, &commit_sha);
    call(KEEPPackClose, &KEEP, &p);

    // Hex of commit SHA for CLI
    a_pad(u8, commit_hex, 40);
    a_rawc(csha_s, commit_sha);
    HEXu8sFeedSome(commit_hex_idle, csha_s);

    // Now checkout via sniff
    sniff s = {};
    call(SNIFFOpen, &h, YES);
    u8cs hex = {u8bDataHead(commit_hex), u8bIdleHead(commit_hex)};
    u8cs no_src_ = {}; call(GETCheckout, root, hex, no_src_);

    // Verify file exists
    a_path(fp, root);
    a_cstr(fn, "/test.txt");
    call(u8bFeed, fp, fn);
    call(PATHu8bTerm, fp);
    struct stat sb = {};
    want(FILEStat(&sb, $path(fp)) == OK);

    // Verify sniff state.  Root "/" is idx 0, test.txt is idx 1.
    want(SNIFFCount() == 2);
    want(SNIFFGet(SNIFF_TREE, 0) != 0);   // root tree base
    want(SNIFFGet(SNIFF_BLOB, 1) != 0);   // test.txt blob
    want(SNIFFGet(SNIFF_CHECKOUT, 1) != 0);

    // Modify file, update, commit
    {
        int fd = -1;
        call(FILECreate, &fd, $path(fp));
        a_cstr(newdata, "modified\n");
        FILEFeedAll(fd, newdata);
        FILEClose(&fd);
    }
    // Need mtime to differ.  Real sleep(1) would work but is slow;
    // explicitly bump the mtime forward instead.
    bump_mtime((char *)u8bDataHead(fp), 2);

    // Record changed mtime (test.txt is idx 1; idx 0 is root "/").
    struct stat sb2 = {};
    call(FILEStat, &sb2, $path(fp));
    SNIFFRecord(SNIFF_CHANGED, 1, (u64)sb2.st_mtim.tv_sec);

    // Commit (HEAD is already set to the initial commit from GETCheckout).
    a_cstr(msg, "second commit");
    a_cstr(author, "Test <t@t>");
    sha1 new_sha = {};
    call(POSTCommit, root, msg, author, &new_sha);

    // Verify new commit exists
    u64 new_hashlet = WHIFFHashlet60(&new_sha);
    want(KEEPHas(&KEEP, new_hashlet, 15) == OK);

    // Verify via KEEPGet
    Bu8 out = {};
    call(u8bAllocate, out, 1UL << 20);
    u8 otype = 0;
    call(KEEPGet, &KEEP, new_hashlet, 10, out, &otype);
    want(otype == DOG_OBJ_COMMIT);

    // Verify commit content mentions parent
    u8cs body = {u8bDataHead(out), u8bIdleHead(out)};
    u8cs scan = {body[0], body[1]};
    want(u8csFind(scan, 'p') == OK);  // "parent ..."

    u8bFree(out);
    call(SNIFFClose);
    call(KEEPClose);
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
    //  Hand-rolled blob→tree→commit sequence for tests — not canonical.
    p.strict_order = NO;

    // Create blobs + tree entries
    a_pad(u8, tree_buf, 4096);
    for (u32 i = 0; i < nfiles; i++) {
        a_cstr(blob, files[i].data);
        sha1 bsha = {};
        u8csc nopath_b = {NULL,NULL}; call(KEEPPackFeed, k, &p, DOG_OBJ_BLOB, blob, nopath_b, 0, &bsha);
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
    u8csc nopath_t = {NULL,NULL}; call(KEEPPackFeed, k, &p, DOG_OBJ_TREE, tc, nopath_t, 0, &tree_sha);

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
    u8csc nopath_c = {NULL,NULL}; call(KEEPPackFeed, k, &p, DOG_OBJ_COMMIT, cc, nopath_c, 0, commit_out);
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
    
    call(KEEPOpen, &h, YES);

    testfile init_files[] = {
        {"a.txt", "alpha\n"},
        {"b.txt", "bravo\n"},
        {"c.txt", "charlie\n"},
    };
    sha1 c1_sha = {};
    call(make_commit, &c1_sha, &KEEP, init_files, 3, NULL);

    a_pad(u8, c1_hex, 40);
    sha2hex(c1_hex, &c1_sha);
    u8cs c1h = {u8bDataHead(c1_hex), u8bIdleHead(c1_hex)};

    // 2. GET: checkout initial commit
    sniff s = {};
    call(SNIFFOpen, &h, YES);
    u8cs no_src1 = {}; call(GETCheckout, root, c1h, no_src1);

    call(check_file, root, "a.txt", "alpha\n");
    call(check_file, root, "b.txt", "bravo\n");
    call(check_file, root, "c.txt", "charlie\n");
    fprintf(stderr, "  get: initial checkout OK\n");

    // Worktree's commit recorded in sniff/at.log.
    {
        a_pad(u8, at_b, 256);
        a_pad(u8, at_s, 64);
        sniff_at tail1 = {.branch = at_b, .sha = at_s};
        call(SNIFFAtRead, &tail1);
        want(u8bDataLen(tail1.sha) == 40);
        want(memcmp(u8bDataHead(tail1.sha), c1h[0], 40) == 0);
    }

    // 3. Modify a.txt, add d.txt.  Force new mtimes without a real
    // sleep: write the files first, then bump.
    call(write_file, root, "a.txt", "ALPHA MODIFIED\n");
    call(write_file, root, "d.txt", "delta new\n");
    {
        a_path(afp, root);
        a_cstr(afn, "/a.txt");
        u8bFeed(afp, afn);
        PATHu8bTerm(afp);
        bump_mtime((char *)u8bDataHead(afp), 2);

        a_path(dfp, root);
        a_cstr(dfn, "/d.txt");
        u8bFeed(dfp, dfn);
        PATHu8bTerm(dfp);
        bump_mtime((char *)u8bDataHead(dfp), 2);
    }

    // Intern d.txt and record changes
    a_cstr(dpath, "d.txt");
    u32 didx = SNIFFIntern(dpath);
    {
        a_path(afp, root);
        a_cstr(asep, "/a.txt");
        u8bFeed(afp, asep);
        PATHu8bTerm(afp);
        struct stat asb = {};
        call(FILEStat, &asb, $path(afp));
        // a.txt is idx 0 after checkout
        a_cstr(apath, "a.txt");
        u32 aidx = SNIFFIntern(apath);
        SNIFFRecord(SNIFF_CHANGED, aidx, (u64)asb.st_mtim.tv_sec);
    }
    {
        a_path(dfp, root);
        a_cstr(dsep, "/d.txt");
        u8bFeed(dfp, dsep);
        PATHu8bTerm(dfp);
        struct stat dsb = {};
        call(FILEStat, &dsb, $path(dfp));
        SNIFFRecord(SNIFF_CHANGED, didx, (u64)dsb.st_mtim.tv_sec);
    }

    // 4. POST: commit changes.  HEAD is set to c1 from GETCheckout;
    //    POSTCommit auto-stages everything dirty, then commits.
    sha1 c2_sha = {};
    a_cstr(msg2, "modify a, add d");
    a_cstr(author, "Test <t@t>");
    call(POSTCommit, root, msg2, author, &c2_sha);

    a_pad(u8, c2_hex, 40);
    sha2hex(c2_hex, &c2_sha);
    u8cs c2h = {u8bDataHead(c2_hex), u8bIdleHead(c2_hex)};
    fprintf(stderr, "  put: commit 2 OK\n");

    // Worktree's commit advanced to commit 2 (via at.log).
    {
        a_pad(u8, at_b2, 256);
        a_pad(u8, at_s2, 64);
        sniff_at tail2 = {.branch = at_b2, .sha = at_s2};
        call(SNIFFAtRead, &tail2);
        want(u8bDataLen(tail2.sha) == 40);
        want(memcmp(u8bDataHead(tail2.sha), c2h[0], 40) == 0);
    }

    // 5. Wipe sniff state + worktree, re-checkout commit 2 cleanly
    call(SNIFFClose);
    {
        char cmd[300];
        snprintf(cmd, sizeof(cmd),
                 "rm -rf %s/.dogs/sniff %s/a.txt %s/b.txt %s/c.txt %s/d.txt",
                 g_tmpdir, g_tmpdir, g_tmpdir, g_tmpdir, g_tmpdir);
        system(cmd);
    }

    call(SNIFFOpen, &h, YES);
    u8cs no_src2 = {}; call(GETCheckout, root, c2h, no_src2);

    call(check_file, root, "a.txt", "ALPHA MODIFIED\n");
    call(check_file, root, "b.txt", "bravo\n");
    call(check_file, root, "c.txt", "charlie\n");
    call(check_file, root, "d.txt", "delta new\n");
    fprintf(stderr, "  get: commit 2 checkout OK\n");

    // 6. DEL + POST: remove b.txt, create new commit
    {
        u32 npaths = SNIFFCount();
        Bu8 dsbuf = {};
        call(u8bAllocate, dsbuf, npaths);
        memset(u8bDataHead(dsbuf), 0, npaths);
        u8p dset = u8bDataHead(dsbuf);

        a_cstr(bpath, "b.txt");
        u32 bidx = SNIFFIntern(bpath);
        dset[bidx] = 1;

        sha1 del_tree = {};
        call(DELStage, &del_tree, root, dset);
        u8bFree(dsbuf);
        (void)del_tree;

        //  Commit the delete via POSTCommit — it owns the canonical
        //  repack of staged objects plus the commit onto main keeper.
        a_cstr(msg, "delete b");
        a_cstr(auth, "Test <t@t>");
        sha1 c3_sha = {};
        call(POSTCommit, root, msg, auth, &c3_sha);

        a_pad(u8, c3_hex, 40);
        sha2hex(c3_hex, &c3_sha);
        u8cs c3h = {u8bDataHead(c3_hex), u8bIdleHead(c3_hex)};

        // 7. GET the delete commit
        u8cs no_src3 = {}; call(GETCheckout, root, c3h, no_src3);

        call(check_file, root, "a.txt", "ALPHA MODIFIED\n");
        want(file_gone(root, "b.txt"));
        call(check_file, root, "c.txt", "charlie\n");
        call(check_file, root, "d.txt", "delta new\n");
        fprintf(stderr, "  get: commit 3 (delete b) OK\n");
    }

    call(SNIFFClose);
    call(KEEPClose);
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
