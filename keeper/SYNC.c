#include "keeper/SYNC.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/TLV.h"
#include "dog/WHIFF.h"
#include "keeper/REFS.h"
#include "keeper/SHA1.h"

//  -- little-endian helpers --

fun void pack_u32_le(u8 *dst, u32 v) {
    dst[0] = (u8)(v);
    dst[1] = (u8)(v >> 8);
    dst[2] = (u8)(v >> 16);
    dst[3] = (u8)(v >> 24);
}

fun u32 read_u32_le(u8c *src) {
    return (u32)src[0] | ((u32)src[1] << 8) |
           ((u32)src[2] << 16) | ((u32)src[3] << 24);
}

fun void pack_u64_le(u8 *dst, u64 v) {
    for (int i = 0; i < 8; i++) dst[i] = (u8)(v >> (i * 8));
}

fun u64 read_u64_le(u8c *src) {
    u64 v = 0;
    for (int i = 0; i < 8; i++) v |= ((u64)src[i]) << (i * 8);
    return v;
}

//  Feed a TLV record into buffer `out`.
static ok64 feed_tlv(u8bp out, u8 tag, u8csc body) {
    sane(out && u8bOK(out));
    u64 need = TLVlen(u8csLen(body));
    test(u8bIdleLen(out) >= need, TLVNOROOM);
    call(TLVu8sFeed, u8bIdle(out), tag, body);
    done;
}

//  -- feeders --

ok64 SYNCFeedHello(u8bp out, u8 version, u8 verb) {
    sane(out);
    u8 buf[2] = {version, verb};
    u8csc body = {buf, buf + 2};
    call(feed_tlv, out, SYNC_TAG_HELLO, body);
    done;
}

ok64 SYNCFeedWater(u8bp out, sync_wm const *wm) {
    sane(out);
    if (wm == NULL) {
        u8csc empty = {};
        call(feed_tlv, out, SYNC_TAG_WATER, empty);
        done;
    }
    u8 buf[20];
    pack_u32_le(buf, wm->pack_seq);
    pack_u64_le(buf + 4, wm->pack_len);
    pack_u64_le(buf + 12, wm->reflog_len);
    u8csc body = {buf, buf + 20};
    call(feed_tlv, out, SYNC_TAG_WATER, body);
    done;
}

ok64 SYNCFeedList(u8bp out, wh128 const *bookmark) {
    sane(out && bookmark);
    u8 buf[16];
    pack_u64_le(buf, bookmark->key);
    pack_u64_le(buf + 8, bookmark->val);
    u8csc body = {buf, buf + 16};
    call(feed_tlv, out, SYNC_TAG_LIST, body);
    done;
}

ok64 SYNCFeedSentinel(u8bp out, u64 final_length) {
    sane(out);
    u8 buf[8];
    pack_u64_le(buf, final_length);
    u8csc body = {buf, buf + 8};
    call(feed_tlv, out, SYNC_TAG_LIST, body);
    done;
}

ok64 SYNCFeedEnd(u8bp out) {
    sane(out);
    u8csc empty = {};
    call(feed_tlv, out, SYNC_TAG_END, empty);
    done;
}

ok64 SYNCFeedPack(u8bp out, u8csc body) {
    sane(out);
    call(feed_tlv, out, SYNC_TAG_PACK, body);
    done;
}

ok64 SYNCFeedRefs(u8bp out, u8csc body) {
    sane(out);
    call(feed_tlv, out, SYNC_TAG_REFS, body);
    done;
}

ok64 SYNCFeedError(u8bp out, ok64 code, u8csc msg) {
    sane(out);
    u64 mlen = $ok(msg) ? u8csLen(msg) : 0;
    test(mlen <= 1024, SYNCFAIL);
    u8 body[1024 + 8];
    pack_u64_le(body, (u64)code);
    for (u64 i = 0; i < mlen; i++) body[8 + i] = msg[0][i];
    u8csc bs = {body, body + 8 + mlen};
    return feed_tlv(out, SYNC_TAG_ERROR, bs);
}

//  -- drainer --

ok64 SYNCDrain(u8cs from, u8p tag, u8csp value) {
    sane(from && tag && value);
    return TLVu8sDrain(from, tag, value);
}

//  -- body parsers --

ok64 SYNCParseHello(u8cs body, u8 *version, u8 *verb) {
    sane(body && version && verb);
    test(u8csLen(body) == 2, SYNCBADREC);
    *version = body[0][0];
    *verb    = body[0][1];
    done;
}

ok64 SYNCParseWater(u8cs body, sync_wm *wm, b8 *present) {
    sane(body && wm && present);
    u64 n = u8csLen(body);
    if (n == 0) {
        wm->pack_seq = 0;
        wm->pack_len = 0;
        wm->reflog_len = 0;
        *present = NO;
        done;
    }
    test(n == 20, SYNCBADREC);
    wm->pack_seq   = read_u32_le(body[0]);
    wm->pack_len   = read_u64_le(body[0] + 4);
    wm->reflog_len = read_u64_le(body[0] + 12);
    *present = YES;
    done;
}

ok64 SYNCParseList(u8cs body, wh128 *bookmark, u64 *sentinel_len,
                   b8 *is_sentinel) {
    sane(body && bookmark && sentinel_len && is_sentinel);
    u64 n = u8csLen(body);
    if (n == 8) {
        *sentinel_len = read_u64_le(body[0]);
        bookmark->key = 0;
        bookmark->val = 0;
        *is_sentinel = YES;
        done;
    }
    test(n == 16, SYNCBADREC);
    bookmark->key = read_u64_le(body[0]);
    bookmark->val = read_u64_le(body[0] + 8);
    *sentinel_len = 0;
    *is_sentinel = NO;
    done;
}

ok64 SYNCParseError(u8cs body, ok64 *code, u8csp msg) {
    sane(body && code && msg);
    u64 n = u8csLen(body);
    test(n >= 8, SYNCBADREC);
    *code = (ok64)read_u64_le(body[0]);
    msg[0] = body[0] + 8;
    msg[1] = body[0] + n;
    done;
}

//  -- I/O helpers over a byte-stream fd --

//  Drain one TLV record from `buf->DATA`, reading more from `fd` if
//  needed.  On success DATA's head has advanced past the record and
//  `tag` + `body` describe it.  `body` points into the buffer — it
//  stays valid until the next read_tlv call.
static ok64 read_tlv(int fd, u8bp buf, u8p tag, u8csp body) {
    sane(fd >= 0 && buf && tag && body);
    for (;;) {
        u64 datalen0 = u8bDataLen(buf);
        a_dup(u8c, data, u8bDataC(buf));
        ok64 rc = TLVu8sDrain(data, tag, body);
        if (rc == OK) {
            u64 consumed = datalen0 - u8csLen(data);
            u8bUsed(buf, consumed);
            done;
        }
        if (rc != TLVNODATA) return rc;
        test(u8bIdleLen(buf) > 0, TLVNOROOM);
        rc = FILEDrain(fd, u8bIdle(buf));
        if (rc == FILEEND) return TLVNODATA;
        if (rc != OK) return rc;
    }
}

//  Flush DATA to `fd`, reset buffer.
static ok64 flush_buf(int fd, u8bp out) {
    sane(fd >= 0 && out);
    if (u8bDataLen(out) == 0) done;
    a_dup(u8c, data, u8bDataC(out));
    call(FILEFeedAll, fd, data);
    u8bReset(out);
    done;
}

//  Emit one Q record directly to `fd` carrying the full pack file
//  bytes of `pack_buf` (whatever the mapped log file currently holds).
static ok64 send_pack_file(int fd, u8bp pack_buf) {
    sane(fd >= 0 && pack_buf);
    u64 flen = u8bDataLen(pack_buf);
    if (flen == 0) done;
    u8cp base = u8bDataHead(pack_buf);
    u8csc body = {base, base + flen};
    u64 need = TLVlen(flen);
    Bu8 out = {};
    call(u8bMap, out, need);
    call(SYNCFeedPack, out, body);
    a_dup(u8c, od, u8bDataC(out));
    call(FILEFeedAll, fd, od);
    u8bUnMap(out);
    done;
}

//  Emit the whole REFS file bytes as one R record.
static ok64 send_refs(int fd, keeper *k) {
    sane(fd >= 0 && k);
    a_path(kdir, u8bDataC(k->h->root), KEEP_DIR_S);
    a_cstr(refs_rel, "/" REFS_FILE);
    a_pad(u8, refspath, 1024);
    call(u8bFeed, refspath, $path(kdir));
    call(u8bFeed, refspath, refs_rel);
    call(PATHu8bTerm, refspath);

    u8bp rmap = NULL;
    ok64 mo = FILEMapRO(&rmap, $path(refspath));
    if (mo != OK) done;  // no refs file yet — nothing to ship
    u64 rlen = u8bDataLen(rmap);
    if (rlen > 0) {
        u8cp rbase = u8bDataHead(rmap);
        u8csc body = {rbase, rbase + rlen};
        u64 need = TLVlen(rlen);
        Bu8 out = {};
        call(u8bMap, out, need);
        call(SYNCFeedRefs, out, body);
        a_dup(u8c, od, u8bDataC(out));
        call(FILEFeedAll, fd, od);
        u8bUnMap(out);
    }
    FILEUnMap(rmap);
    done;
}

//  Receive Q* E → ingest each as a new local pack file.
//  Returns counts via out params (may be NULL).
static ok64 recv_packs(int in_fd, u8bp in_buf, keeper *k,
                       u32 *out_count, u64 *out_bytes) {
    sane(in_fd >= 0 && in_buf && k);
    u32 n = 0;
    u64 bytes = 0;
    for (;;) {
        u8 tag = 0;
        u8cs body = {};
        call(read_tlv, in_fd, in_buf, &tag, body);
        if (tag == SYNC_TAG_END) break;
        test(tag == SYNC_TAG_PACK, SYNCPROTO);
        u8csc pb = {body[0], body[1]};
        call(KEEPIngestFile, k, pb);
        n++;
        bytes += u8csLen(body);
    }
    if (out_count) *out_count = n;
    if (out_bytes) *out_bytes = bytes;
    done;
}

//  Receive R* E → dedup-append to REFS.
static ok64 recv_refs(int in_fd, u8bp in_buf, keeper *k,
                      u32 *out_count, u64 *out_bytes) {
    sane(in_fd >= 0 && in_buf && k);
    a_path(kdir, u8bDataC(k->h->root), KEEP_DIR_S);
    u32 n = 0;
    u64 bytes = 0;
    for (;;) {
        u8 tag = 0;
        u8cs body = {};
        call(read_tlv, in_fd, in_buf, &tag, body);
        if (tag == SYNC_TAG_END) break;
        test(tag == SYNC_TAG_REFS, SYNCPROTO);
        u8csc rb = {body[0], body[1]};
        call(REFSAppendTail, $path(kdir), rb);
        n++;
        bytes += u8csLen(body);
    }
    if (out_count) *out_count = n;
    if (out_bytes) *out_bytes = bytes;
    done;
}

//  -- server flows --

static ok64 serve_get(keeper *k, int out_fd, sync_wm const *wm) {
    sane(k && out_fd >= 0 && wm);
    (void)wm;  // MVP: full sync, ignore watermark

    //  One Q per local log file (mmapped in k->shards[0].packs).
    for (u32 i = 0; i < k->shards[0].npacks; i++) {
        call(send_pack_file, out_fd, k->shards[0].packs[i]);
    }
    //  Terminate Q stream.
    {
        Bu8 tmp = {};
        call(u8bAllocate, tmp, 16);
        call(SYNCFeedEnd, tmp);
        a_dup(u8c, td, u8bDataC(tmp));
        call(FILEFeedAll, out_fd, td);
        u8bFree(tmp);
    }

    //  Ship REFS tail (MVP: whole file).
    call(send_refs, out_fd, k);
    {
        Bu8 tmp = {};
        call(u8bAllocate, tmp, 16);
        call(SYNCFeedEnd, tmp);
        a_dup(u8c, td, u8bDataC(tmp));
        call(FILEFeedAll, out_fd, td);
        u8bFree(tmp);
    }
    done;
}

//  `in_buf` already holds bytes past the H/W prefix — keep them.
static ok64 serve_post(keeper *k, int in_fd, int out_fd,
                       u8bp in_buf, sync_wm const *wm) {
    sane(k && in_fd >= 0 && out_fd >= 0 && in_buf && wm);
    (void)wm;  // MVP: full sync, ignore watermark

    //  Receive Q* E (pack files), then R* E (reflog tail).  The caller's
    //  buffer may already hold bytes past H+W (TLV was short-read).
    u32 qn = 0, rn = 0;
    u64 qb = 0, rb = 0;
    ok64 rp = recv_packs(in_fd, in_buf, k, &qn, &qb);
    if (rp != OK) return rp;
    ok64 rr = recv_refs(in_fd, in_buf, k, &rn, &rb);
    if (rr != OK) return rr;

    fprintf(stderr, "sync: ingested %u pack(s) %llu B, %u refs chunk(s) %llu B\n",
            qn, (unsigned long long)qb, rn, (unsigned long long)rb);

    //  Ack: final E.
    Bu8 out = {};
    call(u8bAllocate, out, 16);
    call(SYNCFeedEnd, out);
    a_dup(u8c, od, u8bDataC(out));
    call(FILEFeedAll, out_fd, od);
    u8bFree(out);
    done;
}

ok64 SYNCServe(keeper *k, int in_fd, int out_fd) {
    sane(k && in_fd >= 0 && out_fd >= 0);
    //  A VA-mapped 256 MiB buffer covers the largest reasonable single
    //  record (a whole pack file); post needs this to stay alive across
    //  H/W and the Q/R records that follow.
    Bu8 in = {};
    call(u8bMap, in, 256ULL << 20);

    u8 tag = 0;
    u8cs body = {};
    call(read_tlv, in_fd, in, &tag, body);
    test(tag == SYNC_TAG_HELLO, SYNCPROTO);
    u8 ver = 0, verb = 0;
    call(SYNCParseHello, body, &ver, &verb);
    test(ver == SYNC_VERSION, SYNCPROTO);

    call(read_tlv, in_fd, in, &tag, body);
    test(tag == SYNC_TAG_WATER, SYNCPROTO);
    sync_wm wm = {};
    b8 present = NO;
    call(SYNCParseWater, body, &wm, &present);

    ok64 rc;
    if (verb == SYNC_VERB_GET)       rc = serve_get(k, out_fd, &wm);
    else if (verb == SYNC_VERB_POST) rc = serve_post(k, in_fd, out_fd, in, &wm);
    else                             rc = SYNCPROTO;

    u8bUnMap(in);
    return rc;
}

//  -- client transport: spawn server via ssh/sh --

static b8 starts_with(u8csc s, char const *pfx) {
    u64 n = strlen(pfx);
    if (u8csLen(s) < n) return NO;
    return memcmp(s[0], pfx, n) == 0;
}

static ok64 spawn_sh(u8cs shcmd, int *wfd, int *rfd, pid_t *pid) {
    a_cstr(sh_path, "/bin/sh");
    u8cs argv_sh  = u8slit("sh");
    u8cs argv_c   = u8slit("-c");
    u8cs argv_arr[3];
    argv_arr[0][0] = argv_sh[0]; argv_arr[0][1] = argv_sh[1];
    argv_arr[1][0] = argv_c[0];  argv_arr[1][1] = argv_c[1];
    argv_arr[2][0] = shcmd[0];   argv_arr[2][1] = shcmd[1];
    u8css argv = {argv_arr, argv_arr + 3};
    return FILESpawn(sh_path, argv, wfd, rfd, pid);
}

//  `be://host/path` → ssh host "cd 'path' && exec keeper sync"
//  `file:///path`   → sh -c "cd 'path' && exec keeper sync"
//  The path is relative to the remote `$HOME` after the ssh login
//  (which starts in $HOME).  Single-quote escaping is not bullet-
//  proof; test infrastructure must avoid paths containing quotes.
static ok64 spawn_server(u8csc uri, int *wfd, int *rfd, pid_t *pid) {
    sane($ok(uri) && wfd && rfd && pid);
    Bu8 cmd_b = {};
    call(u8bAllocate, cmd_b, 2048);
    ok64 rc = SYNCFAIL;

    if (starts_with(uri, "file://")) {
        u8c *after = uri[0] + strlen("file://");
        while (after < uri[1] && *after == '/') after++;
        if (after > uri[0]) after--;  // keep one leading slash
        u8cs path = {after, uri[1]};

        a_cstr(prefix, "cd '");
        u8bFeed(cmd_b, prefix);
        u8bFeed(cmd_b, path);
        a_cstr(suffix, "' && exec keeper sync");
        u8bFeed(cmd_b, suffix);
        a_dup(u8c, cmd, u8bData(cmd_b));
        rc = spawn_sh(cmd, wfd, rfd, pid);
    } else if (starts_with(uri, "be://")) {
        u8c *after = uri[0] + strlen("be://");
        u8c *slash = after;
        while (slash < uri[1] && *slash != '/') slash++;
        if (slash == uri[1]) { u8bFree(cmd_b); return SYNCBADREC; }
        u8cs host = {after, slash};
        //  Skip leading '/' so the remote `cd` is relative to $HOME.
        u8c *path_start = slash + 1;
        while (path_start < uri[1] && *path_start == '/') path_start++;
        u8cs path = {path_start, uri[1]};

        //  $DOG_REMOTE_PATH is prepended to the remote PATH so test
        //  harnesses can point at an out-of-tree `keeper` binary
        //  (e.g. `build-debug/bin/`) without touching the remote's
        //  shell rc files.  Empty/unset = vanilla login PATH.
        char const *rpath = getenv("DOG_REMOTE_PATH");
        a_cstr(pre1, "ssh -o BatchMode=yes '");
        u8bFeed(cmd_b, pre1);
        u8bFeed(cmd_b, host);
        if (rpath && *rpath) {
            a_cstr(pre2, "' \"export PATH='");
            u8bFeed(cmd_b, pre2);
            a_cstr(rp, rpath);
            u8bFeed(cmd_b, rp);
            a_cstr(pre3, "':\\$PATH; cd '");
            u8bFeed(cmd_b, pre3);
        } else {
            a_cstr(pre2, "' \"cd '");
            u8bFeed(cmd_b, pre2);
        }
        u8bFeed(cmd_b, path);
        a_cstr(post, "' && exec keeper sync\"");
        u8bFeed(cmd_b, post);
        a_dup(u8c, cmd, u8bData(cmd_b));
        rc = spawn_sh(cmd, wfd, rfd, pid);
    } else {
        rc = SYNCBADREC;
    }
    u8bFree(cmd_b);
    return rc;
}

//  Read the final E from `rfd` (drain remaining bytes until EOF or
//  an END record).  Used by SYNCPost to wait for server ack.
static ok64 read_final_e(int rfd, u8bp buf) {
    sane(rfd >= 0 && buf);
    u8 tag = 0;
    u8cs body = {};
    ok64 rc = read_tlv(rfd, buf, &tag, body);
    if (rc == TLVNODATA) fail(SYNCPROTO);
    if (rc != OK) return rc;
    test(tag == SYNC_TAG_END, SYNCPROTO);
    done;
}

//  -- client: Get (pull) --

ok64 SYNCGet(keeper *k, u8csc uri) {
    sane(k && $ok(uri));
    int wfd = -1, rfd = -1;
    pid_t pid = 0;
    call(spawn_server, uri, &wfd, &rfd, &pid);

    //  Send H + W(empty: full sync).
    Bu8 out = {};
    call(u8bAllocate, out, 128);
    call(SYNCFeedHello, out, SYNC_VERSION, SYNC_VERB_GET);
    call(SYNCFeedWater, out, NULL);
    a_dup(u8c, ob, u8bDataC(out));
    call(FILEFeedAll, wfd, ob);
    close(wfd);  // half-close: no more to send
    u8bFree(out);

    //  Receive Q* E (ingest each), then R* E (dedup-append).
    Bu8 in = {};
    call(u8bMap, in, 256ULL << 20);
    u32 qn = 0, rn = 0;
    u64 qb = 0, rb = 0;
    ok64 rp = recv_packs(rfd, in, k, &qn, &qb);
    if (rp != OK) { u8bUnMap(in); close(rfd); waitpid(pid, NULL, 0); return rp; }
    ok64 rr = recv_refs(rfd, in, k, &rn, &rb);
    u8bUnMap(in);
    close(rfd);
    int st = 0;
    waitpid(pid, &st, 0);
    if (rr != OK) return rr;

    fprintf(stderr, "sync: received %u pack(s), %llu byte(s)\n",
            qn, (unsigned long long)qb);
    fprintf(stderr, "sync: received %u reflog chunk(s), %llu byte(s)\n",
            rn, (unsigned long long)rb);
    done;
}

//  -- client: Post (push) --

ok64 SYNCPost(keeper *k, u8csc uri) {
    sane(k && $ok(uri));
    int wfd = -1, rfd = -1;
    pid_t pid = 0;
    call(spawn_server, uri, &wfd, &rfd, &pid);

    //  Send H + W(empty) + Q* E + R* E.
    Bu8 hdr = {};
    call(u8bAllocate, hdr, 128);
    call(SYNCFeedHello, hdr, SYNC_VERSION, SYNC_VERB_POST);
    call(SYNCFeedWater, hdr, NULL);
    a_dup(u8c, hd, u8bDataC(hdr));
    call(FILEFeedAll, wfd, hd);
    u8bFree(hdr);

    //  Q per local pack file.
    for (u32 i = 0; i < k->shards[0].npacks; i++) {
        call(send_pack_file, wfd, k->shards[0].packs[i]);
    }
    {
        Bu8 tmp = {};
        call(u8bAllocate, tmp, 16);
        call(SYNCFeedEnd, tmp);
        a_dup(u8c, td, u8bDataC(tmp));
        call(FILEFeedAll, wfd, td);
        u8bFree(tmp);
    }

    //  R: whole REFS file.
    call(send_refs, wfd, k);
    {
        Bu8 tmp = {};
        call(u8bAllocate, tmp, 16);
        call(SYNCFeedEnd, tmp);
        a_dup(u8c, td, u8bDataC(tmp));
        call(FILEFeedAll, wfd, td);
        u8bFree(tmp);
    }
    close(wfd);

    //  Wait for server ack.
    Bu8 in = {};
    call(u8bAllocate, in, 1u << 16);
    ok64 ae = read_final_e(rfd, in);
    u8bFree(in);
    close(rfd);
    int st = 0;
    waitpid(pid, &st, 0);
    if (ae != OK) return ae;
    fprintf(stderr, "sync: post ack\n");
    done;
}
