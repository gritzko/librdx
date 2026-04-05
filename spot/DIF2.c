#include "CAPOi.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "abc/PRO.h"
#include "spot/HUNK.h"
#include "spot/NEIL.h"
#include "tok/DEF.h"
#include "tok/JOIN.h"

// --- Merge: token-level 3-way merge ---

static ok64 CAPOMergeRead(u8cs *data, u8bp *mapped, u8csc path_arg) {
    sane(data != NULL && mapped != NULL);
    a_path(path, path_arg);
    call(FILEMapRO, mapped, PATHu8cgIn(path));
    (*data)[0] = u8bDataHead(*mapped);
    (*data)[1] = u8bIdleHead(*mapped);
    done;
}

ok64 CAPOMerge(u8csc base_path, u8csc ours_path, u8csc theirs_path,
               u8csc outpath) {
    sane($ok(base_path) && $ok(ours_path) && $ok(theirs_path));

    // Detect extension from ours
    u8cs ext = {};
    size_t olen = (size_t)$len(ours_path);
    CAPOFindExt(ext, ours_path[0], olen);
    if (!$empty(ext) && ext[0][0] == '.') {
        ext[0] = ext[0] + 1;  // strip dot for tok/
    }

    // Read three files
    u8bp map_b = NULL, map_o = NULL, map_t = NULL;
    u8cs base_data = {}, ours_data = {}, theirs_data = {};
    call(CAPOMergeRead, &base_data, &map_b, base_path);
    call(CAPOMergeRead, &ours_data, &map_o, ours_path);
    call(CAPOMergeRead, &theirs_data, &map_t, theirs_path);

    // Tokenize
    JOINfile base = {}, ours = {}, theirs = {};
    ok64 o = JOINTokenize(&base, base_data, ext);
    if (o != OK) goto cleanup;
    o = JOINTokenize(&ours, ours_data, ext);
    if (o != OK) goto cleanup;
    o = JOINTokenize(&theirs, theirs_data, ext);
    if (o != OK) goto cleanup;

    // Merge
    {
        u64 outsz = $len(ours_data) + $len(theirs_data) + 4096;
        u8 *out[4] = {};
        o = u8bAlloc(out, outsz);
        if (o != OK) goto cleanup;
        o = JOINMerge(out, &base, &ours, &theirs);
        if (o != OK) { u8bFree(out); goto cleanup; }

        u8cs result = {out[1], out[2]};
        if (!$empty(outpath)) {
            // Write to file
            a_path(opath, outpath);
            int fd = -1;
            o = FILECreate(&fd, PATHu8cgIn(opath));
            if (o != OK) { u8bFree(out); goto cleanup; }
            o = FILEFeedall(fd, result);
            close(fd);
        } else {
            o = FILEFeedall(STDOUT_FILENO, result);
        }
        u8bFree(out);
    }

cleanup:
    JOINFree(&base);
    JOINFree(&ours);
    JOINFree(&theirs);
    if (map_b) FILEUnMap(map_b);
    if (map_o) FILEUnMap(map_o);
    if (map_t) FILEUnMap(map_t);
    return o;
}

// --- Diff: token-level colored diff ---

// Helper: get token slice from a JOINfile
#define CAPOJoinToks(ts, jf) \
    u32cs ts = {(u32cp)(jf)->toks[1], (u32cp)(jf)->toks[2]}

ok64 CAPODiff(u8csc old_path, u8csc new_path, u8csc name,
              u8csc old_mode, u8csc new_mode) {
    sane($ok(old_path) && $ok(new_path));

    // Detect extension from logical name
    u8cs ext = {};
    size_t nlen = (size_t)$len(name);
    CAPOFindExt(ext, name[0], nlen);
    if (!$empty(ext) && ext[0][0] == '.') {
        ext[0] = ext[0] + 1;  // strip dot for tok/
    }

    // Read files (either side may be empty for new/deleted files)
    u8bp map_old = NULL, map_new = NULL;
    u8cs old_data = {}, new_data = {};
    ok64 oro = CAPOMergeRead(&old_data, &map_old, old_path);
    ok64 nro = CAPOMergeRead(&new_data, &map_new, new_path);
    if (oro != OK && nro != OK) return oro;  // both failed

    // Byte-identical content: emit mode-change hunk if modes differ
    if (oro == OK && nro == OK &&
        $len(old_data) == $len(new_data) &&
        ($len(old_data) == 0 ||
         memcmp(old_data[0], new_data[0], (size_t)$len(old_data)) == 0)) {
        b8 mode_diff = !$empty(old_mode) && !$empty(new_mode) &&
            ($len(old_mode) != $len(new_mode) ||
             memcmp(old_mode[0], new_mode[0], (size_t)$len(old_mode)) != 0);
        if (mode_diff) {
            if (LESSArenaInit() != OK) {
                if (map_old) FILEUnMap(map_old);
                if (map_new) FILEUnMap(map_new);
                return NOROOM;
            }
            char hdr[512];
            int tl = snprintf(hdr, sizeof(hdr), "--- %.*s ---",
                              (int)$len(name), (char *)name[0]);
            char body[256];
            int bl = snprintf(body, sizeof(body),
                              "old mode %.*s\nnew mode %.*s\n",
                              (int)$len(old_mode), (char *)old_mode[0],
                              (int)$len(new_mode), (char *)new_mode[0]);
            LESShunk *hk = &less_hunks[less_nhunks];
            *hk = (LESShunk){};
            if (tl > 0) {
                u8p tp = LESSArenaWrite(hdr, (size_t)tl);
                if (tp) { hk->title[0] = tp; hk->title[1] = tp + tl; }
            }
            if (bl > 0) {
                u8p xp = LESSArenaWrite(body, (size_t)bl);
                if (xp) { hk->text[0] = xp; hk->text[1] = xp + bl; }
            }
            LESSHunkEmit();
            LESSRun(less_hunks, less_nhunks);
            LESSArenaCleanup();
        }
        if (map_old) FILEUnMap(map_old);
        if (map_new) FILEUnMap(map_new);
        done;
    }

    if (LESSArenaInit() != OK) {
        if (map_old) FILEUnMap(map_old);
        if (map_new) FILEUnMap(map_new);
        return NOROOM;
    }

    // Display name (null-terminated) for hunk titles
    char dispname[FILE_PATH_MAX_LEN];
    size_t dlen = (size_t)$len(name);
    if (dlen >= sizeof(dispname)) dlen = sizeof(dispname) - 1;
    memcpy(dispname, name[0], dlen);
    dispname[dlen] = 0;

    // Deleted file: red header + all old content as DEL
    if (nro != OK) {
        JOINfile old_f = {};
        ok64 o = JOINTokenize(&old_f, old_data, ext);
        if (o == OK) {
            CAPOJoinToks(old_ts, &old_f);
            u32 olen = (u32)$len(old_data);
            LESShunk *hk = &less_hunks[less_nhunks];
            *hk = (LESShunk){};
            u8gp _tg = u8aOpen(less_arena);
            call(CAPOFormatTitle, u8gRest(_tg), dispname, "");
            u8cs _title = {};
            u8aClose(less_arena, _title);
            if (!$empty(_title)) {
                hk->title[0] = _title[0];
                hk->title[1] = _title[1];
            }
            u8p txp = LESSArenaWrite(old_data[0], olen);
            if (txp) { hk->text[0] = txp; hk->text[1] = txp + olen; }
            // Arena-copy toks
            u8p tkp = LESSArenaWrite(old_ts[0],
                          $len(old_ts) * sizeof(u32));
            if (tkp) {
                hk->toks[0] = (u32cp)tkp;
                hk->toks[1] = (u32cp)(tkp +
                    $len(old_ts) * sizeof(u32));
            }
            // Single hili entry: entire file is DEL
            u32 del_tok = tok32Pack('D', olen);
            u8p dhp = LESSArenaWrite(&del_tok, sizeof(u32));
            if (dhp) {
                hk->hili[0] = (u32cp)dhp;
                hk->hili[1] = (u32cp)(dhp + sizeof(u32));
            }
            LESSHunkEmit();
        }
        JOINFree(&old_f);
        LESSRun(less_hunks, less_nhunks);
        LESSArenaCleanup();
        if (map_old) FILEUnMap(map_old);
        done;
    }

    // New file: green header + all new content as INS
    if (oro != OK) {
        JOINfile new_f = {};
        ok64 o = JOINTokenize(&new_f, new_data, ext);
        if (o != OK) {
            JOINFree(&new_f);
            if (map_new) FILEUnMap(map_new);
            LESSArenaCleanup();
            return o;
        }
        CAPOJoinToks(new_ts, &new_f);
        u32 nlen2 = (u32)$len(new_data);
        LESShunk *hk = &less_hunks[less_nhunks];
        memset(hk, 0, sizeof(*hk));
        char hdr[512];
        int tlen = snprintf(hdr, sizeof(hdr), "+++ %s ---", dispname);
        if (tlen > 0) {
            u8p tp = LESSArenaWrite(hdr, (size_t)tlen);
            if (tp) { hk->title[0] = tp; hk->title[1] = tp + tlen; }
        }
        u8p txp = LESSArenaWrite(new_data[0], nlen2);
        if (txp) { hk->text[0] = txp; hk->text[1] = txp + nlen2; }
        // Arena-copy toks
        u8p tkp = LESSArenaWrite(new_ts[0],
                      $len(new_ts) * sizeof(u32));
        if (tkp) {
            hk->toks[0] = (u32cp)tkp;
            hk->toks[1] = (u32cp)(tkp +
                $len(new_ts) * sizeof(u32));
        }
        // Single hili entry: entire file is INS
        u32 ins_tok = tok32Pack('I', nlen2);
        u8p ihp = LESSArenaWrite(&ins_tok, sizeof(u32));
        if (ihp) {
            hk->hili[0] = (u32cp)ihp;
            hk->hili[1] = (u32cp)(ihp + sizeof(u32));
        }
        LESSHunkEmit();
        LESSRun(less_hunks, less_nhunks);
        LESSArenaCleanup();
        JOINFree(&new_f);
        if (map_new) FILEUnMap(map_new);
        done;
    }

    // Tokenize
    JOINfile old_f = {}, new_f = {};
    ok64 o = JOINTokenize(&old_f, old_data, ext);
    if (o != OK) goto diff_cleanup;
    o = JOINTokenize(&new_f, new_data, ext);
    if (o != OK) goto diff_cleanup;

    {
        u64 on = u64bDataLen(old_f.hashes);
        u64 nn = u64bDataLen(new_f.hashes);

        // Allocate diff workspace
        u64 wsize = DIFFWorkSize(on, nn);
        u64 emax = DIFFEdlMaxEntries(on, nn);
        u64 total = wsize * sizeof(i32) + emax * sizeof(e32);
        u8 *mem[4] = {};
        o = u8bAlloc(mem, total);
        if (o != OK) goto diff_cleanup;

        i32p workp = (i32p)mem[1];
        i32s ws = {workp, workp + wsize};
        e32 *edl_buf = (e32 *)(workp + wsize);
        e32g edl = {edl_buf, edl_buf + emax, edl_buf};

        CAPOJoinToks(old_ts, &old_f);
        CAPOJoinToks(new_ts, &new_f);

        u64cs oh = {old_f.hashes[1], old_f.hashes[2]};
        u64cs nh = {new_f.hashes[1], new_f.hashes[2]};
        o = DIFFu64s(edl, ws, oh, nh);
        if (o != OK) { u8bFree(mem); goto diff_cleanup; }

        // Semantic cleanup: remove false short equalities
        NEILCleanup(edl, old_ts, new_ts, old_data, new_data);

        // Lossless shift: align edit boundaries on line breaks
        NEILShift(edl, old_ts, new_ts, old_data, new_data);

        // dispname built above, before deleted/new branches

        // Walk EDL, emit colored output with context trimming.
        // Build u32 visible-line intervals, then emit per-token.

        #define CTX_LINES 3

        u32 nedl = (u32)(edl[0] - edl[2]);

        // Phase 1: scan EDL, track new-side line numbers.
        // For each change, record visible interval [lo, hi] as
        // a u32 pair: vis[2*i]=lo, vis[2*i+1]=hi.
        Bu32 visbuf = {};
        u32 *vis = NULL;
        u32 nvis = 0;
        if (nedl > 0 && u32bAlloc(visbuf, 2 * nedl) == OK)
            vis = visbuf[0];
        if (vis != NULL) {
            u64 sni = 0;
            u32 nl = 0;
            for (u32 k = 0; k < nedl; k++) {
                e32 e = edl[2][k];
                u32 elen = DIFF_LEN(e);
                u32 op = DIFF_OP(e);
                if (op == DIFF_EQ) {
                    for (u32 j = 0; j < elen; j++) {
                        u8cs v = {};
                        tok32Val(v,new_ts,new_f.data[0],(int)(sni + j));
                        $for(u8c, cp, v)
                            if (*cp == '\n') nl++;
                    }
                    sni += elen;
                } else if (op == DIFF_INS) {
                    u32 sl = nl;
                    for (u32 j = 0; j < elen; j++) {
                        u8cs v = {};
                        tok32Val(v,new_ts,new_f.data[0],(int)(sni + j));
                        $for(u8c, cp, v)
                            if (*cp == '\n') nl++;
                    }
                    u32 lo = (sl > CTX_LINES) ? sl - CTX_LINES : 0;
                    u32 hi = nl + CTX_LINES;
                    vis[nvis * 2] = lo;
                    vis[nvis * 2 + 1] = hi;
                    nvis++;
                    sni += elen;
                } else {  // DIFF_DEL
                    u32 lo = (nl > CTX_LINES) ? nl - CTX_LINES : 0;
                    u32 hi = nl + CTX_LINES;
                    vis[nvis * 2] = lo;
                    vis[nvis * 2 + 1] = hi;
                    nvis++;
                }
            }
            // Merge overlapping intervals (sorted by lo already)
            u32 m = 0;
            for (u32 i = 0; i < nvis; i++) {
                u32 lo = vis[i * 2], hi = vis[i * 2 + 1];
                if (m > 0 && lo <= vis[(m - 1) * 2 + 1]) {
                    if (hi > vis[(m - 1) * 2 + 1])
                        vis[(m - 1) * 2 + 1] = hi;
                } else {
                    vis[m * 2] = lo;
                    vis[m * 2 + 1] = hi;
                    m++;
                }
            }
            nvis = m;
        }

        // Phase 2: walk EDL, build LESS hunks.
        // Allocate text + toks + hili buffers in arena.
        // Worst case: all old bytes (DEL) + all new bytes (INS).
        u32 old_len = (u32)$len(old_data);
        u32 new_len = (u32)$len(new_data);
        u32 arena_need = old_len + new_len;
        u32 max_toks = (u32)($len(old_ts) + $len(new_ts) + 100);
        u8s diff_text_s = {};
        u8p dtxp = NULL;
        u32p dtokp = NULL, dhilp = NULL;
        LESShunk *cur_hunk = NULL;

        if (arena_need > 0 &&
            LESSArenaAlloc(diff_text_s, arena_need) == OK) {
            dtxp = diff_text_s[0];
            // Allocate toks and hili arrays in arena
            u8s toks_arena = {}, hili_arena = {};
            if (LESSArenaAlloc(toks_arena, max_toks * sizeof(u32)) == OK &&
                LESSArenaAlloc(hili_arena, max_toks * sizeof(u32)) == OK) {
                dtokp = (u32p)toks_arena[0];
                dhilp = (u32p)hili_arena[0];
            }
        }

        // Helper macros for copying tokens into the LESS buffer
        #define DIFF_COPY_TOK(toks_s, base, idx, hflag) do {   \
            u8cs _v = {};                                       \
            tok32Val(_v,toks_s,base,(int)(idx));              \
            u32 _n = (u32)$len(_v);                             \
            u8 _tag = tok32Tag((toks_s)[0][(idx)]);             \
            u32 _eoff = (u32)(dtxp - (u8p)cur_hunk->text[0]) + _n; \
            memcpy(dtxp, _v[0], _n);                            \
            dtxp += _n;                                         \
            if (dtokp) *dtokp++ = tok32Pack(_tag, _eoff);       \
            if (dhilp) *dhilp++ = tok32Pack(                    \
                (hflag) ? (hflag) : 'A', _eoff);               \
        } while(0)

        // Start a new LESS hunk with title
        #define DIFF_START_HUNK(boff) do {                      \
            if (cur_hunk != NULL) {                             \
                cur_hunk->text[1] = dtxp;                       \
                cur_hunk->toks[1] = (u32cp)dtokp;              \
                cur_hunk->hili[1] = (u32cp)dhilp;              \
                if (less_pipe_fd >= 0) {                        \
                    /* emit completed prev hunk to pipe */      \
                    u32 _pi = (u32)(cur_hunk - less_hunks);     \
                    HUNKhunk _ph = {};                          \
                    $mv(_ph.title, cur_hunk->title);            \
                    $mv(_ph.text, cur_hunk->text);              \
                    _ph.toks[0] = cur_hunk->toks[0];            \
                    _ph.toks[1] = cur_hunk->toks[1];            \
                    _ph.hili[0] = cur_hunk->hili[0];            \
                    _ph.hili[1] = cur_hunk->hili[1];            \
                    a_pad(u8, _pb, 1 << 16);                    \
                    if (HUNKu8sFeed(u8bIdle(_pb), &_ph) == OK) {\
                        u8cp _pp = _pb[1];                      \
                        u8cp _pe = _pb[2];                      \
                        while (_pp < _pe) {                     \
                            ssize_t _w = write(less_pipe_fd,    \
                                _pp, (size_t)(_pe - _pp));      \
                            if (_w <= 0) break;                 \
                            _pp += _w;                          \
                        }                                       \
                    }                                           \
                    less_nhunks = _pi; /* recycle */             \
                }                                               \
            }                                                   \
            if (less_nhunks < LESS_MAX_HUNKS) {                 \
                cur_hunk = &less_hunks[less_nhunks];            \
                *cur_hunk = (LESShunk){};                       \
                char _funcname[256];                            \
                CAPOFindFunc(new_data, (boff), ext,             \
                             _funcname, sizeof(_funcname));      \
                u8gp _tg = u8aOpen(less_arena);                  \
                call(CAPOFormatTitle, u8gRest(_tg),              \
                    dispname, _funcname);                       \
                u8cs _ttl = {};                                 \
                u8aClose(less_arena, _ttl);                     \
                if (!$empty(_ttl)) {                            \
                    cur_hunk->title[0] = _ttl[0];               \
                    cur_hunk->title[1] = _ttl[1];               \
                }                                               \
                cur_hunk->text[0] = dtxp;                       \
                cur_hunk->toks[0] = (u32cp)dtokp;              \
                cur_hunk->hili[0] = (u32cp)dhilp;              \
                less_nhunks++;                                  \
            }                                                   \
        } while(0)

        // Copy leading whitespace on current line into hunk
        #define DIFF_COPY_LINE_PREFIX(boff) do {                \
            u32 _ls = (boff);                                   \
            while (_ls > 0 && new_data[0][_ls-1] != '\n') _ls--;\
            if (_ls < (boff)) {                                 \
                u32 _pn = (boff) - _ls;                         \
                u32 _eoff = (u32)(dtxp - (u8p)cur_hunk->text[0]) + _pn; \
                memcpy(dtxp, new_data[0] + _ls, _pn);           \
                dtxp += _pn;                                    \
                if (dtokp) *dtokp++ = tok32Pack('S', _eoff);   \
                if (dhilp) *dhilp++ = tok32Pack('A', _eoff);   \
            }                                                   \
        } while(0)

        u64 oi = 0, ni = 0;
        u32 cur_line = 0, cur_iv = 0;
        b8 in_gap = YES;

        for (u32 k = 0; k < nedl; ) {
            e32 e = edl[2][k];
            if (DIFF_OP(e) == DIFF_EQ) {
                u32 len = DIFF_LEN(e);
                for (u32 j = 0; j < len; j++) {
                    while (cur_iv < nvis &&
                           vis[cur_iv * 2 + 1] < cur_line)
                        cur_iv++;
                    b8 show = (vis == NULL) ||
                        (cur_iv < nvis &&
                         cur_line >= vis[cur_iv * 2] &&
                         cur_line <= vis[cur_iv * 2 + 1]);
                    if (show) {
                        if (in_gap) {
                            u32 boff = (ni > 0) ? tok32Offset(new_ts[0][ni-1]) : 0;
                            DIFF_START_HUNK(boff);
                            DIFF_COPY_LINE_PREFIX(boff);
                            in_gap = NO;
                        }
                        DIFF_COPY_TOK(new_ts, new_f.data[0], ni, 0);
                    } else {
                        in_gap = YES;
                    }
                    u8cs v = {};
                    tok32Val(v,new_ts,new_f.data[0],(int)ni);
                    $for(u8c, cp, v)
                        if (*cp == '\n') cur_line++;
                    oi++; ni++;
                }
                k++;
            } else {
                // Change region: scan to end of consecutive DEL/INS
                u32 kend = k;
                while (kend < nedl && DIFF_OP(edl[2][kend]) != DIFF_EQ)
                    kend++;

                // Count total DEL and INS tokens in this region
                u32 del_total = 0, ins_total = 0;
                b8 ws_only = YES;
                for (u32 kk = k; kk < kend; kk++) {
                    u32 klen = DIFF_LEN(edl[2][kk]);
                    if (DIFF_OP(edl[2][kk]) == DIFF_DEL) {
                        del_total += klen;
                        for (u32 j = 0; j < klen && ws_only; j++)
                            if (!NEILIsWS(old_ts, old_f.data[0],
                                          oi + del_total - klen + j))
                                ws_only = NO;
                    } else {
                        ins_total += klen;
                        for (u32 j = 0; j < klen && ws_only; j++)
                            if (!NEILIsWS(new_ts, new_f.data[0],
                                          ni + ins_total - klen + j))
                                ws_only = NO;
                    }
                }

                // Whitespace-only changes: emit new side as EQ context
                if (ws_only && ins_total > 0) {
                    for (u32 j = 0; j < ins_total; j++) {
                        while (cur_iv < nvis &&
                               vis[cur_iv * 2 + 1] < cur_line)
                            cur_iv++;
                        b8 show = (vis == NULL) ||
                            (cur_iv < nvis &&
                             cur_line >= vis[cur_iv * 2] &&
                             cur_line <= vis[cur_iv * 2 + 1]);
                        if (show) {
                            if (in_gap) {
                                u64 ti = ni + j;
                                u32 boff = (ti > 0)
                                    ? tok32Offset(new_ts[0][ti-1]) : 0;
                                DIFF_START_HUNK(boff);
                                DIFF_COPY_LINE_PREFIX(boff);
                                in_gap = NO;
                            }
                            DIFF_COPY_TOK(new_ts, new_f.data[0], ni + j, 0);
                        } else {
                            in_gap = YES;
                        }
                        u8cs v = {};
                        tok32Val(v,new_ts,new_f.data[0],(int)(ni + j));
                        $for(u8c, cp, v)
                            if (*cp == '\n') cur_line++;
                    }
                    oi += del_total;
                    ni += ins_total;
                    k = kend;
                    continue;
                }

                // Extract common prefix: compare old/new token
                // source bytes to find misaligned EQ tokens.
                u32 prefix = 0;
                {
                    u32 lim = (del_total < ins_total)
                              ? del_total : ins_total;
                    u64 ti = oi, tj = ni;
                    while (prefix < lim) {
                        u8cs ov = {}, nv = {};
                        tok32Val(ov,old_ts,old_f.data[0],(int)ti);
                        tok32Val(nv,new_ts,new_f.data[0],(int)tj);
                        if ($len(ov) != $len(nv)) break;
                        if (memcmp(ov[0], nv[0], (size_t)$len(ov)))
                            break;
                        prefix++; ti++; tj++;
                    }
                }

                // Extract common suffix
                u32 suffix = 0;
                {
                    u32 lim = (del_total < ins_total)
                              ? del_total : ins_total;
                    lim -= prefix;
                    u64 ti = oi + del_total - 1;
                    u64 tj = ni + ins_total - 1;
                    while (suffix < lim) {
                        u8cs ov = {}, nv = {};
                        tok32Val(ov,old_ts,old_f.data[0],(int)ti);
                        tok32Val(nv,new_ts,new_f.data[0],(int)tj);
                        if ($len(ov) != $len(nv)) break;
                        if (memcmp(ov[0], nv[0], (size_t)$len(ov)))
                            break;
                        suffix++; ti--; tj--;
                    }
                }

                // Save base positions before prefix extraction
                u64 base_oi = oi, base_ni = ni;

                // Emit common prefix as EQ context
                for (u32 j = 0; j < prefix; j++) {
                    while (cur_iv < nvis &&
                           vis[cur_iv * 2 + 1] < cur_line)
                        cur_iv++;
                    b8 show = (vis == NULL) ||
                        (cur_iv < nvis &&
                         cur_line >= vis[cur_iv * 2] &&
                         cur_line <= vis[cur_iv * 2 + 1]);
                    if (show) {
                        if (in_gap) {
                            u64 ti = base_ni + j;
                            u32 boff = (ti > 0)
                                ? tok32Offset(new_ts[0][ti-1]) : 0;
                            DIFF_START_HUNK(boff);
                            DIFF_COPY_LINE_PREFIX(boff);
                            in_gap = NO;
                        }
                        DIFF_COPY_TOK(new_ts, new_f.data[0], base_ni + j, 0);
                    } else {
                        in_gap = YES;
                    }
                    u8cs v = {};
                    tok32Val(v,new_ts,new_f.data[0],(int)(base_ni + j));
                    $for(u8c, cp, v)
                        if (*cp == '\n') cur_line++;
                }

                if (in_gap) {
                    u32 boff = (base_ni > 0)
                        ? tok32Offset(new_ts[0][base_ni-1]) : 0;
                    DIFF_START_HUNK(boff);
                    in_gap = NO;
                }

                // Emit DEL tokens (excluding prefix/suffix)
                u64 toi = base_oi;
                for (u32 kk = k; kk < kend; kk++) {
                    if (DIFF_OP(edl[2][kk]) == DIFF_DEL) {
                        u32 dlen = DIFF_LEN(edl[2][kk]);
                        for (u32 j = 0; j < dlen; j++) {
                            u32 pos = (u32)(toi - base_oi);
                            if (pos >= prefix &&
                                pos < del_total - suffix) {
                                DIFF_COPY_TOK(old_ts, old_f.data[0],
                                              toi, 'D');
                            }
                            toi++;
                        }
                    }
                }

                // Newline separator between DEL and INS output:
                // if DEL text didn't end with '\n', insert one so
                // deletions and insertions don't merge onto one line.
                // Also copy the INS line's indentation so it aligns.
                if (del_total > prefix + suffix &&
                    ins_total > prefix + suffix &&
                    (dtxp == (u8p)cur_hunk->text[0] || *(dtxp - 1) == '\n')) {
                    u64 last_del = base_oi + del_total - suffix - 1;
                    u8cs ldv = {};
                    tok32Val(ldv,old_ts,old_f.data[0],(int)last_del);
                    if (!$empty(ldv) && *(ldv[1] - 1) != '\n') {
                        // Find indentation of first INS token's line
                        u64 ins_ti = base_ni + prefix;
                        u32 ins_boff = (ins_ti > 0)
                            ? tok32Offset(new_ts[0][ins_ti - 1]) : 0;
                        u32 ls = ins_boff;
                        while (ls > 0 && new_data[0][ls - 1] != '\n')
                            ls--;
                        u32 _nloff = (u32)(dtxp - (u8p)cur_hunk->text[0]) + 1;
                        *dtxp++ = '\n';
                        if (dtokp) *dtokp++ = tok32Pack('S', _nloff);
                        if (dhilp) *dhilp++ = tok32Pack('D', _nloff);
                        if (ls < ins_boff) {
                            u32 pn = ins_boff - ls;
                            u32 _poff = _nloff + pn;
                            memcpy(dtxp, new_data[0] + ls, pn);
                            dtxp += pn;
                            if (dtokp) *dtokp++ = tok32Pack('S', _poff);
                            if (dhilp) *dhilp++ = tok32Pack('I', _poff);
                        }
                    }
                }

                // Emit INS tokens (excluding prefix/suffix)
                u64 tni = base_ni;
                for (u32 kk = k; kk < kend; kk++) {
                    if (DIFF_OP(edl[2][kk]) == DIFF_INS) {
                        u32 ilen = DIFF_LEN(edl[2][kk]);
                        for (u32 j = 0; j < ilen; j++) {
                            u32 pos = (u32)(tni - base_ni);
                            if (pos >= prefix &&
                                pos < ins_total - suffix) {
                                DIFF_COPY_TOK(new_ts, new_f.data[0],
                                              tni, 'I');
                                u8cs v = {};
                                tok32Val(v,new_ts,new_f.data[0],(int)tni);
                                $for(u8c, cp, v)
                                    if (*cp == '\n') cur_line++;
                            }
                            tni++;
                        }
                    }
                }

                // Emit common suffix as EQ context
                for (u32 j = 0; j < suffix; j++) {
                    u64 sn = base_ni + ins_total - suffix + j;
                    while (cur_iv < nvis &&
                           vis[cur_iv * 2 + 1] < cur_line)
                        cur_iv++;
                    b8 show = (vis == NULL) ||
                        (cur_iv < nvis &&
                         cur_line >= vis[cur_iv * 2] &&
                         cur_line <= vis[cur_iv * 2 + 1]);
                    if (show) {
                        DIFF_COPY_TOK(new_ts, new_f.data[0], sn, 0);
                    } else {
                        in_gap = YES;
                    }
                    u8cs v = {};
                    tok32Val(v,new_ts,new_f.data[0],(int)sn);
                    $for(u8c, cp, v)
                        if (*cp == '\n') cur_line++;
                }

                oi = toi;
                ni = tni;
                k = kend;
            }
        }

        // Finalize last LESS hunk
        if (cur_hunk != NULL) {
            cur_hunk->text[1] = dtxp;
            cur_hunk->toks[1] = (u32cp)dtokp;
            cur_hunk->hili[1] = (u32cp)dhilp;
            if (less_pipe_fd >= 0) {
                u32 _pi = (u32)(cur_hunk - less_hunks);
                HUNKhunk _ph = {};
                $mv(_ph.title, cur_hunk->title);
                $mv(_ph.text, cur_hunk->text);
                _ph.toks[0] = cur_hunk->toks[0];
                _ph.toks[1] = cur_hunk->toks[1];
                _ph.hili[0] = cur_hunk->hili[0];
                _ph.hili[1] = cur_hunk->hili[1];
                a_pad(u8, _pb, 1 << 16);
                if (HUNKu8sFeed(u8bIdle(_pb), &_ph) == OK) {
                    u8cp _pp = _pb[1];
                    u8cp _pe = _pb[2];
                    while (_pp < _pe) {
                        ssize_t _w = write(less_pipe_fd,
                            _pp, (size_t)(_pe - _pp));
                        if (_w <= 0) break;
                        _pp += _w;
                    }
                }
                less_nhunks = _pi;  // recycle
            }
        }

        #undef DIFF_COPY_TOK
        #undef DIFF_START_HUNK
        #undef DIFF_COPY_LINE_PREFIX

        u32bFree(visbuf);
        u8bFree(mem);
    }

    if (less_nhunks > 0)
        LESSRun(less_hunks, less_nhunks);
    LESSArenaCleanup();

diff_cleanup:
    JOINFree(&old_f);
    JOINFree(&new_f);
    if (map_old) FILEUnMap(map_old);
    if (map_new) FILEUnMap(map_new);
    return o;
}
