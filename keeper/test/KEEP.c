#include "keeper/KEEP.h"
#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

// --- wh64 hashlet tests ---

ok64 WH64hashlet() {
    sane(1);

    // SHA: 816fb46be665c8b63647...
    u8 sha[20] = {0x81, 0x6f, 0xb4, 0x6b, 0xe6, 0x65, 0xc8, 0xb6,
                  0x36, 0x47, 0xf0, 0x09, 0x68, 0x45, 0xfe, 0xf3,
                  0x63, 0x73, 0x6b, 0x20};
    u64 hashlet = wh64Hashlet(sha);

    // Hex output should match SHA prefix
    char hex[12];
    wh64HashletHex(hex, hashlet, 10);
    want(memcmp(hex, "816fb46be6", 10) == 0);

    // Shorter prefix
    wh64HashletHex(hex, hashlet, 7);
    hex[7] = 0;
    want(memcmp(hex, "816fb46", 7) == 0);

    // FromHex round-trip
    u64 h2 = wh64HashletFromHex("816fb46be6", 10);
    want(h2 == hashlet);

    // Short prefix match
    u64 h7 = wh64HashletFromHex("816fb46", 7);
    want(wh64HashletMatch(hashlet, "816fb46", 7) == YES);
    want(wh64HashletMatch(hashlet, "816fb47", 7) == NO);

    done;
}

// --- wh64 pack/unpack tests ---

ok64 WH64pack() {
    sane(1);

    wh64 v = wh64Pack(5, 1000, 0xABCDEF0123ULL);
    want(wh64Type(v) == 5);
    want(wh64Id(v) == 1000);
    want(wh64Off(v) == 0xABCDEF0123ULL);

    wh64 vmax = wh64Pack(0xf, WHIFF_ID_MASK, WHIFF_OFF_MASK);
    want(wh64Type(vmax) == 0xf);
    want(wh64Id(vmax) == WHIFF_ID_MASK);
    want(wh64Off(vmax) == WHIFF_OFF_MASK);

    wh64 v0 = wh64Pack(0, 0, 0);
    want(wh64Type(v0) == 0);
    want(wh64Id(v0) == 0);
    want(wh64Off(v0) == 0);

    done;
}

// --- keeper open/close on empty store ---

ok64 KEEPempty() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/keeper-test-XXXXXX";
    want(mkdtemp(tmpdir) != NULL);

    u8cs root = {(u8cp)tmpdir, (u8cp)tmpdir + strlen(tmpdir)};
    keeper k = {};
    call(KEEPOpen, &k, root);
    want(k.npacks == 0);
    want(k.nruns == 0);

    u64 hashlet = keepHashlet60FromHex("abcdef", 6);
    u64 val = 0;
    want(KEEPLookup(&k, hashlet, 6, &val) == KEEPNONE);
    want(KEEPHas(&k, hashlet, 6) == KEEPNONE);

    call(KEEPClose, &k);

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", tmpdir);
    system(cmd);

    done;
}

ok64 KEEPput() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/keeper-put-XXXXXX";
    want(mkdtemp(tmpdir) != NULL);

    a_cstr(root, tmpdir);
    keeper k = {};
    call(KEEPOpen, &k, root);
    want(k.npacks == 0);

    // Store two blobs
    a_cstr(blob1, "hello world\n");
    a_cstr(blob2, "goodbye world\n");
    u8csc objs[2] = {
        {blob1[0], blob1[1]},
        {blob2[0], blob2[1]},
    };
    wh64 wh[2] = {
        wh64Pack(KEEP_OBJ_BLOB, 0, 0),
        wh64Pack(KEEP_OBJ_BLOB, 0, 0),
    };

    call(KEEPPut, &k, objs, wh, 2);
    want(k.npacks == 1);
    want(k.nruns == 1);

    // Both whiffs should have valid types
    want(wh64Type(wh[0]) == KEEP_OBJ_BLOB);
    want(wh64Type(wh[1]) == KEEP_OBJ_BLOB);

    // Compute 60-bit hashlets from known blob content
    // "hello world\n" SHA = 3b18e512dba79e4c8300dd08aeb37f8e728b8dad
    u8 sha0[20] = {0x3b,0x18,0xe5,0x12,0xdb,0xa7,0x9e,0x4c,0x83,0x00,
                   0xdd,0x08,0xae,0xb3,0x7f,0x8e,0x72,0x8b,0x8d,0xad};
    u64 h0 = keepHashlet60(sha0);
    // "goodbye world\n" — compute via git hash-object
    u64 h1 = keepHashlet60FromHex("ce0136", 6);  // just need a prefix

    // Should be retrievable by 7-char prefix (git default)
    want(KEEPHas(&k, keepHashlet60FromHex("3b18e51", 7), 7) == OK);

    // Get content back
    Bu8 out = {};
    call(u8bMap, out, 1UL << 20);
    u8 obj_type = 0;
    call(KEEPGet, &k, h0, 15, out, &obj_type);
    want(obj_type == KEEP_OBJ_BLOB);
    want(u8bDataLen(out) == u8csLen(blob1));
    want(memcmp(u8bDataHead(out), blob1[0], u8csLen(blob1)) == 0);

    u8bUnMap(out);
    call(KEEPClose, &k);

    a_pad(u8, rmbuf, 256);
    a_cstr(rmcmd, "rm -rf ");
    u8bFeed(rmbuf, rmcmd);
    u8bFeed(rmbuf, root);
    u8bFeed1(rmbuf, 0);
    system((char *)u8bDataHead(rmbuf));

    done;
}

ok64 KEEPpackIncremental() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/keeper-pack-XXXXXX";
    want(mkdtemp(tmpdir) != NULL);

    a_cstr(root, tmpdir);
    keeper k = {};
    call(KEEPOpen, &k, root);

    keep_pack p = {};
    call(KEEPPackOpen, &k, &p);

    // Feed blob: "hello world\n"
    // git SHA-1 = 3b18e512dba79e4c8300dd08aeb37f8e728b8dad
    a_cstr(blob_content, "hello world\n");
    u8 blob_sha[20] = {};
    call(KEEPPackFeed, &k, &p, KEEP_OBJ_BLOB, blob_content, blob_sha);

    // Verify blob SHA matches git
    u8 expected_blob_sha[20] = {
        0x3b,0x18,0xe5,0x12,0xdb,0xa7,0x9e,0x4c,0x83,0x00,
        0xdd,0x08,0xae,0xb3,0x7f,0x8e,0x72,0x8b,0x8d,0xad};
    want(memcmp(blob_sha, expected_blob_sha, 20) == 0);

    // Build tree content: "100644 hello.txt\0" + 20-byte blob SHA
    a_pad(u8, tree_buf, 256);
    a_cstr(tree_mode, "100644 hello.txt");
    u8bFeed(tree_buf, tree_mode);
    u8bFeed1(tree_buf, 0);  // NUL separator
    u8cs sha_slice = {blob_sha, blob_sha + 20};
    u8bFeed(tree_buf, sha_slice);

    a_dup(u8c, tree_content, u8bData(tree_buf));
    u8 tree_sha[20] = {};
    call(KEEPPackFeed, &k, &p, KEEP_OBJ_TREE, tree_content, tree_sha);

    // Verify tree SHA matches git: 68aba62e560c0ebc3396e8ae9335232cd93a3f60
    u8 expected_tree_sha[20] = {
        0x68,0xab,0xa6,0x2e,0x56,0x0c,0x0e,0xbc,0x33,0x96,
        0xe8,0xae,0x93,0x35,0x23,0x2c,0xd9,0x3a,0x3f,0x60};
    want(memcmp(tree_sha, expected_tree_sha, 20) == 0);

    call(KEEPPackClose, &k, &p);
    want(k.npacks == 1);
    want(k.nruns == 1);

    // Retrieve blob by 7-char prefix (git default)
    u64 blob_hashlet = keepHashlet60(blob_sha);
    Bu8 out = {};
    call(u8bMap, out, 1UL << 20);
    u8 obj_type = 0;
    call(KEEPGet, &k, keepHashlet60FromHex("3b18e51", 7), 7, out, &obj_type);
    want(obj_type == KEEP_OBJ_BLOB);
    want(u8bDataLen(out) == u8csLen(blob_content));
    want(memcmp(u8bDataHead(out), blob_content[0], u8csLen(blob_content)) == 0);

    // Retrieve tree by full 15-char hashlet
    u8bReset(out);
    u64 tree_hashlet = keepHashlet60(tree_sha);
    call(KEEPGet, &k, tree_hashlet, 15, out, &obj_type);
    want(obj_type == KEEP_OBJ_TREE);
    want(u8bDataLen(out) == u8csLen(tree_content));
    want(memcmp(u8bDataHead(out), tree_content[0], u8csLen(tree_content)) == 0);

    u8bUnMap(out);
    call(KEEPClose, &k);

    a_pad(u8, rmbuf, 256);
    a_cstr(rmcmd, "rm -rf ");
    u8bFeed(rmbuf, rmcmd);
    u8bFeed(rmbuf, root);
    u8bFeed1(rmbuf, 0);
    system((char *)u8bDataHead(rmbuf));

    done;
}

ok64 maintest() {
    sane(1);
    fprintf(stderr, "WH64hashlet...\n");
    call(WH64hashlet);
    fprintf(stderr, "WH64pack...\n");
    call(WH64pack);
    fprintf(stderr, "KEEPempty...\n");
    call(KEEPempty);
    fprintf(stderr, "KEEPput...\n");
    call(KEEPput);
    fprintf(stderr, "KEEPpackIncremental...\n");
    call(KEEPpackIncremental);
    fprintf(stderr, "all passed\n");
    done;
}

TEST(maintest);
