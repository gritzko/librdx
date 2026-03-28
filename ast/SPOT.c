#include "SPOT.h"

#include <string.h>

#include "abc/PRO.h"
#include "ast/BAST.h"
#include "json/BASON.h"

// Internal binding state: slices for fast comparison, offsets for result.
typedef struct {
    u8cs slices[SPOT_MAX_BINDS];  // value slices for comparison
    u64  offsets[SPOT_MAX_BINDS]; // TLV tag byte offsets for result
    u32  srclo[SPOT_MAX_BINDS];   // source byte range lo
    u32  srchi[SPOT_MAX_BINDS];   // source byte range hi
    u64  bound;
    u64  subs[SPOT_MAX_SUBS];    // segment start offsets
    int  nsubs;
    u32  match_container;         // parent container of first matched leaf
    u64p mlog[4];  // match log (copied from SPOTstate)
    u64p alog[4];  // alias log (copied from SPOTstate)
    SPOTrange bind_ranges[SPOT_MAX_BINDS];
    SPOTrangep ranges[4];  // caller-provided range buffer
} SPOTbinds;

// Check if a leaf is a placeholder: any non-container with single A-Z/a-z value
static b8 SPOTIsPlaceholder(u8 type, u8cs val) {
    if (BASONCollection(type)) return (NO);
    if ($len(val) != 1) return (NO);
    u8 c = val[0][0];
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// Placeholder index: a-z -> 0-25, A-Z -> 26-51
static int SPOTBindIndex(u8 c) {
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= 'A' && c <= 'Z') return 26 + (c - 'A');
    return (-1);
}

static b8 SPOTIsLower(u8 c) { return c >= 'a' && c <= 'z'; }

// Check if an S leaf is pure whitespace
static b8 SPOTIsWhitespace(u8cs val) {
    $for(u8c, p, val) {
        if (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') return (NO);
    }
    return (YES);
}

// Count leading spaces in an S leaf value
static int SPOTCountSpaces(u8cs val) {
    int n = 0;
    $for(u8c, p, val) {
        if (*p == ' ')
            n++;
        else
            break;
    }
    return (n);
}

// Compare two const slices for equality
static b8 SPOTSliceEq(u8cs a, u8cs b) {
    size_t alen = (size_t)$len(a);
    size_t blen = (size_t)$len(b);
    if (alen != blen) return (NO);
    if (alen == 0) return (YES);
    return memcmp(a[0], b[0], alen) == 0;
}

// --- Flat walk engine ---

// Advance to the next meaningful leaf in a DFS walk.
// Container nodes are entered automatically; when a level is exhausted
// we pop up.  Whitespace and comment leaves are skipped (but src_pos
// is advanced by their length).
// On success: *type, val, *boff are set to the leaf's info.
// Returns OK on leaf, SPOTEND when tree is exhausted.
// srclo/srchi: source byte range of the returned leaf
// hay_out: if non-NULL, receives BASON slice for the leaf [cursor, val end)
static ok64 SPOTFlatNext(u64bp stk, u8csc data, int *depth,
                          u64 *src_pos, u32 *parents,
                          u8p type, u8cs val, u32p boff,
                          u32p srclo, u32p srchi,
                          u8cs hay_out) {
    sane(stk != NULL);
    for (;;) {
        u32 cursor = (u32)*u64bLast(stk);
        u8cs key = {};
        ok64 o = BASONDrain(stk, data, type, key, val);
        if (o != OK) {
            if (*depth <= 0) return (SPOTEND);
            BASONOuto(stk);
            (*depth)--;
            continue;
        }
        if (BASONCollection(*type)) {
            (*depth)++;
            if (*depth < 64) parents[*depth] = cursor;
            call(BASONInto, stk, data, val);
            continue;
        }
        // Leaf
        size_t vlen = (size_t)$len(val);
        if ((*type == 'S' && SPOTIsWhitespace(val)) || *type == 'D') {
            *src_pos += vlen;
            continue;
        }
        if (vlen == 0) continue;
        *boff = cursor;
        *srclo = (u32)*src_pos;
        *src_pos += vlen;
        *srchi = (u32)*src_pos;
        if (hay_out != NULL) {
            hay_out[0] = data[0] + cursor;
            hay_out[1] = val[1];
        }
        return (OK);
    }
}

// --- Flatten needle into token array ---

static ok64 SPOTFlattenNeedle(SPOTntok *ntoks, int *nntoks,
                                u8csc ndata) {
    sane(ntoks != NULL);
    aBpad(u64, stk, 256);
    call(BASONOpen, stk, ndata);
    int depth = 0;
    u64 src_pos = 0;
    u32 parents[64] = {};
    *nntoks = 0;
    b8 pending_skip = NO;
    u64 prev_src_end = 0;

    for (;;) {
        // Track whitespace gaps between meaningful tokens
        u32 cursor = (u32)*u64bLast(stk);
        u8 t = 0;
        u8cs key = {}, val = {};
        ok64 o = BASONDrain(stk, ndata, &t, key, val);
        if (o != OK) {
            if (depth <= 0) break;
            BASONOuto(stk);
            depth--;
            continue;
        }
        if (BASONCollection(t)) {
            depth++;
            if (depth < 64) parents[depth] = cursor;
            call(BASONInto, stk, ndata, val);
            continue;
        }
        size_t vlen = (size_t)$len(val);
        if (t == 'D') {
            src_pos += vlen;
            continue;
        }
        if (t == 'S' && SPOTIsWhitespace(val)) {
            int sp = SPOTCountSpaces(val);
            if (sp >= 2) pending_skip = YES;
            src_pos += vlen;
            continue;
        }
        if (vlen == 0) continue;
        // Meaningful leaf
        if (*nntoks >= SPOT_MAX_NTOKS) fail(SPOTBAD);
        ntoks[*nntoks].type = t;
        ntoks[*nntoks].val[0] = val[0];
        ntoks[*nntoks].val[1] = val[1];
        ntoks[*nntoks].skip = (*nntoks > 0) ? pending_skip : NO;
        ntoks[*nntoks].parent = (depth < 64) ? parents[depth] : 0;
        (*nntoks)++;
        pending_skip = NO;
        prev_src_end = src_pos + vlen;
        src_pos += vlen;
    }
    done;
}

// --- Flat matching engine ---

// Save/restore state for backtracking
typedef struct {
    u64  stk_data[256];
    int  stk_used;
    int  depth;
    u64  src_pos;
    u32  parents[64];
    SPOTbinds binds;
} SPOTsave;

static void SPOTSaveState(SPOTsave *s, u64bp stk, int depth,
                           u64 src_pos, u32 *parents, SPOTbinds *b) {
    s->stk_used = (int)(stk[2] - stk[0]);
    if (s->stk_used > 256) s->stk_used = 256;
    memcpy(s->stk_data, stk[0], (size_t)s->stk_used * sizeof(u64));
    s->depth = depth;
    s->src_pos = src_pos;
    int pcopy = depth + 1;
    if (pcopy > 64) pcopy = 64;
    if (pcopy > 0) memcpy(s->parents, parents, (size_t)pcopy * sizeof(u32));
    s->binds = *b;
}

static void SPOTRestoreState(SPOTsave *s, u64bp stk, int *depth,
                              u64 *src_pos, u32 *parents, SPOTbinds *b) {
    memcpy(stk[0], s->stk_data, (size_t)s->stk_used * sizeof(u64));
    ((u64p *)stk)[2] = stk[0] + s->stk_used;
    *depth = s->depth;
    *src_pos = s->src_pos;
    int pcopy = s->depth + 1;
    if (pcopy > 64) pcopy = 64;
    if (pcopy > 0) memcpy(parents, s->parents, (size_t)pcopy * sizeof(u32));
    *b = s->binds;
}

// Check if a leaf value is an open/close bracket. Returns +1 for open, -1
// for close, 0 otherwise.
static int SPOTBracketDir(u8cs val) {
    if ($len(val) != 1) return (0);
    u8 c = val[0][0];
    if (c == '{' || c == '(' || c == '[') return (1);
    if (c == '}' || c == ')' || c == ']') return (-1);
    return (0);
}

// Match needle tokens[from..nntoks) against haystack leaves from current
// stk position.  Recursive backtracking for skip tokens.
// brace: current bracket depth (skip scans stop if depth goes negative).
static ok64 SPOTMatchFlat(SPOTbinds *b, SPOTntok *ntoks, int nntoks,
                           int from, u64bp stk, u8csc data,
                           int *depth, u64 *src_pos, u32 *parents,
                           u32 *match_srclo, u32 *match_srchi,
                           int brace) {
    sane(b != NULL);
    if (from >= nntoks) done;

    SPOTntok *cur = &ntoks[from];

    if (!cur->skip) {
        // EXACT: get next leaf, must match
        u8 ht = 0;
        u8cs hv = {};
        u32 hoff = 0;
        u32 leaf_srclo = 0, leaf_srchi = 0;
        u8cs leaf_hay = {};
        ok64 o = SPOTFlatNext(stk, data, depth, src_pos, parents,
                               &ht, hv, &hoff, &leaf_srclo, &leaf_srchi,
                               leaf_hay);
        if (o != OK) fail(SPOTBAD);

        // Record first token position for segment tracking
        if (from == 0) {
            if (b->nsubs < SPOT_MAX_SUBS)
                b->subs[b->nsubs++] = (u64)hoff;
            *match_srclo = leaf_srclo;
            b->match_container = (*depth < 64) ? parents[*depth] : 0;
        }
        *match_srchi = leaf_srchi;

        // Check placeholder
        if (SPOTIsPlaceholder(cur->type, cur->val)) {
            u8 c = cur->val[0][0];
            int idx = SPOTBindIndex(c);
            if (idx < 0) fail(SPOTBAD);
            u64 bit = 1ULL << idx;

            if (SPOTIsLower(c)) {
                // Lowercase: bind to single leaf value
                if (BASONCollection(ht)) fail(SPOTBAD);
                if (b->bound & bit) {
                    if (!SPOTSliceEq(b->slices[idx], hv)) fail(SPOTBAD);
                } else {
                    b->slices[idx][0] = hv[0];
                    b->slices[idx][1] = hv[1];
                    b->offsets[idx] = (u64)hoff;
                    b->srclo[idx] = leaf_srclo;
                    b->srchi[idx] = leaf_srchi;
                    b->bound |= bit;
                    b->bind_ranges[idx] = (SPOTrange){
                        .hay = {leaf_hay[0], leaf_hay[1]},
                        .ndl = {cur->val[0], cur->val[1]}};
                    if (b->ranges[0] != NULL) {
                        SPOTrange r = b->bind_ranges[idx];
                        SPOTrangebFeed1(b->ranges, r);
                    }
                    if (b->alog[0] != NULL) {
                        u64 ae = SPOTLogPack(hoff, 0, 0);
                        u64bFeed1(b->alog, ae);
                    }
                }
            } else {
                // Uppercase: consume leaves until remaining needle matches
                if (b->bound & bit) {
                    // Already bound — multi-leaf; compare source ranges
                    // by length (source text identity assumed from range)
                    u32 bound_len = b->srchi[idx] - b->srclo[idx];
                    // Consume same number of source bytes
                    u32 cap_end = leaf_srclo + bound_len;
                    // Advance past leaves until we've consumed enough
                    u32 consumed = leaf_srchi;
                    while (consumed < cap_end) {
                        u8 ht2 = 0;
                        u8cs hv2 = {};
                        u32 hoff2 = 0;
                        u32 sl2 = 0, sh2 = 0;
                        ok64 o2 = SPOTFlatNext(stk, data, depth, src_pos,
                                                parents, &ht2, hv2, &hoff2,
                                                &sl2, &sh2, NULL);
                        if (o2 != OK) fail(SPOTBAD);
                        consumed = sh2;
                    }
                    if (consumed != cap_end) fail(SPOTBAD);
                    *match_srchi = consumed;
                } else {
                    // Bind first leaf, then try remaining. If remaining
                    // fails, extend by consuming another leaf.
                    u32 cap_srclo = leaf_srclo;
                    u32 cap_srchi = leaf_srchi;
                    u32 cap_off = hoff;
                    b->slices[idx][0] = hv[0];
                    b->slices[idx][1] = hv[1];
                    b->offsets[idx] = (u64)hoff;
                    b->srclo[idx] = cap_srclo;
                    b->srchi[idx] = cap_srchi;
                    b->bound |= bit;
                    b->bind_ranges[idx] = (SPOTrange){
                        .hay = {leaf_hay[0], leaf_hay[1]},
                        .ndl = {cur->val[0], cur->val[1]}};
                    if (b->ranges[0] != NULL) {
                        SPOTrange r = b->bind_ranges[idx];
                        SPOTrangebFeed1(b->ranges, r);
                    }
                    if (b->alog[0] != NULL) {
                        u64 ae = SPOTLogPack(hoff, 0, 0);
                        u64bFeed1(b->alog, ae);
                    }

                    // Push match log entry for first token of each segment
                    if (from == 0 && b->mlog[0] != NULL) {
                        u16 alen2 = b->alog[0] != NULL
                                        ? (u16)u64bDataLen(b->alog) : 0;
                        u64 me = SPOTLogPack(hoff, 0, alen2);
                        u64bFeed1(b->mlog, me);
                    }

                    // Try shortest-first: try matching remaining, if fail
                    // consume more leaves.  Track bracket depth —
                    // stop extending if we'd escape the current scope.
                    int cap_brace = SPOTBracketDir(hv);
                    for (;;) {
                        *match_srchi = cap_srchi;
                        SPOTsave sv;
                        SPOTSaveState(&sv, stk, *depth, *src_pos,
                                      parents, b);
                        ok64 r = SPOTMatchFlat(b, ntoks, nntoks, from + 1,
                                               stk, data, depth, src_pos,
                                               parents, match_srclo,
                                               match_srchi, brace);
                        if (r == OK) done;
                        SPOTRestoreState(&sv, stk, depth, src_pos,
                                         parents, b);

                        // Consume one more leaf
                        u8 ht2 = 0;
                        u8cs hv2 = {};
                        u32 hoff2 = 0;
                        u32 sl2 = 0, sh2 = 0;
                        ok64 o2 = SPOTFlatNext(stk, data, depth, src_pos,
                                                parents, &ht2, hv2, &hoff2,
                                                &sl2, &sh2, NULL);
                        if (o2 != OK) fail(SPOTBAD);
                        cap_brace += SPOTBracketDir(hv2);
                        if (cap_brace < 0) fail(SPOTBAD);
                        cap_srchi = sh2;
                        b->srchi[idx] = cap_srchi;
                    }
                }
            }
        } else {
            // Literal: match by value (type-agnostic)
            if (!SPOTSliceEq(cur->val, hv)) fail(SPOTBAD);
            // Track bracket depth
            brace += SPOTBracketDir(cur->val);
        }

        // Push range for this literal match
        if (b->ranges[0] != NULL) {
            SPOTrange r = {
                .hay = {leaf_hay[0], leaf_hay[1]},
                .ndl = {cur->val[0], cur->val[1]}};
            SPOTrangebFeed1(b->ranges, r);
        }

        // Push match log entry for first token of each segment
        if (from == 0 && b->mlog[0] != NULL) {
            u16 alen = b->alog[0] != NULL ? (u16)u64bDataLen(b->alog) : 0;
            u64 me = SPOTLogPack(hoff, 0, alen);
            u64bFeed1(b->mlog, me);
        }

        // Recurse for remaining
        call(SPOTMatchFlat, b, ntoks, nntoks, from + 1, stk, data,
             depth, src_pos, parents, match_srclo, match_srchi, brace);
        done;
    }

    // SKIP: scan forward trying each leaf position with backtracking.
    // Save bindings before each attempt; restore on failure.
    // Cursor (stk/depth/src_pos/parents) advances — only binds are restored.
    // Bracket depth is tracked: skip scans stay within the current scope.
    // Special case: if the needle token is a closing bracket (})]  ),
    // match only at scan_brace == -1 (the bracket closing our scope).
    {
        SPOTbinds saved_binds = *b;
        int scan_brace = 0;  // bracket depth within the scan
        b8 is_close_ph = NO;
        int ndl_bdir = 0;
        if (!SPOTIsPlaceholder(cur->type, cur->val)) {
            ndl_bdir = SPOTBracketDir(cur->val);
            is_close_ph = ndl_bdir < 0;
        }
        for (;;) {
            // Save full state so match recursion can be rolled back
            SPOTsave saved;
            SPOTSaveState(&saved, stk, *depth, *src_pos, parents, b);

            u8 ht = 0;
            u8cs hv = {};
            u32 hoff = 0;
            u32 leaf_srclo = 0, leaf_srchi = 0;
            u8cs leaf_hay = {};
            ok64 o = SPOTFlatNext(stk, data, depth, src_pos, parents,
                                   &ht, hv, &hoff, &leaf_srclo, &leaf_srchi,
                                   leaf_hay);
            if (o != OK) fail(SPOTBAD);

            // Track bracket depth of scanned leaves.
            int bd = SPOTBracketDir(hv);
            scan_brace += bd;
            // When seeking a closing bracket, allow scan_brace to reach -1
            // (that's the matching bracket). For other tokens, fail if we
            // escape the current scope.
            if (is_close_ph) {
                if (scan_brace < -brace) fail(SPOTBAD);
            } else {
                if (scan_brace < 0) fail(SPOTBAD);
            }

            // Save cursor state AFTER consuming this leaf (for retry)
            SPOTsave post_leaf;
            SPOTSaveState(&post_leaf, stk, *depth, *src_pos, parents, b);

            // Record new segment
            if (b->nsubs < SPOT_MAX_SUBS)
                b->subs[b->nsubs++] = (u64)hoff;

            // Try matching this token
            b8 is_ph = SPOTIsPlaceholder(cur->type, cur->val);
            u8 ph_c = is_ph ? cur->val[0][0] : 0;
            int ph_idx = is_ph ? SPOTBindIndex(ph_c) : -1;
            b8 is_upper = is_ph && !SPOTIsLower(ph_c);

            if (is_upper && ph_idx >= 0) {
                // Uppercase placeholder: shortest-first multi-leaf
                u64 bit = 1ULL << ph_idx;
                if (b->bound & bit) {
                    // Already bound: skip and try next leaf
                    // (rebinding uppercase at different positions is rare)
                    *b = saved_binds;
                    continue;
                }
                b->slices[ph_idx][0] = hv[0];
                b->slices[ph_idx][1] = hv[1];
                b->offsets[ph_idx] = (u64)hoff;
                b->srclo[ph_idx] = leaf_srclo;
                b->srchi[ph_idx] = leaf_srchi;
                b->bound |= bit;
                b->bind_ranges[ph_idx] = (SPOTrange){
                    .hay = {leaf_hay[0], leaf_hay[1]},
                    .ndl = {cur->val[0], cur->val[1]}};
                if (b->ranges[0] != NULL) {
                    SPOTrange r = b->bind_ranges[ph_idx];
                    SPOTrangebFeed1(b->ranges, r);
                }
                if (b->alog[0] != NULL) {
                    u64 ae = SPOTLogPack(hoff, 0, 0);
                    u64bFeed1(b->alog, ae);
                }
                if (b->mlog[0] != NULL) {
                    u16 al = b->alog[0] != NULL
                                 ? (u16)u64bDataLen(b->alog) : 0;
                    u64 me = SPOTLogPack(hoff, 0, al);
                    u64bFeed1(b->mlog, me);
                }
                u32 cap_srchi = leaf_srchi;
                int cap_brace2 = SPOTBracketDir(hv);
                for (;;) {
                    *match_srchi = cap_srchi;
                    SPOTsave sv;
                    SPOTSaveState(&sv, stk, *depth, *src_pos, parents, b);
                    ok64 r = SPOTMatchFlat(b, ntoks, nntoks, from + 1,
                                            stk, data, depth, src_pos,
                                            parents, match_srclo,
                                            match_srchi,
                                            brace + scan_brace);
                    if (r == OK) done;
                    SPOTRestoreState(&sv, stk, depth, src_pos, parents, b);
                    // Consume one more leaf to extend capture
                    u8 ht2 = 0;
                    u8cs hv2 = {};
                    u32 hoff2 = 0;
                    u32 sl2 = 0, sh2 = 0;
                    ok64 o2 = SPOTFlatNext(stk, data, depth, src_pos,
                                            parents, &ht2, hv2, &hoff2,
                                            &sl2, &sh2, NULL);
                    if (o2 != OK) break;
                    cap_brace2 += SPOTBracketDir(hv2);
                    if (cap_brace2 < 0) break;
                    cap_srchi = sh2;
                    b->srchi[ph_idx] = cap_srchi;
                }
                // Multi-leaf extend exhausted; restore binds, advance cursor
                // Restore to post-leaf state (cursor past the first leaf)
                SPOTRestoreState(&post_leaf, stk, depth, src_pos,
                                 parents, b);
                *b = saved_binds;
                continue;
            }

            b8 tok_match = NO;
            if (is_ph && ph_idx >= 0 && SPOTIsLower(ph_c)) {
                if (!BASONCollection(ht)) {
                    u64 bit = 1ULL << ph_idx;
                    if (b->bound & bit) {
                        tok_match = SPOTSliceEq(b->slices[ph_idx], hv);
                    } else {
                        b->slices[ph_idx][0] = hv[0];
                        b->slices[ph_idx][1] = hv[1];
                        b->offsets[ph_idx] = (u64)hoff;
                        b->srclo[ph_idx] = leaf_srclo;
                        b->srchi[ph_idx] = leaf_srchi;
                        b->bound |= bit;
                        b->bind_ranges[ph_idx] = (SPOTrange){
                            .hay = {leaf_hay[0], leaf_hay[1]},
                            .ndl = {cur->val[0], cur->val[1]}};
                        if (b->ranges[0] != NULL) {
                            SPOTrange r = b->bind_ranges[ph_idx];
                            SPOTrangebFeed1(b->ranges, r);
                        }
                        if (b->alog[0] != NULL) {
                            u64 ae = SPOTLogPack(hoff, 0, 0);
                            u64bFeed1(b->alog, ae);
                        }
                        tok_match = YES;
                    }
                }
            } else if (!is_ph) {
                tok_match = SPOTSliceEq(cur->val, hv);
                // For closing brackets: only match at the right depth
                if (tok_match && is_close_ph && scan_brace != -brace)
                    tok_match = NO;
            }

            if (tok_match) {
                *match_srchi = leaf_srchi;
                // Push range for this skip+match
                if (b->ranges[0] != NULL) {
                    SPOTrange r = {
                        .hay = {leaf_hay[0], leaf_hay[1]},
                        .ndl = {cur->val[0], cur->val[1]}};
                    SPOTrangebFeed1(b->ranges, r);
                }
                if (b->mlog[0] != NULL) {
                    u16 alen = b->alog[0] != NULL
                                   ? (u16)u64bDataLen(b->alog) : 0;
                    u64 me = SPOTLogPack(hoff, 0, alen);
                    u64bFeed1(b->mlog, me);
                }
                // Save state for match recursion
                SPOTsave mr;
                SPOTSaveState(&mr, stk, *depth, *src_pos, parents, b);
                ok64 r = SPOTMatchFlat(b, ntoks, nntoks, from + 1, stk,
                                        data, depth, src_pos, parents,
                                        match_srclo, match_srchi,
                                        brace + scan_brace);
                if (r == OK) done;
                // Restore to post-leaf cursor with clean binds
                SPOTRestoreState(&post_leaf, stk, depth, src_pos,
                                 parents, b);
            }

            // Restore binds for next attempt, cursor stays advanced
            *b = saved_binds;
        }
    }
}

// --- SPOTInit ---

ok64 SPOTInit(SPOTstate *st, u8bp ndl_buf, u64bp ndl_idx,
              u8csc needle_src, u8csc ext, u8csc hay) {
    sane(st != NULL && ndl_buf != NULL);
    memset(st, 0, sizeof(SPOTstate));

    // Parse needle source into BASON
    call(BASTParse, ndl_buf, ndl_idx, needle_src, ext);

    st->ndl[0] = u8bDataHead(ndl_buf);
    st->ndl[1] = u8bIdleHead(ndl_buf);
    st->hay[0] = hay[0];
    st->hay[1] = hay[1];

    if ($empty(st->ndl)) fail(SPOTBAD);
    if ($empty(st->hay)) fail(SPOTBAD);

    // Initialize haystack stack
    st->hstk[0] = st->hstk_store;
    st->hstk[1] = st->hstk_store;
    st->hstk[2] = st->hstk_store;
    st->hstk[3] = st->hstk_store + 256;

    call(BASONOpen, st->hstk, st->hay);

    // Flatten needle into token array
    call(SPOTFlattenNeedle, st->ntoks, &st->nntoks, st->ndl);

    st->src_pos = 0;
    st->exhausted = NO;
    done;
}

// --- SPOTNext: linear scan, try match at every leaf position ---

ok64 SPOTNext(SPOTstate *st) {
    sane(st != NULL);
    if (st->exhausted) return (SPOTEND);
    if (st->nntoks == 0) return (SPOTEND);

    for (;;) {
        // Save haystack state for match attempt
        u64 match_stk[256];
        int ms = (int)(st->hstk[2] - st->hstk[0]);
        if (ms > 256) ms = 256;
        memcpy(match_stk, st->hstk[0], (size_t)ms * sizeof(u64));

        u64p mstk[4];
        mstk[0] = match_stk;
        mstk[1] = match_stk;
        mstk[2] = match_stk + ms;
        mstk[3] = match_stk + 256;

        SPOTbinds b = {};
        if (st->mlog[0] != NULL) {
            u64bReset(st->mlog);
            b.mlog[0] = st->mlog[0]; b.mlog[1] = st->mlog[1];
            b.mlog[2] = st->mlog[2]; b.mlog[3] = st->mlog[3];
        }
        if (st->alog[0] != NULL) {
            u64bReset(st->alog);
            b.alog[0] = st->alog[0]; b.alog[1] = st->alog[1];
            b.alog[2] = st->alog[2]; b.alog[3] = st->alog[3];
        }
        if (st->ranges[0] != NULL) {
            SPOTrangebReset(st->ranges);
            b.ranges[0] = st->ranges[0]; b.ranges[1] = st->ranges[1];
            b.ranges[2] = st->ranges[2]; b.ranges[3] = st->ranges[3];
        }

        int md = st->depth;
        u64 msrc = st->src_pos;
        u32 mp[64];
        int pcopy = st->depth + 1;
        if (pcopy > 64) pcopy = 64;
        if (pcopy > 0) memcpy(mp, st->parents, (size_t)pcopy * sizeof(u32));

        u32 srclo = 0, srchi = 0;
        ok64 m = SPOTMatchFlat(&b, st->ntoks, st->nntoks, 0,
                                mstk, st->hay, &md, &msrc, mp,
                                &srclo, &srchi, 0);
        if (m == OK) {
            // Copy back log idle pointers
            if (st->mlog[0] != NULL) st->mlog[2] = b.mlog[2];
            if (st->alog[0] != NULL) st->alog[2] = b.alog[2];
            if (st->ranges[0] != NULL) st->ranges[2] = b.ranges[2];

            // Store results
            st->match = (u64)b.match_container;
            st->bound = b.bound;
            st->src_lo = srclo;
            st->src_hi = srchi;
            for (int i = 0; i < SPOT_MAX_BINDS; i++) {
                if (b.bound & (1ULL << i)) {
                    st->binds[i] = b.offsets[i];
                    st->bind_srclo[i] = b.srclo[i];
                    st->bind_srchi[i] = b.srchi[i];
                    st->bind_ranges[i] = b.bind_ranges[i];
                } else {
                    st->binds[i] = 0;
                    st->bind_srclo[i] = 0;
                    st->bind_srchi[i] = 0;
                    st->bind_ranges[i] = (SPOTrange){};
                }
            }
            // Set match_range hay from matched container BASON slice
            st->match_range.hay[0] = st->hay[0] + b.match_container;
            st->match_range.hay[1] = st->hay[0] + srchi;
            st->match_range.ndl[0] = NULL;
            st->match_range.ndl[1] = NULL;
            st->nsubs = (u8)b.nsubs;
            for (int i = 0; i < b.nsubs; i++)
                st->subs[i] = b.subs[i];

            // Advance main haystack by one leaf from current position
            // (not to end of match) to allow overlapping match starts
            {
                u8 adv_t = 0;
                u8cs adv_v = {};
                u32 adv_off = 0, adv_lo = 0, adv_hi = 0;
                ok64 adv = SPOTFlatNext(st->hstk, st->hay,
                                         (int *)&st->depth,
                                         &st->src_pos, st->parents,
                                         &adv_t, adv_v, &adv_off,
                                         &adv_lo, &adv_hi, NULL);
                if (adv != OK) st->exhausted = YES;
            }

            done;
        }

        // No match at this position — advance by one leaf
        u8 ht = 0;
        u8cs hv = {};
        u32 hoff = 0;
        u32 dummy_lo = 0, dummy_hi = 0;
        ok64 a = SPOTFlatNext(st->hstk, st->hay, (int *)&st->depth,
                               &st->src_pos, st->parents,
                               &ht, hv, &hoff, &dummy_lo, &dummy_hi,
                               NULL);
        if (a != OK) {
            st->exhausted = YES;
            return (SPOTEND);
        }
    }
}

// --- SPOTSourceRange: map BASON byte offset -> source byte range ---

ok64 SPOTSourceRange(u8csc hay, u64 bson_off, u64p lo, u64p hi) {
    sane(hay[0] && lo && hi);
    *lo = UINT64_MAX;
    *hi = 0;

    u64 bson_end = 0;
    {
        aBpad(u64, stk, 256);
        call(u64bFeed1, stk, (u64)$len(hay));
        call(u64bFeed1, stk, bson_off);
        u8 t = 0;
        u8cs k = {}, v = {};
        ok64 o = BASONDrain(stk, hay, &t, k, v);
        if (o != OK) return (o);
        bson_end = (u64)(v[1] - hay[0]);
    }

    aBpad(u64, stk2, 256);
    call(BASONOpen, stk2, hay);
    u64 src_pos = 0;
    int depth = 0;
    for (;;) {
        u64 cursor = *u64bLast(stk2);
        u8 t = 0;
        u8cs k = {}, v = {};
        ok64 o = BASONDrain(stk2, hay, &t, k, v);
        if (o != OK) {
            if (depth <= 0) break;
            call(BASONOuto, stk2);
            depth--;
            continue;
        }
        if (BASONCollection(t)) {
            call(BASONInto, stk2, hay, v);
            depth++;
        } else {
            size_t vlen = $len(v);
            if (cursor >= bson_off && cursor < bson_end) {
                if (src_pos < *lo) *lo = src_pos;
                if (src_pos + vlen > *hi) *hi = src_pos + vlen;
            }
            src_pos += vlen;
        }
    }

    if (*lo == UINT64_MAX) *lo = *hi = 0;
    done;
}

// --- SPOTInstTemplate: instantiate replacement template ---

static ok64 SPOTInstTemplate(u8s out, u8csc tmpl,
                              u64 bound,
                              u32 *bind_srclo, u32 *bind_srchi,
                              u8csc source) {
    sane(out[0] != NULL);
    u8cp p = tmpl[0];
    u8cp end = tmpl[1];
    while (p < end) {
        u8 c = *p;
        b8 is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        if (is_alpha) {
            b8 prev_id = (p > tmpl[0]) && (
                (*(p - 1) >= 'a' && *(p - 1) <= 'z') ||
                (*(p - 1) >= 'A' && *(p - 1) <= 'Z') ||
                (*(p - 1) >= '0' && *(p - 1) <= '9') ||
                *(p - 1) == '_');
            b8 next_id = (p + 1 < end) && (
                (*(p + 1) >= 'a' && *(p + 1) <= 'z') ||
                (*(p + 1) >= 'A' && *(p + 1) <= 'Z') ||
                (*(p + 1) >= '0' && *(p + 1) <= '9') ||
                *(p + 1) == '_');
            if (!prev_id && !next_id) {
                int idx = SPOTBindIndex(c);
                u64 bit = 1ULL << idx;
                if (idx >= 0 && (bound & bit)) {
                    u32 lo = bind_srclo[idx];
                    u32 hi = bind_srchi[idx];
                    if (hi > lo && hi <= (u32)$len(source)) {
                        u8cs src_slice = {source[0] + lo, source[0] + hi};
                        call(u8sFeed, out, src_slice);
                    }
                    p++;
                    continue;
                }
            }
        }
        call(u8sFeed1, out, c);
        p++;
    }
    done;
}

// --- SPOTReplace: end-to-end replacement ---

#define SPOT_MAX_MATCHES 4096

typedef struct {
    u32 src_lo;
    u32 src_hi;
    u64 bound;
    u32 bind_srclo[SPOT_MAX_BINDS];
    u32 bind_srchi[SPOT_MAX_BINDS];
    SPOTrange match_range;
    SPOTrange bind_ranges[SPOT_MAX_BINDS];
} SPOTrep;

ok64 SPOTReplace(u8s out, u8csc source, u8csc hay,
                 u8csc needle_src, u8csc replace_src, u8csc ext) {
    sane(out[0] != NULL && source[0] != NULL);

    aBpad(u8, nbuf, 16384);
    aBpad(u64, nidx, 256);
    SPOTstate st = {};

    aBpad(u64, mlog, 1024);
    aBpad(u64, alog, 1024);

    call(SPOTInit, &st, nbuf, nidx, needle_src, ext, hay);
    st.mlog[0] = mlog[0]; st.mlog[1] = mlog[1];
    st.mlog[2] = mlog[2]; st.mlog[3] = mlog[3];
    st.alog[0] = alog[0]; st.alog[1] = alog[1];
    st.alog[2] = alog[2]; st.alog[3] = alog[3];

    // Collect all matches (heap-allocated, SPOTrep is large)
    SPOTrep *matches = (SPOTrep *)malloc(SPOT_MAX_MATCHES * sizeof(SPOTrep));
    test(matches != NULL, FAILSANITY);
    int nmatch = 0;

    while (SPOTNext(&st) == OK && nmatch < SPOT_MAX_MATCHES) {
        matches[nmatch].src_lo = st.src_lo;
        matches[nmatch].src_hi = st.src_hi;
        matches[nmatch].bound = st.bound;
        matches[nmatch].match_range = st.match_range;
        for (int i = 0; i < SPOT_MAX_BINDS; i++) {
            matches[nmatch].bind_srclo[i] = st.bind_srclo[i];
            matches[nmatch].bind_srchi[i] = st.bind_srchi[i];
            matches[nmatch].bind_ranges[i] = st.bind_ranges[i];
        }
        nmatch++;
    }

    if (nmatch == 0) { free(matches); return (SPOTEND); }

    // Sort matches by src_lo ascending
    for (int i = 1; i < nmatch; i++) {
        SPOTrep tmp = matches[i];
        int j = i - 1;
        while (j >= 0 && matches[j].src_lo > tmp.src_lo) {
            matches[j + 1] = matches[j];
            j--;
        }
        matches[j + 1] = tmp;
    }

    // Parse replacement template
    aBpad(u8, rbuf, 16384);
    aBpad(u64, ridx, 256);
    ok64 po = BASTParse(rbuf, ridx, replace_src, ext);
    if (po != OK) { free(matches); fail(po); }
    a_dup(u8c, rdata, u8bDataC(rbuf));

    // Flatten replacement BASON to text
    aBpad(u8, rtxt, 16384);
    {
        aBpad(u64, rstk, 256);
        ok64 ro = BASONOpen(rstk, rdata);
        if (ro != OK) { free(matches); fail(ro); }
        int rd = 0;
        for (;;) {
            u8 t = 0;
            u8cs k = {}, v = {};
            ok64 o = BASONDrain(rstk, rdata, &t, k, v);
            if (o != OK) {
                if (rd <= 0) break;
                BASONOuto(rstk);
                rd--;
                continue;
            }
            if (BASONCollection(t)) {
                BASONInto(rstk, rdata, v);
                rd++;
            } else {
                u8bFeed(rtxt, v);
            }
        }
    }
    a_dup(u8c, rtxt_slice, u8bDataC(rtxt));

    // Apply replacements
    u64 pos = 0;
    for (int i = 0; i < nmatch; i++) {
        if (matches[i].src_lo < (u32)pos) continue;

        if (matches[i].src_lo > (u32)pos) {
            u8cs gap = {source[0] + pos, source[0] + matches[i].src_lo};
            ok64 fo = u8sFeed(out, gap);
            if (fo != OK) { free(matches); fail(fo); }
        }

        ok64 io = SPOTInstTemplate(out, rtxt_slice,
             matches[i].bound, matches[i].bind_srclo, matches[i].bind_srchi,
             source);
        if (io != OK) { free(matches); fail(io); }

        pos = matches[i].src_hi;
    }

    if (pos < (u64)$len(source)) {
        u8cs tail = {source[0] + pos, source[1]};
        ok64 fo = u8sFeed(out, tail);
        if (fo != OK) { free(matches); fail(fo); }
    }

    free(matches);
    done;
}
