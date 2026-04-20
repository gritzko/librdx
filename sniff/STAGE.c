//  STAGE: per-branch staging pack.  See sniff/STAGE.md.
//
//  Only file/dir plumbing + staging-local .idx.  All pack encoding
//  goes through KEEPPackFeed / PACKDrainObjHdr / PACKInflate.
//
#include "STAGE.h"
#include "SNIFF.h"
#include "AT.h"

#include <string.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/WHIFF.h"
#include "keeper/PACK.h"
#include "keeper/SHA1.h"

#define X(M, name) M##wh128##name
#include "abc/QSORTx.h"
#undef X

#define STAGE_LOG_S     (a_cstr(_log_s, "log"), _log_s)
#define STAGE_IDX_S     (a_cstr(_idx_s, "idx"), _idx_s)
#define STAGE_PACK_S    (a_cstr(_pk_s, "0000000001.pack"), _pk_s)
#define STAGE_IDXFN_S   (a_cstr(_ix_s, "0000000001.idx"), _ix_s)
#define STAGE_MAX_OBJS  KEEP_PACK_MAX_OBJS
#define STAGE_FILE_ID   1

//  .dogs/sniff relative to the worktree root.
static u8c *const STAGE_REL_S[2] = {
    (u8c *)".dogs/sniff",
    (u8c *)".dogs/sniff" + sizeof(".dogs/sniff") - 1,
};

ok64 STAGEDir(path8b out, u8cs reporoot, u8cs branch) {
    sane(out && $ok(reporoot) && $ok(branch));
    //  <reporoot>/.dogs/sniff/<branch>/
    call(PATHu8bFeed, out, reporoot);
    call(PATHu8bAdd,  out, STAGE_REL_S);
    call(PATHu8bAdd,  out, branch);
    done;
}

static ok64 stage_dir(path8b out, u8cs branch) {
    sane(out && $ok(branch));
    a_dup(u8c, root, u8bDataC(SNIFF.h->root));
    return STAGEDir(out, root, branch);
}

ok64 STAGEBranch(u8bp out) {
    sane(out);
    a_pad(u8, bbuf, 256);
    a_pad(u8, sbuf, 64);
    sniff_at tail = {.branch = bbuf, .sha = sbuf};
    ok64 o = SNIFFAtRead(&tail);
    u8bReset(out);
    if (o == OK && u8bDataLen(tail.branch) > 0) {
        u8bFeed(out, u8bDataC(tail.branch));
        done;
    }
    //  Default when at.log is missing/empty: heads/master.
    a_cstr(def_s, "heads/master");
    u8bFeed(out, def_s);
    done;
}

ok64 STAGEOpen(keep_pack *p, u8cs branch) {
    sane(p && SNIFF.h && $ok(branch) && !u8csEmpty(branch));
    zerop(p);

    a_path(sdir);
    call(stage_dir, sdir, branch);
    call(FILEMakeDirP, $path(sdir));

    a_cstr(log_s, "log");
    a_cstr(idx_s, "idx");
    a_cstr(pack_s, "0000000001.pack");

    a_path(logdir, $path(sdir), log_s);
    call(FILEMakeDirP, $path(logdir));
    a_path(idxdir, $path(sdir), idx_s);
    call(FILEMakeDirP, $path(idxdir));

    a_path(ppath, $path(logdir), pack_s);

    u8bp probe = NULL;
    ok64 ro = FILEMapRO(&probe, $path(ppath));
    if (ro == OK) {
        u8bUnMap(probe);
        call(FILEBook, &p->log, $path(ppath), 1ULL << 30);
        ((u8 **)p->log)[2] = p->log[3];
        p->pack_offset = u8bDataLen(p->log);
    } else {
        call(FILEBookCreate, &p->log, $path(ppath), 1ULL << 30, 4096);
        call(PACKu8sFeedHdr, u8bIdle(p->log), 0);
        u8bFed(p->log, 12);
        p->pack_offset = 12;
    }

    p->file_id = STAGE_FILE_ID;
    p->nobjs = 0;
    p->last_type = 0;
    call(wh128bAllocate, p->entries, STAGE_MAX_OBJS);
    done;
}

//  Map the current idx file.  NULL out params when file is missing.
static ok64 stage_map_idx(u8bp *out_map, wh128cs out_run, u8cs branch) {
    sane(out_map && $ok(branch));
    *out_map = NULL;
    out_run[0] = out_run[1] = NULL;

    a_path(sdir);
    call(stage_dir, sdir, branch);
    a_cstr(idx_s, "idx");
    a_cstr(idxfn_s, "0000000001.idx");
    a_path(ipath, $path(sdir), idx_s, idxfn_s);

    ok64 mo = FILEMapRO(out_map, $path(ipath));
    if (mo != OK) done;
    wh128cp base = (wh128cp)u8bDataHead(*out_map);
    u32 n = (u32)(u8bDataLen(*out_map) / sizeof(wh128));
    out_run[0] = base;
    out_run[1] = base + n;
    done;
}

ok64 STAGEClose(keep_pack *p, u8cs branch) {
    sane(p && p->log && SNIFF.h && $ok(branch));

    //  Pack bookmark.
    sha1 pack_sha = {};
    u8cp file_base = u8bDataHead(p->log);
    u64 file_len = u8bDataLen(p->log);
    u8cs pack_bytes = {file_base + p->pack_offset, file_base + file_len};
    SHA1Sum(&pack_sha, pack_bytes);
    u64 ph = WHIFFHashlet60(&pack_sha);
    wh128 bm = {
        .key = wh64Pack(KEEP_TYPE_PACK, p->file_id, p->pack_offset),
        .val = keepKeyPack(0, ph),
    };
    wh128bPush(p->entries, &bm);

    call(FILETrimBook, p->log);
    FILEUnBook(p->log);
    p->log = NULL;

    //  Merge new entries with old idx, sort, rewrite.
    u32 nnew = (u32)wh128bDataLen(p->entries);
    u8bp old_map = NULL;
    wh128cs old_run = {};
    call(stage_map_idx, &old_map, old_run, branch);
    u32 nold = old_run[0] ? (u32)(old_run[1] - old_run[0]) : 0;

    Bwh128 merged = {};
    call(wh128bAllocate, merged, nnew + nold);
    if (nold > 0) {
        u8cs raw = {(u8cp)old_run[0], (u8cp)old_run[1]};
        u8p *idle = (u8p *)&merged[2];
        memcpy(*idle, raw[0], u8csLen(raw));
        *idle += u8csLen(raw);
    }
    if (nnew > 0) {
        a_dup(wh128, nfresh, wh128bData(p->entries));
        u8cs raw = {(u8cp)nfresh[0], (u8cp)nfresh[1]};
        u8p *idle = (u8p *)&merged[2];
        memcpy(*idle, raw[0], u8csLen(raw));
        *idle += u8csLen(raw);
    }
    a_dup(wh128, sorted, wh128bData(merged));
    wh128sSort(sorted);
    if (old_map) u8bUnMap(old_map);

    a_path(sdir);
    call(stage_dir, sdir, branch);
    a_cstr(idx_s, "idx");
    a_cstr(idxfn_s, "0000000001.idx");
    a_path(ipath, $path(sdir), idx_s, idxfn_s);
    int ifd = -1;
    call(FILECreate, &ifd, $path(ipath));
    if (ifd >= 0) {
        u8cs raw = {(u8cp)sorted[0], (u8cp)sorted[1]};
        FILEFeedAll(ifd, raw);
        close(ifd);
    }

    wh128bFree(merged);
    wh128bFree(p->entries);
    done;
}

ok64 STAGELookup(u8cs branch, u64 hashlet60, size_t hexlen, u64p val_out) {
    sane(val_out && $ok(branch));
    u8bp map = NULL;
    wh128cs run = {};
    call(stage_map_idx, &map, run, branch);
    if (run[0] == NULL) fail(KEEPNONE);

    //  Match keeper's KEEPLookup: compute hashlet prefix, search entries
    //  whose type is a real object (1..4) and whose hashlet prefix-matches.
    if (hexlen > 15) hexlen = 15;
    u64 nbits = hexlen * 4;
    u64 shift = 60 - nbits;
    u64 hmask = shift < 60 ? (WHIFF_HASHLET60_MASK >> shift) << shift
                           : WHIFF_HASHLET60_MASK;
    u64 hpre = hashlet60 & hmask;

    ok64 rv = KEEPNONE;
    for (wh128cp e = run[0]; e < run[1]; e++) {
        u8 t = keepKeyType(e->key);
        if (t < KEEP_OBJ_COMMIT || t > KEEP_OBJ_TAG) continue;
        u64 eh = keepKeyHashlet(e->key);
        if ((eh & hmask) == hpre) {
            *val_out = e->val;
            rv = OK;
            break;
        }
    }
    u8bUnMap(map);
    return rv;
}

static ok64 stage_inflate_at(u8bp pmap, u64 offset,
                             u8bp out, u8p out_type) {
    sane(pmap && out && out_type);
    u8cs from = {u8bDataHead(pmap) + offset, u8bIdleHead(pmap)};
    pack_obj po = {};
    call(PACKDrainObjHdr, from, &po);
    *out_type = po.type;
    u8bReset(out);
    call(u8bReserve, out, po.size);
    u8s into = {u8bIdleHead(out), u8bIdleHead(out) + po.size};
    call(PACKInflate, from, into, po.size);
    u8bFed(out, po.size);
    done;
}

ok64 STAGEGet(u8cs branch, u64 hashlet60, size_t hexlen,
              u8bp out, u8p out_type) {
    sane(out && out_type && $ok(branch));
    u64 val = 0;
    call(STAGELookup, branch, hashlet60, hexlen, &val);

    a_path(sdir);
    call(stage_dir, sdir, branch);
    a_cstr(log_s, "log");
    a_cstr(pack_s, "0000000001.pack");
    a_path(ppath, $path(sdir), log_s, pack_s);

    u8bp pmap = NULL;
    call(FILEMapRO, &pmap, $path(ppath));
    ok64 rv = stage_inflate_at(pmap, wh64Off(val), out, out_type);
    u8bUnMap(pmap);
    return rv;
}

ok64 STAGEEach(u8cs branch, keep_cb cb, void *ctx) {
    sane(cb && $ok(branch));
    u8bp imap = NULL;
    wh128cs run = {};
    call(stage_map_idx, &imap, run, branch);
    if (run[0] == NULL) done;

    a_path(sdir);
    call(stage_dir, sdir, branch);
    a_cstr(log_s, "log");
    a_cstr(pack_s, "0000000001.pack");
    a_path(ppath, $path(sdir), log_s, pack_s);
    u8bp pmap = NULL;
    ok64 po = FILEMapRO(&pmap, $path(ppath));
    if (po != OK) { u8bUnMap(imap); return po; }

    Bu8 obj_buf = {};
    ok64 ao = u8bAllocate(obj_buf, 1UL << 20);
    if (ao != OK) { u8bUnMap(pmap); u8bUnMap(imap); return ao; }

    ok64 rv = OK;
    for (wh128cp e = run[0]; e < run[1] && rv == OK; e++) {
        u8 t = keepKeyType(e->key);
        if (t < KEEP_OBJ_COMMIT || t > KEEP_OBJ_TAG) continue;
        u64 hashlet = keepKeyHashlet(e->key);
        u64 offset = wh64Off(e->val);
        u8 otype = 0;
        rv = stage_inflate_at(pmap, offset, obj_buf, &otype);
        if (rv != OK) break;
        a_dup(u8c, body, u8bData(obj_buf));
        rv = cb(otype, body, hashlet, ctx);
    }

    u8bFree(obj_buf);
    u8bUnMap(pmap);
    u8bUnMap(imap);
    return rv;
}

ok64 STAGEDrop(u8cs branch) {
    sane($ok(branch));
    if (!SNIFF.h) done;
    a_path(sdir);
    call(stage_dir, sdir, branch);
    (void)FILErmrf($path(sdir));
    done;
}
