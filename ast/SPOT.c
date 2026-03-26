#include "SPOT.h"

#include <string.h>

#include "abc/PRO.h"
#include "ast/BAST.h"
#include "json/BASON.h"

// Skip mode for gaps between needle tokens
typedef enum {
    SPOT_EXACT = 0,
    SPOT_SKIP_SIBLINGS = 1,
    SPOT_SKIP_DFS = 2,
} SPOTskip;

// Pre-extracted needle child with associated skip mode
typedef struct {
    u8 type;
    u8cs key;
    u8cs val;
    SPOTskip skip;
    u32 npos;  // byte offset of TLKV record in needle BASON
} SPOTnchild;

// Internal binding state: slices for fast comparison, offsets for result.
typedef struct {
    u8cs slices[SPOT_MAX_BINDS];  // value slices for comparison
    u64  offsets[SPOT_MAX_BINDS]; // TLV tag byte offsets for result
    u64  bound;
    u64  subs[SPOT_MAX_SUBS];    // segment start offsets
    int  nsubs;
    u64p mlog[4];  // match log (copied from SPOTstate)
    u64p alog[4];  // alias log (copied from SPOTstate)
} SPOTbinds;

#define SPOT_MAX_NC 64
#define SPOT_MAX_DFS 64

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

// Determine skip mode from a whitespace gap leaf
static SPOTskip SPOTGapMode(u8cs val) {
    int sp = SPOTCountSpaces(val);
    if (sp >= 3) return (SPOT_SKIP_DFS);
    if (sp >= 2) return (SPOT_SKIP_SIBLINGS);
    return (SPOT_EXACT);
}

// Skip whitespace and comment leaves on the haystack side
static ok64 SPOTSkipWS(u64bp stk, u8csc data, u8p type, u8cs key, u8cs val) {
    sane(stk != NULL);
    while (*type == 'S' && SPOTIsWhitespace(val)) {
        ok64 o = BASONDrain(stk, data, type, key, val);
        if (o != OK) return (o);
    }
    while (*type == 'D') {
        ok64 o = BASONDrain(stk, data, type, key, val);
        if (o != OK) return (o);
    }
    done;
}

// Compare two const slices for equality
static b8 SPOTSliceEq(u8cs a, u8cs b) {
    size_t alen = (size_t)$len(a);
    size_t blen = (size_t)$len(b);
    if (alen != blen) return (NO);
    if (alen == 0) return (YES);
    return memcmp(a[0], b[0], alen) == 0;
}

// Read one BASON element at byte offset, advance cursor.
// Returns !OK when level is exhausted.
static ok64 SPOTReadAt(u8csc data, u64 lvl_end, u64 *cursor,
                         u8p type, u8cs key, u8cs val) {
    sane(data[0] != NULL);
    aBpad(u64, stk, 8);
    call(u64bFeed1, stk, lvl_end);
    call(u64bFeed1, stk, *cursor);
    ok64 o = BASONDrain(stk, data, type, key, val);
    if (o == OK) *cursor = *u64bLast(stk);
    return (o);
}

// Pre-extract needle children into array with skip modes.
// Consumes all children from nstk via BASONDrain.
static ok64 SPOTExtractChildren(u64bp nstk, u8csc ndata,
                                  SPOTnchild *out, int *nout) {
    sane(out != NULL);
    SPOTskip skip = SPOT_EXACT;
    *nout = 0;
    u8 nt = 0;
    u8cs nk = {}, nv = {};
    while (1) {
        u32 pos = (u32)*u64bLast(nstk);
        ok64 dr = BASONDrain(nstk, ndata, &nt, nk, nv);
        if (dr != OK) break;
        if (nt == 'S' && SPOTIsWhitespace(nv)) {
            skip = SPOTGapMode(nv);
            continue;
        }
        if (nt == 'D') continue;
        if (!BASONCollection(nt) && $len(nv) == 0) continue;
        if (*nout >= SPOT_MAX_NC) fail(SPOTBAD);
        out[*nout].type = nt;
        out[*nout].key[0] = nk[0];
        out[*nout].key[1] = nk[1];
        out[*nout].val[0] = nv[0];
        out[*nout].val[1] = nv[1];
        out[*nout].skip = skip;
        out[*nout].npos = pos;
        (*nout)++;
        skip = SPOT_EXACT;
    }
    done;
}

// Forward declaration
static ok64 SPOTMatchChildren(SPOTbinds *b, b8 track,
                               u64bp nstk, u8csc ndata,
                               u64bp hstk, u8csc hdata);

// Match a single needle node vs haystack node.
// hpos is the TLV tag byte offset of the haystack node.
// npos is the byte offset of the needle node (for alias logging).
static ok64 SPOTMatchNode(SPOTbinds *b,
                           u8 ntype, u8cs nval, u32 npos,
                           u8 htype, u8cs hkey, u8cs hval,
                           u64 hpos,
                           u8csc ndata, u64bp hstk, u8csc hdata) {
    sane(b != NULL);

    // Placeholder check
    if (SPOTIsPlaceholder(ntype, nval)) {
        int idx = SPOTBindIndex(nval[0][0]);
        if (idx < 0) fail(SPOTBAD);
        u64 bit = 1ULL << idx;

        if (SPOTIsLower(nval[0][0])) {
            // Lowercase: bind to leaf text value only
            if (BASONCollection(htype)) fail(SPOTBAD);
            if (b->bound & bit) {
                if (!SPOTSliceEq(b->slices[idx], hval)) fail(SPOTBAD);
            } else {
                b->slices[idx][0] = hval[0];
                b->slices[idx][1] = hval[1];
                b->offsets[idx] = hpos;
                b->bound |= bit;
                if (b->alog[0] != NULL) {
                    u64 ae = SPOTLogPack((u32)hpos, (u16)npos, 0);
                    u64bFeed1(b->alog, ae);
                }
            }
        } else {
            // Uppercase: bind to any node (leaf value or container slice)
            if (b->bound & bit) {
                if (!SPOTSliceEq(b->slices[idx], hval)) fail(SPOTBAD);
            } else {
                b->slices[idx][0] = hval[0];
                b->slices[idx][1] = hval[1];
                b->offsets[idx] = hpos;
                b->bound |= bit;
                if (b->alog[0] != NULL) {
                    u64 ae = SPOTLogPack((u32)hpos, (u16)npos, 0);
                    u64bFeed1(b->alog, ae);
                }
            }
        }
        done;
    }

    // Container: types must match, recurse into children
    if (BASONCollection(ntype)) {
        if (!BASONCollection(htype)) fail(SPOTBAD);
        if (ntype != htype) fail(SPOTBAD);
        // Create temp needle stack at container's children level
        aBpad(u64, nstk, 256);
        u64 nc_end = (u64)(nval[1] - ndata[0]);
        u64 nc_start = (u64)(nval[0] - ndata[0]);
        call(u64bFeed1, nstk, nc_end);
        call(u64bFeed1, nstk, nc_start);
        // Enter haystack container children
        call(BASONInto, hstk, hdata, hval);
        ok64 o = SPOTMatchChildren(b, NO, nstk, ndata, hstk, hdata);
        BASONOuto(hstk);
        if (o != OK) fail(SPOTBAD);
        done;
    }

    // Literal leaf: match by value only, ignore type.
    // Tree-sitter assigns different leaf types (S, V, F, T, R, L, P)
    // based on context, so the same token may get different types in
    // the needle vs the haystack.
    if (!SPOTSliceEq(nval, hval)) fail(SPOTBAD);
    done;
}

// Recursive matching of needle children[from..n) against haystack
// siblings starting at byte offset h_cur within level ending at h_lvl.
// Supports backtracking: at SKIP points, tries each candidate and
// recurses for the remainder; on failure, restores bindings and retries.
static ok64 SPOTMatchFrom(SPOTbinds *b, b8 track,
                            SPOTnchild *nc, int n, int from,
                            u8csc ndata,
                            u64 h_lvl, u64 h_cur,
                            u8csc hdata) {
    sane(b != NULL);
    if (from >= n) done;  // all needle children matched

    SPOTnchild *cur = &nc[from];

    if (cur->skip == SPOT_EXACT) {
        // Read next meaningful haystack node (skip ws/comments)
        u64 cursor = h_cur;
        u64 node_pos = cursor;
        u8 ht = 0;
        u8cs hk = {}, hv = {};
        for (;;) {
            node_pos = cursor;
            ok64 ho = SPOTReadAt(hdata, h_lvl, &cursor, &ht, hk, hv);
            if (ho != OK) fail(SPOTBAD);
            if ((ht == 'S' && SPOTIsWhitespace(hv)) || ht == 'D') continue;
            break;
        }
        // First child starts segment 0
        if (track && from == 0 && b->nsubs < SPOT_MAX_SUBS)
            b->subs[b->nsubs++] = node_pos;
        // Create temp hstk for SPOTMatchNode (container Into/Outo)
        aBpad(u64, hstk, 256);
        call(u64bFeed1, hstk, h_lvl);
        call(u64bFeed1, hstk, cursor);
        call(SPOTMatchNode, b, cur->type, cur->val, cur->npos,
             ht, hk, hv, node_pos, ndata, hstk, hdata);
        // Push match log entry
        if (track && b->mlog[0] != NULL) {
            u16 alen = b->alog[0] != NULL ? (u16)u64bDataLen(b->alog) : 0;
            u64 me = SPOTLogPack((u32)node_pos, (u16)cur->npos, alen);
            u64bFeed1(b->mlog, me);
        }
        // Recurse for remaining children
        call(SPOTMatchFrom, b, track, nc, n, from + 1,
             ndata, h_lvl, cursor, hdata);
        done;
    }

    if (cur->skip == SPOT_SKIP_SIBLINGS) {
        // Scan forward through siblings with backtracking
        u64 cursor = h_cur;
        for (;;) {
            u64 node_pos = cursor;
            u8 ht = 0;
            u8cs hk = {}, hv = {};
            for (;;) {
                node_pos = cursor;
                ok64 ho = SPOTReadAt(hdata, h_lvl, &cursor, &ht, hk, hv);
                if (ho != OK) fail(SPOTBAD);  // exhausted level
                if ((ht == 'S' && SPOTIsWhitespace(hv)) || ht == 'D')
                    continue;
                break;
            }
            // Save bindings for backtracking
            SPOTbinds saved = *b;

            // Record new segment
            if (track && b->nsubs < SPOT_MAX_SUBS)
                b->subs[b->nsubs++] = node_pos;

            // Try matching this node
            aBpad(u64, hstk, 256);
            call(u64bFeed1, hstk, h_lvl);
            call(u64bFeed1, hstk, cursor);
            ok64 m = SPOTMatchNode(b, cur->type, cur->val, cur->npos,
                                     ht, hk, hv, node_pos,
                                     ndata, hstk, hdata);
            if (m == OK) {
                // Push match log entry
                if (track && b->mlog[0] != NULL) {
                    u16 alen = b->alog[0] != NULL ? (u16)u64bDataLen(b->alog) : 0;
                    u64 me = SPOTLogPack((u32)node_pos, (u16)cur->npos, alen);
                    u64bFeed1(b->mlog, me);
                }
                // Node matched — try remaining children
                ok64 r = SPOTMatchFrom(b, track, nc, n, from + 1,
                                         ndata, h_lvl, cursor, hdata);
                if (r == OK) done;  // full match!
            }
            // Restore bindings + segments, continue scanning
            *b = saved;
        }
    }

    // SPOT_SKIP_DFS: depth-first search with backtracking
    {
        u64 dfs_lvl[SPOT_MAX_DFS];
        u64 dfs_cur[SPOT_MAX_DFS];
        int dd = 0;
        dfs_lvl[0] = h_lvl;
        dfs_cur[0] = h_cur;

        for (;;) {
            u64 node_pos = dfs_cur[dd];
            u8 ht = 0;
            u8cs hk = {}, hv = {};
            ok64 ho = SPOTReadAt(hdata, dfs_lvl[dd], &dfs_cur[dd],
                                   &ht, hk, hv);
            if (ho != OK) {
                if (dd <= 0) fail(SPOTBAD);  // exhausted
                dd--;
                continue;
            }
            if ((ht == 'S' && SPOTIsWhitespace(hv)) || ht == 'D') continue;

            // Save bindings for backtracking
            SPOTbinds saved = *b;

            // Record new segment
            if (track && b->nsubs < SPOT_MAX_SUBS)
                b->subs[b->nsubs++] = node_pos;

            // Try matching this node
            aBpad(u64, hstk, 256);
            call(u64bFeed1, hstk, dfs_lvl[dd]);
            call(u64bFeed1, hstk, dfs_cur[dd]);
            ok64 m = SPOTMatchNode(b, cur->type, cur->val, cur->npos,
                                     ht, hk, hv, node_pos,
                                     ndata, hstk, hdata);
            if (m == OK) {
                // Push match log entry
                if (track && b->mlog[0] != NULL) {
                    u16 alen = b->alog[0] != NULL ? (u16)u64bDataLen(b->alog) : 0;
                    u64 me = SPOTLogPack((u32)node_pos, (u16)cur->npos, alen);
                    u64bFeed1(b->mlog, me);
                }
                // Try remaining at original sibling level (depth 0)
                ok64 r = SPOTMatchFrom(b, track, nc, n, from + 1,
                                         ndata, h_lvl, dfs_cur[0], hdata);
                if (r == OK) done;  // full match!
            }
            // Restore bindings + segments
            *b = saved;

            // Descend into container for deeper search
            if (BASONCollection(ht)) {
                if (dd + 1 >= SPOT_MAX_DFS) fail(SPOTBAD);
                dd++;
                dfs_lvl[dd] = (u64)(hv[1] - hdata[0]);
                dfs_cur[dd] = (u64)(hv[0] - hdata[0]);
            }
        }
    }
}

// Match needle children against haystack children with skip modes.
// Pre-extracts needle children, then uses recursive backtracking.
static ok64 SPOTMatchChildren(SPOTbinds *b, b8 track,
                               u64bp nstk, u8csc ndata,
                               u64bp hstk, u8csc hdata) {
    sane(b != NULL);
    // Pre-extract needle children
    SPOTnchild nc[SPOT_MAX_NC];
    int nn = 0;
    call(SPOTExtractChildren, nstk, ndata, nc, &nn);
    if (nn == 0) done;  // no meaningful children

    // Get haystack position from stack
    u64 h_cur = *u64bLast(hstk);
    u64 h_lvl = *(u64bLast(hstk) - 1);

    // Match with backtracking
    call(SPOTMatchFrom, b, track, nc, nn, 0, ndata, h_lvl, h_cur, hdata);
    done;
}

// Try matching needle as a subsequence of siblings starting at byte
// offset `start` within a level ending at byte offset `lvl_end`.
static ok64 SPOTTryMatchSeq(SPOTbinds *b,
                              u8csc ndata, u8csc hdata,
                              u64 lvl_end, u64 start,
                              u64bp mlog, u64bp alog) {
    sane(b != NULL);

    // Clear bindings
    memset(b, 0, sizeof(SPOTbinds));

    // Copy log buffer descriptors if provided
    if (mlog != NULL) {
        b->mlog[0] = mlog[0]; b->mlog[1] = mlog[1];
        b->mlog[2] = mlog[2]; b->mlog[3] = mlog[3];
    }
    if (alog != NULL) {
        b->alog[0] = alog[0]; b->alog[1] = alog[1];
        b->alog[2] = alog[2]; b->alog[3] = alog[3];
    }

    // Open needle, strip translation_unit wrapper
    aBpad(u64, nstk, 256);
    call(BASONOpen, nstk, ndata);
    u8 nt = 0;
    u8cs nk = {}, nv = {};
    ok64 no = BASONDrain(nstk, ndata, &nt, nk, nv);
    if (no != OK) fail(SPOTBAD);
    if (!BASONCollection(nt)) fail(SPOTBAD);
    call(BASONInto, nstk, ndata, nv);

    // Create temp haystack stack at the given sibling position
    aBpad(u64, hstk, 256);
    call(u64bFeed1, hstk, lvl_end);
    call(u64bFeed1, hstk, start);

    // Match needle's TU children against haystack siblings
    call(SPOTMatchChildren, b, YES, nstk, ndata, hstk, hdata);

    done;
}

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
    st->exhausted = NO;
    done;
}

// Iterative DFS walk of the haystack, trying to match at each sibling.
// At each container node, tries matching the needle as a subsequence
// of siblings starting from that position (using byte offset bookmarks).
// Returns after finding one match (or exhausting the tree).
static ok64 SPOTWalkAndMatch(SPOTstate *st, b8 *found) {
    sane(st != NULL);
    u8 ht = 0;
    u8cs hk = {}, hv = {};

    for (;;) {
        // Bookmark: save cursor before drain (byte offset into hay data)
        u64 cursor_before = *u64bLast(st->hstk);
        u64 level_end = *(u64bLast(st->hstk) - 1);

        ok64 ho = BASONDrain(st->hstk, st->hay, &ht, hk, hv);
        if (ho != OK) {
            if (st->depth == 0) done;
            BASONOuto(st->hstk);
            st->depth--;
            continue;
        }

        if ((ht == 'S' && SPOTIsWhitespace(hv)) || ht == 'D') continue;

        if (BASONCollection(ht)) {
            // Reset logs before each attempt
            if (st->mlog[0] != NULL) u64bReset(st->mlog);
            if (st->alog[0] != NULL) u64bReset(st->alog);

            // Try matching needle as sibling subsequence from this position.
            SPOTbinds b = {};

            try(SPOTTryMatchSeq, &b, st->ndl, st->hay,
                level_end, cursor_before, st->mlog, st->alog);
            then {
                // Copy back updated idle pointers
                if (st->mlog[0] != NULL) st->mlog[2] = b.mlog[2];
                if (st->alog[0] != NULL) st->alog[2] = b.alog[2];
                // Match found — store offsets for caller
                st->match = cursor_before;
                st->bound = b.bound;
                for (int i = 0; i < SPOT_MAX_BINDS; i++) {
                    if (b.bound & (1ULL << i))
                        st->binds[i] = b.offsets[i];
                    else
                        st->binds[i] = 0;
                }
                st->nsubs = (u8)b.nsubs;
                for (int i = 0; i < b.nsubs; i++)
                    st->subs[i] = b.subs[i];
                *found = YES;
                done;
            }

            // No match — descend into children for deeper matches
            call(BASONInto, st->hstk, st->hay, hv);
            st->depth++;
        }
    }
    done;
}

ok64 SPOTNext(SPOTstate *st) {
    sane(st != NULL);
    if (st->exhausted) return (SPOTEND);

    b8 found = NO;
    call(SPOTWalkAndMatch, st, &found);

    if (!found) {
        st->exhausted = YES;
        return (SPOTEND);
    }
    done;
}

// --- SPOTSourceRange: map BASON byte offset → source byte range ---

// Count meaningful children in a BASON container at bson_off.
// Meaningful = not whitespace-only S, not D comment, not empty-val leaf,
// not empty container (only whitespace/empty children).
static int SPOTCountInner(u8csc data, u64 bson_off) {
    aBpad(u64, stk, 256);
    u64bFeed1(stk, (u64)$len(data));
    u64bFeed1(stk, bson_off);
    u8 t = 0;
    u8cs k = {}, v = {};
    if (BASONDrain(stk, data, &t, k, v) != OK) return (0);
    if (!BASONCollection(t)) return (0);
    BASONInto(stk, data, v);
    int count = 0;
    while (BASONDrain(stk, data, &t, k, v) == OK) {
        if (t == 'S' && SPOTIsWhitespace(v)) continue;
        if (t == 'D') continue;
        if (!BASONCollection(t) && $len(v) == 0) continue;
        if (BASONCollection(t)) {
            // Skip empty containers (e.g. compound_statement from "if(x)")
            b8 has_content = NO;
            BASONInto(stk, data, v);
            u8 ct = 0;
            u8cs ck = {}, cv = {};
            while (BASONDrain(stk, data, &ct, ck, cv) == OK) {
                if (ct == 'S' && SPOTIsWhitespace(cv)) continue;
                if (ct == 'D') continue;
                if ($len(cv) == 0) continue;
                has_content = YES;
                break;
            }
            BASONOuto(stk);
            if (!has_content) continue;
        }
        count++;
    }
    return (count);
}

ok64 SPOTSourceRange(u8csc hay, u64 bson_off, u64p lo, u64p hi) {
    sane(hay[0] && lo && hi);
    // Walk the entire tree, tracking source position as cumulative leaf lengths.
    // When we find a leaf whose BASON offset falls within bson_off's subtree,
    // update lo/hi.
    *lo = UINT64_MAX;
    *hi = 0;

    // First, find the element at bson_off and determine its BASON end.
    u64 bson_end = 0;
    {
        aBpad(u64, stk, 256);
        call(u64bFeed1, stk, (u64)$len(hay));
        call(u64bFeed1, stk, bson_off);
        u8 t = 0;
        u8cs k = {}, v = {};
        ok64 o = BASONDrain(stk, hay, &t, k, v);
        if (o != OK) return (o);
        if (BASONCollection(t))
            bson_end = (u64)(v[1] - hay[0]);
        else
            bson_end = (u64)(v[1] - hay[0]);
    }

    // Walk entire tree, tracking source position
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
            // cursor is the BASON offset of this leaf's TLKV record
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

// Like SPOTSourceRange but only includes the first max_inner meaningful
// children of the container at bson_off.  Used when the needle matches
// only a prefix of a haystack container's children.
static ok64 SPOTSourceRangePartial(u8csc hay, u64 bson_off,
                                    int max_inner, u64p lo, u64p hi) {
    sane(hay[0] && lo && hi && max_inner > 0);
    *lo = UINT64_MAX;
    *hi = 0;

    // Read the container element and find child offsets for first max_inner
    // meaningful children.
    u64 child_start = 0, child_end = 0;
    {
        aBpad(u64, stk, 256);
        u64bFeed1(stk, (u64)$len(hay));
        u64bFeed1(stk, bson_off);
        u8 t = 0;
        u8cs k = {}, v = {};
        if (BASONDrain(stk, hay, &t, k, v) != OK) return (SPOTEND);
        if (!BASONCollection(t)) {
            // Not a container — just return full range
            return SPOTSourceRange(hay, bson_off, lo, hi);
        }
        BASONInto(stk, hay, v);
        child_start = *u64bLast(stk);
        int count = 0;
        while (count < max_inner) {
            u64 cur = *u64bLast(stk);
            if (BASONDrain(stk, hay, &t, k, v) != OK) break;
            if (t == 'S' && SPOTIsWhitespace(v)) continue;
            if (t == 'D') continue;
            count++;
        }
        child_end = *u64bLast(stk);
    }

    // Walk entire tree to map [child_start, child_end) to source positions
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
            BASONOuto(stk2);
            depth--;
            continue;
        }
        if (BASONCollection(t)) {
            BASONInto(stk2, hay, v);
            depth++;
        } else {
            size_t vlen = $len(v);
            if (cursor >= child_start && cursor < child_end) {
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

// Scan replacement template. Single-letter placeholders [a-zA-Z] not
// adjacent to other identifier chars are substituted with the source text
// of the bound placeholder value.
static ok64 SPOTInstTemplate(u8s out, u8csc tmpl,
                              u64 bound, u64cp binds,
                              u8csc hay, u8csc source) {
    sane(out[0] != NULL);
    u8cp p = tmpl[0];
    u8cp end = tmpl[1];
    while (p < end) {
        u8 c = *p;
        // Check if single-letter placeholder
        b8 is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        if (is_alpha) {
            // Check prev and next chars for identifier adjacency
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
                    // Resolve bound value: get source range
                    u64 lo = 0, hi = 0;
                    ok64 o = SPOTSourceRange(hay, binds[idx], &lo, &hi);
                    if (o == OK && hi > lo && hi <= (u64)$len(source)) {
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
    u64 src_lo;
    u64 src_hi;
    u64 bound;
    u64 binds[SPOT_MAX_BINDS];
} SPOTrep;

ok64 SPOTReplace(u8s out, u8csc source, u8csc hay,
                 u8csc needle_src, u8csc replace_src, u8csc ext) {
    sane(out[0] != NULL && source[0] != NULL);

    // Parse needle
    aBpad(u8, nbuf, 16384);
    aBpad(u64, nidx, 256);
    SPOTstate st = {};

    // Enable mlog for tracking match positions
    aBpad(u64, mlog, 1024);
    aBpad(u64, alog, 1024);

    call(SPOTInit, &st, nbuf, nidx, needle_src, ext, hay);
    st.mlog[0] = mlog[0]; st.mlog[1] = mlog[1];
    st.mlog[2] = mlog[2]; st.mlog[3] = mlog[3];
    st.alog[0] = alog[0]; st.alog[1] = alog[1];
    st.alog[2] = alog[2]; st.alog[3] = alog[3];

    u8cs ndata = {u8bDataHead(nbuf), u8bIdleHead(nbuf)};

    // Collect all matches using mlog entries for precise source ranges.
    SPOTrep matches[SPOT_MAX_MATCHES];
    int nmatch = 0;

    while (SPOTNext(&st) == OK && nmatch < SPOT_MAX_MATCHES) {
        // Use mlog entries to find the source range of matched region.
        // Each mlog entry records the haystack BASON offset of a matched
        // needle child.  The first entry's offset gives src_lo, the last
        // entry's offset gives src_hi (end of that element's subtree).
        size_t mlen = u64bDataLen(st.mlog);
        if (mlen == 0) continue;

        u64p mp = u64bDataHead(st.mlog);
        u64 first_off = (u64)SPOTLogHay(mp[0]);
        u64 last_off  = (u64)SPOTLogHay(mp[mlen - 1]);

        u64 first_lo = 0, first_hi = 0;
        u64 last_lo = 0, last_hi = 0;
        call(SPOTSourceRange, hay, first_off, &first_lo, &first_hi);

        // For the last mlog entry, check if needle has fewer meaningful
        // children than the haystack container (e.g. "if (x)" matches
        // only the condition, not the body).  Use partial range if so.
        u16 ndl_off = SPOTLogNdl(mp[mlen - 1]);
        int ndl_inner = SPOTCountInner(ndata, (u64)ndl_off);
        int hay_inner = SPOTCountInner(hay, last_off);
        if (ndl_inner > 0 && ndl_inner < hay_inner) {
            call(SPOTSourceRangePartial, hay, last_off,
                 ndl_inner, &last_lo, &last_hi);
        } else {
            call(SPOTSourceRange, hay, last_off, &last_lo, &last_hi);
        }

        u64 src_lo = first_lo;
        u64 src_hi = last_hi;

        if (src_hi > src_lo) {
            matches[nmatch].src_lo = src_lo;
            matches[nmatch].src_hi = src_hi;
            matches[nmatch].bound = st.bound;
            for (int i = 0; i < SPOT_MAX_BINDS; i++)
                matches[nmatch].binds[i] = st.binds[i];
            nmatch++;
        }
    }

    if (nmatch == 0) return (SPOTEND);

    // Sort matches by src_lo ascending (simple insertion sort)
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
    call(BASTParse, rbuf, ridx, replace_src, ext);
    u8cs rdata = {u8bDataHead(rbuf), u8bIdleHead(rbuf)};

    // Extract replacement text from parsed BASON (flatten leaves)
    aBpad(u8, rtxt, 16384);
    {
        aBpad(u64, rstk, 256);
        call(BASONOpen, rstk, rdata);
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
    u8cs rtxt_slice = {u8bDataHead(rtxt), u8bIdleHead(rtxt)};

    // Walk source left-to-right, applying replacements
    u64 pos = 0;
    for (int i = 0; i < nmatch; i++) {
        // Skip overlapping matches
        if (matches[i].src_lo < pos) continue;

        // Copy source[pos..match.src_lo]
        if (matches[i].src_lo > pos) {
            u8cs gap = {source[0] + pos, source[0] + matches[i].src_lo};
            call(u8sFeed, out, gap);
        }

        // Instantiate replacement template
        call(SPOTInstTemplate, out, rtxt_slice,
             matches[i].bound, matches[i].binds,
             hay, source);

        pos = matches[i].src_hi;
    }

    // Copy remaining source
    if (pos < (u64)$len(source)) {
        u8cs tail = {source[0] + pos, source[1]};
        call(u8sFeed, out, tail);
    }

    done;
}
