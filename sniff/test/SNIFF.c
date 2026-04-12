#include "sniff/SNIFF.h"
#include "sniff/CHE.h"
#include "sniff/COM.h"

#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/TEST.h"
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
    sniff s = {};
    call(SNIFFOpen, &s, root, YES);

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

    want(SNIFFCount(&s) == 3);

    call(SNIFFClose, &s);
    rm_tmpdir();
    done;
}

// --- Test: record + get round-trip ---

ok64 SNIFFRecordGet() {
    sane(1);
    call(FILEInit);
    call(make_tmpdir);

    a_cstr(root, g_tmpdir);
    sniff s = {};
    call(SNIFFOpen, &s, root, YES);

    a_cstr(p, "test.c");
    u32 idx = SNIFFIntern(&s, p);

    SNIFFRecord(&s, SNIFF_HASHLET, idx, 0x123456789AULL);
    SNIFFRecord(&s, SNIFF_CHECKOUT, idx, 1700000000ULL);
    SNIFFRecord(&s, SNIFF_CHANGED, idx, 1700000010ULL);

    want(SNIFFGet(&s, SNIFF_HASHLET, idx) == 0x123456789AULL);
    want(SNIFFGet(&s, SNIFF_CHECKOUT, idx) == 1700000000ULL);
    want(SNIFFGet(&s, SNIFF_CHANGED, idx) == 1700000010ULL);

    // Later record overwrites
    SNIFFRecord(&s, SNIFF_CHANGED, idx, 1700000020ULL);
    want(SNIFFGet(&s, SNIFF_CHANGED, idx) == 1700000020ULL);

    call(SNIFFClose, &s);
    rm_tmpdir();
    done;
}

// --- Test: state persists across close/reopen ---

ok64 SNIFFPersist() {
    sane(1);
    call(FILEInit);
    call(make_tmpdir);

    a_cstr(root, g_tmpdir);

    // Write
    {
        sniff s = {};
        call(SNIFFOpen, &s, root, YES);
        a_cstr(p, "persist.c");
        u32 idx = SNIFFIntern(&s, p);
        SNIFFRecord(&s, SNIFF_HASHLET, idx, 0xABCDEF0123ULL);
        SNIFFRecord(&s, SNIFF_CHECKOUT, idx, 1700000000ULL);
        call(SNIFFClose, &s);
    }

    // Reopen and verify
    {
        sniff s = {};
        call(SNIFFOpen, &s, root, NO);
        want(SNIFFCount(&s) == 1);

        u8cs out = {};
        call(SNIFFPath, out, &s, 0);
        want($len(out) == 9);
        want(memcmp(out[0], "persist.c", 9) == 0);

        want(SNIFFGet(&s, SNIFF_HASHLET, 0) == 0xABCDEF0123ULL);
        want(SNIFFGet(&s, SNIFF_CHECKOUT, 0) == 1700000000ULL);

        call(SNIFFClose, &s);
    }

    rm_tmpdir();
    done;
}

// --- Test: checkout + commit round-trip ---

ok64 SNIFFCheckoutCommit() {
    sane(1);
    call(FILEInit);
    call(make_tmpdir);

    a_cstr(root, g_tmpdir);

    // Open keeper, create a blob + tree + commit manually
    keeper k = {};
    call(KEEPOpen, &k, root);

    keep_pack p = {};
    call(KEEPPackOpen, &k, &p);

    // Blob: "hello\n"
    a_cstr(blob_data, "hello\n");
    u8 blob_sha[20] = {};
    call(KEEPPackFeed, &k, &p, KEEP_OBJ_BLOB, blob_data, blob_sha);

    // Tree: one entry "100644 test.txt\0<sha>"
    a_pad(u8, tree_buf, 256);
    a_cstr(tm, "100644 test.txt");
    u8bFeed(tree_buf, tm);
    u8bFeed1(tree_buf, 0);
    u8cs sha_s = {blob_sha, blob_sha + 20};
    u8bFeed(tree_buf, sha_s);
    a_dup(u8c, tree_content, u8bData(tree_buf));

    u8 tree_sha[20] = {};
    call(KEEPPackFeed, &k, &p, KEEP_OBJ_TREE, tree_content, tree_sha);

    // Commit
    a_pad(u8, cbuf, 512);
    a_cstr(c1, "tree ");
    u8bFeed(cbuf, c1);
    a_pad(u8, thex, 40);
    u8cs ts = {tree_sha, tree_sha + 20};
    HEXu8sFeedSome(thex_idle, ts);
    u8bFeed(cbuf, u8bDataC(thex));
    a_cstr(c2, "\nauthor Test <t@t> 1700000000 +0000\n"
               "committer Test <t@t> 1700000000 +0000\n\ninitial\n");
    u8bFeed(cbuf, c2);
    a_dup(u8c, commit_content, u8bData(cbuf));

    u8 commit_sha[20] = {};
    call(KEEPPackFeed, &k, &p, KEEP_OBJ_COMMIT, commit_content, commit_sha);
    call(KEEPPackClose, &k, &p);

    // Hex of commit SHA for CLI
    a_pad(u8, commit_hex, 40);
    u8cs csha_s = {commit_sha, commit_sha + 20};
    HEXu8sFeedSome(commit_hex_idle, csha_s);

    // Now checkout via sniff
    sniff s = {};
    call(SNIFFOpen, &s, root, YES);
    u8cs hex = {u8bDataHead(commit_hex), u8bIdleHead(commit_hex)};
    call(CHECheckout, &s, &k, root, hex);

    // Verify file exists
    a_path(fp, root);
    a_cstr(fn, "/test.txt");
    call(u8bFeed, fp, fn);
    call(PATHu8gTerm, PATHu8gIn(fp));
    struct stat sb = {};
    want(FILEStat(&sb, PATHu8cgIn(fp)) == OK);

    // Verify sniff state
    want(SNIFFCount(&s) == 1);
    want(SNIFFGet(&s, SNIFF_HASHLET, 0) != 0);
    want(SNIFFGet(&s, SNIFF_CHECKOUT, 0) != 0);

    // Modify file, update, commit
    {
        int fd = -1;
        call(FILECreate, &fd, PATHu8cgIn(fp));
        a_cstr(newdata, "modified\n");
        FILEFeedAll(fd, newdata);
        FILEClose(&fd);
    }
    // Need mtime to differ
    sleep(1);

    // Record changed mtime
    struct stat sb2 = {};
    call(FILEStat, &sb2, PATHu8cgIn(fp));
    SNIFFRecord(&s, SNIFF_CHANGED, 0, (u64)sb2.st_mtim.tv_sec);

    // Commit
    a_cstr(msg, "second commit");
    a_cstr(author, "Test <t@t>");
    u8 new_sha[20] = {};
    call(COMCommit, &s, &k, root, hex, msg, author, NULL, new_sha);

    // Verify new commit exists
    u64 new_hashlet = wh64Hashlet(new_sha);
    want(KEEPHas(&k, new_hashlet, 10) == OK);

    // Verify via KEEPGet
    Bu8 out = {};
    call(u8bAllocate, out, 1UL << 20);
    u8 otype = 0;
    call(KEEPGet, &k, new_hashlet, 10, out, &otype);
    want(otype == KEEP_OBJ_COMMIT);

    // Verify commit content mentions parent
    u8cs body = {u8bDataHead(out), u8bIdleHead(out)};
    u8cs scan = {body[0], body[1]};
    want(u8csFind(scan, 'p') == OK);  // "parent ..."

    u8bFree(out);
    call(SNIFFClose, &s);
    call(KEEPClose, &k);
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
    fprintf(stderr, "all passed\n");
    done;
}

TEST(maintest);
