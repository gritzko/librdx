#include "spot/CAPOi.h"
#include "spot/SPOT.h"

#include <string.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/SORT.h"
#include "abc/TEST.h"

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
    Bu64 entries = {};
    call(u64bAlloc, entries, maxentries);
    u64 *ebuf = entries[0];

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

    u64bFree(entries);
    done;
}

// --- Test 3: Sort + HIT dedup ---
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
    u64sSort(data);

    // After sort: entries grouped by trigram, then by path hash
    want(arr[0] <= arr[1]);
    want(arr[1] <= arr[2]);

    // Merge with HIT should dedup
    u64cs runs[1] = {{(u64cp)arr, (u64cp)arr + 3}};
    u64css iter = {runs, runs + 1};
    HITu64Start(iter);

    u64 out[3];
    u64p op = out;
    HITu64Merge(iter, &op);

    // Should have 2 unique entries (e1 deduped)
    testeq((size_t)(op - out), (size_t)2);

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

// --- Test 5: HIT seek by trigram prefix ---
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
    u64sSort(data);

    u64cs runs[1] = {{(u64cp)entries, (u64cp)entries + 4}};
    u64css iter = {runs, runs + 1};
    HITu64Start(iter);

    // Seek to tri2 prefix
    u64 prefix = CAPOTriPack(tri2);
    ok64 o = HITu64Seek(iter, &prefix);
    want(o == OK);
    want(!$empty(iter));
    testeq(CAPOTriOf(*(*iter[0])[0]), prefix);

    done;
}

// --- Test 6: idx64 type/key/path roundtrip ---
ok64 CAPO6() {
    sane(1);
    a_cstr(name, "myFunc");
    a_cstr(path, "src/main.c");

    idx64 men = CAPOSymEntry(IDX64_MEN, name, path);
    idx64 def = CAPOSymEntry(IDX64_DEF, name, path);
    a_cstr(tri6, "foo");
    idx64 tri_entry = CAPOEntry(tri6, path);

    // Type field
    testeq(idx64Type(men), (u64)IDX64_MEN);
    testeq(idx64Type(def), (u64)IDX64_DEF);
    testeq(idx64Type(tri_entry), (u64)IDX64_TRI);

    // Key: same name -> same key for both mention and definition
    testeq(idx64Key(men), idx64Key(def));

    // Path hash
    testeq(idx64PathHash(men), CAPOPathHash(path));
    testeq(idx64PathHash(def), CAPOPathHash(path));

    // Different name -> different key
    a_cstr(name2, "otherFunc");
    idx64 men2 = CAPOSymEntry(IDX64_MEN, name2, path);
    want(idx64Key(men2) != idx64Key(men));

    done;
}

// --- Test 7: CAPOIndexFile emits both trigram and symbol entries ---
ok64 CAPO7() {
    sane(1);
    const char *src = "int foo(int x) { return x + 1; }";
    u8csc source = {(u8cp)src, (u8cp)src + strlen(src)};
    u8cs ext = $u8str(".c");
    a_cstr(path, "test.c");

    size_t maxentries = 4096;
    Bu64 entries = {};
    call(u64bAlloc, entries, maxentries);
    u64 *ebuf = entries[0];

    call(CAPOIndexFile, entries, source, ext, path);

    size_t nentries = u64bIdleHead(entries) - ebuf;
    want(nentries > 0);

    // Count by type
    size_t ntri = 0, nmen = 0, ndef = 0;
    for (size_t i = 0; i < nentries; i++) {
        u64 t = idx64Type(ebuf[i]);
        if (t == IDX64_TRI) ntri++;
        else if (t == IDX64_MEN) nmen++;
        else if (t == IDX64_DEF) ndef++;
    }
    // Must have some trigrams
    want(ntri > 0);
    // Must have some symbol entries (mentions or definitions)
    want(nmen + ndef > 0);

    // All entries should have the same path hash
    u32 phash = CAPOPathHash(path);
    for (size_t i = 0; i < nentries; i++)
        testeq(idx64PathHash(ebuf[i]), phash);

    u64bFree(entries);
    done;
}

// --- Test 8: symbol entries sort after trigrams ---
ok64 CAPO8() {
    sane(1);
    a_cstr(name, "myVar");
    a_cstr(path, "a.c");
    a_cstr(tri, "foo");

    idx64 tri_e = CAPOEntry(tri, path);
    idx64 men_e = CAPOSymEntry(IDX64_MEN, name, path);
    idx64 def_e = CAPOSymEntry(IDX64_DEF, name, path);

    // Trigram (type 00) < mention (type 01) < definition (type 10)
    want(tri_e < men_e);
    want(men_e < def_e);

    // Sort should preserve this ordering
    u64 arr[] = {def_e, tri_e, men_e};
    u64s data = {arr, arr + 3};
    u64sSort(data);
    testeq(idx64Type(arr[0]), (u64)IDX64_TRI);
    testeq(idx64Type(arr[1]), (u64)IDX64_MEN);
    testeq(idx64Type(arr[2]), (u64)IDX64_DEF);

    done;
}

// --- Test 9: CAPOIndexFile produces no duplicate entries after sort ---
ok64 CAPO9() {
    sane(1);
    // Source with repeated identifiers generates duplicate trigrams
    const char *src = "int aaa = aaa + aaa;";
    u8csc source = {(u8cp)src, (u8cp)src + strlen(src)};
    u8cs ext = $u8str(".c");
    a_cstr(path, "dup.c");

    size_t maxentries = 4096;
    Bu64 entries = {};
    call(u64bAlloc, entries, maxentries);
    u64 *ebuf = entries[0];

    call(CAPOIndexFile, entries, source, ext, path);

    size_t nentries = u64bIdleHead(entries) - ebuf;
    want(nentries > 0);

    // Sort and check for duplicates
    u64s data = {ebuf, ebuf + nentries};
    u64sSort(data);

    size_t dups = 0;
    for (size_t i = 1; i < nentries; i++) {
        if (ebuf[i] == ebuf[i - 1]) dups++;
    }
    // "aaa" trigram appears 3 times for same file → must have duplicates pre-dedup
    want(dups > 0);

    // After HIT merge, duplicates should be gone
    u64cs runs[1] = {{(u64cp)ebuf, (u64cp)ebuf + nentries}};
    u64css iter = {runs, runs + 1};
    HITu64Start(iter);

    u64 out[4096];
    u64p op = out;
    HITu64Merge(iter, &op);
    size_t unique = (size_t)(op - out);
    want(unique < nentries);
    // Verify no dups in merged output
    for (size_t i = 1; i < unique; i++) {
        want(out[i] != out[i - 1]);
    }

    u64bFree(entries);
    done;
}

// --- Test A: HunkEmit produces valid TLV that roundtrips via Drain ---

ok64 CAPOtestHunkEmit() {
    sane(1);
    // Set up a capture buffer instead of a pipe.
    // spot_emit = HUNKu8sFeed, spot_out_fd = a temp file.
    char tmppath[] = "/tmp/spot_hunk_test_XXXXXX";
    int fd = mkstemp(tmppath);
    test(fd >= 0, FAILSANITY);

    spot_emit   = HUNKu8sFeed;
    spot_out_fd = fd;
    call(LESSArenaInit);

    // Synthesize a hunk via CAPOBuildHunk
    const char *src = "void foo() {\n    int x = 1;\n    int y = 2;\n}\n";
    u8csc source = {(u8cp)src, (u8cp)src + strlen(src)};
    u8cs ext = $u8str(".c");
    Bu32 toks = {};
    call(u32bMap, toks, strlen(src) + 1);
    call(SPOTTokenize, toks, source, ext);
    u32cs htoks = {(u32cp)u32bDataHead(toks), (u32cp)u32bIdleHead(toks)};

    // One highlight range covering "int x = 1;" (bytes 18..28 approx)
    range32 hls[1] = {{18, 28}};
    b8 first = YES;
    call(CAPOBuildHunk, source, htoks, 0, (u32)strlen(src),
         hls, 1, ext, "test.c", YES, &first);

    // Close and verify the file has content.
    close(fd);
    spot_out_fd = -1;
    spot_emit   = NULL;

    // Read back and drain
    u8bp mapped = NULL;
    a_pad(u8, pathbuf, 256);
    u8cs ps = {(u8cp)tmppath, (u8cp)tmppath + strlen(tmppath)};
    call(u8bFeed, pathbuf, ps);
    call(PATHu8bTerm, pathbuf);
    call(FILEMapRO, &mapped, $path(pathbuf));

    a_dup(u8c, data, u8bDataC(mapped));
    want($len(data) > 20);  // non-trivial TLV

    hunk h = {};
    ok64 o = HUNKu8sDrain(data, &h);
    testeq(o, OK);
    want(!$empty(h.text));
    want(!$empty(h.uri));
    // text should contain the source
    want($len(h.text) == strlen(src));
    want(memcmp(h.text[0], src, strlen(src)) == 0);
    // URI should start with "test.c"
    want($len(h.uri) >= 6);
    want(memcmp(h.uri[0], "test.c", 6) == 0);

    FILEUnMap(mapped);
    u32bUnMap(toks);
    LESSArenaCleanup();
    unlink(tmppath);
    done;
}

// --- Test B: CAPOKnownExt recognizes standard extensions ---
ok64 CAPOtestKnownExt() {
    sane(1);
    u8cs c_ext = $u8str(".c");
    u8cs h_ext = $u8str(".h");
    u8cs py_ext = $u8str(".py");
    u8cs go_ext = $u8str(".go");
    u8cs txt_ext = $u8str(".xyz_unknown");

    want(CAPOKnownExt(c_ext) == YES);
    want(CAPOKnownExt(h_ext) == YES);
    want(CAPOKnownExt(py_ext) == YES);
    want(CAPOKnownExt(go_ext) == YES);
    want(CAPOKnownExt(txt_ext) == NO);

    u8cs empty = {};
    want(CAPOKnownExt(empty) == NO);
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
    call(CAPO6);
    call(CAPO7);
    call(CAPO8);
    call(CAPO9);
    call(CAPOtestHunkEmit);
    call(CAPOtestKnownExt);
    done;
}

TEST(CAPOtest);
