#include "keeper/GIT.h"

#include <string.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

// ---- Test 1: tree parser, single entry ----

ok64 GITtest1() {
    sane(1);
    // tree entry: "100644 hello.c\0" + 20 bytes SHA1
    u8 raw[] = "100644 hello.c\0"
               "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"
               "\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14";
    u8cs obj = {raw, raw + sizeof(raw) - 1};  // exclude C string NUL
    u8cs file = {};
    u8cs sha1 = {};

    ok64 o = GITu8sDrainTree(obj, file, sha1);
    want(o == OK);
    want($len(file) == 14);  // "100644 hello.c"
    want(memcmp(file[0], "100644 hello.c", 14) == 0);
    want($len(sha1) == 20);
    want(*sha1[0] == 0x01);
    want(*(sha1[1] - 1) == 0x14);

    // should be exhausted
    want($empty(obj));
    o = GITu8sDrainTree(obj, file, sha1);
    want(o == NODATA);

    done;
}

// ---- Test 2: tree parser, two entries ----

ok64 GITtest2() {
    sane(1);
    u8 raw[] = "100644 a.c\0"
               "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"
               "\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14"
               "40000 subdir\0"
               "\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a"
               "\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34";
    u8cs obj = {raw, raw + sizeof(raw) - 1};
    u8cs file = {};
    u8cs sha1 = {};

    // first entry
    ok64 o = GITu8sDrainTree(obj, file, sha1);
    want(o == OK);
    want($len(file) == 10);  // "100644 a.c"
    want(*sha1[0] == 0x01);

    // second entry
    o = GITu8sDrainTree(obj, file, sha1);
    want(o == OK);
    want($len(file) == 12);  // "40000 subdir"
    want(*sha1[0] == 0x21);

    // exhausted
    o = GITu8sDrainTree(obj, file, sha1);
    want(o == NODATA);

    done;
}

// ---- Test 3: tree parser, truncated SHA1 ----

ok64 GITtest3() {
    sane(1);
    // only 5 bytes after NUL instead of 20
    u8 raw[] = "100644 x\0\x01\x02\x03\x04\x05";
    u8cs obj = {raw, raw + sizeof(raw) - 1};
    u8cs file = {};
    u8cs sha1 = {};

    ok64 o = GITu8sDrainTree(obj, file, sha1);
    want(o == GITBADFMT);

    done;
}

// ---- Test 4: commit parser, typical commit ----

ok64 GITtest4() {
    sane(1);
    con char commit[] =
        "tree abc123\n"
        "parent def456\n"
        "author Alice <a@b> 1234 +0000\n"
        "committer Bob <b@c> 5678 +0000\n"
        "\n"
        "Fix the frobnicator\n"
        "\n"
        "It was broken.\n";

    u8cs obj = {(u8cp)commit, (u8cp)commit + sizeof(commit) - 1};
    u8cs field = {};
    u8cs value = {};

    // tree
    ok64 o = GITu8sDrainCommit(obj, field, value);
    want(o == OK);
    want($len(field) == 4);
    want(memcmp(field[0], "tree", 4) == 0);
    want($len(value) == 6);
    want(memcmp(value[0], "abc123", 6) == 0);

    // parent
    o = GITu8sDrainCommit(obj, field, value);
    want(o == OK);
    want($len(field) == 6);
    want(memcmp(field[0], "parent", 6) == 0);
    want(memcmp(value[0], "def456", 6) == 0);

    // author
    o = GITu8sDrainCommit(obj, field, value);
    want(o == OK);
    want(memcmp(field[0], "author", 6) == 0);

    // committer
    o = GITu8sDrainCommit(obj, field, value);
    want(o == OK);
    want(memcmp(field[0], "committer", 9) == 0);

    // blank line -> body
    o = GITu8sDrainCommit(obj, field, value);
    want(o == OK);
    want($empty(field));  // empty field signals body
    want($len(value) > 0);
    want(memcmp(value[0], "Fix the frobnicator", 19) == 0);

    // exhausted
    o = GITu8sDrainCommit(obj, field, value);
    want(o == NODATA);

    done;
}

// ---- Test 5: commit parser, no body ----

ok64 GITtest5() {
    sane(1);
    con char commit[] =
        "tree aaa\n"
        "\n";

    u8cs obj = {(u8cp)commit, (u8cp)commit + sizeof(commit) - 1};
    u8cs field = {};
    u8cs value = {};

    // tree
    ok64 o = GITu8sDrainCommit(obj, field, value);
    want(o == OK);
    want(memcmp(field[0], "tree", 4) == 0);

    // blank line -> empty body
    o = GITu8sDrainCommit(obj, field, value);
    want(o == OK);
    want($empty(field));
    want($empty(value));

    // done
    o = GITu8sDrainCommit(obj, field, value);
    want(o == NODATA);

    done;
}

// ---- Test 6: commit parser, malformed (no space) ----

ok64 GITtest6() {
    sane(1);
    con char commit[] = "badline\n";
    u8cs obj = {(u8cp)commit, (u8cp)commit + sizeof(commit) - 1};
    u8cs field = {};
    u8cs value = {};

    ok64 o = GITu8sDrainCommit(obj, field, value);
    want(o == GITBADFMT);

    done;
}

ok64 maintest() {
    sane(1);
    call(GITtest1);
    call(GITtest2);
    call(GITtest3);
    call(GITtest4);
    call(GITtest5);
    call(GITtest6);
    done;
}

TEST(maintest)
