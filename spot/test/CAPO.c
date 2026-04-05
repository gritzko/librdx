#include "spot/CAPO.h"

#include <string.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/SORT.h"
#include "abc/TEST.h"

// HIT for u64cs
fun void u64csSwap(u64cs *a, u64cs *b) {
    u64c *t0 = (*a)[0], *t1 = (*a)[1];
    (*a)[0] = (*b)[0]; (*a)[1] = (*b)[1];
    (*b)[0] = t0; (*b)[1] = t1;
}
typedef b8 (*u64csz)(u64cs const *, u64cs const *);
typedef ok64 (*u64csx)(u64cs *, u64cs const *);
typedef ok64 (*u64csy)(u64cs *, u64css);

#define HIT_ENTRY_IS_SLICE
#define X(M, name) M##u64cs##name
#include "abc/HITx.h"
#undef X
#undef HIT_ENTRY_IS_SLICE

fun b8 u64csHeadZ(u64cs const *a, u64cs const *b) {
    return *(*a)[0] < *(*b)[0];
}

fun ok64 u64csSeekX(u64cs *a, u64cs const *b) {
    u64c *const run[2] = {(*a)[0], (*a)[1]};
    u64c *pos = $u64findge(run, (*b)[0]);
    (*a)[0] = pos;
    return $empty(*a) ? NODATA : OK;
}

fun ok64 CAPOHITSeek(u64css heap, u64 key) {
    u64cs keyentry = {&key, &key + 1};
    return HITu64csSeekXZ(heap, &keyentry, u64csSeekX, u64csHeadZ);
}

fun void CAPOHITStep(u64css heap) {
    ++(*heap[0])[0];
    if ($empty(*heap[0])) {
        HITu64csEject(heap, 0);
    }
    if (!$empty(heap)) HITu64csDownYZ(heap, 0, u64csHeadZ);
}

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
    $sort(data, u64cmp);

    // After sort: entries grouped by trigram, then by path hash
    want(arr[0] <= arr[1]);
    want(arr[1] <= arr[2]);

    // Merge with HIT should dedup
    u64cs runs[1] = {{(u64cp)arr, (u64cp)arr + 3}};
    u64css iter = {runs, runs + 1};
    HITu64csStartZ(iter, u64csHeadZ);

    u64 out[3];
    u64p op = out;
    u64 prev = 0;
    b8 first = YES;
    while (!$empty(iter)) {
        u64 entry = *(*iter[0])[0];
        if (first || entry != prev) {
            *op++ = entry;
            prev = entry;
            first = NO;
        }
        CAPOHITStep(iter);
    }

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
    $sort(data, u64cmp);

    u64cs runs[1] = {{(u64cp)entries, (u64cp)entries + 4}};
    u64css iter = {runs, runs + 1};
    HITu64csStartZ(iter, u64csHeadZ);

    // Seek to tri2 prefix
    u64 prefix = CAPOTriPack(tri2);
    ok64 o = CAPOHITSeek(iter, prefix);
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
    $sort(data, u64cmp);
    testeq(idx64Type(arr[0]), (u64)IDX64_TRI);
    testeq(idx64Type(arr[1]), (u64)IDX64_MEN);
    testeq(idx64Type(arr[2]), (u64)IDX64_DEF);

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
    done;
}

TEST(CAPOtest);
