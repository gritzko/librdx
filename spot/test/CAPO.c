#include "spot/CAPO.h"

#include <string.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/SORT.h"
#include "abc/TEST.h"
#include "ast/BAST.h"
#include "json/BASON.h"

// MSET for u64
#define X(M, name) M##u64##name
#include "abc/MSETx.h"
#undef X

// --- Test 0: TriPack roundtrip ---
ok64 CAPO0() {
    sane(1);
    // "abc" -> pack -> extract upper bits -> consistent
    a_cstr(tri, "abc");
    u64 packed = CAPOTriPack(tri);
    // Upper 32 bits hold the trigram, lower 32 are zero
    testeq((u32)packed, (u32)0);
    want(packed != 0);

    // Same input -> same output
    u64 packed2 = CAPOTriPack(tri);
    testeq(packed, packed2);

    // Different trigram -> different result
    a_cstr(tri2, "xyz");
    u64 packed3 = CAPOTriPack(tri2);
    want(packed3 != packed);

    // CAPOTriOf extracts just the trigram part
    u64 entry = packed | 0x12345678ULL;
    testeq(CAPOTriOf(entry), packed);

    done;
}

// --- Test 1: CAPOEntry packs both parts ---
ok64 CAPO1() {
    sane(1);
    a_cstr(tri, "foo");
    a_cstr(path, "src/main.c");
    u64 entry = CAPOEntry(tri, path);

    // Upper bits = trigram
    testeq(CAPOTriOf(entry), CAPOTriPack(tri));
    // Lower bits = path hash
    testeq((u32)entry, CAPOPathHash(path));

    // Different path, same trigram -> same upper, different lower
    a_cstr(path2, "src/other.c");
    u64 entry2 = CAPOEntry(tri, path2);
    testeq(CAPOTriOf(entry), CAPOTriOf(entry2));
    want((u32)entry != (u32)entry2);

    done;
}

// --- Test 2: CAPOIndexFile extracts trigrams ---
ok64 CAPO2() {
    sane(1);
    const char *src = "int foo(int x) { return x + 1; }";
    u8csc source = {(u8cp)src, (u8cp)src + strlen(src)};
    u8cs ext = $u8str(".c");
    a_cstr(path, "test.c");

    size_t maxentries = 4096;
    u64 *ebuf = (u64 *)malloc(maxentries * sizeof(u64));
    test(ebuf != NULL, FAILSANITY);
    u64b entries = {ebuf, ebuf, ebuf, ebuf + maxentries};

    call(CAPOIndexFile, entries, source, ext, path);

    size_t nentries = u64bIdleHead(entries) - ebuf;
    want(nentries > 0);

    // All entries should have the same path hash
    u32 phash = CAPOPathHash(path);
    for (size_t i = 0; i < nentries; i++)
        testeq((u32)ebuf[i], phash);

    // Should contain trigrams from "foo", "int", "return"
    a_cstr(tri_foo, "foo");
    a_cstr(tri_int, "int");
    u64 prefix_foo = CAPOTriPack(tri_foo);
    u64 prefix_int = CAPOTriPack(tri_int);

    b8 found_foo = NO, found_int = NO;
    for (size_t i = 0; i < nentries; i++) {
        if (CAPOTriOf(ebuf[i]) == prefix_foo) found_foo = YES;
        if (CAPOTriOf(ebuf[i]) == prefix_int) found_int = YES;
    }
    want(found_foo == YES);
    want(found_int == YES);

    free(ebuf);
    done;
}

// --- Test 3: Sort + MSET dedup ---
ok64 CAPO3() {
    sane(1);
    // Two files with overlapping trigrams
    a_cstr(tri, "foo");
    a_cstr(path1, "a.c");
    a_cstr(path2, "b.c");
    u64 e1 = CAPOEntry(tri, path1);
    u64 e2 = CAPOEntry(tri, path2);
    u64 e1dup = e1;  // duplicate

    u64 arr[] = {e2, e1, e1dup};
    u64s data = {arr, arr + 3};
    $sort(data, u64cmp);

    // After sort: entries grouped by trigram, then by path hash
    want(arr[0] <= arr[1]);
    want(arr[1] <= arr[2]);

    // Merge with MSET should dedup
    u64cs runs[1] = {{(u64cp)arr, (u64cp)arr + 3}};
    u64css iter = {runs, runs + 1};
    u64 out[3];
    u64s into = {out, out + 3};
    call(MSETu64Merge, into, iter);

    // Should have 2 unique entries (e1 deduped)
    testeq((size_t)(into[0] - out), (size_t)2);

    done;
}

// --- Test 4: TriChar filter ---
ok64 CAPO4() {
    sane(1);
    // RON64 chars should pass
    want(CAPOTriChar('a') != 0);
    want(CAPOTriChar('Z') != 0);
    want(CAPOTriChar('0') != 0);
    want(CAPOTriChar('_') != 0);

    // Non-RON64 chars should fail
    want(CAPOTriChar(' ') == 0);
    want(CAPOTriChar('(') == 0);
    want(CAPOTriChar('{') == 0);
    want(CAPOTriChar('\n') == 0);

    done;
}

// --- Test 5: MSET seek by trigram prefix ---
ok64 CAPO5() {
    sane(1);
    a_cstr(tri1, "aaa");
    a_cstr(tri2, "mmm");
    a_cstr(tri3, "zzz");
    a_cstr(p1, "x.c");
    a_cstr(p2, "y.c");

    u64 entries[] = {
        CAPOEntry(tri1, p1), CAPOEntry(tri1, p2),
        CAPOEntry(tri2, p1), CAPOEntry(tri3, p2),
    };
    u64s data = {entries, entries + 4};
    $sort(data, u64cmp);

    u64cs runs[1] = {{(u64cp)entries, (u64cp)entries + 4}};
    u64css iter = {runs, runs + 1};
    MSETu64Start(iter);

    // Seek to tri2 prefix
    u64 prefix = CAPOTriPack(tri2);
    call(MSETu64Seek, iter, prefix);
    want(!$empty(iter));
    testeq(CAPOTriOf(****iter), prefix);

    done;
}

ok64 CAPOtest() {
    sane(1);
    call(CAPO0);
    call(CAPO1);
    call(CAPO2);
    call(CAPO3);
    call(CAPO4);
    call(CAPO5);
    done;
}

TEST(CAPOtest);
