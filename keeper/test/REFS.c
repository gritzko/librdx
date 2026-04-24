//  REFS — ULOG-backed ref reflog: end-to-end property tests.
//
//  REFS is thin glue over dog/ULOG: every Append emits one `set`
//  row into a `refs` file in the dir, Load reverse-walks the log
//  and dedups by key, Resolve does host-substring + refname
//  matching, Compact rewrites keeping latest-per-key.  These tests
//  exercise each path against a fresh tmp dir; no keeper/HOME
//  state is needed.
//
//  Cases:
//    1. Empty: Load on a missing dir returns 0 refs, no error.
//    2. Append + Load: single row round-trips; key/val slices
//       point into the caller's arena.
//    3. Append dedup: same key twice → Load returns one entry with
//       the latest val.
//    4. Multi-key Load: two distinct keys → both survive; order
//       not required.
//    5. Resolve local ref: `?heads/main` matches the stored row.
//    6. Resolve host alias: `//github?master` finds a row whose
//       authority contains `github` and whose query is `heads/master`.
//    7. Resolve miss: unknown key → REFSNONE.
//    8. SyncRecord: bulk-append three entries, verify all present.
//    9. Compact: append four rows, two keys, compact → Load still
//       returns latest-per-key, file is smaller (one row per key).
//   10. Monotonicity: ULOG enforces strictly increasing ts via
//       REFSAppend's clamp — two rapid appends produce distinct ts.
//
//  On-disk shape verified indirectly by re-opening the ULOG and
//  checking row count after each mutation.

#include "keeper/REFS.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/S.h"
#include "abc/TEST.h"
#include "dog/ULOG.h"

// --- tmp dir scaffolding ---

static ok64 tmp_make(char *tmpl) {
    if (mkdtemp(tmpl) == NULL) return FAIL;
    return OK;
}

static void tmp_rm(char const *path) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", path);
    int _ = system(cmd);
    (void)_;
}

// --- slice helpers for literal test data ---

//  Drop a literal C string into an a_cstr-style slice with a unique
//  name so two slices can coexist in one scope.
#define LIT(name, text) a_cstr(name, text)

// --- row count helper: re-open the ULOG and count rows ---

static u32 count_rows(char const *dir_cstr) {
    a_cstr(dir_s, dir_cstr);
    a_path(fname, dir_s);
    LIT(refs_seg, "refs");
    if (PATHu8bPush(fname, refs_seg) != OK) return (u32)-1;
    a_dup(u8c, path, u8bDataC(fname));
    ulog l = {};
    if (ULOGOpen(&l, path) != OK) return 0;
    u32 n = ULOGCount(&l);
    ULOGClose(&l);
    return n;
}

// --- measure refs-file byte size (compaction sanity) ---

static ssize_t file_bytes(char const *dir_cstr) {
    char p[1024];
    snprintf(p, sizeof(p), "%s/refs", dir_cstr);
    struct stat st;
    if (stat(p, &st) != 0) return -1;
    return (ssize_t)st.st_size;
}

// --- shared test helpers ---

//  Wrap a C string as a const slice.
#define CSTR_SLICE(name, text)                                          \
    a_cstr(name, text)

//  Append (key, val) to a dir via REFSAppend; cstr inputs.
static ok64 append_kv(char const *dir_cstr,
                      char const *key_cstr, char const *val_cstr) {
    sane(dir_cstr && key_cstr && val_cstr);
    a_cstr(dir_s, dir_cstr);
    CSTR_SLICE(k, key_cstr);
    CSTR_SLICE(v, val_cstr);
    call(REFSAppend, dir_s, k, v);
    done;
}

//  Search a ref[] array for an entry whose key equals `want`.
//  Returns pointer into arr or NULL.
static ref const *find_by_key(ref const *arr, u32 n, u8csc want) {
    for (u32 i = 0; i < n; i++) {
        if (u8csLen(arr[i].key) == u8csLen(want) &&
            memcmp(arr[i].key[0], want[0], u8csLen(want)) == 0)
            return &arr[i];
    }
    return NULL;
}

// ---- 1. Empty: Load on a missing dir returns 0 refs ----

ok64 REFStest_empty() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/refs-empty-XXXXXX";
    want(tmp_make(tmpdir) == OK);
    a_cstr(dir_s, tmpdir);

    Bu8 arena = {};
    call(u8bMap, arena, 4096);
    ref arr[8] = {};
    u32 n = 99;
    ok64 o = REFSLoad(arr, &n, 8, arena, dir_s);
    want(o == OK);
    want(n == 0);
    u8bUnMap(arena);

    //  Resolve against a missing log → REFSNONE.
    Bu8 arena2 = {};
    call(u8bMap, arena2, 1024);
    uri res = {};
    CSTR_SLICE(q, "?heads/main");
    ok64 r = REFSResolve(&res, arena2, dir_s, q);
    want(r == REFSNONE);
    u8bUnMap(arena2);

    tmp_rm(tmpdir);
    done;
}

// ---- 2. Append + Load round-trip ----

ok64 REFStest_append_load() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/refs-al-XXXXXX";
    want(tmp_make(tmpdir) == OK);

    call(append_kv, tmpdir,
         "?heads/main",
         "?0123456789abcdef0123456789abcdef01234567");

    want(count_rows(tmpdir) == 1);

    Bu8 arena = {};
    call(u8bMap, arena, 4096);
    ref arr[8] = {};
    u32 n = 0;
    a_cstr(dir_s, tmpdir);
    call(REFSLoad, arr, &n, 8, arena, dir_s);
    want(n == 1);
    want(arr[0].type == REF_SHA);

    CSTR_SLICE(want_k, "?heads/main");
    want(u8csLen(arr[0].key) == u8csLen(want_k));
    want(memcmp(arr[0].key[0], want_k[0], u8csLen(want_k)) == 0);
    //  val is stored as `?<40-hex>` after REFSLoad's hash-split.
    CSTR_SLICE(want_v, "?0123456789abcdef0123456789abcdef01234567");
    want(u8csLen(arr[0].val) == u8csLen(want_v));
    want(memcmp(arr[0].val[0], want_v[0], u8csLen(want_v)) == 0);

    u8bUnMap(arena);
    tmp_rm(tmpdir);
    done;
}

// ---- 3. Append dedup: same key twice → latest wins ----

ok64 REFStest_dedup_latest() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/refs-dd-XXXXXX";
    want(tmp_make(tmpdir) == OK);

    call(append_kv, tmpdir,
         "?heads/main",
         "?1111111111111111111111111111111111111111");
    call(append_kv, tmpdir,
         "?heads/main",
         "?2222222222222222222222222222222222222222");

    //  Both rows on disk; Load dedups to the latest.
    want(count_rows(tmpdir) == 2);

    Bu8 arena = {};
    call(u8bMap, arena, 4096);
    ref arr[8] = {};
    u32 n = 0;
    a_cstr(dir_s, tmpdir);
    call(REFSLoad, arr, &n, 8, arena, dir_s);
    want(n == 1);
    CSTR_SLICE(want_v, "?2222222222222222222222222222222222222222");
    want(u8csLen(arr[0].val) == u8csLen(want_v));
    want(memcmp(arr[0].val[0], want_v[0], u8csLen(want_v)) == 0);

    u8bUnMap(arena);
    tmp_rm(tmpdir);
    done;
}

// ---- 4. Multi-key Load ----

ok64 REFStest_multi_key() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/refs-mk-XXXXXX";
    want(tmp_make(tmpdir) == OK);

    call(append_kv, tmpdir,
         "?heads/main",
         "?aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    call(append_kv, tmpdir,
         "?tags/v1.0",
         "?bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");

    Bu8 arena = {};
    call(u8bMap, arena, 4096);
    ref arr[8] = {};
    u32 n = 0;
    a_cstr(dir_s, tmpdir);
    call(REFSLoad, arr, &n, 8, arena, dir_s);
    want(n == 2);

    CSTR_SLICE(k_main, "?heads/main");
    CSTR_SLICE(k_tag,  "?tags/v1.0");
    ref const *m = find_by_key(arr, n, k_main);
    ref const *t = find_by_key(arr, n, k_tag);
    want(m != NULL);
    want(t != NULL);
    CSTR_SLICE(v_main, "?aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    CSTR_SLICE(v_tag,  "?bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    want(memcmp(m->val[0], v_main[0], u8csLen(v_main)) == 0);
    want(memcmp(t->val[0], v_tag[0],  u8csLen(v_tag))  == 0);

    u8bUnMap(arena);
    tmp_rm(tmpdir);
    done;
}

// ---- 5. Resolve local ref ----

ok64 REFStest_resolve_local() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/refs-rl-XXXXXX";
    want(tmp_make(tmpdir) == OK);

    call(append_kv, tmpdir,
         "?heads/main",
         "?cafef00dcafef00dcafef00dcafef00dcafef00d");

    Bu8 arena = {};
    call(u8bMap, arena, 4096);
    uri res = {};
    a_cstr(dir_s, tmpdir);
    CSTR_SLICE(q, "?heads/main");
    call(REFSResolve, &res, arena, dir_s, q);
    want(u8csLen(res.query) == 40);
    want(memcmp(res.query[0],
                "cafef00dcafef00dcafef00dcafef00dcafef00d", 40) == 0);
    u8bUnMap(arena);

    //  Short form `?main` — heads|tags variant match.
    Bu8 arena2 = {};
    call(u8bMap, arena2, 4096);
    uri res2 = {};
    CSTR_SLICE(q2, "?main");
    call(REFSResolve, &res2, arena2, dir_s, q2);
    want(u8csLen(res2.query) == 40);
    u8bUnMap(arena2);

    tmp_rm(tmpdir);
    done;
}

// ---- 6. Resolve host alias (substring match) ----

ok64 REFStest_resolve_alias() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/refs-ra-XXXXXX";
    want(tmp_make(tmpdir) == OK);

    //  A real-world-ish alias row: an origin URI key with a tip val.
    call(append_kv, tmpdir,
         "https://github.com/torvalds/linux.git?heads/master",
         "?deadbeefdeadbeefdeadbeefdeadbeefdeadbeef");

    //  `//github?master` should host-substring-match `github.com`
    //  and heads/-variant-match `master` → returns the same sha.
    Bu8 arena = {};
    call(u8bMap, arena, 4096);
    uri res = {};
    a_cstr(dir_s, tmpdir);
    CSTR_SLICE(q, "//github?master");
    call(REFSResolve, &res, arena, dir_s, q);
    want(u8csLen(res.query) == 40);
    want(memcmp(res.query[0],
                "deadbeefdeadbeefdeadbeefdeadbeefdeadbeef", 40) == 0);
    //  scheme/host/path filled from the matched row.
    CSTR_SLICE(exp_scheme, "https");
    want(u8csLen(res.scheme) == u8csLen(exp_scheme));
    want(memcmp(res.scheme[0], exp_scheme[0], u8csLen(exp_scheme)) == 0);
    u8bUnMap(arena);

    tmp_rm(tmpdir);
    done;
}

// ---- 7. Resolve miss ----

ok64 REFStest_resolve_miss() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/refs-rm-XXXXXX";
    want(tmp_make(tmpdir) == OK);

    call(append_kv, tmpdir,
         "?heads/main",
         "?1234567890123456789012345678901234567890");

    Bu8 arena = {};
    call(u8bMap, arena, 4096);
    uri res = {};
    a_cstr(dir_s, tmpdir);
    CSTR_SLICE(q, "?heads/nonexistent");
    ok64 o = REFSResolve(&res, arena, dir_s, q);
    want(o == REFSNONE);
    u8bUnMap(arena);

    tmp_rm(tmpdir);
    done;
}

// ---- 8. SyncRecord: bulk append ----

ok64 REFStest_sync_record() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/refs-sr-XXXXXX";
    want(tmp_make(tmpdir) == OK);
    a_cstr(dir_s, tmpdir);

    //  Build three entries in a local array.  Key/val point at
    //  stable literal storage for the duration of the call.
    ref entries[3] = {};
    CSTR_SLICE(k0, "?heads/main");
    CSTR_SLICE(v0, "?1111111111111111111111111111111111111111");
    CSTR_SLICE(k1, "?heads/feat");
    CSTR_SLICE(v1, "?2222222222222222222222222222222222222222");
    CSTR_SLICE(k2, "?tags/v1");
    CSTR_SLICE(v2, "?3333333333333333333333333333333333333333");
    entries[0].key[0] = k0[0]; entries[0].key[1] = k0[1];
    entries[0].val[0] = v0[0]; entries[0].val[1] = v0[1];
    entries[1].key[0] = k1[0]; entries[1].key[1] = k1[1];
    entries[1].val[0] = v1[0]; entries[1].val[1] = v1[1];
    entries[2].key[0] = k2[0]; entries[2].key[1] = k2[1];
    entries[2].val[0] = v2[0]; entries[2].val[1] = v2[1];

    call(REFSSyncRecord, dir_s, entries, 3);
    want(count_rows(tmpdir) == 3);

    //  All three survive Load.
    Bu8 arena = {};
    call(u8bMap, arena, 4096);
    ref arr[8] = {};
    u32 n = 0;
    call(REFSLoad, arr, &n, 8, arena, dir_s);
    want(n == 3);
    want(find_by_key(arr, n, k0) != NULL);
    want(find_by_key(arr, n, k1) != NULL);
    want(find_by_key(arr, n, k2) != NULL);
    u8bUnMap(arena);

    tmp_rm(tmpdir);
    done;
}

// ---- 9. Compact: shrink to latest-per-key ----

ok64 REFStest_compact() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/refs-co-XXXXXX";
    want(tmp_make(tmpdir) == OK);
    a_cstr(dir_s, tmpdir);

    //  Two keys, two revisions each → four rows.
    call(append_kv, tmpdir, "?heads/main",
         "?1111111111111111111111111111111111111111");
    call(append_kv, tmpdir, "?heads/feat",
         "?aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    call(append_kv, tmpdir, "?heads/main",
         "?2222222222222222222222222222222222222222");
    call(append_kv, tmpdir, "?heads/feat",
         "?bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    want(count_rows(tmpdir) == 4);
    ssize_t before = file_bytes(tmpdir);
    want(before > 0);

    call(REFSCompact, dir_s);

    //  Post-compact: one row per key.
    want(count_rows(tmpdir) == 2);
    ssize_t after = file_bytes(tmpdir);
    want(after > 0 && after < before);

    //  Semantics preserved — Load still returns the same latest-per-key.
    Bu8 arena = {};
    call(u8bMap, arena, 4096);
    ref arr[8] = {};
    u32 n = 0;
    call(REFSLoad, arr, &n, 8, arena, dir_s);
    want(n == 2);
    CSTR_SLICE(k_main, "?heads/main");
    CSTR_SLICE(k_feat, "?heads/feat");
    ref const *m = find_by_key(arr, n, k_main);
    ref const *f = find_by_key(arr, n, k_feat);
    want(m != NULL && f != NULL);
    CSTR_SLICE(v_main, "?2222222222222222222222222222222222222222");
    CSTR_SLICE(v_feat, "?bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    want(memcmp(m->val[0], v_main[0], u8csLen(v_main)) == 0);
    want(memcmp(f->val[0], v_feat[0], u8csLen(v_feat)) == 0);
    u8bUnMap(arena);

    tmp_rm(tmpdir);
    done;
}

// ---- 10. Monotonicity: consecutive appends get distinct ts ----

ok64 REFStest_monotonic_ts() {
    sane(1);
    call(FILEInit);

    char tmpdir[] = "/tmp/refs-mo-XXXXXX";
    want(tmp_make(tmpdir) == OK);

    //  Rapid appends — even if RONNow() returns the same ms twice,
    //  REFSAppend clamps ts above the tail.  Both rows must land.
    for (int i = 0; i < 5; i++) {
        char val[64];
        snprintf(val, sizeof(val),
                 "?%040d", i);
        call(append_kv, tmpdir, "?heads/main", val);
    }
    want(count_rows(tmpdir) == 5);

    //  Re-open the ULOG, walk rows, verify strictly increasing ts.
    a_cstr(dir_s, tmpdir);
    a_path(fname, dir_s);
    LIT(refs_seg, "refs");
    want(PATHu8bPush(fname, refs_seg) == OK);
    a_dup(u8c, path, u8bDataC(fname));
    ulog l = {};
    call(ULOGOpen, &l, path);
    u32 nrows = ULOGCount(&l);
    want(nrows == 5);
    ron60 prev = 0;
    for (u32 i = 0; i < nrows; i++) {
        ron60 ts = 0, verb = 0;
        uri u = {};
        call(ULOGRow, &l, i, &ts, &verb, &u);
        want(ts > prev);
        prev = ts;
    }
    ULOGClose(&l);

    tmp_rm(tmpdir);
    done;
}

// --- main ---

ok64 maintest() {
    sane(1);
    fprintf(stderr, "REFStest_empty...\n");         call(REFStest_empty);
    fprintf(stderr, "REFStest_append_load...\n");   call(REFStest_append_load);
    fprintf(stderr, "REFStest_dedup_latest...\n");  call(REFStest_dedup_latest);
    fprintf(stderr, "REFStest_multi_key...\n");     call(REFStest_multi_key);
    fprintf(stderr, "REFStest_resolve_local...\n"); call(REFStest_resolve_local);
    fprintf(stderr, "REFStest_resolve_alias...\n"); call(REFStest_resolve_alias);
    fprintf(stderr, "REFStest_resolve_miss...\n");  call(REFStest_resolve_miss);
    fprintf(stderr, "REFStest_sync_record...\n");   call(REFStest_sync_record);
    fprintf(stderr, "REFStest_compact...\n");       call(REFStest_compact);
    fprintf(stderr, "REFStest_monotonic_ts...\n");  call(REFStest_monotonic_ts);
    fprintf(stderr, "all passed\n");
    done;
}

TEST(maintest)
