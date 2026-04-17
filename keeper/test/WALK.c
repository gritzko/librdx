//  WALK tests: tree walker on KEEP (eager / lazy / skip / stop).
//
#include "keeper/GIT.h"
#include "keeper/KEEP.h"
#include "keeper/SHA1.h"
#include "keeper/WALK.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/TEST.h"
#include "dog/DOG.h"

// ---- Test 1: WALKu8sModeKind table-driven ----

ok64 WALKtest1() {
    sane(1);

    typedef struct {
        char const *mode;
        u8          want_kind;
    } row;

    row cases[] = {
        {"40000",  WALK_KIND_DIR},
        {"100644", WALK_KIND_REG},
        {"100755", WALK_KIND_EXE},
        {"120000", WALK_KIND_LNK},
        {"160000", WALK_KIND_SUB},
        {"",       0},
        {"1",      0},               // too short after '1'
        {"2",      0},               // unknown first digit
        {"999999", 0},               // unknown first digit
        {"100000", WALK_KIND_REG},   // '0' at pos 3 → REG
        {"100700", WALK_KIND_EXE},   // '7' at pos 3 → EXE
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        u8cs m = {(u8cp)cases[i].mode,
                  (u8cp)cases[i].mode + strlen(cases[i].mode)};
        u8 got = WALKu8sModeKind(m);
        if (got != cases[i].want_kind) {
            fprintf(stderr, "mode='%s': want %u got %u\n",
                    cases[i].mode, cases[i].want_kind, got);
        }
        want(got == cases[i].want_kind);
    }

    done;
}

// ---- Test 2: WALKTree / WALKTreeLazy on a synthetic KEEP store ----
//
// Tree layout:
//   hello.txt       "hi\n"            (100644)
//   run.sh          "#!/bin/sh\n"     (100755)
//   sub/nested.txt  "deep\n"          (100644)
//
// Verifies: visit count, kind dispatch, eager-vs-lazy blob filling,
// WALKSKIP pruning a subtree.

typedef struct {
    u32 n_entries;
    u32 n_files;              // REG+EXE+LNK
    u32 n_dirs;
    u32 n_files_with_blob;
    char last_path[256];
    char dir_to_skip[64];     // if non-empty, skip the DIR entry matching
} w2_ctx;

static ok64 w2_visit(u8cs path, u8 kind, u8cp esha, u8cs blob,
                      void0p vctx) {
    (void)esha;
    w2_ctx *c = (w2_ctx *)vctx;
    c->n_entries++;

    size_t plen = $len(path);
    if (plen >= sizeof(c->last_path)) plen = sizeof(c->last_path) - 1;
    memcpy(c->last_path, path[0], plen);
    c->last_path[plen] = 0;

    if (kind == WALK_KIND_DIR) {
        c->n_dirs++;
        if (c->dir_to_skip[0]) {
            size_t dlen = strlen(c->dir_to_skip);
            if (plen == dlen &&
                memcmp(c->last_path, c->dir_to_skip, dlen) == 0)
                return WALKSKIP;
        }
    } else if (kind == WALK_KIND_REG || kind == WALK_KIND_EXE ||
               kind == WALK_KIND_LNK) {
        c->n_files++;
        if (!$empty(blob)) c->n_files_with_blob++;
    }
    return OK;
}

// Build one "leaf tree" — a single-entry tree containing a blob.
static ok64 build_leaf_tree(keeper *k, keep_pack *p,
                             u8cs mode_name, u8cs content,
                             sha1 *tree_out) {
    sane(k && p && tree_out);
    sha1 bsha = {};
    call(KEEPPackFeed, k, p, DOG_OBJ_BLOB, content, &bsha);
    a_pad(u8, tb, 256);
    call(u8bFeed, tb, mode_name);
    u8bFeed1(tb, 0);
    a_rawc(ss, bsha);
    call(u8bFeed, tb, ss);
    a_dup(u8c, tc, u8bData(tb));
    call(KEEPPackFeed, k, p, DOG_OBJ_TREE, tc, tree_out);
    done;
}

ok64 WALKtest2() {
    sane(1);
    call(FILEInit);

    char tmp[] = "/tmp/walktest-XXXXXX";
    want(mkdtemp(tmp) != NULL);
    u8cs home = {(u8cp)tmp, (u8cp)tmp + strlen(tmp)};

    keeper k = {};
    call(KEEPOpen, &k, home, YES);
    keep_pack p = {};
    call(KEEPPackOpen, &k, &p);

    // Leaf blobs.
    a_cstr(hi_content, "hi\n");
    sha1 hi_sha = {};
    call(KEEPPackFeed, &k, &p, DOG_OBJ_BLOB, hi_content, &hi_sha);

    a_cstr(run_content, "#!/bin/sh\n");
    sha1 run_sha = {};
    call(KEEPPackFeed, &k, &p, DOG_OBJ_BLOB, run_content, &run_sha);

    // Inner "sub" tree.
    a_cstr(nested_mn, "100644 nested.txt");
    a_cstr(nested_content, "deep\n");
    sha1 sub_sha = {};
    call(build_leaf_tree, &k, &p, nested_mn, nested_content, &sub_sha);

    // Root tree — git sort order: hello.txt < run.sh < sub.
    a_pad(u8, rtb, 512);
    a_cstr(e1, "100644 hello.txt");
    call(u8bFeed, rtb, e1);
    u8bFeed1(rtb, 0);
    a_rawc(hi_ss, hi_sha);
    call(u8bFeed, rtb, hi_ss);

    a_cstr(e2, "100755 run.sh");
    call(u8bFeed, rtb, e2);
    u8bFeed1(rtb, 0);
    a_rawc(run_ss, run_sha);
    call(u8bFeed, rtb, run_ss);

    a_cstr(e3, "40000 sub");
    call(u8bFeed, rtb, e3);
    u8bFeed1(rtb, 0);
    a_rawc(sub_ss, sub_sha);
    call(u8bFeed, rtb, sub_ss);

    a_dup(u8c, rtc, u8bData(rtb));
    sha1 root_sha = {};
    call(KEEPPackFeed, &k, &p, DOG_OBJ_TREE, rtc, &root_sha);

    call(KEEPPackClose, &k, &p);

    // Eager walk.  One root DIR visit, three files, one sub DIR.
    {
        w2_ctx c = {};
        call(WALKTree, &k, root_sha.data, w2_visit, &c);
        want(c.n_entries == 5);
        want(c.n_files == 3);
        want(c.n_dirs == 2);
        want(c.n_files_with_blob == 3);  // eager: all files carry blob
    }

    // Lazy walk — blobs empty.
    {
        w2_ctx c = {};
        call(WALKTreeLazy, &k, root_sha.data, w2_visit, &c);
        want(c.n_entries == 5);
        want(c.n_files == 3);
        want(c.n_dirs == 2);
        want(c.n_files_with_blob == 0);
    }

    // Skip "sub" subtree — nested.txt must not be visited.
    {
        w2_ctx c = {};
        strcpy(c.dir_to_skip, "sub");
        call(WALKTreeLazy, &k, root_sha.data, w2_visit, &c);
        want(c.n_entries == 4);   // root, hello.txt, run.sh, sub (skipped)
        want(c.n_files == 2);
        want(c.n_dirs == 2);
    }

    call(KEEPClose, &k);

    {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", tmp);
        system(cmd);
    }

    done;
}

ok64 maintest() {
    sane(1);
    call(WALKtest1);
    call(WALKtest2);
    done;
}

TEST(maintest)
