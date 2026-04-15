//  WALK tests: belt128 gen encoding + graph walk integration
//
#include "keeper/BELT.h"
#include "keeper/WALK.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

// ---- Test 1: belt128 gen round-trip ----

ok64 WALKtest1() {
    sane(1);

    // basic encoding
    belt128 e = BELTEntry(0xdeadbeef0, BELT_COMMIT, 12345, 42);
    want(BELTType(e) == BELT_COMMIT);
    want(BELTOffset(e) == 12345);
    want(BELTGen(e) == 42);
    want((BELTHashlet(e) & ~BELT_TYPE_MASK) == (0xdeadbeef0 & ~BELT_TYPE_MASK));

    // gen=0 for non-commit types
    belt128 b = BELTEntry(0x1234567890, BELT_BLOB, 999, 0);
    want(BELTType(b) == BELT_BLOB);
    want(BELTOffset(b) == 999);
    want(BELTGen(b) == 0);

    // max values
    u64 max_off = ((u64)1 << 40) - 1;
    u32 max_gen = ((u32)1 << 20) - 1;
    belt128 m = BELTEntry(~(u64)0, BELT_TAG, max_off, max_gen);
    want(BELTType(m) == BELT_TAG);
    want(BELTOffset(m) == max_off);
    want(BELTGen(m) == max_gen);

    // gen doesn't corrupt offset and vice versa
    belt128 x = BELTEntry(0, BELT_COMMIT, 0, 1);
    want(BELTOffset(x) == 0);
    want(BELTGen(x) == 1);

    belt128 y = BELTEntry(0, BELT_COMMIT, 1, 0);
    want(BELTOffset(y) == 1);
    want(BELTGen(y) == 0);

    done;
}

// ---- Test 2: WALKCommitTree ----

ok64 WALKtest2() {
    sane(1);
    con char commit[] =
        "tree 4b825dc642cb6eb9a060e54bf899d69f7af0d5f3\n"
        "parent aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
        "author A <a@b> 1 +0000\n"
        "\n"
        "msg\n";

    u8cs obj = {(u8cp)commit, (u8cp)commit + sizeof(commit) - 1};
    u8 tree_sha[20];
    ok64 o = WALKCommitTree(obj, tree_sha);
    want(o == OK);

    // verify first byte: 0x4b
    want(tree_sha[0] == 0x4b);
    want(tree_sha[1] == 0x82);
    want(tree_sha[2] == 0x5d);

    done;
}

// ---- Test 3: BELTCommitGen with no parents ----

ok64 WALKtest3() {
    sane(1);
    con char commit[] =
        "tree 4b825dc642cb6eb9a060e54bf899d69f7af0d5f3\n"
        "author A <a@b> 1 +0000\n"
        "\n"
        "root\n";

    // empty stack — no parents to look up
    belt128cs runs[1];
    memset(runs, 0, sizeof(runs));
    belt128css stack = {runs, runs};

    u32 gen = BELTCommitGen((u8cp)commit, sizeof(commit) - 1, stack);
    want(gen == 1);  // root commit

    done;
}

// ---- Test 4: belt128 sort order — gen in .b high bits ----

ok64 WALKtest4() {
    sane(1);

    // Two entries with same hashlet, different gen
    belt128 a = BELTEntry(0x100, BELT_COMMIT, 100, 5);
    belt128 b = BELTEntry(0x100, BELT_COMMIT, 200, 10);

    // higher gen sorts later (b.b > a.b because gen is in high bits)
    want(belt128cmp(&a, &b) < 0);

    // same hashlet + gen, different offset
    belt128 c = BELTEntry(0x100, BELT_COMMIT, 100, 5);
    belt128 d = BELTEntry(0x100, BELT_COMMIT, 200, 5);
    want(belt128cmp(&c, &d) < 0);

    done;
}

// ---- Test 5: integration — clone + walk (requires WITH_SSH) ----

static u32 commit_count;
static u32 max_gen_seen;

static ok64 count_commits(u64 hashlet, u8 type, u8cs content, void0p ctx) {
    (void)hashlet; (void)content; (void)ctx;
    if (type == BELT_COMMIT) {
        commit_count++;
    }
    return OK;
}

ok64 WALKtest5() {
    sane(1);

    char const *repo = getenv("WALK_REPO");
    if (!repo) repo = "/home/gritzko/src/treadmill/gits/repo";

    // create temp belt dir
    char belt_dir[] = "/tmp/walk-test-XXXXXX";
    want(mkdtemp(belt_dir) != NULL);

    u8cs repo_s = {(u8cp)repo, (u8cp)repo + strlen(repo)};
    u8cs belt_s = {(u8cp)belt_dir, (u8cp)belt_dir + strlen(belt_dir)};
    call(BELTClone, repo_s, belt_s);

    // read HEAD
    char head_path[256];
    snprintf(head_path, sizeof(head_path), "%s/HEAD", belt_dir);
    FILE *f = fopen(head_path, "r");
    want(f != NULL);
    char head_hex[41] = {};
    want(fgets(head_hex, sizeof(head_hex), f) != NULL);
    fclose(f);

    u8 head_sha[20];
    u8s hbin = {head_sha, head_sha + 20};
    u8cs hhex = {(u8cp)head_hex, (u8cp)head_hex + 40};
    call(HEXu8sDrainSome, hbin, hhex);

    // open walker
    walk w = {};
    call(WALKOpen, &w, belt_s);

    // test WALKGetSha
    {
        u8 objbuf[1 << 20];
        u8g out = {objbuf, objbuf, objbuf + sizeof(objbuf)};
        u8 otype = 0;
        call(WALKGetSha, &w, head_sha, out, &otype);
        want(otype == BELT_COMMIT);
        want(u8gLeftLen(out) > 0);
        fprintf(stderr, "  HEAD commit: %lu bytes\n",
                (unsigned long)u8gLeftLen(out));
    }

    // test WALKCommits — count all commits
    commit_count = 0;
    call(WALKCommits, &w, head_sha, NULL, count_commits, NULL);
    want(commit_count > 0);
    fprintf(stderr, "  commits walked: %u\n", commit_count);

    // test WALKCheckout
    {
        u8 objbuf[1 << 20];
        u8g out = {objbuf, objbuf, objbuf + sizeof(objbuf)};
        u8 otype = 0;
        call(WALKGetSha, &w, head_sha, out, &otype);

        u8 tree_sha[20];
        u8cs commit_s = {objbuf, out[1]};
        call(WALKCommitTree, commit_s, tree_sha);

        char co_dir[256];
        snprintf(co_dir, sizeof(co_dir), "%s/checkout", belt_dir);
        mkdir(co_dir, 0755);
        u8cs co_s = {(u8cp)co_dir, (u8cp)co_dir + strlen(co_dir)};
        call(WALKCheckout, &w, tree_sha, co_s);
        fprintf(stderr, "  checkout to %s\n", co_dir);
    }

    call(WALKClose, &w);

    // cleanup
    {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", belt_dir);
        system(cmd);
    }

    done;
}

ok64 maintest() {
    sane(1);
    call(WALKtest1);
    call(WALKtest2);
    call(WALKtest3);
    call(WALKtest4);
    done;
}

TEST(maintest)
