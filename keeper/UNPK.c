//  UNPK: single-pass packfile indexer.  See UNPK.h for the contract.
//
#include "UNPK.h"

#include "DELT.h"
#include "GIT.h"
#include "PACK.h"
#include "PATHS.h"
#include "ZINF.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abc/PATH.h"
#include "abc/PRO.h"

// wh128 sort/dedup templates
#define X(M, name) M##wh128##name
#include "abc/QSORTx.h"
#undef X

// kv64 hashtable for sha-prefix -> LS path offset
#define X(M, name) M##kv64##name
#include "abc/HASHx.h"
#undef X

#define UNPK_MAX_CHAIN 64

//  DFS node for the delta forest.
//  nodes[] is 1-based; index 0 is sentinel.
typedef struct { u64 offset; u32 child; u32 sibling; } unpk_node;

//  Waiter: REF_DELTA indexed by sha8 of its base.
//  val = 1-based index into nodes[].
static void unpk_drain_waiters(wh128cs waiters, unpk_node *nodes,
                               b8 *resolved, u64 sha_key, u32 parent_idx) {
    wh128cp wbuf = waiters[0];
    size_t wlen = (size_t)(waiters[1] - waiters[0]);
    size_t lo = 0, hi = wlen;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (wbuf[mid].key < sha_key) lo = mid + 1;
        else hi = mid;
    }
    for (size_t j = lo; j < wlen && wbuf[j].key == sha_key; j++) {
        u32 w = (u32)wbuf[j].val;
        if (resolved[w]) continue;
        nodes[w].sibling = nodes[parent_idx].child;
        nodes[parent_idx].child = w;
    }
}

//  Emit one wh128 entry for a resolved object at `obj_off` in the log.
static ok64 unpk_emit(Bwh128 out, u32 file_id,
                       u8 type, sha1 const *sha, u64 obj_off) {
    wh128 e = {
        .key = keepKeyPack(type, WHIFFHashlet60(sha)),
        .val = wh64Pack(KEEP_VAL_FLAGS, file_id, obj_off),
    };
    return wh128bPush(out, &e);
}

//  Side-map bookkeeping for sha -> path derivation.
//
//  Commits: parse `tree <hex>` header, register that tree sha with
//  path "" (keeper path index 0).  Trees: enumerate entries, build
//  "self/name" for each, KEEPIntern it, record entry sha -> idx.

static void unpk_note_commit(keeper *k, u8cs content, kv64s sha2idx) {
    u8 tree_sha[20] = {};
    a_dup(u8c, body, content);
    if (GITu8sCommitTree(body, tree_sha) != OK) return;
    u64 key = 0;
    memcpy(&key, tree_sha, 8);
    //  Root tree's path = "" (interned via keeper; whatever index it lands at).
    a_cstr(empty, "");
    u32 root_idx = KEEPIntern(k, empty);
    kv64 e = { .key = key, .val = (u64)root_idx };
    HASHkv64Put(sha2idx, &e);
}

static void unpk_note_tree(keeper *k, u8cs content, u32 self_idx,
                            kv64s sha2idx) {
    u8cs self_path = {};
    (void)KEEPPath(k, self_idx, self_path);

    u8cs scan = {};
    a_dup(u8c, body, content);
    u8csMv(scan, body);
    for (;;) {
        u8cs file = {}, sha = {};
        if (GITu8sDrainTree(scan, file, sha) != OK) break;
        //  `file` = "<mode> <name>"; name after the first space.
        //  Mode determines whether this child is a directory.
        u8cs mode = {};
        u8csMv(mode, file);
        u8cs name = {};
        u8csMv(name, file);
        if (u8csFind(name, ' ') != OK) continue;
        //  Mode slice ends at the space; name starts after.
        mode[1] = name[0];
        name[0]++;  // skip the space
        if ($empty(name)) continue;
        b8 is_dir = (!$empty(mode) && *mode[0] == '4');  // 40000

        //  Build "<self>/<name>" (or "<name>" at root), "/"-appended for dirs.
        a_pad(u8, pbuf, 4096);
        if (u8csLen(self_path) > 0) {
            u8bFeed(pbuf, self_path);
            u8bFeed1(pbuf, '/');
        }
        u8bFeed(pbuf, name);
        if (is_dir) u8bFeed1(pbuf, '/');
        a_dup(u8c, pdata, u8bData(pbuf));

        u32 child_idx = KEEPIntern(k, pdata);

        u64 key = 0;
        memcpy(&key, sha[0], 8);
        kv64 e = { .key = key, .val = (u64)child_idx };
        HASHkv64Put(sha2idx, &e);
    }
}

//  Invoke the user callback with the derived path (or empty on miss).
static void unpk_dispatch(keeper *k, unpk_in const *in, kv64s sha2idx,
                           u8 type, sha1 const *sha, u8cs content,
                           unpk_stats *st) {
    if (!in->emit) return;

    u64 key = 0;
    memcpy(&key, sha->data, 8);
    kv64 probe = { .key = key, .val = 0 };
    b8 have = (HASHkv64Get(&probe, sha2idx) == OK);

    u8cs path = {};
    if (have) {
        (void)KEEPPath(k, (u32)probe.val, path);
        st->paths_known++;
    } else {
        st->paths_empty++;
    }

    in->emit(in->emit_ctx, type, sha, path, content);
}

ok64 UNPKIndex(keeper *k, unpk_in const *in,
               Bwh128 out, unpk_stats *stats) {
    sane(k && in && out);

    u8cp packbase = in->pack[0];
    if (in->pack[1] < packbase) return UNPKBADFMT;
    u64 packlen = (u64)(in->pack[1] - packbase);
    if (in->scan_start > packlen || in->scan_end > packlen ||
        in->scan_start > in->scan_end) return UNPKBADFMT;

    u32 count = in->count;
    u32 file_id = in->file_id;

    unpk_stats st = {};

    //  Path-tracking: map a resolved object's sha-prefix to its
    //  keeper-persistent path index.  Built while scanning trees +
    //  commits; used at dispatch time to look up an event's path.
    //  Paths themselves live in keeper's paths.log (persistent).
    kv64 *s2p   = NULL;
    kv64s sha2idx = {NULL, NULL};
    b8  with_paths = (in->emit != NULL);
    if (with_paths) {
        u64 tblsz = (u64)count * 2;
        if (tblsz < 256) tblsz = 256;
        s2p = calloc(tblsz, sizeof(kv64));
        if (!s2p) with_paths = NO;
        else { sha2idx[0] = s2p; sha2idx[1] = s2p + tblsz; }
    }

    //  Pre-scan: record (offset, type) per object.  Inflates each
    //  object into k->buf1 purely to advance the read cursor past
    //  compressed bytes.
    u64 *offsets = calloc(count, sizeof(u64));
    u8  *types   = calloc(count, 1);
    if (!offsets || !types) { free(offsets); free(types); return UNPKNOROOM; }

    u8cs scan = {packbase + in->scan_start, packbase + in->scan_end};
    u32 scanned = 0;
    for (u32 i = 0; i < count; i++) {
        offsets[i] = (u64)(scan[0] - packbase);
        pack_obj obj = {};
        if (PACKDrainObjHdr(scan, &obj) != OK) break;
        types[i] = obj.type;
        u8bReset(k->buf1);
        if (ZINFInflate(u8bIdle(k->buf1), scan) != OK) break;
        scanned++;
    }
    if (scanned < count) {
        fprintf(stderr, "unpk: scan incomplete %u/%u\n", scanned, count);
    }

    //  Forest: link OFS_DELTA -> parent by offset, stash REF_DELTA waiters.
    unpk_node *nodes = calloc((size_t)count + 1, sizeof(unpk_node));
    b8 *resolved = calloc((size_t)count + 1, 1);
    Bwh128 waiters_buf = {};
    wh128bAllocate(waiters_buf, count ? count : 1);
    if (!nodes || !resolved) {
        free(nodes); free(resolved); free(offsets); free(types);
        wh128bFree(waiters_buf);
        return UNPKNOROOM;
    }
    for (u32 i = 0; i < count; i++) nodes[i + 1].offset = offsets[i];

    //  OFS_DELTA edges
    for (u32 i = 0; i < count; i++) {
        if (types[i] != PACK_OBJ_OFS_DELTA) continue;
        pack_obj obj = {};
        u8cs from = {packbase + offsets[i], packbase + packlen};
        if (PACKDrainObjHdr(from, &obj) != OK) continue;
        if (obj.ofs_delta > offsets[i]) continue;
        u64 base_off = offsets[i] - obj.ofs_delta;
        u32 lo = 0, hi = count;
        while (lo < hi) {
            u32 mid = lo + (hi - lo) / 2;
            if (offsets[mid] < base_off) lo = mid + 1;
            else hi = mid;
        }
        if (lo < count && offsets[lo] == base_off) {
            u32 parent = lo + 1, me = i + 1;
            nodes[me].sibling = nodes[parent].child;
            nodes[parent].child = me;
        }
    }

    //  REF_DELTA waiters keyed by sha8 of base
    for (u32 i = 0; i < count; i++) {
        if (types[i] != PACK_OBJ_REF_DELTA) continue;
        pack_obj obj = {};
        u8cs from = {packbase + offsets[i], packbase + packlen};
        if (PACKDrainObjHdr(from, &obj) != OK) { st.skipped++; continue; }
        u64 sha_key = 0;
        memcpy(&sha_key, obj.ref_delta[0], 8);
        wh128 w = { .key = sha_key, .val = i + 1 };
        wh128bPush(waiters_buf, &w);
    }
    a_dup(wh128, wsorted, wh128bData(waiters_buf));
    wh128sSort(wsorted);
    wh128cs waiters = {wsorted[0], wsorted[1]};

    //  Resolve base objects; drain waiters on each base's sha.
    u8bReset(k->buf1);
    for (u32 i = 0; i < count; i++) {
        if (types[i] < 1 || types[i] > 4) continue;
        pack_obj obj = {};
        u8cs from = {packbase + offsets[i], packbase + packlen};
        if (PACKDrainObjHdr(from, &obj) != OK) { st.skipped++; continue; }
        if (obj.size > u8bIdleLen(k->buf1)) { st.skipped++; continue; }
        u8p cs = u8bIdleHead(k->buf1);
        u8s into = {cs, u8bTerm(k->buf1)};
        if (PACKInflate(from, into, obj.size) != OK) { st.skipped++; continue; }

        sha1 sha = {};
        u8csc content = {cs, cs + obj.size};
        KEEPObjSha(&sha, obj.type, content);
        if (unpk_emit(out, file_id, types[i], &sha, offsets[i]) != OK) {
            st.skipped++; continue;
        }
        st.indexed++;
        st.base_count++;
        resolved[i + 1] = YES;

        if (with_paths) {
            u8cs ct = {cs, cs + obj.size};
            if (types[i] == PACK_OBJ_COMMIT)
                unpk_note_commit(k, ct, sha2idx);
            else if (types[i] == PACK_OBJ_TREE) {
                u32 self_idx = 0;
                u64 k_ = 0; memcpy(&k_, sha.data, 8);
                kv64 probe = { .key = k_, .val = 0 };
                if (HASHkv64Get(&probe, sha2idx) == OK)
                    self_idx = (u32)probe.val;
                unpk_note_tree(k, ct, self_idx, sha2idx);
            }
            u8cs dct = {cs, cs + obj.size};
            unpk_dispatch(k, in, sha2idx, types[i], &sha, dct, &st);
        }

        u64 sha_key = 0;
        memcpy(&sha_key, sha.data, 8);
        unpk_drain_waiters(waiters, nodes, resolved, sha_key, i + 1);
        //  keep buf1 contents for the DFS walk below
    }

    //  DFS through each base's subtree.  Stack holds pointers into
    //  k->buf1 so each delta sees its parent's inflated content still
    //  live; on unwind we truncate buf1 back to the parent's start.
    struct { u8p d_start; u8p d_end; u32 node; u8 base_type; } stk[UNPK_MAX_CHAIN];

    for (u32 root_idx = 1; root_idx <= count; root_idx++) {
        if (!nodes[root_idx].child) continue;
        if (!resolved[root_idx]) continue;
        if (types[root_idx - 1] < 1 || types[root_idx - 1] > 4) continue;

        u8 root_type = types[root_idx - 1];
        u8bReset(k->buf1);
        pack_obj robj = {};
        u8cs rfrom = {packbase + offsets[root_idx - 1], packbase + packlen};
        if (PACKDrainObjHdr(rfrom, &robj) != OK) { st.skipped++; continue; }
        if (robj.size > u8bIdleLen(k->buf1)) { st.skipped++; continue; }
        u8p rs = u8bIdleHead(k->buf1);
        u8s rinto = {rs, u8bTerm(k->buf1)};
        if (PACKInflate(rfrom, rinto, robj.size) != OK) { st.skipped++; continue; }
        u8bFed(k->buf1, robj.size);

        int top = 0;
        stk[0].d_start = rs;
        stk[0].d_end = rs + robj.size;
        stk[0].node = root_idx;
        stk[0].base_type = root_type;

        while (top >= 0) {
            u32 cur = stk[top].node;
            u32 child = nodes[cur].child;
            if (!child) {
                if (top > 0) ((u8**)k->buf1)[2] = stk[top].d_start;
                top--;
                continue;
            }
            nodes[cur].child = nodes[child].sibling;

            if (top + 1 >= UNPK_MAX_CHAIN) { st.skipped++; continue; }

            u8p  base_s  = stk[top].d_start;
            u64  base_sz = (u64)(stk[top].d_end - stk[top].d_start);

            pack_obj dobj = {};
            u8cs dfrom = {packbase + offsets[child - 1], packbase + packlen};
            if (PACKDrainObjHdr(dfrom, &dobj) != OK) { st.skipped++; continue; }
            if (dobj.size > u8bIdleLen(k->buf2)) { st.skipped++; continue; }

            u8bReset(k->buf2);
            u8s dinto = {u8bIdleHead(k->buf2), u8bTerm(k->buf2)};
            if (PACKInflate(dfrom, dinto, dobj.size) != OK) { st.skipped++; continue; }

            u8cs delta_sl = {u8bIdleHead(k->buf2), u8bIdleHead(k->buf2) + dobj.size};
            u8cs base_sl  = {base_s, base_s + base_sz};
            u8p rstart = u8bIdleHead(k->buf1);
            u8g aout = {rstart, rstart, u8bTerm(k->buf1)};
            if (DELTApply(delta_sl, base_sl, aout) != OK) { st.skipped++; continue; }
            u64 rsz = u8gLeftLen(aout);
            u8bFed(k->buf1, rsz);

            sha1 sha = {};
            u8csc content = {rstart, rstart + rsz};
            KEEPObjSha(&sha, stk[0].base_type, content);
            if (unpk_emit(out, file_id, stk[0].base_type, &sha,
                          offsets[child - 1]) != OK) {
                st.skipped++; continue;
            }
            st.indexed++;
            resolved[child] = YES;

            if (with_paths) {
                u8cs ct = {rstart, rstart + rsz};
                if (stk[0].base_type == PACK_OBJ_COMMIT)
                    unpk_note_commit(k, ct, sha2idx);
                else if (stk[0].base_type == PACK_OBJ_TREE) {
                    u32 self_idx = 0;
                    u64 k_ = 0; memcpy(&k_, sha.data, 8);
                    kv64 probe = { .key = k_, .val = 0 };
                    if (HASHkv64Get(&probe, sha2idx) == OK)
                        self_idx = (u32)probe.val;
                    unpk_note_tree(k, ct, self_idx, sha2idx);
                }
                u8cs dct = {rstart, rstart + rsz};
                unpk_dispatch(k, in, sha2idx,
                              stk[0].base_type, &sha, dct, &st);
            }

            u64 sha_key = 0;
            memcpy(&sha_key, sha.data, 8);
            unpk_drain_waiters(waiters, nodes, resolved, sha_key, child);

            top++;
            stk[top].d_start = rstart;
            stk[top].d_end = rstart + rsz;
            stk[top].node = child;
            stk[top].base_type = stk[0].base_type;
        }
    }

    //  Thin-pack fallback: REF_DELTAs whose base lives in an earlier
    //  pack.  KEEPGet pulls the base into k->buf3; delta inflates to
    //  k->buf4; apply into k->buf1.
    for (u32 i = 0; i < count; i++) {
        if (resolved[i + 1]) continue;
        if (types[i] != PACK_OBJ_REF_DELTA) continue;

        pack_obj obj = {};
        u8cs from = {packbase + offsets[i], packbase + packlen};
        if (PACKDrainObjHdr(from, &obj) != OK) { st.skipped++; continue; }

        u64 base_hashlet = WHIFFHashlet60((sha1cp)obj.ref_delta[0]);
        u8 base_type = 0;
        u8bReset(k->buf3);
        if (KEEPGet(k, base_hashlet, 15, k->buf3, &base_type) != OK) {
            st.skipped++; continue;
        }

        u8bReset(k->buf4);
        u64 idle_before = u8bIdleLen(k->buf4);
        if (ZINFInflate(u8bIdle(k->buf4), from) != OK) { st.skipped++; continue; }
        u64 produced = idle_before - u8bIdleLen(k->buf4);
        if (produced == 0) { st.skipped++; continue; }
        u8bFed(k->buf4, produced);

        u8bReset(k->buf1);
        a_dup(u8c, delta_sl, u8bDataC(k->buf4));
        a_dup(u8c, base_sl, u8bData(k->buf3));
        u8p rstart = u8bIdleHead(k->buf1);
        u8g aout = {rstart, rstart, u8bTerm(k->buf1)};
        if (DELTApply(delta_sl, base_sl, aout) != OK) { st.skipped++; continue; }
        u64 rsz = u8gLeftLen(aout);

        sha1 sha = {};
        u8csc content = {rstart, rstart + rsz};
        KEEPObjSha(&sha, base_type, content);
        if (unpk_emit(out, file_id, base_type, &sha, offsets[i]) != OK) {
            st.skipped++; continue;
        }
        st.indexed++;
        st.cross++;
        resolved[i + 1] = YES;

        if (with_paths) {
            u8cs ct = {rstart, rstart + rsz};
            if (base_type == PACK_OBJ_COMMIT)
                unpk_note_commit(k, ct, sha2idx);
            else if (base_type == PACK_OBJ_TREE) {
                u32 self_idx = 0;
                u64 k_ = 0; memcpy(&k_, sha.data, 8);
                kv64 probe = { .key = k_, .val = 0 };
                if (HASHkv64Get(&probe, sha2idx) == OK)
                    self_idx = (u32)probe.val;
                unpk_note_tree(k, ct, self_idx, sha2idx);
            }
            u8cs dct = {rstart, rstart + rsz};
            unpk_dispatch(k, in, sha2idx, base_type, &sha, dct, &st);
        }
    }

    wh128bFree(waiters_buf);
    free(resolved);
    free(nodes);
    free(offsets);
    free(types);
    if (with_paths) free(s2p);

    if (stats) *stats = st;
    done;
}
