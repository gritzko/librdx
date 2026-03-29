#include "SPOT.h"

#include <string.h>

#include "abc/PRO.h"

// --- Tokenization callback context ---

typedef struct {
    u32bp toks;
    u32   off;  // running byte offset (start of current token)
} SPOTTokCtx;

static ok64 SPOTTokCB(u8 tag, u8cs tok, void *ctx) {
    sane(ctx != NULL);
    SPOTTokCtx *c = (SPOTTokCtx *)ctx;
    u32 end = c->off + (u32)$len(tok);
    u32 packed = TOK_PACK(tag, end);
    call(u32bFeed1, c->toks, packed);
    c->off = end;
    return OK;
}

// --- SPOTTokenize ---

ok64 SPOTTokenize(u32bp toks, u8csc source, u8csc ext) {
    sane(toks != NULL && $ok(source));
    if ($empty(source)) done;

    SPOTTokCtx ctx = {.toks = toks, .off = 0};

    // Strip leading dot from ext for tok/
    u8cs ext_nodot = {};
    if (!$empty(ext) && ext[0][0] == '.') {
        ext_nodot[0] = ext[0] + 1;
        ext_nodot[1] = ext[1];
    } else {
        $mv(ext_nodot, ext);
    }

    // Try tok/ lexer
    TOKstate ts = {
        .data = {source[0], source[1]},
        .cb = SPOTTokCB,
        .ctx = &ctx,
    };
    call(TOKLexer, &ts, ext_nodot);
    done;
}

// --- Helper functions ---

static b8 SPOTIsPlaceholder(u8cs val) {
    if ($len(val) != 1) return (NO);
    u8 c = val[0][0];
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int SPOTBindIndex(u8 c) {
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= 'A' && c <= 'Z') return 26 + (c - 'A');
    return (-1);
}

static b8 SPOTIsLower(u8 c) { return c >= 'a' && c <= 'z'; }

static b8 SPOTIsWhitespace(u8cs val) {
    $for(u8c, p, val) {
        if (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') return (NO);
    }
    return (YES);
}

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

static b8 SPOTSliceEq(u8cs a, u8cs b) {
    size_t alen = (size_t)$len(a);
    size_t blen = (size_t)$len(b);
    if (alen != blen) return (NO);
    if (alen == 0) return (YES);
    return memcmp(a[0], b[0], alen) == 0;
}

static int SPOTBracketDir(u8cs val) {
    if ($len(val) != 1) return (0);
    u8 c = val[0][0];
    if (c == '{' || c == '(' || c == '[') return (1);
    if (c == '}' || c == ')' || c == ']') return (-1);
    return (0);
}

// --- Advance to next meaningful token position ---
// Skips comments (D) and pure-whitespace S tokens.
// Returns the position of the next meaningful token, or len if exhausted.
static int SPOTSkipWS(u32cs toks, u8cp base, int pos, int len) {
    while (pos < len) {
        u8 tag = TOK_TAG(toks[0][pos]);
        if (tag == 'D') { pos++; continue; }
        if (tag == 'S') {
            u8cs val = {}; TOK_VAL(val, toks, base, pos);
            if (SPOTIsWhitespace(val)) { pos++; continue; }
        }
        break;
    }
    return pos;
}

// --- Flatten needle tokens ---

static ok64 SPOTFlattenNeedle(SPOTntok *flat, int *nflat,
                                u32cs toks, u8cp base) {
    sane(flat != NULL);
    int len = (int)$len(toks);
    *nflat = 0;
    b8 pending_skip = NO;

    for (int i = 0; i < len; i++) {
        u8 tag = TOK_TAG(toks[0][i]);
        u8cs val = {}; TOK_VAL(val, toks, base, i);

        if (tag == 'D') continue;  // skip comments
        if (tag == 'S' && SPOTIsWhitespace(val)) {
            int sp = SPOTCountSpaces(val);
            if (sp >= 2) pending_skip = YES;
            continue;
        }
        if ($len(val) == 0) continue;

        if (*nflat >= SPOT_MAX_NTOKS) fail(SPOTBAD);
        flat[*nflat].type = tag;
        flat[*nflat].val[0] = val[0];
        flat[*nflat].val[1] = val[1];
        flat[*nflat].skip = (*nflat > 0) ? pending_skip : NO;
        (*nflat)++;
        pending_skip = NO;
    }
    done;
}

// --- Backtracking state ---

typedef struct {
    u64  bound;
    range32 subs[SPOT_MAX_SUBS];
    int  nsubs;
    match32 bind_matches[SPOT_MAX_BINDS];
    match32p ranges[4];
    u8cp source_base;
    u8cp ndl_base;
} SPOTbinds;

typedef struct {
    int pos;
    SPOTbinds binds;
} SPOTsave;

// Record a matched token range into the ranges buffer (if present)
static void SPOTRecordRange(SPOTbinds *b, u32 ndl_lo, u32 ndl_hi,
                             u32 hay_lo, u32 hay_hi) {
    if (b->ranges[0] == NULL) return;
    match32 entry = {.hay = {hay_lo, hay_hi}, .ndl = {ndl_lo, ndl_hi}};
    match32bFeed1(b->ranges, entry);
}

// --- Flat matching engine ---

static ok64 SPOTMatchFlat(SPOTbinds *b, SPOTntok *ntoks, int nntoks,
                           int from, u32cs htoks, u8cp hbase,
                           int *hpos, int brace);

static ok64 SPOTMatchFlat(SPOTbinds *b, SPOTntok *ntoks, int nntoks,
                           int from, u32cs htoks, u8cp hbase,
                           int *hpos, int brace) {
    sane(b != NULL);
    int hlen = (int)$len(htoks);
    if (from >= nntoks) done;

    SPOTntok *cur = &ntoks[from];

    if (!cur->skip) {
        // EXACT: get next meaningful token, must match
        int pos = SPOTSkipWS(htoks, hbase, *hpos, hlen);
        if (pos >= hlen) fail(SPOTBAD);

        u8cs hv = {}; TOK_VAL(hv, htoks, hbase, pos);
        u32 leaf_srclo = (pos > 0) ? TOK_OFF(htoks[0][pos - 1]) : 0;
        u32 leaf_srchi = TOK_OFF(htoks[0][pos]);
        *hpos = pos + 1;

        // Record first token position for segment tracking
        if (from == 0) {
            if (b->nsubs < SPOT_MAX_SUBS)
                b->subs[b->nsubs++] = (range32){leaf_srclo, leaf_srchi};
        }
        if (b->nsubs > 0) b->subs[b->nsubs - 1].hi = leaf_srchi;

        // Needle token range
        u32 ndl_lo = (u32)(cur->val[0] - b->ndl_base);
        u32 ndl_hi = (u32)(cur->val[1] - b->ndl_base);

        // Check placeholder
        if (SPOTIsPlaceholder(cur->val)) {
            u8 c = cur->val[0][0];
            int idx = SPOTBindIndex(c);
            if (idx < 0) fail(SPOTBAD);
            u64 bit = 1ULL << idx;

            if (SPOTIsLower(c)) {
                // Lowercase: bind to single leaf value
                if (b->bound & bit) {
                    u8cs bnd = {b->source_base + b->bind_matches[idx].hay.lo,
                                b->source_base + b->bind_matches[idx].hay.hi};
                    if (!SPOTSliceEq(bnd, hv))
                        fail(SPOTBAD);
                } else {
                    b->bound |= bit;
                    b->bind_matches[idx] = (match32){
                        .hay = {leaf_srclo, leaf_srchi},
                        .ndl = {ndl_lo, ndl_hi}};
                }
                SPOTRecordRange(b, ndl_lo, ndl_hi, leaf_srclo, leaf_srchi);
            } else {
                // Uppercase: consume tokens until remaining needle matches
                if (b->bound & bit) {
                    u32 bound_len = b->bind_matches[idx].hay.hi -
                                    b->bind_matches[idx].hay.lo;
                    u32 cap_end = leaf_srclo + bound_len;
                    u32 consumed = leaf_srchi;
                    while (consumed < cap_end) {
                        int p2 = SPOTSkipWS(htoks, hbase, *hpos, hlen);
                        if (p2 >= hlen) fail(SPOTBAD);
                        *hpos = p2 + 1;
                        consumed = TOK_OFF(htoks[0][p2]);
                    }
                    if (consumed != cap_end) fail(SPOTBAD);
                    if (b->nsubs > 0) b->subs[b->nsubs - 1].hi = consumed;
                } else {
                    // Shortest-first: try matching remaining, extend on fail
                    u32 cap_srchi = leaf_srchi;
                    b->bound |= bit;
                    b->bind_matches[idx] = (match32){
                        .hay = {leaf_srclo, cap_srchi},
                        .ndl = {(u32)(cur->val[0] - b->ndl_base),
                                (u32)(cur->val[1] - b->ndl_base)}};
                    int cap_brace = SPOTBracketDir(hv);
                    for (;;) {
                        if (b->nsubs > 0) b->subs[b->nsubs - 1].hi = cap_srchi;
                        SPOTsave sv = {.pos = *hpos, .binds = *b};
                        ok64 r = SPOTMatchFlat(b, ntoks, nntoks, from + 1,
                                               htoks, hbase, hpos, brace);
                        if (r == OK) done;
                        *hpos = sv.pos;
                        *b = sv.binds;

                        // Consume one more token
                        int p2 = SPOTSkipWS(htoks, hbase, *hpos, hlen);
                        if (p2 >= hlen) fail(SPOTBAD);
                        u8cs hv2 = {}; TOK_VAL(hv2, htoks, hbase, p2);
                        cap_brace += SPOTBracketDir(hv2);
                        if (cap_brace < 0) fail(SPOTBAD);
                        *hpos = p2 + 1;
                        cap_srchi = TOK_OFF(htoks[0][p2]);
                        b->bind_matches[idx].hay.hi = cap_srchi;
                    }
                }
            }
        } else {
            // Literal: match by value
            if (!SPOTSliceEq(cur->val, hv)) fail(SPOTBAD);
            brace += SPOTBracketDir(cur->val);
            SPOTRecordRange(b, ndl_lo, ndl_hi, leaf_srclo, leaf_srchi);
        }

        // Recurse for remaining
        call(SPOTMatchFlat, b, ntoks, nntoks, from + 1,
             htoks, hbase, hpos, brace);
        done;
    }

    // SKIP: scan forward trying each token position with backtracking.
    {
        u32 skip_ndl_lo = (u32)(cur->val[0] - b->ndl_base);
        u32 skip_ndl_hi = (u32)(cur->val[1] - b->ndl_base);
        SPOTbinds saved_binds = *b;
        int scan_brace = 0;
        b8 is_close_ph = NO;
        int ndl_bdir = 0;
        if (!SPOTIsPlaceholder(cur->val)) {
            ndl_bdir = SPOTBracketDir(cur->val);
            is_close_ph = ndl_bdir < 0;
        }
        for (;;) {
            int pos = SPOTSkipWS(htoks, hbase, *hpos, hlen);
            if (pos >= hlen) fail(SPOTBAD);

            u8cs hv = {}; TOK_VAL(hv, htoks, hbase, pos);
            u32 leaf_srclo = (pos > 0) ? TOK_OFF(htoks[0][pos - 1]) : 0;
            u32 leaf_srchi = TOK_OFF(htoks[0][pos]);
            *hpos = pos + 1;

            int bd = SPOTBracketDir(hv);
            scan_brace += bd;
            if (is_close_ph) {
                if (scan_brace < -brace) fail(SPOTBAD);
            } else {
                if (scan_brace < 0) fail(SPOTBAD);
            }

            // Save cursor state after consuming this token
            int post_pos = *hpos;

            // Record new segment
            if (b->nsubs < SPOT_MAX_SUBS)
                b->subs[b->nsubs++] = (range32){leaf_srclo, leaf_srchi};

            b8 is_ph = SPOTIsPlaceholder(cur->val);
            u8 ph_c = is_ph ? cur->val[0][0] : 0;
            int ph_idx = is_ph ? SPOTBindIndex(ph_c) : -1;
            b8 is_upper = is_ph && !SPOTIsLower(ph_c);

            if (is_upper && ph_idx >= 0) {
                u64 bit = 1ULL << ph_idx;
                if (b->bound & bit) {
                    *b = saved_binds;
                    continue;
                }
                b->bound |= bit;
                b->bind_matches[ph_idx] = (match32){
                    .hay = {leaf_srclo, leaf_srchi},
                    .ndl = {(u32)(cur->val[0] - b->ndl_base),
                            (u32)(cur->val[1] - b->ndl_base)}};
                u32 cap_srchi = leaf_srchi;
                int cap_brace2 = SPOTBracketDir(hv);
                for (;;) {
                    if (b->nsubs > 0) b->subs[b->nsubs - 1].hi = cap_srchi;
                    SPOTsave sv = {.pos = *hpos, .binds = *b};
                    ok64 r = SPOTMatchFlat(b, ntoks, nntoks, from + 1,
                                            htoks, hbase, hpos,
                                            brace + scan_brace);
                    if (r == OK) done;
                    *hpos = sv.pos;
                    *b = sv.binds;
                    // Consume one more token to extend capture
                    int p2 = SPOTSkipWS(htoks, hbase, *hpos, hlen);
                    if (p2 >= hlen) break;
                    u8cs hv2 = {}; TOK_VAL(hv2, htoks, hbase, p2);
                    cap_brace2 += SPOTBracketDir(hv2);
                    if (cap_brace2 < 0) break;
                    *hpos = p2 + 1;
                    cap_srchi = TOK_OFF(htoks[0][p2]);
                    b->bind_matches[ph_idx].hay.hi = cap_srchi;
                }
                *hpos = post_pos;
                *b = saved_binds;
                continue;
            }

            b8 tok_match = NO;
            if (is_ph && ph_idx >= 0 && SPOTIsLower(ph_c)) {
                u64 bit = 1ULL << ph_idx;
                if (b->bound & bit) {
                    u8cs bnd2 = {b->source_base + b->bind_matches[ph_idx].hay.lo,
                                 b->source_base + b->bind_matches[ph_idx].hay.hi};
                    tok_match = SPOTSliceEq(bnd2, hv);
                } else {
                    b->bound |= bit;
                    b->bind_matches[ph_idx] = (match32){
                        .hay = {leaf_srclo, leaf_srchi},
                        .ndl = {(u32)(cur->val[0] - b->ndl_base),
                                (u32)(cur->val[1] - b->ndl_base)}};
                    tok_match = YES;
                }
            } else if (!is_ph) {
                tok_match = SPOTSliceEq(cur->val, hv);
                if (tok_match && is_close_ph && scan_brace != -brace)
                    tok_match = NO;
            }

            if (tok_match) {
                SPOTRecordRange(b, skip_ndl_lo, skip_ndl_hi, leaf_srclo, leaf_srchi);
                SPOTsave mr = {.pos = *hpos, .binds = *b};
                ok64 r = SPOTMatchFlat(b, ntoks, nntoks, from + 1,
                                        htoks, hbase, hpos,
                                        brace + scan_brace);
                if (r == OK) done;
                *hpos = post_pos;
                *b = mr.binds;
            }

            *b = saved_binds;
        }
    }
}

// --- SPOTInit ---

ok64 SPOTInit(SPOTstate *st, u32bp ndl_toks,
              u8csc needle_src, u8csc ext,
              u32cs hay_toks, u8csc source) {
    sane(st != NULL && ndl_toks != NULL);
    memset(st, 0, sizeof(SPOTstate));

    // Tokenize needle
    call(SPOTTokenize, ndl_toks, needle_src, ext);

    st->ntoks[0] = (u32cp)u32bDataHead(ndl_toks);
    st->ntoks[1] = (u32cp)u32bIdleHead(ndl_toks);
    st->htoks[0] = hay_toks[0];
    st->htoks[1] = hay_toks[1];
    st->nsrc[0] = needle_src[0];
    st->nsrc[1] = needle_src[1];
    st->source[0] = source[0];
    st->source[1] = source[1];

    if ($empty(st->ntoks)) fail(SPOTBAD);
    if ($empty(st->htoks)) fail(SPOTBAD);

    // Flatten needle tokens into flat[]
    call(SPOTFlattenNeedle, st->flat, &st->nflat,
         st->ntoks, (u8cp)needle_src[0]);

    st->hpos = 0;
    st->exhausted = NO;
    done;
}

// --- SPOTNext ---

ok64 SPOTNext(SPOTstate *st) {
    sane(st != NULL);
    if (st->exhausted) return (SPOTEND);
    if (st->nflat == 0) return (SPOTEND);

    int hlen = (int)$len(st->htoks);

    for (;;) {
        // Try matching at current position
        int mpos = st->hpos;
        SPOTbinds b = {};
        b.source_base = st->source[0];
        b.ndl_base = (u8cp)st->nsrc[0];
        if (st->ranges[0] != NULL) {
            match32bReset(st->ranges);
            b.ranges[0] = st->ranges[0]; b.ranges[1] = st->ranges[1];
            b.ranges[2] = st->ranges[2]; b.ranges[3] = st->ranges[3];
        }

        ok64 m = SPOTMatchFlat(&b, st->flat, st->nflat, 0,
                                st->htoks, st->source[0],
                                &mpos, 0);
        if (m == OK) {
            if (st->ranges[0] != NULL) st->ranges[2] = b.ranges[2];

            st->bound = b.bound;
            st->src_rng = (b.nsubs > 0)
                ? (range32){b.subs[0].lo, b.subs[b.nsubs - 1].hi}
                : range32Z;
            for (int i = 0; i < SPOT_MAX_BINDS; i++) {
                if (b.bound & (1ULL << i))
                    st->bind_matches[i] = b.bind_matches[i];
                else
                    st->bind_matches[i] = match32Z;
            }
            st->nsubs = (u8)b.nsubs;
            for (int i = 0; i < b.nsubs; i++)
                st->subs[i] = b.subs[i];

            // Advance main haystack by one meaningful token
            int adv = SPOTSkipWS(st->htoks, st->source[0], st->hpos, hlen);
            if (adv < hlen)
                st->hpos = adv + 1;
            else
                st->exhausted = YES;

            done;
        }

        // No match — advance by one meaningful token
        int pos = SPOTSkipWS(st->htoks, st->source[0], st->hpos, hlen);
        if (pos >= hlen) {
            st->exhausted = YES;
            return (SPOTEND);
        }
        st->hpos = pos + 1;
    }
}

// --- SPOTInstTemplate ---

static ok64 SPOTInstTemplate(u8s out, u8csc tmpl,
                              u64 bound,
                              match32 *bind_matches,
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
                    if (bind_matches[idx].hay.lo !=
                        bind_matches[idx].hay.hi) {
                        u8cs bval = {
                            source[0] + bind_matches[idx].hay.lo,
                            source[0] + bind_matches[idx].hay.hi};
                        call(u8sFeed, out, bval);
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

// --- SPOTReplace ---

#define SPOT_MAX_MATCHES 4096

typedef struct {
    range32 src_rng;
    u64 bound;
    match32 bind_matches[SPOT_MAX_BINDS];
} SPOTrep;

ok64 SPOTReplace(u8s out, u8csc source, u32cs hay_toks,
                 u8csc needle_src, u8csc replace_src, u8csc ext) {
    sane(out[0] != NULL && source[0] != NULL);

    aBpad(u32, ntoks, 4096);
    SPOTstate st = {};

    call(SPOTInit, &st, ntoks, needle_src, ext, hay_toks, source);

    // Collect all matches (heap-allocated, SPOTrep is large)
    SPOTrep *matches = (SPOTrep *)malloc(SPOT_MAX_MATCHES * sizeof(SPOTrep));
    test(matches != NULL, FAILSANITY);
    int nmatch = 0;

    while (SPOTNext(&st) == OK && nmatch < SPOT_MAX_MATCHES) {
        matches[nmatch].src_rng = st.src_rng;
        matches[nmatch].bound = st.bound;
        for (int i = 0; i < SPOT_MAX_BINDS; i++)
            matches[nmatch].bind_matches[i] = st.bind_matches[i];
        nmatch++;
    }

    if (nmatch == 0) { free(matches); return (SPOTEND); }

    // Sort matches by source range ascending
    for (int i = 1; i < nmatch; i++) {
        SPOTrep tmp = matches[i];
        int j = i - 1;
        while (j >= 0 && matches[j].src_rng.lo > tmp.src_rng.lo) {
            matches[j + 1] = matches[j];
            j--;
        }
        matches[j + 1] = tmp;
    }

    // Apply replacements
    u64 pos = 0;
    for (int i = 0; i < nmatch; i++) {
        if (matches[i].src_rng.lo < (u32)pos) continue;

        if (matches[i].src_rng.lo > (u32)pos) {
            u8cs gap = {source[0] + pos, source[0] + matches[i].src_rng.lo};
            ok64 fo = u8sFeed(out, gap);
            if (fo != OK) { free(matches); fail(fo); }
        }

        ok64 io = SPOTInstTemplate(out, replace_src,
             matches[i].bound, matches[i].bind_matches, source);
        if (io != OK) { free(matches); fail(io); }

        pos = matches[i].src_rng.hi;
    }

    if (pos < (u64)$len(source)) {
        u8cs tail = {source[0] + pos, source[1]};
        ok64 fo = u8sFeed(out, tail);
        if (fo != OK) { free(matches); fail(fo); }
    }

    free(matches);
    done;
}
