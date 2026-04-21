//  SYNCFRAME — round-trip encoding of SYNC TLV records.

#include "keeper/SYNC.h"

#include "abc/PRO.h"
#include "abc/TEST.h"

ok64 hellotest() {
    sane(1);
    a_pad(u8, out, 32);
    call(SYNCFeedHello, out, SYNC_VERSION, SYNC_VERB_GET);

    a_dup(u8c, data, u8bData(out));
    u8 tag = 0;
    u8cs body;
    call(SYNCDrain, data, &tag, body);
    want(tag == SYNC_TAG_HELLO);

    u8 ver = 0, verb = 0;
    call(SYNCParseHello, body, &ver, &verb);
    want(ver  == SYNC_VERSION);
    want(verb == SYNC_VERB_GET);
    want($empty(data));
    done;
}

ok64 watertest_empty() {
    sane(1);
    a_pad(u8, out, 32);
    call(SYNCFeedWater, out, NULL);

    a_dup(u8c, data, u8bData(out));
    u8 tag = 0;
    u8cs body;
    call(SYNCDrain, data, &tag, body);
    want(tag == SYNC_TAG_WATER);

    sync_wm wm = {.pack_seq = 999, .pack_len = 999, .reflog_len = 999};
    b8 present = YES;
    call(SYNCParseWater, body, &wm, &present);
    want(present == NO);
    want(wm.pack_seq == 0);
    done;
}

ok64 watertest_full() {
    sane(1);
    a_pad(u8, out, 32);
    sync_wm in = {.pack_seq = 0xdeadbeef, .pack_len = 0x0123456789abcdefULL,
                  .reflog_len = 0xcafebabe12345678ULL};
    call(SYNCFeedWater, out, &in);

    a_dup(u8c, data, u8bData(out));
    u8 tag = 0;
    u8cs body;
    call(SYNCDrain, data, &tag, body);

    sync_wm rt = {};
    b8 present = NO;
    call(SYNCParseWater, body, &rt, &present);
    want(present == YES);
    want(rt.pack_seq == in.pack_seq);
    want(rt.pack_len == in.pack_len);
    want(rt.reflog_len == in.reflog_len);
    done;
}

ok64 listtest_bookmark() {
    sane(1);
    a_pad(u8, out, 64);
    wh128 bm = {.key = 0xaaaaaaaa12345678ULL,
                .val = 0xbbbbbbbb87654321ULL};
    call(SYNCFeedList, out, &bm);

    a_dup(u8c, data, u8bData(out));
    u8 tag = 0;
    u8cs body;
    call(SYNCDrain, data, &tag, body);
    want(tag == SYNC_TAG_LIST);

    wh128 rt = {};
    u64 sl = 999;
    b8 is_sent = YES;
    call(SYNCParseList, body, &rt, &sl, &is_sent);
    want(is_sent == NO);
    want(rt.key == bm.key);
    want(rt.val == bm.val);
    done;
}

ok64 listtest_sentinel() {
    sane(1);
    a_pad(u8, out, 64);
    call(SYNCFeedSentinel, out, 0x1234567890abcdefULL);

    a_dup(u8c, data, u8bData(out));
    u8 tag = 0;
    u8cs body;
    call(SYNCDrain, data, &tag, body);
    want(tag == SYNC_TAG_LIST);

    wh128 rt = {.key = 1, .val = 1};
    u64 sl = 0;
    b8 is_sent = NO;
    call(SYNCParseList, body, &rt, &sl, &is_sent);
    want(is_sent == YES);
    want(sl == 0x1234567890abcdefULL);
    want(rt.key == 0);
    want(rt.val == 0);
    done;
}

ok64 endtest() {
    sane(1);
    a_pad(u8, out, 8);
    call(SYNCFeedEnd, out);

    a_dup(u8c, data, u8bData(out));
    u8 tag = 0;
    u8cs body;
    call(SYNCDrain, data, &tag, body);
    want(tag == SYNC_TAG_END);
    want($empty(body));
    done;
}

ok64 packtest_small() {
    sane(1);
    u8 bytes[40];
    for (int i = 0; i < 40; i++) bytes[i] = (u8)i;
    u8csc body = {bytes, bytes + 40};
    a_pad(u8, out, 64);
    call(SYNCFeedPack, out, body);

    a_dup(u8c, data, u8bData(out));
    u8 tag = 0;
    u8cs got;
    call(SYNCDrain, data, &tag, got);
    want(tag == SYNC_TAG_PACK);
    want(u8csLen(got) == 40);
    for (int i = 0; i < 40; i++) want(got[0][i] == (u8)i);
    done;
}

ok64 packtest_long() {
    sane(1);
    //  Body > 255 bytes triggers TLV long form.
    u64 n = 1024;
    Bu8 body_b = {};
    call(u8bAllocate, body_b, n);
    for (u64 i = 0; i < n; i++) u8bFeed1(body_b, (u8)(i & 0xff));
    a_dup(u8c, body, u8bData(body_b));

    Bu8 out = {};
    call(u8bAllocate, out, n + 16);
    call(SYNCFeedPack, out, body);

    a_dup(u8c, data, u8bData(out));
    u8 tag = 0;
    u8cs got;
    call(SYNCDrain, data, &tag, got);
    want(tag == SYNC_TAG_PACK);
    want(u8csLen(got) == n);
    for (u64 i = 0; i < n; i++) want(got[0][i] == (u8)(i & 0xff));

    u8bFree(body_b);
    u8bFree(out);
    done;
}

ok64 errortest() {
    sane(1);
    a_pad(u8, out, 64);
    a_cstr(m, "unrelated histories");
    call(SYNCFeedError, out, SYNCUNRELAT, m);

    a_dup(u8c, data, u8bData(out));
    u8 tag = 0;
    u8cs body;
    call(SYNCDrain, data, &tag, body);
    want(tag == SYNC_TAG_ERROR);

    ok64 code = 0;
    u8cs msg;
    call(SYNCParseError, body, &code, msg);
    want(code == SYNCUNRELAT);
    want(u8csLen(msg) == u8csLen(m));
    done;
}

ok64 maintest() {
    sane(1);
    call(hellotest);
    call(watertest_empty);
    call(watertest_full);
    call(listtest_bookmark);
    call(listtest_sentinel);
    call(endtest);
    call(packtest_small);
    call(packtest_long);
    call(errortest);
    done;
}

TEST(maintest)
