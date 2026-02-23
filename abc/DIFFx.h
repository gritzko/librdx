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

    for (i32 d = 0; d <= (max + 1) / 2; d++) {
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

    *pd = max;
    *px = *py = *plen = 0;
    return OK;
}

static inline ok64 X(DIFF, Recurse)(e32g edl, i32s vf, i32s vb, Tcs a, Tcs b) {
    i32 n = $len(a), m = $len(b);
    ok64 o;

    if (n == 0) {
        if (m > 0) return X(DIFF, AddEntry)(edl, DIFF_INS, m);
        return OK;
    }
    if (m == 0) {
        if (n > 0) return X(DIFF, AddEntry)(edl, DIFF_DEL, n);
        return OK;
    }

    i32 x, y, len, d;
    o = X(DIFF, FindMiddle)(a, b, vf, vb, &x, &y, &len, &d);
    if (o != OK) return o;

    if (d == 0) return X(DIFF, AddEntry)(edl, DIFF_EQ, n);

    // Validate snake: recompute actual matching length from (x,y)
    // FindMiddle may return incorrect snake length due to algorithm quirks
    {
        i32 valid_len = 0;
        while (x + valid_len < n && y + valid_len < m &&
               a[0][x + valid_len] == b[0][y + valid_len]) {
            valid_len++;
        }
        len = valid_len;
    }

    if (d == 1) {
        if (n > m) {
            if (y > 0) { o = X(DIFF, AddEntry)(edl, DIFF_EQ, y); if (o != OK) return o; }
            o = X(DIFF, AddEntry)(edl, DIFF_DEL, 1); if (o != OK) return o;
            if (len > 0) return X(DIFF, AddEntry)(edl, DIFF_EQ, len);
        } else {
            if (x > 0) { o = X(DIFF, AddEntry)(edl, DIFF_EQ, x); if (o != OK) return o; }
            o = X(DIFF, AddEntry)(edl, DIFF_INS, 1); if (o != OK) return o;
            if (len > 0) return X(DIFF, AddEntry)(edl, DIFF_EQ, len);
        }
        return OK;
    }

    a_head(Tc, a1, a, x);
    a_head(Tc, b1, b, y);
    o = X(DIFF, Recurse)(edl, vf, vb, a1, b1);
    if (o != OK) return o;

    if (len > 0) {
        o = X(DIFF, AddEntry)(edl, DIFF_EQ, len);
        if (o != OK) return o;
    }

    a_rest(Tc, a2, a, x + len);
    a_rest(Tc, b2, b, y + len);
    return X(DIFF, Recurse)(edl, vf, vb, a2, b2);
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
