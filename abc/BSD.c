// BSD.c — in-memory bsdiff/bspatch implementation
// Ported from libbdiff (Colin Percival / William Woodruff) to ABC style.
// No bzip2, no malloc, no file I/O — pure in-memory with caller buffers.

#include "BSD.h"

#include "PRO.h"

// --- suffix sort internals ---

static void BSDsplit(i64p I, i64p V, i64 start, i64 len, i64 h) {
    i64 i, j, k, x, tmp, jj, kk;

    if (len < 16) {
        for (k = start; k < start + len; k += j) {
            j = 1;
            x = V[I[k] + h];
            for (i = 1; k + i < start + len; i++) {
                if (V[I[k + i] + h] < x) {
                    x = V[I[k + i] + h];
                    j = 0;
                }
                if (V[I[k + i] + h] == x) {
                    tmp = I[k + j];
                    I[k + j] = I[k + i];
                    I[k + i] = tmp;
                    j++;
                }
            }
            for (i = 0; i < j; i++) V[I[k + i]] = k + j - 1;
            if (j == 1) I[k] = -1;
        }
        return;
    }

    x = V[I[start + len / 2] + h];
    jj = 0;
    kk = 0;

    for (i = start; i < start + len; i++) {
        if (V[I[i] + h] < x) jj++;
        if (V[I[i] + h] == x) kk++;
    }

    jj += start;
    kk += jj;

    i = start;
    j = 0;
    k = 0;

    while (i < jj) {
        if (V[I[i] + h] < x) {
            i++;
        } else if (V[I[i] + h] == x) {
            tmp = I[i];
            I[i] = I[jj + j];
            I[jj + j] = tmp;
            j++;
        } else {
            tmp = I[i];
            I[i] = I[kk + k];
            I[kk + k] = tmp;
            k++;
        }
    }

    while (jj + j < kk) {
        if (V[I[jj + j] + h] == x) {
            j++;
        } else {
            tmp = I[jj + j];
            I[jj + j] = I[kk + k];
            I[kk + k] = tmp;
            k++;
        }
    }

    if (jj > start) BSDsplit(I, V, start, jj - start, h);

    for (i = 0; i < kk - jj; i++) V[I[jj + i]] = kk - 1;

    if (jj == kk - 1) I[jj] = -1;

    if (start + len > kk) BSDsplit(I, V, kk, start + len - kk, h);
}

static void BSDqsufsort(i64p I, i64p V, u8cp old, i64 oldsize) {
    i64 buckets[256];
    i64 i, h, len;

    for (i = 0; i < 256; i++) buckets[i] = 0;
    for (i = 0; i < oldsize; i++) buckets[old[i]]++;
    for (i = 1; i < 256; i++) buckets[i] += buckets[i - 1];
    for (i = 255; i > 0; i--) buckets[i] = buckets[i - 1];
    buckets[0] = 0;

    for (i = 0; i < oldsize; i++) I[++buckets[old[i]]] = i;
    I[0] = oldsize;

    for (i = 0; i < oldsize; i++) V[i] = buckets[old[i]];
    V[oldsize] = 0;

    for (i = 1; i < 256; i++) {
        if (buckets[i] == buckets[i - 1] + 1) I[buckets[i]] = -1;
    }
    I[0] = -1;

    for (h = 1; I[0] != -(oldsize + 1); h += h) {
        len = 0;
        for (i = 0; i < oldsize + 1;) {
            if (I[i] < 0) {
                len -= I[i];
                i -= I[i];
            } else {
                if (len) I[i - len] = -len;
                len = V[I[i]] + 1 - i;
                BSDsplit(I, V, i, len, h);
                i += len;
                len = 0;
            }
        }
        if (len) I[i - len] = -len;
    }

    for (i = 0; i < oldsize + 1; i++) I[V[i]] = i;
}

// --- search ---

static i64 BSDmatchlen(u8cp old, i64 oldsize, u8cp neu, i64 neusize) {
    i64 i;
    i64 lim = oldsize < neusize ? oldsize : neusize;
    for (i = 0; i < lim; i++) {
        if (old[i] != neu[i]) break;
    }
    return i;
}

static i64 BSDsearch(i64p I, u8cp old, i64 oldsize, u8cp neu, i64 neusize,
                      i64 st, i64 en, i64p pos) {
    // iterative binary search
    while (en - st >= 2) {
        i64 pivot = st + (en - st) / 2;
        i64 ilen = oldsize - I[pivot];
        i64 cmplen = ilen < neusize ? ilen : neusize;
        if (memcmp(old + I[pivot], neu, cmplen) < 0)
            st = pivot;
        else
            en = pivot;
    }

    i64 x = BSDmatchlen(old + I[st], oldsize - I[st], neu, neusize);
    i64 y = BSDmatchlen(old + I[en], oldsize - I[en], neu, neusize);

    if (x > y) {
        *pos = I[st];
        return x;
    } else {
        *pos = I[en];
        return y;
    }
}

// --- offset encoding (little-endian signed i64) ---

static void BSDofftout(i64 x, u8p buf) {
    i64 y = x < 0 ? -x : x;
    buf[0] = y & 0xff;
    buf[1] = (y >> 8) & 0xff;
    buf[2] = (y >> 16) & 0xff;
    buf[3] = (y >> 24) & 0xff;
    buf[4] = (y >> 32) & 0xff;
    buf[5] = (y >> 40) & 0xff;
    buf[6] = (y >> 48) & 0xff;
    buf[7] = (y >> 56) & 0xff;
    if (x < 0) buf[7] |= 0x80;
}

static i64 BSDofftin(u8cp buf) {
    i64 y;
    y = buf[7] & 0x7F;
    y = y * 256 + buf[6];
    y = y * 256 + buf[5];
    y = y * 256 + buf[4];
    y = y * 256 + buf[3];
    y = y * 256 + buf[2];
    y = y * 256 + buf[1];
    y = y * 256 + buf[0];
    if (buf[7] & 0x80) y = -y;
    return y;
}

// --- diff ---

ok64 BSDDiff(u8s patch, u8csc old, u8csc neu, i64s work) {
    sane(patch != NULL && old != NULL && neu != NULL && work != NULL);

    i64 oldsize = (i64)$len(old);
    i64 neusize = (i64)$len(neu);
    u8cp ob = old[0];
    u8cp nb = neu[0];

    // workspace: I[0..oldsize], V[0..oldsize]
    test($len(work) >= (i64)BSDWorkLen(oldsize), BADARG);

    i64p I = work[0];
    i64p V = I + oldsize + 1;

    BSDqsufsort(I, V, ob, oldsize);
    // V no longer needed after sort; I has suffix array

    // Pass 1: compute control triples + diff + extra into patch buffer
    // Layout: header(24) + ctrl(N*24) + diff + extra
    // We write ctrl triples first, then diff, then extra.
    // We need to accumulate them because sizes are interdependent.

    u8p out = patch[0];
    u8p outend = patch[1];

    // Reserve 24 bytes for header
    test(outend - out >= 24, BSDnoroom);
    u8p header = out;
    out += 24;

    // Phase 1: accumulate ctrl triples in-place
    u8p ctrl_start = out;
    i64 scan = 0, len = 0;
    i64 lastscan = 0, lastpos = 0, lastoffset = 0;

    // We also need to accumulate diff and extra bytes.
    // Strategy: write ctrl triples forward, then we know ctrl_end.
    // Then write diff bytes, then extra bytes.
    // But diff/extra depend on ctrl, so we do TWO passes through the
    // main diff loop: first to compute ctrl, second to emit diff+extra.
    // To avoid two passes, we accumulate ctrl triples and diff/extra
    // sizes, then lay them out.

    // Actually, we can do it in one pass if we write ctrl triples first,
    // keeping diff and extra bytes in temporary region at end of buffer.
    // But that's fragile. Instead: single pass, accumulate ctrl in-place,
    // then a second pass to emit diff and extra given the ctrl data.

    // Accumulate ctrl triples
    i64 ctrl_count = 0;

    scan = 0;
    len = 0;
    lastscan = 0;
    lastpos = 0;
    lastoffset = 0;

    while (scan < neusize) {
        i64 oldscore = 0;
        i64 scsc;

        for (scsc = (scan += len); scan < neusize; scan++) {
            len = BSDsearch(I, ob, oldsize, nb + scan, neusize - scan, 0,
                            oldsize, &V[0]);  // reuse V[0] as pos
            i64 pos = V[0];

            for (; scsc < scan + len; scsc++) {
                if (scsc + lastoffset < oldsize &&
                    ob[scsc + lastoffset] == nb[scsc])
                    oldscore++;
            }

            if ((len == oldscore && len != 0) || len > oldscore + 8) break;

            if (scan + lastoffset < oldsize &&
                ob[scan + lastoffset] == nb[scan])
                oldscore--;
        }

        if (len != oldscore || scan == neusize) {
            i64 pos = V[0];
            i64 s = 0, Sf = 0, lenf = 0;
            i64 i;

            for (i = 0; lastscan + i < scan && lastpos + i < oldsize;) {
                if (ob[lastpos + i] == nb[lastscan + i]) s++;
                i++;
                if (s * 2 - i > Sf * 2 - lenf) {
                    Sf = s;
                    lenf = i;
                }
            }

            i64 lenb = 0;
            if (scan < neusize) {
                s = 0;
                i64 Sb = 0;
                for (i = 1; scan >= lastscan + i && pos >= i; i++) {
                    if (ob[pos - i] == nb[scan - i]) s++;
                    if (s * 2 - i > Sb * 2 - lenb) {
                        Sb = s;
                        lenb = i;
                    }
                }
            }

            if (lastscan + lenf > scan - lenb) {
                i64 overlap = (lastscan + lenf) - (scan - lenb);
                s = 0;
                i64 Ss = 0, lens = 0;
                for (i = 0; i < overlap; i++) {
                    if (nb[lastscan + lenf - overlap + i] ==
                        ob[lastpos + lenf - overlap + i])
                        s++;
                    if (nb[scan - lenb + i] == ob[pos - lenb + i]) s--;
                    if (s > Ss) {
                        Ss = s;
                        lens = i + 1;
                    }
                }
                lenf += lens - overlap;
                lenb -= lens;
            }

            i64 diff_len = lenf;
            i64 extra_len = (scan - lenb) - (lastscan + lenf);
            i64 seek_off = (pos - lenb) - (lastpos + lenf);

            // Write ctrl triple (3 * 8 bytes)
            test(outend - out >= 24, BSDnoroom);
            BSDofftout(diff_len, out);
            out += 8;
            BSDofftout(extra_len, out);
            out += 8;
            BSDofftout(seek_off, out);
            out += 8;
            ctrl_count++;

            lastscan = scan - lenb;
            lastpos = pos - lenb;
            lastoffset = pos - scan;
        }
    }

    u8p ctrl_end = out;

    // Now emit diff block
    u8p diff_start = out;
    lastscan = 0;
    lastpos = 0;
    lastoffset = 0;
    scan = 0;
    len = 0;

    // Re-read ctrl triples to emit diff and extra
    u8cp cp = ctrl_start;
    for (i64 ci = 0; ci < ctrl_count; ci++) {
        i64 diff_len = BSDofftin(cp);
        cp += 8;
        i64 extra_len = BSDofftin(cp);
        cp += 8;
        i64 seek_off = BSDofftin(cp);
        cp += 8;

        // diff bytes
        test(outend - out >= diff_len, BSDnoroom);
        for (i64 i = 0; i < diff_len; i++) {
            u8 ob_val =
                (lastpos + i >= 0 && lastpos + i < oldsize) ? ob[lastpos + i] : 0;
            out[i] = nb[lastscan + i] - ob_val;
        }
        out += diff_len;

        lastscan += diff_len;
        lastpos += diff_len;

        // extra bytes
        test(outend - out >= extra_len, BSDnoroom);
        memcpy(out, nb + lastscan, extra_len);
        out += extra_len;

        lastscan += extra_len;
        lastpos += seek_off;
        lastoffset = lastpos - lastscan;
    }

    // Write header
    memcpy(header, "BSDIFF01", 8);
    BSDofftout(neusize, header + 8);
    BSDofftout(ctrl_count, header + 16);

    // Update patch slice to reflect written size
    patch[0] = out;

    done;
}

// --- patch ---

ok64 BSDPatch(u8s neu, u8csc old, u8csc patch) {
    sane(neu != NULL && old != NULL && patch != NULL);

    i64 patchlen = (i64)$len(patch);
    test(patchlen >= 24, BSDcorrupt);

    u8cp pb = patch[0];

    // Check magic
    test(memcmp(pb, "BSDIFF01", 8) == 0, BSDbadmagic);

    i64 neusize = BSDofftin(pb + 8);
    i64 ctrl_count = BSDofftin(pb + 16);
    test(neusize >= 0 && ctrl_count >= 0, BSDcorrupt);
    test($len(neu) >= neusize, BSDnoroom);

    i64 oldsize = (i64)$len(old);
    u8cp ob = old[0];
    u8p nb = neu[0];

    // ctrl block starts at offset 24
    u8cp cp = pb + 24;
    i64 ctrl_bytes = ctrl_count * 24;
    test(patchlen >= 24 + ctrl_bytes, BSDcorrupt);

    // diff block follows ctrl
    u8cp dp = cp + ctrl_bytes;
    // extra block follows diff (we compute pointer as we go)
    // First, compute total diff_len to find extra start
    i64 total_diff = 0;
    i64 total_extra = 0;
    {
        u8cp tp = cp;
        for (i64 ci = 0; ci < ctrl_count; ci++) {
            i64 dl = BSDofftin(tp);
            tp += 8;
            i64 el = BSDofftin(tp);
            tp += 8;
            tp += 8;  // skip seek
            test(dl >= 0 && el >= 0, BSDcorrupt);
            total_diff += dl;
            total_extra += el;
        }
    }

    test(patchlen >= 24 + ctrl_bytes + total_diff + total_extra, BSDcorrupt);

    u8cp ep = dp + total_diff;
    i64 oldpos = 0, newpos = 0;

    for (i64 ci = 0; ci < ctrl_count; ci++) {
        i64 diff_len = BSDofftin(cp);
        cp += 8;
        i64 extra_len = BSDofftin(cp);
        cp += 8;
        i64 seek_off = BSDofftin(cp);
        cp += 8;

        test(newpos + diff_len <= neusize, BSDcorrupt);

        // Read diff bytes and add old data
        memcpy(nb + newpos, dp, diff_len);
        for (i64 i = 0; i < diff_len; i++) {
            if (oldpos + i >= 0 && oldpos + i < oldsize)
                nb[newpos + i] += ob[oldpos + i];
        }
        dp += diff_len;
        newpos += diff_len;
        oldpos += diff_len;

        test(newpos + extra_len <= neusize, BSDcorrupt);

        // Copy extra bytes
        memcpy(nb + newpos, ep, extra_len);
        ep += extra_len;
        newpos += extra_len;
        oldpos += seek_off;
    }

    // Advance output slice past written data
    neu[0] += neusize;

    done;
}
