#include "ROCK.h"

#include <stdlib.h>
#include <string.h>

#include "FILE.h"
#include "PRO.h"
#include "TEST.h"

// Test 1: open/close, reopen, verify
ok64 ROCKtest1() {
    sane(1);
    a_path(path, "/tmp");
    a_cstr(tmpl, "ROCKtest1_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);
    ROCKdb db = {};
    call(ROCKOpen, &db, path8cgIn(path));
    want(db.db != NULL);
    call(ROCKClose, &db);
    want(db.db == NULL);
    // reopen
    call(ROCKOpen, &db, path8cgIn(path));
    want(db.db != NULL);
    call(ROCKClose, &db);
    call(FILErmrf, path8cgIn(path));
    done;
}

// Test 2: put/get roundtrip
ok64 ROCKtest2() {
    sane(1);
    a_path(path, "/tmp");
    a_cstr(tmpl, "ROCKtest2_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);
    ROCKdb db = {};
    call(ROCKOpen, &db, path8cgIn(path));

    u8cs keys[] = {
        $u8str("key1"),
        $u8str("key2"),
        $u8str("key3"),
        $u8str("hello"),
    };
    u8cs vals[] = {
        $u8str("val1"),
        $u8str("val2"),
        $u8str("val3"),
        $u8str("world"),
    };
    int n = sizeof(keys) / sizeof(keys[0]);

    for (int i = 0; i < n; i++) {
        call(ROCKPut, &db, keys[i], vals[i]);
    }

    for (int i = 0; i < n; i++) {
        aBpad2(u8, buf, 256);
        call(ROCKGet, &db, bufbuf, keys[i]);
        u8cs got = {bufdata[0], bufidle[0]};
        want($eq(got, vals[i]));
    }

    call(ROCKClose, &db);
    call(FILErmrf, path8cgIn(path));
    done;
}

// Test 3: delete
ok64 ROCKtest3() {
    sane(1);
    a_path(path, "/tmp");
    a_cstr(tmpl, "ROCKtest3_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);
    ROCKdb db = {};
    call(ROCKOpen, &db, path8cgIn(path));

    u8cs key = $u8str("delme");
    u8cs val = $u8str("gone");
    call(ROCKPut, &db, key, val);

    aBpad2(u8, buf, 256);
    call(ROCKGet, &db, bufbuf, key);
    u8cs got = {bufdata[0], bufidle[0]};
    want($eq(got, val));

    call(ROCKDel, &db, key);
    aBpad2(u8, buf2, 256);
    ok64 o = ROCKGet(&db, buf2buf, key);
    same(o, ROCKNONE);

    call(ROCKClose, &db);
    call(FILErmrf, path8cgIn(path));
    done;
}

// Test 4: iterator scan (forward order)
ok64 ROCKtest4() {
    sane(1);
    a_path(path, "/tmp");
    a_cstr(tmpl, "ROCKtest4_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);
    ROCKdb db = {};
    call(ROCKOpen, &db, path8cgIn(path));

    u8cs keys[] = {
        $u8str("aaa"),
        $u8str("bbb"),
        $u8str("ccc"),
        $u8str("ddd"),
    };
    u8cs vals[] = {
        $u8str("v1"),
        $u8str("v2"),
        $u8str("v3"),
        $u8str("v4"),
    };
    int n = sizeof(keys) / sizeof(keys[0]);

    for (int i = 0; i < n; i++) {
        call(ROCKPut, &db, keys[i], vals[i]);
    }

    ROCKiter it = {};
    call(ROCKIterOpen, &it, &db);
    call(ROCKIterSeekFirst, &it);
    int count = 0;
    while (ROCKIterValid(&it)) {
        u8cs k = {};
        u8cs v = {};
        ROCKIterKey(&it, k);
        ROCKIterVal(&it, v);
        want(count < n);
        want($eq(k, keys[count]));
        want($eq(v, vals[count]));
        count++;
        call(ROCKIterNext, &it);
    }
    same(count, n);

    call(ROCKIterClose, &it);
    call(ROCKClose, &db);
    call(FILErmrf, path8cgIn(path));
    done;
}

// Test 5: iterator seek
ok64 ROCKtest5() {
    sane(1);
    a_path(path, "/tmp");
    a_cstr(tmpl, "ROCKtest5_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);
    ROCKdb db = {};
    call(ROCKOpen, &db, path8cgIn(path));

    u8cs keys[] = {
        $u8str("aaa"),
        $u8str("bbb"),
        $u8str("ccc"),
        $u8str("ddd"),
    };
    for (int i = 0; i < 4; i++) {
        call(ROCKPut, &db, keys[i], keys[i]);
    }

    ROCKiter it = {};
    call(ROCKIterOpen, &it, &db);
    // Seek to "bbb"
    u8cs seek = $u8str("bbb");
    call(ROCKIterSeek, &it, seek);
    want(ROCKIterValid(&it));
    u8cs k = {};
    ROCKIterKey(&it, k);
    want($eq(k, keys[1]));

    // Seek past end
    u8cs seek2 = $u8str("zzz");
    call(ROCKIterSeek, &it, seek2);
    want(!ROCKIterValid(&it));

    call(ROCKIterClose, &it);
    call(ROCKClose, &db);
    call(FILErmrf, path8cgIn(path));
    done;
}

// Test 6: write batch
ok64 ROCKtest6() {
    sane(1);
    a_path(path, "/tmp");
    a_cstr(tmpl, "ROCKtest6_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);
    ROCKdb db = {};
    call(ROCKOpen, &db, path8cgIn(path));

    ROCKbatch wb = {};
    call(ROCKBatchOpen, &wb);
    u8cs k1 = $u8str("batch1");
    u8cs v1 = $u8str("val1");
    u8cs k2 = $u8str("batch2");
    u8cs v2 = $u8str("val2");
    u8cs k3 = $u8str("batch3");
    u8cs v3 = $u8str("val3");
    call(ROCKBatchPut, &wb, k1, v1);
    call(ROCKBatchPut, &wb, k2, v2);
    call(ROCKBatchPut, &wb, k3, v3);
    call(ROCKBatchWrite, &db, &wb);
    call(ROCKBatchClose, &wb);

    // Verify all three written atomically
    aBpad2(u8, buf1, 256);
    call(ROCKGet, &db, buf1buf, k1);
    u8cs got1 = {buf1data[0], buf1idle[0]};
    want($eq(got1, v1));

    aBpad2(u8, buf2, 256);
    call(ROCKGet, &db, buf2buf, k2);
    u8cs got2 = {buf2data[0], buf2idle[0]};
    want($eq(got2, v2));

    aBpad2(u8, buf3, 256);
    call(ROCKGet, &db, buf3buf, k3);
    u8cs got3 = {buf3data[0], buf3idle[0]};
    want($eq(got3, v3));

    call(ROCKClose, &db);
    call(FILErmrf, path8cgIn(path));
    done;
}

// Concat merge: simply concatenate all records
fun ok64 ConcatMerge(u8s merged, u8css records) {
    for (int i = 0; i < $len(records); i++) {
        ok64 o = u8sFeed(merged, $at(records, i));
        if (o != OK) return o;
    }
    return OK;
}

// Test 7: merge operator
ok64 ROCKtest7() {
    sane(1);
    a_path(path, "/tmp");
    a_cstr(tmpl, "ROCKtest7_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);
    ROCKdb db = {};
    call(ROCKOpenMerge, &db, path8cgIn(path), ConcatMerge);

    u8cs key = $u8str("mkey");
    u8cs v1 = $u8str("aaa");
    u8cs v2 = $u8str("bbb");
    u8cs v3 = $u8str("ccc");

    call(ROCKPut, &db, key, v1);
    call(ROCKMerge, &db, key, v2);
    call(ROCKMerge, &db, key, v3);

    aBpad2(u8, buf, 256);
    call(ROCKGet, &db, bufbuf, key);
    u8cs got = {bufdata[0], bufidle[0]};
    u8cs expect = $u8str("aaabbbccc");
    want($eq(got, expect));

    call(ROCKClose, &db);
    call(FILErmrf, path8cgIn(path));
    done;
}

// Test 8: ROCKScan prefix callback
typedef struct {
    int count;
    u8 keybuf[256];
    u8 valbuf[256];
    u8cs last_key;
    u8cs last_val;
} ScanCtx;

static ok64 ScanCB(voidp arg, u8cs key, u8cs val) {
    ScanCtx *ctx = (ScanCtx *)arg;
    ctx->count++;
    size_t kl = $len(key);
    if (kl > sizeof(ctx->keybuf)) kl = sizeof(ctx->keybuf);
    memcpy(ctx->keybuf, key[0], kl);
    ctx->last_key[0] = ctx->keybuf;
    ctx->last_key[1] = ctx->keybuf + kl;
    size_t vl = $len(val);
    if (vl > sizeof(ctx->valbuf)) vl = sizeof(ctx->valbuf);
    memcpy(ctx->valbuf, val[0], vl);
    ctx->last_val[0] = ctx->valbuf;
    ctx->last_val[1] = ctx->valbuf + vl;
    return OK;
}

static ok64 ScanStopCB(voidp arg, u8cs key, u8cs val) {
    ScanCtx *ctx = (ScanCtx *)arg;
    ctx->count++;
    if (ctx->count >= 2) return ROCKFAIL;  // stop after 2
    return OK;
}

ok64 ROCKtest8() {
    sane(1);
    a_path(path, "/tmp");
    a_cstr(tmpl, "ROCKtest8_XXXXXX");
    call(path8gAddTmp, path8gIn(path), tmpl);
    ROCKdb db = {};
    call(ROCKOpen, &db, path8cgIn(path));

    // Insert keys with two prefixes
    u8cs k1 = $u8str("proj/aaa");
    u8cs k2 = $u8str("proj/bbb");
    u8cs k3 = $u8str("proj/ccc");
    u8cs k4 = $u8str("other/ddd");
    u8cs v1 = $u8str("v1");
    u8cs v2 = $u8str("v2");
    u8cs v3 = $u8str("v3");
    u8cs v4 = $u8str("v4");
    call(ROCKPut, &db, k1, v1);
    call(ROCKPut, &db, k2, v2);
    call(ROCKPut, &db, k3, v3);
    call(ROCKPut, &db, k4, v4);

    // Scan "proj/" prefix — should get 3 entries
    u8cs pfx = $u8str("proj/");
    ScanCtx ctx = {};
    call(ROCKScan, &db, pfx, ScanCB, &ctx);
    same(ctx.count, 3);
    want($eq(ctx.last_key, k3));
    want($eq(ctx.last_val, v3));

    // Scan "other/" prefix — should get 1 entry
    u8cs pfx2 = $u8str("other/");
    ScanCtx ctx2 = {};
    call(ROCKScan, &db, pfx2, ScanCB, &ctx2);
    same(ctx2.count, 1);
    want($eq(ctx2.last_key, k4));

    // Scan "nope/" prefix — should get 0 entries
    u8cs pfx3 = $u8str("nope/");
    ScanCtx ctx3 = {};
    call(ROCKScan, &db, pfx3, ScanCB, &ctx3);
    same(ctx3.count, 0);

    // Early stop: callback returns non-OK after 2
    ScanCtx ctx4 = {};
    ok64 o = ROCKScan(&db, pfx, ScanStopCB, &ctx4);
    same(ctx4.count, 2);
    same(o, ROCKFAIL);

    call(ROCKClose, &db);
    call(FILErmrf, path8cgIn(path));
    done;
}

ok64 maintest() {
    sane(1);
    call(ROCKtest1);
    call(ROCKtest2);
    call(ROCKtest3);
    call(ROCKtest4);
    call(ROCKtest5);
    call(ROCKtest6);
    call(ROCKtest7);
    call(ROCKtest8);
    done;
}

TEST(maintest)
