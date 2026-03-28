//
// DIFFx.h - Linear-space Myers diff template
// Include with X(M, name) defined, e.g. #define X(M, name) M##u8##name
//

#define T X(, )
#define Tc X(, c)
#define Ts X(, s)
#define Tcs X(, cs)
#define Tcp X(, cp)

static inline ok64 X(DIFF, AddEntry)(e32g edl, u8 op, u32 len) {
    if (len == 0) return OK;
    if (edl[0] > edl[2]) {
        e32 *prev = edl[0] - 1;
        if (DIFF_OP(*prev) == op) {
            *prev = DIFF_ENTRY(op, DIFF_LEN(*prev) + len);
            return OK;
        }
    }
    if (edl[0] >= edl[1]) return DIFFNOROOM;
    *edl[0]++ = DIFF_ENTRY(op, len);
    return OK;
}

static inline ok64 X(DIFF, FindMiddle)(Tcs a, Tcs b, i32s vf, i32s vb,
                                       i32 *px, i32 *py, i32 *plen, i32 *pd) {
    i32 n = $len(a), m = $len(b);
    i32 max = n + m, delta = n - m;
    b8 odd = delta & 1;

    i32p vfp = vf[0] + max;
    i32p vbp = vb[0] + max;
    vfp[1] = 0;
    vbp[1] = 0;

    i32 d_limit = (max + 1) / 2;
    if (d_limit > 1024) d_limit = 1024;
    for (i32 d = 0; d <= d_limit; d++) {
        for (i32 k = -d; k <= d; k += 2) {
            i32 x = (k == -d || (k != d && vfp[k - 1] < vfp[k + 1]))
                        ? vfp[k + 1]
                        : vfp[k - 1] + 1;
            i32 y = x - k;
            i32 x0 = x;
            while (x < n && y < m && a[0][x] == b[0][y]) {
                x++;
                y++;
            }
            vfp[k] = x;

            if (odd && k >= delta - (d - 1) && k <= delta + (d - 1)) {
                i32 bk = delta - k;
                if (bk >= -(d - 1) && bk <= (d - 1)) {
                    i32 bx = n - vbp[bk];
                    if (x >= bx) {
                        *px = x0;
                        *py = x0 - k;
                        *plen = x - x0;
                        *pd = 2 * d - 1;
                        return OK;
                    }
                }
            }
        }

        for (i32 k = -d; k <= d; k += 2) {
            i32 x = (k == -d || (k != d && vbp[k - 1] < vbp[k + 1]))
                        ? vbp[k + 1]
                        : vbp[k - 1] + 1;
            i32 y = x - k;
            while (x < n && y < m && a[0][n - 1 - x] == b[0][m - 1 - y]) {
                x++;
                y++;
            }
            vbp[k] = x;

            if (!odd && k >= delta - d && k <= delta + d) {
                i32 fk = delta - k;
                if (fk >= -d && fk <= d) {
                    i32 fx = vfp[fk];
                    i32 bx = n - x;
                    if (fx >= bx) {
                        *px = bx;
                        *py = bx - fk;
                        *plen = fx - bx;
                        *pd = 2 * d;
                        return OK;
                    }
                }
            }
        }
    }

    *pd = -1;  // bail-out: d_limit reached without finding middle
    *px = *py = *plen = 0;
    return OK;
}

static inline ok64 X(DIFF, Recurse)(e32g edl, i32s vf, i32s vb, Tcs a, Tcs b) {
    ok64 o;

    // Strip common prefix/suffix — O(n) scan that reduces the
    // effective input to Myers, applied at every recursion level.
    // Use local pointers to avoid mutating the caller's slices.
    Tcp ap = a[0], ae = a[1];
    Tcp bp = b[0], be = b[1];
    while (ap < ae && bp < be && *ap == *bp) { ap++; bp++; }
    i32 prefix = (i32)(ap - a[0]);
    if (prefix > 0) {
        o = X(DIFF, AddEntry)(edl, DIFF_EQ, prefix);
        if (o != OK) return o;
    }
    while (ap < ae && bp < be && *(ae - 1) == *(be - 1)) { ae--; be--; }
    i32 suffix = (i32)(a[1] - ae);

    Tcs sa = {ap, ae};
    Tcs sb = {bp, be};
    i32 n = $len(sa), m = $len(sb);

    if (n == 0) {
        if (m > 0) { o = X(DIFF, AddEntry)(edl, DIFF_INS, m); if (o != OK) return o; }
        goto emit_suffix;
    }
    if (m == 0) {
        if (n > 0) { o = X(DIFF, AddEntry)(edl, DIFF_DEL, n); if (o != OK) return o; }
        goto emit_suffix;
    }

    i32 x, y, len, d;
    o = X(DIFF, FindMiddle)(sa, sb, vf, vb, &x, &y, &len, &d);
    if (o != OK) return o;

    if (d == 0) {
        o = X(DIFF, AddEntry)(edl, DIFF_EQ, n);
        if (o != OK) return o;
        goto emit_suffix;
    }
    if (d < 0) {
        // FindMiddle bailed (d_limit reached): treat as full replace
        o = X(DIFF, AddEntry)(edl, DIFF_DEL, n);
        if (o != OK) return o;
        o = X(DIFF, AddEntry)(edl, DIFF_INS, m);
        if (o != OK) return o;
        goto emit_suffix;
    }

    // Validate snake: recompute actual matching length from (x,y)
    // FindMiddle may return incorrect snake length due to algorithm quirks
    {
        i32 valid_len = 0;
        while (x + valid_len < n && y + valid_len < m &&
               sa[0][x + valid_len] == sb[0][y + valid_len]) {
            valid_len++;
        }
        len = valid_len;
    }

    if (d == 1) {
        if (n > m) {
            if (y > 0) { o = X(DIFF, AddEntry)(edl, DIFF_EQ, y); if (o != OK) return o; }
            o = X(DIFF, AddEntry)(edl, DIFF_DEL, 1); if (o != OK) return o;
            if (len > 0) { o = X(DIFF, AddEntry)(edl, DIFF_EQ, len); if (o != OK) return o; }
        } else {
            if (x > 0) { o = X(DIFF, AddEntry)(edl, DIFF_EQ, x); if (o != OK) return o; }
            o = X(DIFF, AddEntry)(edl, DIFF_INS, 1); if (o != OK) return o;
            if (len > 0) { o = X(DIFF, AddEntry)(edl, DIFF_EQ, len); if (o != OK) return o; }
        }
        goto emit_suffix;
    }

    a_head(Tc, a1, sa, x);
    a_head(Tc, b1, sb, y);
    o = X(DIFF, Recurse)(edl, vf, vb, a1, b1);
    if (o != OK) return o;

    if (len > 0) {
        o = X(DIFF, AddEntry)(edl, DIFF_EQ, len);
        if (o != OK) return o;
    }

    a_rest(Tc, a2, sa, x + len);
    a_rest(Tc, b2, sb, y + len);
    o = X(DIFF, Recurse)(edl, vf, vb, a2, b2);
    if (o != OK) return o;

emit_suffix:
    if (suffix > 0)
        return X(DIFF, AddEntry)(edl, DIFF_EQ, suffix);
    return OK;
}

static inline ok64 X(DIFF, s)(e32g edl, i32s work, Tcs a, Tcs b) {
    i32 n = $len(a), m = $len(b), max = n + m;

    if (max == 0) return OK;

    u64 vsize = 2 * max + 1;
    if ($len(work) < (i64)(2 * vsize)) return DIFFNOROOM;

    i32s vf = {work[0], work[0] + vsize};
    i32s vb = {work[0] + vsize, work[0] + 2 * vsize};

    return X(DIFF, Recurse)(edl, vf, vb, a, b);
}

static inline ok64 X(DIFF, sApply)(Ts into, Tcs a, Tcs b, e32cs edl) {
    Tcp ap = a[0], bp = b[0];

    $for(e32c, ep, edl) {
        u8 op = DIFF_OP(*ep);
        u32 len = DIFF_LEN(*ep);

        switch (op) {
            case DIFF_EQ:
                if (ap + len > a[1]) return DIFFNOROOM;
                if (into[0] + len > into[1]) return DIFFNOROOM;
                memcpy(into[0], ap, len * sizeof(T));
                into[0] += len;
                ap += len;
                bp += len;
                break;
            case DIFF_DEL:
                if (ap + len > a[1]) return DIFFNOROOM;
                ap += len;
                break;
            case DIFF_INS:
                if (bp + len > b[1]) return DIFFNOROOM;
                if (into[0] + len > into[1]) return DIFFNOROOM;
                memcpy(into[0], bp, len * sizeof(T));
                into[0] += len;
                bp += len;
                break;
        }
    }

    return OK;
}

#undef T
#undef Tc
#undef Ts
#undef Tcs
#undef Tcp
