#include "NEIL.h"

#include "abc/PRO.h"

// Byte length of tokens [from, from+count), optionally skipping whitespace.
static u32 NEILByteSpanX(u32cs toks, u8cp base, u32 from, u32 count,
                         b8 skip_ws) {
    if (count == 0) return 0;
    u32 ntoks = (u32)$len(toks);
    if (from + count > ntoks) return 0;
    u32 total = 0;
    for (u32 i = from; i < from + count; i++) {
        if (skip_ws && NEILIsWS(toks, base, i)) continue;
        u32 lo = (i > 0) ? TOK_OFF(toks[0][i - 1]) : 0;
        u32 hi = TOK_OFF(toks[0][i]);
        if (hi > lo) total += hi - lo;
    }
    return total;
}

static u32 NEILByteSpan(u32cs toks, u8cp base, u32 from, u32 count) {
    return NEILByteSpanX(toks, base, from, count, YES);
}

// Merge adjacent same-op entries in place. Returns new count.
static u32 NEILMerge(e32 *buf, u32 n) {
    u32 m = 0;
    for (u32 k = 0; k < n; k++) {
        u32 op = DIFF_OP(buf[k]);
        u32 len = DIFF_LEN(buf[k]);
        if (len == 0) continue;
        if (m > 0 && DIFF_OP(buf[m - 1]) == op)
            buf[m - 1] = DIFF_ENTRY(op, DIFF_LEN(buf[m - 1]) + len);
        else
            buf[m++] = buf[k];
    }
    return m;
}

// Compare token text at index ia in (toks_a, base_a) with index ib
// in (toks_b, base_b).  Returns YES if identical.
static b8 NEILTokEq(u32cs ta, u8cp ba, u32 ia,
                     u32cs tb, u8cp bb, u32 ib) {
    u8cs va = {}, vb = {};
    TOK_VAL(va, ta, ba, (int)ia);
    TOK_VAL(vb, tb, bb, (int)ib);
    u32 la = (u32)(va[1] - va[0]);
    u32 lb = (u32)(vb[1] - vb[0]);
    if (la != lb) return NO;
    return memcmp(va[0], vb[0], la) == 0;
}

// Score the boundary between token idx-1 and token idx.
// Higher = better alignment.  Mirrors diff-match-patch scoring.
static int NEILBoundaryScore(u32cs toks, u8cp base, u32 idx, u32 ntoks) {
    if (idx == 0 || idx >= ntoks) return 6;  // edge
    u32 hi = TOK_OFF(toks[0][idx - 1]);
    u32 lo = (idx > 1) ? TOK_OFF(toks[0][idx - 2]) : 0;
    if (hi <= lo) return 6;
    u8 c1 = base[hi - 1];  // last byte before boundary

    u32 lo2 = hi;  // contiguous tokens
    u32 hi2 = TOK_OFF(toks[0][idx]);
    if (hi2 <= lo2) return 6;
    u8 c2 = base[lo2];  // first byte at boundary

    b8 na1 = !(c1 == '_' || (c1 >= 'a' && c1 <= 'z') ||
               (c1 >= 'A' && c1 <= 'Z') || (c1 >= '0' && c1 <= '9'));
    b8 na2 = !(c2 == '_' || (c2 >= 'a' && c2 <= 'z') ||
               (c2 >= 'A' && c2 <= 'Z') || (c2 >= '0' && c2 <= '9'));
    b8 ws1 = na1 && (c1 == ' ' || c1 == '\t' || c1 == '\n' || c1 == '\r');
    b8 ws2 = na2 && (c2 == ' ' || c2 == '\t' || c2 == '\n' || c2 == '\r');
    b8 lb1 = ws1 && (c1 == '\n' || c1 == '\r');
    b8 lb2 = ws2 && (c2 == '\n' || c2 == '\r');

    if (lb1 && lb2) return 5;  // blank line
    if (lb1 || lb2) return 4;  // line break
    if (na1 && !ws1 && ws2) return 3;  // end of sentence
    if (ws1 || ws2) return 2;  // whitespace
    if (na1 || na2) return 1;  // non-alphanumeric
    return 0;
}

ok64 NEILCleanup(e32g edl, u32cs old_toks, u32cs new_toks,
                 u8csc old_src, u8csc new_src) {
    sane(edl != NULL);
    u32 nedl = (u32)(edl[0] - edl[2]);
    if (nedl < 3) done;

    // Work in a malloc'd buffer (killed EQ expands to DEL+INS).
    u32 cap = nedl * 2 + 4;
    e32 *buf = (e32 *)malloc(cap * sizeof(e32));
    test(buf != NULL, NEILBAD);
    memcpy(buf, edl[2], nedl * sizeof(e32));
    u32 n = nedl;

    // Iterative semantic cleanup: kill false equalities until stable.
    // Each iteration merges edit regions, potentially exposing new kills.
    for (int iter = 0; iter < 8; iter++) {
        u32 *old_off = (u32 *)malloc(n * sizeof(u32));
        u32 *new_off = (u32 *)malloc(n * sizeof(u32));
        if (old_off == NULL || new_off == NULL) {
            free(old_off); free(new_off); free(buf);
            fail(NEILBAD);
        }
        {
            u32 oi = 0, ni = 0;
            for (u32 k = 0; k < n; k++) {
                old_off[k] = oi;
                new_off[k] = ni;
                u32 len = DIFF_LEN(buf[k]);
                u32 op = DIFF_OP(buf[k]);
                if (op == DIFF_EQ) { oi += len; ni += len; }
                else if (op == DIFF_DEL) { oi += len; }
                else { ni += len; }
            }
        }

        b8 changed = NO;
        u32 tcap = n * 2 + 4;
        e32 *tmp = (e32 *)malloc(tcap * sizeof(e32));
        if (tmp == NULL) {
            free(old_off); free(new_off); free(buf);
            fail(NEILBAD);
        }

        u32 w = 0;
        for (u32 k = 0; k < n; k++) {
            if (DIFF_OP(buf[k]) != DIFF_EQ) { tmp[w++] = buf[k]; continue; }
            u32 eq_len = DIFF_LEN(buf[k]);
            u32 eq_bytes = NEILByteSpan(new_toks, new_src[0],
                                        new_off[k], eq_len);

            // Accumulate edit bytes before (until prev EQ)
            u32 before_bytes = 0;
            for (u32 j = k; j > 0; ) {
                j--;
                u32 jop = DIFF_OP(buf[j]);
                if (jop == DIFF_EQ) break;
                u32 jlen = DIFF_LEN(buf[j]);
                if (jop == DIFF_DEL)
                    before_bytes += NEILByteSpan(old_toks, old_src[0],
                                                 old_off[j], jlen);
                else
                    before_bytes += NEILByteSpan(new_toks, new_src[0],
                                                 new_off[j], jlen);
            }

            // Accumulate edit bytes after (until next EQ)
            u32 after_bytes = 0;
            for (u32 j = k + 1; j < n; j++) {
                u32 jop = DIFF_OP(buf[j]);
                if (jop == DIFF_EQ) break;
                u32 jlen = DIFF_LEN(buf[j]);
                if (jop == DIFF_DEL)
                    after_bytes += NEILByteSpan(old_toks, old_src[0],
                                                 old_off[j], jlen);
                else
                    after_bytes += NEILByteSpan(new_toks, new_src[0],
                                                 new_off[j], jlen);
            }

            if (before_bytes == 0 || after_bytes == 0) {
                tmp[w++] = buf[k]; continue;
            }

            // Whitespace-only EQs: use raw byte span for kill decision.
            if (eq_bytes == 0) {
                u32 raw = NEILByteSpanX(new_toks, new_src[0],
                                        new_off[k], eq_len, NO);
                if (raw < before_bytes + after_bytes) {
                    if (eq_len > 0) tmp[w++] = DIFF_ENTRY(DIFF_DEL, eq_len);
                    if (eq_len > 0) tmp[w++] = DIFF_ENTRY(DIFF_INS, eq_len);
                    changed = YES;
                } else {
                    tmp[w++] = buf[k];
                }
                continue;
            }

            // Never kill EQs larger than the tunable ceiling.
            if (NEIL_MAX_KILL > 0 && eq_bytes >= NEIL_MAX_KILL) {
                tmp[w++] = buf[k]; continue;
            }

            // Protect EQs that contain a complete shared line: a newline
            // followed by >= 6 non-ws bytes of code.  This catches real
            // matches like "\n    if (flag != OK) {" but rejects partial
            // matches like "u8bFeed(fpbuf, fpath_s);\n    " where all
            // the code is BEFORE the newline and only whitespace follows.
            if (eq_bytes >= 6) {
                u32 eq_from = new_off[k];
                u32 eq_blo = (eq_from > 0)
                    ? TOK_OFF(new_toks[0][eq_from - 1]) : 0;
                u32 eq_bhi = TOK_OFF(new_toks[0][eq_from + eq_len - 1]);
                if (eq_bhi > eq_blo &&
                    memchr(new_src[0] + eq_blo, '\n', eq_bhi - eq_blo)) {
                    u32 nw_after_nl = 0;
                    b8 past_nl = NO;
                    for (u32 b = eq_blo; b < eq_bhi; b++) {
                        u8 c = new_src[0][b];
                        if (c == '\n') { past_nl = YES; continue; }
                        if (past_nl && c != ' ' && c != '\t' && c != '\r')
                            nw_after_nl++;
                    }
                    if (nw_after_nl >= 6) {
                        tmp[w++] = buf[k]; continue;
                    }
                }
            }

            // Kill small false EQs: two tiers.
            // Small EQs (< 16 bytes): sum condition (aggressive).
            // Larger EQs: both sides must individually exceed it.
            b8 do_kill = NO;
            if (eq_bytes < 16)
                do_kill = eq_bytes < before_bytes + after_bytes;
            else
                do_kill = eq_bytes < before_bytes &&
                          eq_bytes < after_bytes;
            if (do_kill) {
                if (eq_len > 0) tmp[w++] = DIFF_ENTRY(DIFF_DEL, eq_len);
                if (eq_len > 0) tmp[w++] = DIFF_ENTRY(DIFF_INS, eq_len);
                changed = YES;
            } else {
                tmp[w++] = buf[k];
            }
        }

        free(old_off);
        free(new_off);

        w = NEILMerge(tmp, w);
        free(buf);
        buf = tmp;
        n = w;

        if (!changed) break;
    }

    // Copy back into edl buffer
    u32 ecap = (u32)(edl[1] - edl[2]);
    u32 final_n = (n < ecap) ? n : ecap;
    memcpy(edl[2], buf, final_n * sizeof(e32));
    edl[0] = edl[2] + final_n;

    free(buf);
    done;
}

ok64 NEILShift(e32g edl, u32cs old_toks, u32cs new_toks,
               u8csc old_src, u8csc new_src) {
    sane(edl != NULL);
    u32 nedl = (u32)(edl[0] - edl[2]);
    if (nedl < 3) done;

    u32 new_ntoks = (u32)$len(new_toks);
    u32 old_ntoks = (u32)$len(old_toks);

    // Compute old/new token offsets for each EDL entry.
    u32 *ooff = (u32 *)malloc(nedl * sizeof(u32));
    u32 *noff = (u32 *)malloc(nedl * sizeof(u32));
    if (ooff == NULL || noff == NULL) {
        free(ooff); free(noff);
        fail(NEILBAD);
    }
    {
        u32 oi = 0, ni = 0;
        for (u32 k = 0; k < nedl; k++) {
            ooff[k] = oi;
            noff[k] = ni;
            u32 len = DIFF_LEN(edl[2][k]);
            u32 op = DIFF_OP(edl[2][k]);
            if (op == DIFF_EQ) { oi += len; ni += len; }
            else if (op == DIFF_DEL) { oi += len; }
            else { ni += len; }
        }
    }

    // Process change regions: runs of non-EQ entries between two EQs.
    u32 k = 0;
    while (k < nedl) {
        if (DIFF_OP(edl[2][k]) != DIFF_EQ) { k++; continue; }
        u32 eq1 = k;
        u32 cs = k + 1;  // change region start
        if (cs >= nedl || DIFF_OP(edl[2][cs]) == DIFF_EQ) {
            k++; continue;
        }
        // Find end of change region
        u32 ce = cs;
        while (ce < nedl && DIFF_OP(edl[2][ce]) != DIFF_EQ) ce++;
        if (ce >= nedl) break;  // no trailing EQ
        u32 eq2 = ce;

        u32 n1 = DIFF_LEN(edl[2][eq1]);
        u32 n2 = DIFF_LEN(edl[2][eq2]);
        if (n1 + n2 == 0) { k = ce; continue; }

        // Compute total DEL/INS tokens in the change region.
        // DEL tokens are contiguous on old side, INS on new side.
        u32 dtot = 0, etot = 0;
        for (u32 i = cs; i < ce; i++) {
            if (DIFF_OP(edl[2][i]) == DIFF_DEL) dtot += DIFF_LEN(edl[2][i]);
            else etot += DIFF_LEN(edl[2][i]);
        }
        if (dtot + etot == 0) { k = ce; continue; }

        // Old/new positions at the change region boundary.
        u32 oi_cs = ooff[cs];  // old start of change region
        u32 ni_cs = noff[cs];  // new start of change region

        // 1. Max left shift: compare old/new tokens walking backward
        //    from the end of the change region.  Naturally walks through
        //    DEL/INS then into EQ1 on the respective sides.
        u32 max_left = 0;
        {
            u32 oi_end = oi_cs + dtot;
            u32 ni_end = ni_cs + etot;
            for (u32 j = 0; j < n1; j++) {
                if (!NEILTokEq(old_toks, old_src[0], oi_end - 1 - j,
                               new_toks, new_src[0], ni_end - 1 - j))
                    break;
                max_left = j + 1;
            }
        }

        // 2. Max right shift: compare old/new tokens walking forward
        //    from the start of the change region.  Naturally walks
        //    through DEL/INS then into EQ2 on the respective sides.
        u32 max_right = 0;
        for (u32 j = 0; j < n2; j++) {
            if (!NEILTokEq(old_toks, old_src[0], oi_cs + j,
                           new_toks, new_src[0], ni_cs + j))
                break;
            max_right = j + 1;
        }

        if (max_left + max_right == 0) { k = ce; continue; }

        // 3. Score all positions, pick best.
        //    Score on new side (context display) + old side if DEL present.
        int best_d = 0, best_sc = 0;
        for (int d = -(int)max_left; d <= (int)max_right; d++) {
            int sc = 0;
            if (etot > 0) {
                u32 li = (u32)((int)ni_cs + d);
                u32 ri = (u32)((int)ni_cs + (int)etot + d);
                sc += NEILBoundaryScore(new_toks, new_src[0], li, new_ntoks)
                    + NEILBoundaryScore(new_toks, new_src[0], ri, new_ntoks);
            }
            if (dtot > 0) {
                u32 li = (u32)((int)oi_cs + d);
                u32 ri = (u32)((int)oi_cs + (int)dtot + d);
                sc += NEILBoundaryScore(old_toks, old_src[0], li, old_ntoks)
                    + NEILBoundaryScore(old_toks, old_src[0], ri, old_ntoks);
            }
            if (sc >= best_sc) {  // >= prefers trailing whitespace
                best_sc = sc;
                best_d = d;
            }
        }

        if (best_d != 0) {
            edl[2][eq1] = DIFF_ENTRY(DIFF_EQ, (u32)((int)n1 + best_d));
            edl[2][eq2] = DIFF_ENTRY(DIFF_EQ, (u32)((int)n2 - best_d));
        }

        k = ce;
    }

    // Remove any 0-length entries produced by shifting.
    u32 w = 0;
    for (u32 k2 = 0; k2 < nedl; k2++) {
        if (DIFF_LEN(edl[2][k2]) == 0) continue;
        edl[2][w++] = edl[2][k2];
    }
    edl[0] = edl[2] + w;

    free(ooff);
    free(noff);
    done;
}
