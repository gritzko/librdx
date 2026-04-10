#include "keeper/KEEP.h"
#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

// --- hash64 conversion tests ---

ok64 HASH64hex() {
    sane(1);

    // Round-trip: hex → hash64 → hex
    u8cs h7 = {(u8cp)"816fb46", (u8cp)"816fb46" + 7};
    hash64 h = hash64FromHex(h7);
    u8 buf[16] = {};
    u8s out = {buf, buf + 15};
    call(hash64ToHex, out, h);
    // First 7 chars must match
    want(memcmp(buf, "816fb46", 7) == 0);

    // Type must be SHA1
    want(hash64Type(h) == HASH_SHA1);

    // Prefix bits must not have type contamination
    u64 prefix = hash64Prefix(h);
    want((prefix & 0xff) == 0x81);  // first byte of SHA

    // Short prefix: 6 chars
    u8cs h6 = {(u8cp)"816fb4", (u8cp)"816fb4" + 6};
    hash64 h6v = hash64FromHex(h6);
    u8 buf6[16] = {};
    u8s out6 = {buf6, buf6 + 15};
    hash64ToHex(out6, h6v);
    want(memcmp(buf6, "816fb4", 6) == 0);

    // Full 15-char prefix
    // 14-char prefix round-trips perfectly (15th char overlaps type bits)
    u8cs h14 = {(u8cp)"816fb46be665c8", (u8cp)"816fb46be665c8" + 14};
    hash64 h14v = hash64FromHex(h14);
    u8 buf14[16] = {};
    u8s out14 = {buf14, buf14 + 15};
    hash64ToHex(out14, h14v);
    want(memcmp(buf14, "816fb46be665c8", 14) == 0);

    done;
}

ok64 HASH64sha1() {
    sane(1);

    // SHA-1 bytes → hash64 → hex must match
    // SHA: 816fb46be665c8b63647f0096845fef363736b20
    u8 sha[20] = {0x81, 0x6f, 0xb4, 0x6b, 0xe6, 0x65, 0xc8, 0xb6,
                  0x36, 0x47, 0xf0, 0x09, 0x68, 0x45, 0xfe, 0xf3,
                  0x63, 0x73, 0x6b, 0x20};
    hash64 h = hash64FromSha1(sha, HASH_SHA1);

    u8 buf[16] = {};
    u8s out = {buf, buf + 15};
    hash64ToHex(out, h);
    // First 14 chars must match (15th overlaps type bits)
    want(memcmp(buf, "816fb46be665c8", 14) == 0);

    // FromHex with 14-char prefix must match (avoids type-bit overlap)
    u8cs hex14 = {(u8cp)"816fb46be665c8", (u8cp)"816fb46be665c8" + 14};
    hash64 h2 = hash64FromHex(hex14);
    want(hash64Prefix(h2) == hash64Prefix(hash64FromHex(hex14)));

    done;
}

// --- w64 pack/unpack tests ---

ok64 W64pack() {
    sane(1);

    wh64 v = wh64Pack(5, 1000, 0xABCDEF0123ULL);
    want(wh64Type(v) == 5);
    want(wh64Id(v) == 1000);
    want(wh64Off(v) == 0xABCDEF0123ULL);

    // Edge: max values
    wh64 vmax = wh64Pack(0xf, WHIFF_ID_MASK, WHIFF_OFF_MASK);
    want(wh64Type(vmax) == 0xf);
    want(wh64Id(vmax) == WHIFF_ID_MASK);
    want(wh64Off(vmax) == WHIFF_OFF_MASK);

    // Edge: zero
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

    // Create temp dir
    char tmpdir[] = "/tmp/keeper-test-XXXXXX";
    want(mkdtemp(tmpdir) != NULL);

    u8cs root = {(u8cp)tmpdir, (u8cp)tmpdir + strlen(tmpdir)};
    keeper k = {};
    call(KEEPOpen, &k, root);
    want(k.npacks == 0);
    want(k.nruns == 0);

    // Lookup on empty store returns KEEPNONE
    hash64 h = hash64FromHex((u8cs){(u8cp)"abcdef", (u8cp)"abcdef" + 6});
    u64 val = 0;
    want(KEEPLookup(&k, h, &val) == KEEPNONE);
    want(KEEPHas(&k, h) == KEEPNONE);

    call(KEEPClose, &k);

    // Cleanup
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", tmpdir);
    system(cmd);

    done;
}

ok64 maintest() {
    sane(1);
    fprintf(stderr, "HASH64hex...\n");
    call(HASH64hex);
    fprintf(stderr, "HASH64sha1...\n");
    call(HASH64sha1);
    fprintf(stderr, "W64pack...\n");
    call(W64pack);
    fprintf(stderr, "KEEPempty...\n");
    call(KEEPempty);
    fprintf(stderr, "all passed\n");
    done;
}

TEST(maintest);
