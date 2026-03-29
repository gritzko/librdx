#include "NEIL.h"

#include "abc/PRO.h"

// Byte length of non-whitespace tokens [from, from+count).
// Actual whitespace is not significant for diff weighting.
static u32 NEILByteSpan(u32cs toks, u8cp base, u32 from, u32 count) {
    if (count == 0) return 0;
    u32 ntoks = (u32)$len(toks);
    if (from + count > ntoks) return 0;
    u32 total = 0;
    for (u32 i = from; i < from + count; i++) {
        if (NEILIsWS(toks, base, i)) continue;
        u32 lo = (i > 0) ? TOK_OFF(toks[0][i - 1]) : 0;
        u32 hi = TOK_OFF(toks[0][i]);
        if (hi > lo) total += hi - lo;
    }
    return total;
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

            // Whitespace-only EQs anchor alignment — never kill them.
            if (eq_bytes == 0) { tmp[w++] = buf[k]; continue; }

            // Never kill EQs larger than the tunable ceiling.
            if (NEIL_MAX_KILL > 0 && eq_bytes >= NEIL_MAX_KILL) {
                tmp[w++] = buf[k]; continue;
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
