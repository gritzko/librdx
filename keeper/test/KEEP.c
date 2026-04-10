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

    u64 hashlet = wh64HashletFromHex("abcdef", 6);
    u64 val = 0;
    want(KEEPLookup(&k, hashlet, 6, &val) == KEEPNONE);
    want(KEEPHas(&k, hashlet, 6) == KEEPNONE);

    call(KEEPClose, &k);

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", tmpdir);
    system(cmd);

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
    fprintf(stderr, "all passed\n");
    done;
}

TEST(maintest);
