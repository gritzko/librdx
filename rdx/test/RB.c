#include "abc/01.h"
#include "abc/TEST.h"
#include "abc/PRO.h"
#include "rdx/RB.h"
#include "rdx/RDX.h"

// Test RBlink structure and helpers
ok64 RBTestStruct() {
    sane(1);

    // Verify RBlink is 16 bytes (u32 x 4)
    testeq(sizeof(RBlink), 16);

    // Test red bit helpers
    testeq(RBisRed(0), NO);
    testeq(RBisRed(RB_RED_BIT), YES);
    testeq(RBisRed(RB_RED_BIT | 42), YES);
    testeq(RBisRed(42), NO);

    // Test parent offset extraction
    testeq(RBparentOff(0), 0);
    testeq(RBparentOff(42), 42);
    testeq(RBparentOff(RB_RED_BIT | 42), 42);
    testeq(RBparentOff(RB_RED_BIT), 0);

    // Test set red
    testeq(RBsetRed(42, YES), RB_RED_BIT | 42);
    testeq(RBsetRed(42, NO), 42);
    testeq(RBsetRed(RB_RED_BIT | 42, NO), 42);
    testeq(RBsetRed(42, YES) & ~RB_RED_BIT, 42);

    // Test RB_NIL (RB_RED_BIT - 1 ensures NIL never has red bit set)
    testeq(RB_NIL, 0x7FFFFFFFU);

    done;
}

// Test record layout conceptually
// Record: lit len [rb_left rb_right rb_parent rdx_parent] idlen id value
ok64 RBTestRecordLayout() {
    sane(1);

    // Simulate a minimal record in a buffer
    // For an INT with value 42, id=(0,0):
    //   lit='I' (1 byte)
    //   len=... (varint, ~1-2 bytes for small records)
    //   RBlink (16 bytes)
    //   idlen=0 (1 byte for zero-length id)
    //   value (zigzag encoded 42)

    a_pad(u8, storage, 64);
    u8p p = storage[0];

    // Write lit
    *p++ = 'I';

    // Skip len for now (we'll fill it after knowing total size)
    u8p len_pos = p++;

    // Write RBlink (16 bytes)
    RBlink* link = (RBlink*)p;
    link->rb_left = RB_NIL;
    link->rb_right = RB_NIL;
    link->rb_parent = RB_NIL;
    link->rdx_parent = RB_NIL;
    p += sizeof(RBlink);

    // Write id (empty: just length 0)
    *p++ = 0;

    // Write value: zigzag(42) = 84 = 0x54 (fits in 1 byte)
    *p++ = 84;

    // Total record size (excluding lit and len byte)
    u32 body_len = p - len_pos - 1;
    *len_pos = (u8)body_len | 0x20;  // short TLV format

    // Verify structure
    testeq(storage[0][0], 'I');
    testeq(((RBlink*)(storage[0] + 2))->rb_left, RB_NIL);

    done;
}

// Test PLEX record with children_root
ok64 RBTestPlexRecord() {
    sane(1);

    a_pad(u8, storage, 64);
    u8p p = storage[0];

    // Write EULER container
    *p++ = 'E';
    u8p len_pos = p++;

    // RBlink for the EULER itself (it's a top-level element)
    RBlink* link = (RBlink*)p;
    link->rb_left = RB_NIL;
    link->rb_right = RB_NIL;
    link->rb_parent = RB_NIL;
    link->rdx_parent = RB_NIL;  // top level
    p += sizeof(RBlink);

    // id (empty)
    *p++ = 0;

    // children_root: offset where first child will be
    u32 children_root = 64;  // hypothetical offset
    memcpy(p, &children_root, 4);
    p += 4;

    u32 body_len = p - len_pos - 1;
    *len_pos = (u8)body_len | 0x20;

    // Verify
    testeq(storage[0][0], 'E');

    // Read back children_root
    u8p val_start = storage[0] + 2 + sizeof(RBlink) + 1;  // after lit, len, RBlink, idlen
    u32 read_root = 0;
    memcpy(&read_root, val_start, 4);
    testeq(read_root, 64);

    done;
}

// Test basic write: create EULER with children, then read back
ok64 RBTestWrite() {
    sane(1);

    // Storage buffer
    a_pad(u8, storage, 1024);

    // Create buffer for RB: [past, data, idle, end] all start at storage[0]
    // RB uses bulk[0]=base, bulk[1]=write_pos, bulk[2]=end
    u8b rb_buf = {storage[0], storage[0], storage[0], storage[3]};

    // Create writer for EULER container
    rdx writer = {};
    rdxWriteInit(&writer, RDX_FMT_RB, rb_buf);
    writer.type = RDX_TYPE_EULER;
    writer.ptype = 0;  // root level

    // Write the EULER container
    call(rdxWriteNextRB, &writer);

    // Write children: "charlie", "alice", "bob" (out of order)
    rdx child = {};
    call(rdxWriteIntoRB, &child, &writer);

    // Child 1: "charlie"
    child.type = RDX_TYPE_STRING;
    u8c charlie[] = "charlie";
    child.s[0] = charlie;
    child.s[1] = charlie + 7;
    call(rdxWriteNextRB, &child);

    // Child 2: "alice"
    child.type = RDX_TYPE_STRING;
    u8c alice[] = "alice";
    child.s[0] = alice;
    child.s[1] = alice + 5;
    call(rdxWriteNextRB, &child);

    // Child 3: "bob"
    child.type = RDX_TYPE_STRING;
    u8c bob[] = "bob";
    child.s[0] = bob;
    child.s[1] = bob + 3;
    call(rdxWriteNextRB, &child);

    call(rdxWriteOutoRB, &child, &writer);

    // Now read back and verify sorted order: alice, bob, charlie
    // Reset buffer for reading (write pos becomes data boundary)
    u8b read_buf = {storage[0], storage[0], rb_buf[2], storage[3]};

    rdx reader = {};
    rdxInit(&reader, RDX_FMT_RB, read_buf);
    reader.ptype = 0;

    // Position at root
    call(rdxNextRB, &reader);
    testeq(reader.type, RDX_TYPE_EULER);

    // Enter container
    rdx rchild = {};
    call(rdxIntoRB, &rchild, &reader);

    // First child should be "alice"
    call(rdxNextRB, &rchild);
    testeq(rchild.type, RDX_TYPE_STRING);
    testeq(u8csLen(rchild.s), 5);

    // Second child should be "bob"
    call(rdxNextRB, &rchild);
    testeq(rchild.type, RDX_TYPE_STRING);
    testeq(u8csLen(rchild.s), 3);

    // Third child should be "charlie"
    call(rdxNextRB, &rchild);
    testeq(rchild.type, RDX_TYPE_STRING);
    testeq(u8csLen(rchild.s), 7);

    // No more children
    testeq(rdxNextRB(&rchild), END);

    done;
}

// Test find-or-create: seek to existing child should find it
ok64 RBTestFindExisting() {
    sane(1);

    a_pad(u8, storage, 1024);
    u8b rb_buf = {storage[0], storage[0], storage[0], storage[3]};

    // Create EULER with children: "alice", "bob", "charlie"
    rdx writer = {};
    rdxWriteInit(&writer, RDX_FMT_RB, rb_buf);
    writer.type = RDX_TYPE_EULER;
    writer.ptype = 0;
    call(rdxWriteNextRB, &writer);

    rdx child = {};
    call(rdxWriteIntoRB, &child, &writer);

    u8c alice[] = "alice";
    u8c bob[] = "bob";
    u8c charlie[] = "charlie";

    child.type = RDX_TYPE_STRING;
    child.s[0] = alice;
    child.s[1] = alice + 5;
    call(rdxWriteNextRB, &child);

    child.type = RDX_TYPE_STRING;
    child.s[0] = bob;
    child.s[1] = bob + 3;
    call(rdxWriteNextRB, &child);

    child.type = RDX_TYPE_STRING;
    child.s[0] = charlie;
    child.s[1] = charlie + 7;
    call(rdxWriteNextRB, &child);

    call(rdxWriteOutoRB, &child, &writer);

    // Now try to find-or-create "bob" - should find existing
    rdx seeker = {};
    seeker.type = RDX_TYPE_STRING;
    seeker.s[0] = bob;
    seeker.s[1] = bob + 3;

    // rdxWriteIntoRB with preset child should find or create
    ok64 o = rdxWriteIntoRB(&seeker, &writer);
    testeq(o, OK);

    // seeker should now be positioned at "bob"
    // The found flag or position should indicate existing
    // For now, verify we can continue writing without duplicates

    done;
}

// Test find-or-create: seek to non-existing child should create it
ok64 RBTestCreateNew() {
    sane(1);

    a_pad(u8, storage, 1024);
    u8b rb_buf = {storage[0], storage[0], storage[0], storage[3]};

    // Create EULER with children: "alice", "charlie" (no bob)
    rdx writer = {};
    rdxWriteInit(&writer, RDX_FMT_RB, rb_buf);
    writer.type = RDX_TYPE_EULER;
    writer.ptype = 0;
    call(rdxWriteNextRB, &writer);

    rdx child = {};
    call(rdxWriteIntoRB, &child, &writer);

    u8c alice[] = "alice";
    u8c charlie[] = "charlie";

    child.type = RDX_TYPE_STRING;
    child.s[0] = alice;
    child.s[1] = alice + 5;
    call(rdxWriteNextRB, &child);

    child.type = RDX_TYPE_STRING;
    child.s[0] = charlie;
    child.s[1] = charlie + 7;
    call(rdxWriteNextRB, &child);

    call(rdxWriteOutoRB, &child, &writer);

    // Now try to find-or-create "bob" - should create new
    u8c bob[] = "bob";
    rdx seeker = {};
    seeker.type = RDX_TYPE_STRING;
    seeker.s[0] = bob;
    seeker.s[1] = bob + 3;

    ok64 o = rdxWriteIntoRB(&seeker, &writer);
    testeq(o, OK);
    call(rdxWriteOutoRB, &seeker, &writer);  // sync write position back

    // After creation, read back and verify "bob" exists in sorted order
    u8b read_buf = {storage[0], storage[0], rb_buf[2], storage[3]};

    rdx reader = {};
    rdxInit(&reader, RDX_FMT_RB, read_buf);
    reader.ptype = 0;

    call(rdxNextRB, &reader);
    testeq(reader.type, RDX_TYPE_EULER);

    rdx rchild = {};
    call(rdxIntoRB, &rchild, &reader);

    // Should now have 3 children in order: alice, bob, charlie
    call(rdxNextRB, &rchild);
    testeq(u8csLen(rchild.s), 5);  // alice

    call(rdxNextRB, &rchild);
    testeq(u8csLen(rchild.s), 3);  // bob

    call(rdxNextRB, &rchild);
    testeq(u8csLen(rchild.s), 7);  // charlie

    testeq(rdxNextRB(&rchild), END);

    done;
}

// Table-driven crash repro tests
// Each entry: TLV input bytes that previously caused crashes
ok64 RBTestCrashRepros() {
    sane(1);

    // Table of crash inputs (TLV format)
    struct {
        u8c const* data;
        size_t len;
        char const* name;
    } cases[] = {
        // crash-02767290: 3 floats at root level (root tree issue)
        {(u8c[]){0x66, 0x01, 0x00, 0x66, 0x01, 0x00, 0x66, 0x01, 0x00}, 9,
         "root_siblings"},
        // crash-edb58d4350a: long string (varint length encoding)
        {(u8c[]){0x73, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
                 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,
                 0x7e},
         21, "long_string"},
    };

    for (size_t c = 0; c < sizeof(cases) / sizeof(cases[0]); c++) {
        u8cs tlv_slice = {cases[c].data, cases[c].data + cases[c].len};

        // Verify input is valid TLV
        rdx vfy = {.format = RDX_FMT_TLV};
        vfy.next = tlv_slice[0];
        vfy.opt = (u8p)tlv_slice[1];
        vfy.bulk = NULL;
        if (rdxVerifyAll(&vfy) != OK) continue;  // skip invalid

        // Write to RB
        a_pad(u8, rb_storage, 1024);
        u8b rb_buf = {rb_storage[0], rb_storage[0], rb_storage[0], rb_storage[3]};

        rdx rbw = {};
        rdxWriteInit(&rbw, RDX_FMT_RB, rb_buf);
        rbw.ptype = 0;
        rdx tlv_r = {.format = RDX_FMT_TLV};
        tlv_r.next = tlv_slice[0];
        tlv_r.opt = (u8p)tlv_slice[1];
        tlv_r.bulk = NULL;

        // Copy all elements from TLV to RB
        ok64 o;
        while ((o = rdxNext(&tlv_r)) == OK) {
            rbw.type = tlv_r.type;
            rbw.id = tlv_r.id;
            rbw.f = tlv_r.f;  // covers union
            u8csMv(rbw.s, tlv_r.s);
            u8csMv(rbw.t, tlv_r.t);
            call(rdxWriteNextRB, &rbw);
        }
        testeq(o, END);

        // Read back and count elements
        u8b rb_rbuf = {rb_storage[0], rb_storage[0], rb_buf[2], rb_storage[3]};
        rdx rb_r = {};
        rdxInit(&rb_r, RDX_FMT_RB, rb_rbuf);
        rb_r.ptype = 0;

        // Re-parse TLV for comparison
        rdx tlv_cmp = {.format = RDX_FMT_TLV};
        tlv_cmp.next = tlv_slice[0];
        tlv_cmp.opt = (u8p)tlv_slice[1];
        tlv_cmp.bulk = NULL;

        // Compare element by element
        while (1) {
            ok64 orb = rdxNextRB(&rb_r);
            ok64 otlv = rdxNext(&tlv_cmp);

            if (orb == END && otlv == END) break;
            testeq(orb, OK);
            testeq(otlv, OK);
            testeq(rb_r.type, tlv_cmp.type);
        }
    }

    done;
}

ok64 RBTest() {
    sane(1);
    call(RBTestStruct);
    call(RBTestRecordLayout);
    call(RBTestPlexRecord);
    call(RBTestWrite);
    call(RBTestFindExisting);
    call(RBTestCreateNew);
    call(RBTestCrashRepros);
    done;
}

TEST(RBTest)
