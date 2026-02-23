// Test Y merger lookup - find elements that exist in only one input
#include "rdx/RDX.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

// Test: look up entries by name in Y merger with 2 inputs
// Input A has: alice, bob, charlie
// Input B has: bob, charlie, david
// Should find: alice (A only), bob (both), charlie (both), david (B only)

ok64 YLookupTestJDR() {
    sane(1);

    // Create two JDR inputs with overlapping entries
    // Format: E-tree with string entries
    u8cs jdr_a = u8csOf("{\"alice\":1,\"bob\":2,\"charlie\":3}");
    u8cs jdr_b = u8csOf("{\"bob\":20,\"charlie\":30,\"david\":40}");

    // Set up input iterators
    a_pad(rdx, inputs, 4);
    rdxbZero(inputs);

    rdxp a = NULL;
    call(rdxbFedP, inputs, &a);
    a->format = RDX_FMT_JDR;
    a->next = (u8p)jdr_a[0];
    a->opt = (u8p)jdr_a[1];

    rdxp b = NULL;
    call(rdxbFedP, inputs, &b);
    b->format = RDX_FMT_JDR;
    b->next = (u8p)jdr_b[0];
    b->opt = (u8p)jdr_b[1];

    // Create Y merger
    rdx y = {.format = RDX_FMT_Y, .ptype = RDX_TYPE_ROOT};
    rdxgMv(y.ins, rdxbDataIdle(inputs));

    // Read root element
    ok64 o = rdxNext(&y);
    test(o == OK, RDXBAD);
    test(y.type == RDX_TYPE_EULER, RDXBAD);  // E-tree root

    // Test 1: Look up "alice" (only in A)
    rdx seek_alice = {.type = RDX_TYPE_STRING};
    seek_alice.s[0] = (u8cp)"alice";
    seek_alice.s[1] = (u8cp)"alice" + 5;
    o = rdxInto(&seek_alice, &y);
    if (o == NONE) {
        fprintf(stderr, "JDR FAIL: alice not found (expected: found in A)\n");
        fail(RDXBAD);
    }
    test(o == OK, RDXBAD);
    fprintf(stderr, "JDR OK: alice found\n");

    // Test 2: Look up "bob" (in both)
    rdx seek_bob = {.type = RDX_TYPE_STRING};
    seek_bob.s[0] = (u8cp)"bob";
    seek_bob.s[1] = (u8cp)"bob" + 3;
    o = rdxInto(&seek_bob, &y);
    if (o == NONE) {
        fprintf(stderr, "JDR FAIL: bob not found (expected: found in both)\n");
        fail(RDXBAD);
    }
    test(o == OK, RDXBAD);
    fprintf(stderr, "JDR OK: bob found\n");

    // Test 3: Look up "david" (only in B)
    rdx seek_david = {.type = RDX_TYPE_STRING};
    seek_david.s[0] = (u8cp)"david";
    seek_david.s[1] = (u8cp)"david" + 5;
    o = rdxInto(&seek_david, &y);
    if (o == NONE) {
        fprintf(stderr, "JDR FAIL: david not found (expected: found in B)\n");
        fail(RDXBAD);
    }
    test(o == OK, RDXBAD);
    fprintf(stderr, "JDR OK: david found\n");

    // Test 4: Look up "zebra" (not in either)
    rdx seek_zebra = {.type = RDX_TYPE_STRING};
    seek_zebra.s[0] = (u8cp)"zebra";
    seek_zebra.s[1] = (u8cp)"zebra" + 5;
    o = rdxInto(&seek_zebra, &y);
    test(o == NONE, RDXBAD);  // Should NOT be found
    fprintf(stderr, "JDR OK: zebra not found (expected)\n");

    done;
}

// Helper: write JDR to SKIL
ok64 jdr_to_skil(u8b out, u64bp skipbuf, u8cs jdr) {
    sane(u8bOK(out) && skipbuf && $ok(jdr));

    // Parse JDR
    rdx reader = {.format = RDX_FMT_JDR, .ptype = RDX_TYPE_ROOT};
    reader.next = *jdr;
    reader.opt = (u8p)jdr[1];

    // Write to SKIL
    rdx writer;
    rdxInit(&writer, RDX_FMT_SKIL | RDX_FMT_WRITE, out);
    writer.opt = (u8p)skipbuf;

    call(rdxCopy, &writer, &reader);

    done;
}

ok64 YLookupTestSKIL() {
    sane(1);

    // Create two JDR inputs and convert to SKIL
    u8cs jdr_a = u8csOf("{\"alice\":1,\"bob\":2,\"charlie\":3}");
    u8cs jdr_b = u8csOf("{\"bob\":20,\"charlie\":30,\"david\":40}");

    // Convert to SKIL format
    a_pad(u8, skil_a_buf, 4 * KB);
    a_pad0(u64, skip_a, 256);
    call(jdr_to_skil, skil_a_buf, skip_a, jdr_a);

    a_pad(u8, skil_b_buf, 4 * KB);
    a_pad0(u64, skip_b, 256);
    call(jdr_to_skil, skil_b_buf, skip_b, jdr_b);

    // Set up input iterators (SKIL readers)
    a_pad(rdx, inputs, 4);
    rdxbZero(inputs);

    rdxp a = NULL;
    call(rdxbFedP, inputs, &a);
    rdxInit(a, RDX_FMT_SKIL, skil_a_buf);

    rdxp b = NULL;
    call(rdxbFedP, inputs, &b);
    rdxInit(b, RDX_FMT_SKIL, skil_b_buf);

    // Create Y merger
    rdx y = {.format = RDX_FMT_Y, .ptype = RDX_TYPE_ROOT};
    rdxgMv(y.ins, rdxbDataIdle(inputs));

    // Read root element
    ok64 o = rdxNext(&y);
    test(o == OK, RDXBAD);
    test(y.type == RDX_TYPE_EULER, RDXBAD);  // E-tree root

    // Test 1: Look up "alice" (only in A)
    rdx seek_alice = {.type = RDX_TYPE_STRING};
    seek_alice.s[0] = (u8cp)"alice";
    seek_alice.s[1] = (u8cp)"alice" + 5;
    o = rdxInto(&seek_alice, &y);
    if (o == NONE) {
        fprintf(stderr, "SKIL FAIL: alice not found (expected: found in A)\n");
        fail(RDXBAD);
    }
    test(o == OK, RDXBAD);
    fprintf(stderr, "SKIL OK: alice found\n");

    // Test 2: Look up "bob" (in both)
    rdx seek_bob = {.type = RDX_TYPE_STRING};
    seek_bob.s[0] = (u8cp)"bob";
    seek_bob.s[1] = (u8cp)"bob" + 3;
    o = rdxInto(&seek_bob, &y);
    if (o == NONE) {
        fprintf(stderr, "SKIL FAIL: bob not found (expected: found in both)\n");
        fail(RDXBAD);
    }
    test(o == OK, RDXBAD);
    fprintf(stderr, "SKIL OK: bob found\n");

    // Test 3: Look up "david" (only in B)
    rdx seek_david = {.type = RDX_TYPE_STRING};
    seek_david.s[0] = (u8cp)"david";
    seek_david.s[1] = (u8cp)"david" + 5;
    o = rdxInto(&seek_david, &y);
    if (o == NONE) {
        fprintf(stderr, "SKIL FAIL: david not found (expected: found in B)\n");
        fail(RDXBAD);
    }
    test(o == OK, RDXBAD);
    fprintf(stderr, "SKIL OK: david found\n");

    // Test 4: Look up "zebra" (not in either)
    rdx seek_zebra = {.type = RDX_TYPE_STRING};
    seek_zebra.s[0] = (u8cp)"zebra";
    seek_zebra.s[1] = (u8cp)"zebra" + 5;
    o = rdxInto(&seek_zebra, &y);
    test(o == NONE, RDXBAD);  // Should NOT be found
    fprintf(stderr, "SKIL OK: zebra not found (expected)\n");

    done;
}

// Test nested seek in Y merger with SKIL
// Structure: {dir_a: {file1: 1}, dir_b: {file2: 2}}
// Input A has: dir_a with file1
// Input B has: dir_b with file2
// Seek dir_b (only in B), then iterate its children
ok64 YLookupTestNestedSKIL() {
    sane(1);

    // Input A: {dir_a: {file1: 1}}
    // Input B: {dir_b: {file2: 2}}
    u8cs jdr_a = u8csOf("{\"dir_a\":{\"file1\":1}}");
    u8cs jdr_b = u8csOf("{\"dir_b\":{\"file2\":2}}");

    // Convert to SKIL
    a_pad(u8, skil_a_buf, 4 * KB);
    a_pad0(u64, skip_a, 256);
    call(jdr_to_skil, skil_a_buf, skip_a, jdr_a);

    a_pad(u8, skil_b_buf, 4 * KB);
    a_pad0(u64, skip_b, 256);
    call(jdr_to_skil, skil_b_buf, skip_b, jdr_b);

    // Set up SKIL readers (need more slots for nested Y operations)
    a_pad(rdx, inputs, 32);
    rdxbZero(inputs);

    rdxp a = NULL;
    call(rdxbFedP, inputs, &a);
    rdxInit(a, RDX_FMT_SKIL, skil_a_buf);

    rdxp b = NULL;
    call(rdxbFedP, inputs, &b);
    rdxInit(b, RDX_FMT_SKIL, skil_b_buf);

    // Create Y merger
    rdx y = {.format = RDX_FMT_Y, .ptype = RDX_TYPE_ROOT};
    rdxgMv(y.ins, rdxbDataIdle(inputs));

    // Read root
    ok64 o = rdxNext(&y);
    test(o == OK, RDXBAD);
    test(y.type == RDX_TYPE_EULER, RDXBAD);
    fprintf(stderr, "NESTED: root read OK\n");

    // Seek "dir_b" (only in B)
    rdx seek_dir_b = {.type = RDX_TYPE_STRING};
    seek_dir_b.s[0] = (u8cp)"dir_b";
    seek_dir_b.s[1] = (u8cp)"dir_b" + 5;
    o = rdxInto(&seek_dir_b, &y);
    if (o == NONE) {
        fprintf(stderr, "NESTED FAIL: dir_b not found\n");
        fail(RDXBAD);
    }
    test(o == OK, RDXBAD);
    fprintf(stderr, "NESTED: dir_b found, type=%d\n", seek_dir_b.type);

    // After seek, we're ON the tuple entry (key, value)
    test(seek_dir_b.type == RDX_TYPE_TUPLE, RDXBAD);
    fprintf(stderr, "NESTED: dir_b is TUPLE (key-value pair)\n");

    // Enter tuple to get to value
    rdx tup_child = {};
    o = rdxInto(&tup_child, &seek_dir_b);
    test(o == OK, RDXBAD);

    // Element 0 is the key (skip it)
    o = rdxNext(&tup_child);
    test(o == OK, RDXBAD);
    fprintf(stderr, "NESTED: key type=%d\n", tup_child.type);

    // Element 1 is the value (should be EULER {file2:2})
    o = rdxNext(&tup_child);
    test(o == OK, RDXBAD);
    test(tup_child.type == RDX_TYPE_EULER, RDXBAD);
    fprintf(stderr, "NESTED: value is EULER (nested object)\n");

    // Now iterate children of dir_b's value
    rdx child = {};
    o = rdxInto(&child, &tup_child);
    test(o == OK, RDXBAD);
    fprintf(stderr, "NESTED: entered dir_b value children\n");

    // Should find file2 entry (another tuple)
    o = rdxNext(&child);
    if (o != OK) {
        fprintf(stderr, "NESTED FAIL: no children in dir_b value, o=%s\n", ok64str(o));
        fail(RDXBAD);
    }
    test(child.type == RDX_TYPE_TUPLE, RDXBAD);
    fprintf(stderr, "NESTED: found child tuple\n");

    // Enter file2 tuple
    rdx file2_child = {};
    o = rdxInto(&file2_child, &child);
    test(o == OK, RDXBAD);

    // Skip key
    o = rdxNext(&file2_child);
    test(o == OK, RDXBAD);

    // Get value
    o = rdxNext(&file2_child);
    test(o == OK, RDXBAD);
    test(file2_child.type == RDX_TYPE_INT, RDXBAD);
    fprintf(stderr, "NESTED: file2 value=%ld\n", file2_child.i);
    test(file2_child.i == 2, RDXBAD);

    fprintf(stderr, "NESTED OK: dir_b iteration complete\n");

    done;
}

// Test: same root dir, different files (like repo scenario)
// Input A: {root: {file_a: 1, shared: 2}}
// Input B: {root: {file_b: 3, shared: 4}}
// After merging: root should have file_a, file_b, shared
ok64 YLookupTestRepoScenario() {
    sane(1);

    u8cs jdr_a = u8csOf("{\"root\":{\"file_a\":1,\"shared\":2}}");
    u8cs jdr_b = u8csOf("{\"root\":{\"file_b\":3,\"shared\":4}}");

    // Convert to SKIL
    a_pad(u8, skil_a_buf, 4 * KB);
    a_pad0(u64, skip_a, 256);
    call(jdr_to_skil, skil_a_buf, skip_a, jdr_a);

    a_pad(u8, skil_b_buf, 4 * KB);
    a_pad0(u64, skip_b, 256);
    call(jdr_to_skil, skil_b_buf, skip_b, jdr_b);

    // Set up Y merger
    a_pad(rdx, inputs, 32);
    rdxbZero(inputs);

    rdxp a = NULL;
    call(rdxbFedP, inputs, &a);
    rdxInit(a, RDX_FMT_SKIL, skil_a_buf);

    rdxp b = NULL;
    call(rdxbFedP, inputs, &b);
    rdxInit(b, RDX_FMT_SKIL, skil_b_buf);

    rdx y = {.format = RDX_FMT_Y, .ptype = RDX_TYPE_ROOT};
    rdxgMv(y.ins, rdxbDataIdle(inputs));

    // Read root EULER
    ok64 o = rdxNext(&y);
    test(o == OK, RDXBAD);
    fprintf(stderr, "REPO: root EULER read\n");

    // Seek "root" (exists in both)
    rdx seek_root = {.type = RDX_TYPE_STRING};
    seek_root.s[0] = (u8cp)"root";
    seek_root.s[1] = (u8cp)"root" + 4;
    o = rdxInto(&seek_root, &y);
    test(o == OK, RDXBAD);
    test(seek_root.type == RDX_TYPE_TUPLE, RDXBAD);
    fprintf(stderr, "REPO: 'root' found\n");

    // Enter root tuple, skip key, get value (the inner EULER)
    rdx root_tup = {};
    o = rdxInto(&root_tup, &seek_root);
    test(o == OK, RDXBAD);
    o = rdxNext(&root_tup);  // skip key
    test(o == OK, RDXBAD);
    o = rdxNext(&root_tup);  // get value
    test(o == OK, RDXBAD);
    test(root_tup.type == RDX_TYPE_EULER, RDXBAD);
    fprintf(stderr, "REPO: root value is EULER\n");

    // Now seek file_b (only in B) inside root
    rdx seek_file_b = {.type = RDX_TYPE_STRING};
    seek_file_b.s[0] = (u8cp)"file_b";
    seek_file_b.s[1] = (u8cp)"file_b" + 6;
    o = rdxInto(&seek_file_b, &root_tup);
    if (o == NONE) {
        fprintf(stderr, "REPO FAIL: file_b not found in root\n");
        fail(RDXBAD);
    }
    test(o == OK, RDXBAD);
    fprintf(stderr, "REPO: file_b found in root\n");

    // Get file_b's value
    rdx file_b_tup = {};
    o = rdxInto(&file_b_tup, &seek_file_b);
    test(o == OK, RDXBAD);
    o = rdxNext(&file_b_tup);  // skip key
    test(o == OK, RDXBAD);
    o = rdxNext(&file_b_tup);  // get value
    test(o == OK, RDXBAD);
    test(file_b_tup.type == RDX_TYPE_INT, RDXBAD);
    test(file_b_tup.i == 3, RDXBAD);
    fprintf(stderr, "REPO OK: file_b value = %ld\n", file_b_tup.i);

    done;
}

// Test: seek then iterate siblings (like REPOLocateByPath does)
// After seeking "root", can we find ALL entries inside it by iterating?
ok64 YLookupTestSeekThenIterate() {
    sane(1);

    u8cs jdr_a = u8csOf("{\"root\":{\"alice\":1,\"shared\":2}}");
    u8cs jdr_b = u8csOf("{\"root\":{\"bob\":3,\"shared\":4}}");

    // Convert to SKIL
    a_pad(u8, skil_a_buf, 4 * KB);
    a_pad0(u64, skip_a, 256);
    call(jdr_to_skil, skil_a_buf, skip_a, jdr_a);

    a_pad(u8, skil_b_buf, 4 * KB);
    a_pad0(u64, skip_b, 256);
    call(jdr_to_skil, skil_b_buf, skip_b, jdr_b);

    // Set up Y merger
    a_pad(rdx, inputs, 32);
    rdxbZero(inputs);

    rdxp a = NULL;
    call(rdxbFedP, inputs, &a);
    rdxInit(a, RDX_FMT_SKIL, skil_a_buf);

    rdxp b = NULL;
    call(rdxbFedP, inputs, &b);
    rdxInit(b, RDX_FMT_SKIL, skil_b_buf);

    rdx y = {.format = RDX_FMT_Y, .ptype = RDX_TYPE_ROOT};
    rdxgMv(y.ins, rdxbDataIdle(inputs));

    // Read root
    ok64 o = rdxNext(&y);
    test(o == OK, RDXBAD);

    // Seek "root"
    rdx seek_root = {.type = RDX_TYPE_STRING};
    seek_root.s[0] = (u8cp)"root";
    seek_root.s[1] = (u8cp)"root" + 4;
    o = rdxInto(&seek_root, &y);
    test(o == OK, RDXBAD);

    // Get root value (EULER containing alice, bob, shared)
    rdx root_tup = {};
    o = rdxInto(&root_tup, &seek_root);
    test(o == OK, RDXBAD);
    o = rdxNext(&root_tup);  // skip key
    test(o == OK, RDXBAD);
    o = rdxNext(&root_tup);  // get value
    test(o == OK, RDXBAD);
    test(root_tup.type == RDX_TYPE_EULER, RDXBAD);

    // Enter root value and iterate ALL children
    rdx children = {};
    o = rdxInto(&children, &root_tup);
    test(o == OK, RDXBAD);

    int count = 0;
    int found_alice = 0, found_bob = 0, found_shared = 0;
    while ((o = rdxNext(&children)) == OK) {
        count++;
        // Each child is a TUPLE (key, value)
        if (children.type != RDX_TYPE_TUPLE) continue;

        // Enter tuple to read key
        rdx kv = {};
        o = rdxInto(&kv, &children);
        if (o != OK) continue;
        o = rdxNext(&kv);  // read key
        if (o != OK || kv.type != RDX_TYPE_STRING) continue;

        if (u8csLen(kv.s) == 5 && memcmp(*kv.s, "alice", 5) == 0) found_alice++;
        if (u8csLen(kv.s) == 3 && memcmp(*kv.s, "bob", 3) == 0) found_bob++;
        if (u8csLen(kv.s) == 6 && memcmp(*kv.s, "shared", 6) == 0) found_shared++;
    }
    test(o == END, RDXBAD);

    fprintf(stderr, "ITER: count=%d alice=%d bob=%d shared=%d\n",
            count, found_alice, found_bob, found_shared);

    test(found_alice == 1, RDXBAD);
    test(found_bob == 1, RDXBAD);
    test(found_shared >= 1, RDXBAD);  // might be 2 if not merged
    fprintf(stderr, "ITER OK: all entries found\n");

    done;
}

ok64 YLookupTest() {
    sane(1);
    call(YLookupTestJDR);
    call(YLookupTestSKIL);
    call(YLookupTestNestedSKIL);
    call(YLookupTestRepoScenario);
    call(YLookupTestSeekThenIterate);
    done;
}

TEST(YLookupTest);
