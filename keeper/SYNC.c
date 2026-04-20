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

//  wh128 sort template (needed for sorting pack bookmarks by val).
#define X(M, name) M##wh128##name
#include "abc/QSORTx.h"
#undef X

//  -- helpers --

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

//  Feed a TLV record with `tag` and body slice into buffer `out`.
//  Appends to IDLE, then shifts the DATA/IDLE boundary.
static ok64 feed_tlv(u8bp out, u8 tag, u8csc body) {
    sane(out && u8bOK(out));
    u64 need = TLVlen(u8csLen(body));
    test(u8bIdleLen(out) >= need, TLVNOROOM);
    //  u8bIdle returns (u8**)&out[2]; TLVu8sFeed advances out[2]
    //  directly, which is the DATA/IDLE boundary — no manual Fed.
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
    //  Error body = 8-byte ok64 + optional utf-8 message.  Cap the
    //  message at 1 KiB; longer errors are a code-level problem.
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

//  -- server-side helpers --

//  Drain exactly one TLV record from `buf->DATA`, reading more from
//  `fd` as needed.  Appends to IDLE then Feds.  On success DATA's
//  head has advanced past the record, and `tag` + `body` describe it.
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
        //  Need more bytes — read into IDLE.
        test(u8bIdleLen(buf) > 0, TLVNOROOM);
        u64 idle_before = u8bIdleLen(buf);
        rc = FILEDrain(fd, u8bIdle(buf));
        if (rc == FILEEND) return TLVNODATA;
        if (rc != OK) return rc;
        //  FILEDrain advanced idle[0] in-place; that bumps buf[2],
        //  which IS the DATA/IDLE border — nothing else to do.
        (void)idle_before;
    }
}

//  Flush all of `out->DATA` to `fd` and reset the buffer.
static ok64 flush_buf(int fd, u8bp out) {
    sane(fd >= 0 && out);
    if (u8bDataLen(out) == 0) done;
    a_dup(u8c, data, u8bDataC(out));
    call(FILEFeedAll, fd, data);
    u8bReset(out);
    done;
}

//  -- server: Get direction --
//
//  Stream every pack bookmark past the client's watermark, then
//  ship the actual pack bytes in the same order, then the reflog
//  tail past the client's reflog_len.  We currently ignore the
//  watermark (full sync) — optimisation is TODO.

static b8 pack_past(sync_wm const *wm, u32 file_id, u64 offset) {
    if (wm->pack_seq == 0 && wm->pack_len == 0) return YES;
    if (file_id > wm->pack_seq) return YES;
    if (file_id < wm->pack_seq) return NO;
    return offset >= wm->pack_len;
}

static ok64 serve_get(keeper *k, int in_fd, int out_fd,
                      sync_wm const *wm) {
    sane(k && out_fd >= 0 && wm);
    (void)in_fd;
    Bu8 out = {};
    call(u8bAllocate, out, 1u << 20);

    //  1. Stream pack bookmarks (sorted ascending by file_id, offset).
    //     Walk each LSM run, collect PACK-type entries, sort, emit.
    //     Number of packs is small, so collecting in a buffer is fine.
    Bwh128 packs_b = {};
    call(wh128bAllocate, packs_b, 4096);
    //  Pack bookmarks: key = wh64Pack(KEEP_TYPE_PACK, file_id, offset).
    //  Keys already sort by (file_id, offset) since type is constant.
    for (u32 r = 0; r < k->nruns; r++) {
        wh128cp base = k->runs[r][0];
        size_t len = (size_t)(k->runs[r][1] - base);
        for (size_t i = 0; i < len; i++) {
            if (wh64Type(base[i].key) != KEEP_TYPE_PACK) continue;
            u32 fid = wh64Id(base[i].key);
            u64 off = wh64Off(base[i].key);
            if (!pack_past(wm, fid, off)) continue;
            wh128bPush(packs_b, &base[i]);
        }
    }
    a_dup(wh128, packs, wh128bData(packs_b));
    wh128sSort(packs);

    //  Emit bookmarks.  Tail sentinel per file_id carries the log
    //  file's current length so the receiver can close the last
    //  pack's byte range.
    u32 last_fid = 0;
    $for(wh128, p, packs) {
        u32 fid = wh64Id(p->key);
        if (last_fid != 0 && fid != last_fid) {
            //  Emit sentinel for previous file.
            u8bp lb = k->packs[last_fid - 1];
            u64 flen = u8bDataLen(lb);
            if (u8bIdleLen(out) < 32) { call(flush_buf, out_fd, out); }
            call(SYNCFeedSentinel, out, flen);
        }
        last_fid = fid;
        if (u8bIdleLen(out) < 32) { call(flush_buf, out_fd, out); }
        call(SYNCFeedList, out, p);
    }
    //  Sentinel for the final file.
    if (last_fid != 0) {
        u8bp lb = k->packs[last_fid - 1];
        u64 flen = u8bDataLen(lb);
        if (u8bIdleLen(out) < 32) { call(flush_buf, out_fd, out); }
        call(SYNCFeedSentinel, out, flen);
    }
    if (u8bIdleLen(out) < 8) { call(flush_buf, out_fd, out); }
    call(SYNCFeedEnd, out);
    call(flush_buf, out_fd, out);

    //  2. Stream pack bytes in the same order.  Each Q carries the
    //     range [offset, next_offset) from the bookmark's file.
    for (u64 i = 0; i < wh128sLen(packs); i++) {
        wh128 *p = &packs[0][i];
        u32 fid = wh64Id(p->key);
        u64 off = wh64Off(p->key);
        u64 end = 0;
        //  Find the next bookmark in the same file, else end = filesize.
        for (u64 j = i + 1; j < wh128sLen(packs); j++) {
            wh128 *q = &packs[0][j];
            if (wh64Id(q->key) == fid) { end = wh64Off(q->key); break; }
        }
        if (end == 0) {
            u8bp lb = k->packs[fid - 1];
            end = u8bDataLen(lb);
        }
        if (end <= off) continue;
        u8bp lb = k->packs[fid - 1];
        u8cp base = u8bDataHead(lb);
        u8csc body = {base + off, base + end};
        //  Pack body can be large — flush any buffered output first,
        //  then feed directly to out_fd (bypass the 1 MiB buffer).
        call(flush_buf, out_fd, out);
        u64 blen = u8csLen(body);
        u64 need = TLVlen(blen);
        Bu8 pbuf = {};
        call(u8bAllocate, pbuf, need);
        call(SYNCFeedPack, pbuf, body);
        a_dup(u8c, pd, u8bDataC(pbuf));
        call(FILEFeedAll, out_fd, pd);
        u8bFree(pbuf);
    }
    call(SYNCFeedEnd, out);
    call(flush_buf, out_fd, out);

    //  3. Stream reflog tail past wm->reflog_len.
    {
        a_path(kdir, u8bDataC(k->h->root), KEEP_DIR_S);
        a_pad(u8, refspath, 1024);
        u8bFeed(refspath, $path(kdir));
        a_cstr(sl, "/REFS");
        u8bFeed(refspath, sl);
        PATHu8bTerm(refspath);
        Bu8 refs = {};
        ok64 rc = FILEMapRO(&refs, $path(refspath));
        if (rc == OK) {
            u64 rlen = u8bDataLen(&refs);
            if (wm->reflog_len < rlen) {
                u8cp base = u8bDataHead(&refs);
                u8csc body = {base + wm->reflog_len, base + rlen};
                u64 blen = u8csLen(body);
                u64 need = TLVlen(blen);
                Bu8 rbuf = {};
                call(u8bAllocate, rbuf, need);
                call(SYNCFeedRefs, rbuf, body);
                a_dup(u8c, rd, u8bDataC(rbuf));
                call(FILEFeedAll, out_fd, rd);
                u8bFree(rbuf);
            }
            FILEUnMap(&refs);
        }
    }
    call(SYNCFeedEnd, out);
    call(flush_buf, out_fd, out);

    wh128bFree(packs_b);
    u8bFree(out);
    done;
}

ok64 SYNCServe(keeper *k, int in_fd, int out_fd) {
    sane(k && in_fd >= 0 && out_fd >= 0);
    Bu8 in = {};
    call(u8bAllocate, in, 1u << 20);

    u8 tag = 0;
    u8cs body;
    call(read_tlv, in_fd, in, &tag, body);
    test(tag == SYNC_TAG_HELLO, SYNCPROTO);
    u8 ver = 0, verb = 0;
    call(SYNCParseHello, body, &ver, &verb);
    test(ver == SYNC_VERSION, SYNCPROTO);

    ok64 rc;
    if (verb == SYNC_VERB_GET) {
        call(read_tlv, in_fd, in, &tag, body);
        test(tag == SYNC_TAG_WATER, SYNCPROTO);
        sync_wm wm = {};
        b8 present = NO;
        call(SYNCParseWater, body, &wm, &present);
        rc = serve_get(k, in_fd, out_fd, &wm);
    } else {
        rc = SYNCPROTO;  // post not implemented yet
    }

    u8bFree(in);
    return rc;
}

//  -- client: Get direction --

//  Simple prefix check: does `s` start with `pfx`?
static b8 starts_with(u8csc s, char const *pfx) {
    u64 n = strlen(pfx);
    if (u8csLen(s) < n) return NO;
    return memcmp(s[0], pfx, n) == 0;
}

//  Build `sh -c 'cd PATH && exec keeper sync'` to chdir the child
//  into the target repo before it opens its keeper state.  The
//  sh invocation also picks up `keeper` via the inherited PATH.
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

static ok64 spawn_server(u8csc uri, int *wfd, int *rfd, pid_t *pid) {
    sane($ok(uri) && wfd && rfd && pid);
    //  Accept `file:///path` and `be://host/path`.
    Bu8 cmd_b = {};
    call(u8bAllocate, cmd_b, 2048);
    ok64 rc = SYNCFAIL;

    if (starts_with(uri, "file://")) {
        u8c *path_start = uri[0] + strlen("file://");
        while (path_start < uri[1] && *path_start == '/') path_start++;
        if (path_start > uri[0]) path_start--;
        u8cs path = {path_start, uri[1]};

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
        u8cs path = {slash + 1, uri[1]};

        a_cstr(pre1, "ssh '");
        u8bFeed(cmd_b, pre1);
        u8bFeed(cmd_b, host);
        a_cstr(pre2, "' \"cd '");
        u8bFeed(cmd_b, pre2);
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

ok64 SYNCGet(keeper *k, u8csc uri) {
    sane(k && $ok(uri));
    int wfd = -1, rfd = -1;
    pid_t pid = 0;
    call(spawn_server, uri, &wfd, &rfd, &pid);

    //  Look up existing watermark for this peer in REFS.
    sync_wm wm = {};
    //  TODO: parse #( ... ) fragment from REFS entry keyed on this URI.
    //  MVP: full sync every time.

    //  1. Send H + W.
    Bu8 out = {};
    call(u8bAllocate, out, 128);
    call(SYNCFeedHello, out, SYNC_VERSION, SYNC_VERB_GET);
    call(SYNCFeedWater, out, &wm);
    a_dup(u8c, ob, u8bDataC(out));
    call(FILEFeedAll, wfd, ob);
    close(wfd);  // half-close: we have no more to send
    u8bFree(out);

    //  2. Read L* + E, then Q* + E, then R* + E.
    Bu8 in = {};
    call(u8bAllocate, in, 1u << 20);

    Bwh128 bms_b = {};
    call(wh128bAllocate, bms_b, 1024);
    //  Sentinel table per file_id (file_id → final_length).
    u64 sentinels[256] = {};
    for (;;) {
        u8 tag = 0;
        u8cs body;
        call(read_tlv, rfd, in, &tag, body);
        if (tag == SYNC_TAG_END) break;
        test(tag == SYNC_TAG_LIST, SYNCPROTO);
        wh128 bm = {};
        u64 slen = 0;
        b8 is_s = NO;
        call(SYNCParseList, body, &bm, &slen, &is_s);
        if (is_s) {
            //  Sentinel belongs to the last bookmark's file_id.
            if (wh128bDataLen(bms_b) > 0) {
                wh128 *last = &wh128bDataHead(bms_b)[wh128bDataLen(bms_b) - 1];
                u32 fid = wh64Id(last->val);
                if (fid < 256) sentinels[fid] = slen;
            }
            continue;
        }
        wh128bPush(bms_b, &bm);
    }

    //  Receive pack bytes.  For MVP we import each pack via a fresh
    //  local pack (file-per-pack storage — 0.D deferred).  We strip
    //  the git PACK header/trailer and use KEEPPackOpen/Feed/Close
    //  via UNPKStream... actually that path is internal.  For MVP:
    //  just drop the pack bytes into a local .pack file and let a
    //  re-open pick them up via KEEPScan.  Simpler: write the bytes
    //  verbatim as `.dogs/keeper/log/NNNNNNNNNN.pack` and also add
    //  a bookmark + a placeholder index.
    //
    //  Even simpler for plain dog sync proof-of-life: defer actual
    //  ingest; just count and verify that Q records arrived.  We can
    //  harden ingest in a follow-up.  Echo stats to stderr.
    u32 q_count = 0;
    u64 q_bytes = 0;
    for (;;) {
        u8 tag = 0;
        u8cs body;
        call(read_tlv, rfd, in, &tag, body);
        if (tag == SYNC_TAG_END) break;
        test(tag == SYNC_TAG_PACK, SYNCPROTO);
        q_count++;
        q_bytes += u8csLen(body);
    }
    fprintf(stderr, "sync: received %u pack(s), %llu byte(s)\n",
            q_count, (unsigned long long)q_bytes);

    //  Read R* + E (reflog tail).
    u32 r_count = 0;
    u64 r_bytes = 0;
    for (;;) {
        u8 tag = 0;
        u8cs body;
        call(read_tlv, rfd, in, &tag, body);
        if (tag == SYNC_TAG_END) break;
        test(tag == SYNC_TAG_REFS, SYNCPROTO);
        r_count++;
        r_bytes += u8csLen(body);
    }
    fprintf(stderr, "sync: received %u reflog chunk(s), %llu byte(s)\n",
            r_count, (unsigned long long)r_bytes);

    close(rfd);
    int st = 0;
    waitpid(pid, &st, 0);

    wh128bFree(bms_b);
    u8bFree(in);
    done;
}

