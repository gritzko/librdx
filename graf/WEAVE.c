//  WEAVE: interleaved token-level file history.
//
//  Incrementally built by diffing adjacent blob versions.
//  Double-buffer swap avoids splice overhead.
//
#include "WEAVE.h"

#include <string.h>

#include "abc/DIFF.h"
#include "abc/PRO.h"
#include "abc/RAP.h"
#include "dog/TOK.h"

// u64 diff for token hash arrays
#define X(M, name) M##u64##name
#include "abc/DIFFx.h"
#undef X

// --- Tokenizer: produces parallel tok32 + u64 hash + token slices ---

typedef struct {
    Bu32 toks;
    Bu64 hashes;
    u8cp base;
} weave_tok_ctx;

static ok64 weave_tok_cb(u8 tag, u8cs tok, void *vctx) {
    sane(vctx);
    weave_tok_ctx *ctx = vctx;
    u32 end = (u32)(tok[1] - ctx->base);
    call(u32bFeed1, ctx->toks, tok32Pack(tag, end));
    call(u64bFeed1, ctx->hashes, RAPHash(tok));
    done;
}

static ok64 weave_tokenize(weave_tok_ctx *ctx, u8csc data, u8cs ext) {
    sane(ctx);
    ctx->base = data[0];
    TOKstate st = {.data = {data[0], data[1]}, .cb = weave_tok_cb, .ctx = ctx};
    call(TOKLexer, &st, ext);
    done;
}

// --- Extract token slice from data + tok32 array ---

static void weave_tok_slice(u8cs out, u8cp base, u32cp toks, u32 i) {
    u32 start = (i == 0) ? 0 : tok32Offset(toks[i - 1]);
    u32 end = tok32Offset(toks[i]);
    out[0] = base + start;
    out[1] = base + end;
}

// --- Init ---

#define WEAVE_TEXT_MAX  (64UL << 20)  // 64 MB for token text
#define WEAVE_TOK_MAX   (1 << 20)    // 1M wtok entries

ok64 WEAVEInit(weave *w, size_t est_tokens) {
    sane(w);
    (void)est_tokens;
    memset(w, 0, sizeof(*w));
    call(u8bMap, w->text, WEAVE_TEXT_MAX);
    call(wtokbMap, w->src, WEAVE_TOK_MAX);
    call(wtokbMap, w->dst, WEAVE_TOK_MAX);
    done;
}

// --- Add a version ---

ok64 WEAVEAdd(weave *w, u8cs old_data, u8cs new_data,
              u8cs ext, u32 gen) {
    sane(w);

    // All buffers declared up front for goto-safe cleanup
    weave_tok_ctx old_ctx = {};
    weave_tok_ctx new_ctx = {};
    Bi32 work = {};
    Bu32 edlbuf = {};

    u64 est = $len(new_data);
    if (est < 256) est = 256;
    __ = u32bAlloc(old_ctx.toks, est); if (__ != OK) goto cleanup;
    __ = u64bAlloc(old_ctx.hashes, est); if (__ != OK) goto cleanup;
    __ = u32bAlloc(new_ctx.toks, est); if (__ != OK) goto cleanup;
    __ = u64bAlloc(new_ctx.hashes, est); if (__ != OK) goto cleanup;

    if (!$empty(old_data))
        weave_tokenize(&old_ctx, old_data, ext);  // best effort
    weave_tokenize(&new_ctx, new_data, ext);       // best effort

    // If tokenizer produced nothing, treat whole content as one token
    if (u64bDataLen(new_ctx.hashes) == 0 && !$empty(new_data)) {
        u64 h = RAPHash(new_data);
        u32 end = (u32)$len(new_data);
        u32bFeed1(new_ctx.toks, tok32Pack('S', end));
        u64bFeed1(new_ctx.hashes, h);
    }
    if (u64bDataLen(old_ctx.hashes) == 0 && !$empty(old_data)) {
        u64 h = RAPHash(old_data);
        u32 end = (u32)$len(old_data);
        u32bFeed1(old_ctx.toks, tok32Pack('S', end));
        u64bFeed1(old_ctx.hashes, h);
    }

    u64 olen = u64bDataLen(old_ctx.hashes);
    u64 nlen = u64bDataLen(new_ctx.hashes);

    u64 work_sz = DIFFWorkSize(olen, nlen);
    u64 edl_sz = DIFFEdlMaxEntries(olen, nlen);
    if (work_sz > 0) { __ = i32bAllocate(work, work_sz); if (__ != OK) goto cleanup; }
    if (edl_sz > 0) { __ = u32bAllocate(edlbuf, edl_sz); if (__ != OK) goto cleanup; }

    u64cs oh = {u64bDataHead(old_ctx.hashes),
                u64bDataHead(old_ctx.hashes) + olen};
    u64cs nh = {u64bDataHead(new_ctx.hashes),
                u64bDataHead(new_ctx.hashes) + nlen};
    e32g edlg = {edlbuf[0], edlbuf[3], edlbuf[0]};
    i32s ws = {i32bHead(work), i32bTerm(work)};
    ok64 diff_o = DIFFu64s(edlg, ws, oh, nh);
    if (diff_o != OK) {
        // Fallback: treat as full delete + full insert
        edlg[1] = edlg[0];
        if (olen > 0) { *edlg[1]++ = DIFF_ENTRY(DIFF_DEL, (u32)olen); }
        if (nlen > 0) { *edlg[1]++ = DIFF_ENTRY(DIFF_INS, (u32)nlen); }
    }
    e32cs edl = {edlbuf[0], edlg[0]};  // edlg[0] advanced past last entry

    // First version: no old weave, just insert everything
    if (wtokbDataLen(w->src) == 0 && $empty(old_data)) {
        u32cp ntoks = u32bDataHead(new_ctx.toks);
        for (u64 i = 0; i < nlen; i++) {
            // Append token text to w->text
            u8cs ts = {};
            weave_tok_slice(ts, new_data[0], ntoks, (u32)i);
            size_t tlen = (size_t)(ts[1] - ts[0]);
            call(u8bReserve, w->text, tlen);
            u8p tstart = u8bIdleHead(w->text);
            memcpy(tstart, ts[0], tlen);
            u8 **ti = u8bIdle(w->text);
            *ti += tlen;

            wtok wt = {
                .tok = {tstart, tstart + tlen},
                .intro_gen = gen,
                .del_gen = 0
            };
            call(wtokbPush, w->src, &wt);
        }
        goto cleanup;
    }

    // Subsequent version: walk EDL, merge old weave with diff
    {
        wtokbReset(w->dst);
        wtokcp wsrc = wtokbDataHead(w->src);
        u32 wi = 0;   // index into old weave (all tokens incl deleted)
        u32 wlen = (u32)wtokbDataLen(w->src);
        u32 ni = 0;   // index into new token sequence

        u32cp ntoks = u32bDataHead(new_ctx.toks);

        $for(e32c, ep, edl) {
            u32 len = DIFF_LEN(*ep);
            switch (DIFF_OP(*ep)) {
            case DIFF_EQ:
                // Copy weave tokens matching these old alive tokens
                for (u32 j = 0; j < len; j++) {
                    while (wi < wlen && wsrc[wi].del_gen != 0) {
                        call(wtokbPush, w->dst, &wsrc[wi]);
                        wi++;
                    }
                    if (wi < wlen) {
                        call(wtokbPush, w->dst, &wsrc[wi]);
                        wi++;
                    }
                    ni++;
                }
                break;

            case DIFF_DEL:
                // Mark weave tokens as deleted at this gen
                for (u32 j = 0; j < len; j++) {
                    while (wi < wlen && wsrc[wi].del_gen != 0) {
                        call(wtokbPush, w->dst, &wsrc[wi]);
                        wi++;
                    }
                    if (wi < wlen) {
                        wtok del = wsrc[wi];
                        del.del_gen = gen;
                        call(wtokbPush, w->dst, &del);
                        wi++;
                    }
                }
                break;

            case DIFF_INS:
                // Insert new tokens into the weave
                for (u32 j = 0; j < len; j++) {
                    u8cs ts = {};
                    weave_tok_slice(ts, new_data[0], ntoks, ni);
                    size_t tlen = (size_t)(ts[1] - ts[0]);
                    call(u8bReserve, w->text, tlen);
                    u8p tstart = u8bIdleHead(w->text);
                    memcpy(tstart, ts[0], tlen);
                    u8 **ti = u8bIdle(w->text);
                    *ti += tlen;

                    wtok wt = {
                        .tok = {tstart, tstart + tlen},
                        .intro_gen = gen,
                        .del_gen = 0
                    };
                    call(wtokbPush, w->dst, &wt);
                    ni++;
                }
                break;
            }
        }
        // Flush remaining deleted weave tokens
        while (wi < wlen) {
            call(wtokbPush, w->dst, &wsrc[wi]);
            wi++;
        }

        // Swap src ↔ dst
        Bwtok tmp;
        memcpy(tmp, w->src, sizeof(Bwtok));
        memcpy(w->src, w->dst, sizeof(Bwtok));
        memcpy(w->dst, tmp, sizeof(Bwtok));
    }

cleanup:
    u32bFree(old_ctx.toks);
    u64bFree(old_ctx.hashes);
    u32bFree(new_ctx.toks);
    u64bFree(new_ctx.hashes);
    i32bFree(work);
    u32bFree(edlbuf);
    done;
}

// --- Free ---

void WEAVEFree(weave *w) {
    if (!w) return;
    if (w->text[0]) u8bUnMap(w->text);
    if (w->src[0]) wtokbUnMap(w->src);
    if (w->dst[0]) wtokbUnMap(w->dst);
    memset(w, 0, sizeof(*w));
}
