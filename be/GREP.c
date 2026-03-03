#include "BE.h"

#include <string.h>

#include "abc/PRO.h"

// Check if char is a RON64 character (alphanumeric + _ ~)
#define BETriChar(c) (RON64_REV[(u8)(c)] != 0xff)

// Compute 2-char RON64 hashlet from path (12 bits = 4096 buckets)
ok64 BEHashlet(u8s into, u8cs path) {
    sane($ok(into) && $len(into) >= 2 && $ok(path));
    u64 h = RAPHash(path);
    u16 bucket = (u16)(h & 0xFFF);  // 12 bits
    into[0][0] = RON64_CHARS[bucket & 63];
    into[0][1] = RON64_CHARS[(bucket >> 6) & 63];
    into[0] += 2;
    done;
}

// Extract trigrams from BASON string leaves, call cb for each unique
ok64 BETriExtract(u8csc bason, BETriCBf cb, voidp arg) {
    sane($ok(bason) && cb != NULL);
    if ($empty(bason)) done;
    aBpad(u64, stk, 64);
    call(BASONOpen, stk, bason);
    int depth = 0;
    for (;;) {
        u8 type = 0;
        u8cs key = {};
        u8cs val = {};
        ok64 o = BASONDrain(stk, bason, &type, key, val);
        if (o != OK) {
            if (depth <= 0) break;
            call(BASONOuto, stk);
            depth--;
            continue;
        }
        u8 raw = type & ~0x20;
        if (raw == 'S' && $len(val) >= 3) {
            u8cp p = val[0];
            u8cp end = val[1] - 2;
            while (p <= end) {
                if (BETriChar(p[0]) && BETriChar(p[1]) && BETriChar(p[2])) {
                    u8cs tri = {p, p + 3};
                    call(cb, arg, tri);
                }
                p++;
            }
        } else if (raw == 'A' || raw == 'O') {
            call(BASONInto, stk, bason, val);
            depth++;
        }
    }
    done;
}

// ---- Grep: trigram-accelerated search ----

// Verify substring match in a file, call back per matching line
static ok64 BEGrepVerify(BEp be, u8cs frel, u8cs query,
                          BEGrepCBf cb, voidp arg) {
    sane(be != NULL && cb != NULL);
    u8bp mbuf = be->scratch[BE_PATCH];
    u8bReset(mbuf);
    BEmeta meta = {};
    ok64 go = BEGetFileMerged(be, be->loc.path, frel, mbuf, &meta);
    if (go != OK) done;
    u8bp out = be->scratch[BE_RENDER];
    u8bReset(out);
    aBpad(u64, stk, 256);
    u8cs bdata = {mbuf[1], mbuf[2]};
    if (BASTExport(u8bIdle(out), stk, bdata) != OK) done;
    u8cs txt = {out[1], out[2]};
    size_t qlen = $len(query);
    if ($len(txt) < qlen) done;
    u8cp ls = txt[0];
    int ln = 1;
    while (ls < txt[1]) {
        u8cp le = ls;
        while (le < txt[1] && *le != '\n') le++;
        size_t ll = (size_t)(le - ls);
        if (ll >= qlen) {
            u8cp sp = ls;
            u8cp se = ls + ll - qlen + 1;
            while (sp <= se) {
                if (memcmp(sp, query[0], qlen) == 0) {
                    u8cs line = {ls, le};
                    cb(arg, frel, ln, line);
                    break;
                }
                sp++;
            }
        }
        ls = (le < txt[1]) ? le + 1 : le;
        ln++;
    }
    done;
}

// Extract RON64 trigrams from query string, returns count
static int BEGrepTrigrams(u8cs query, u8cs trigrams[256]) {
    int tric = 0;
    if ($len(query) < 3) return 0;
    u8cp p = query[0];
    u8cp end = query[1] - 2;
    while (p <= end && tric < 256) {
        if (BETriChar(p[0]) && BETriChar(p[1]) && BETriChar(p[2])) {
            trigrams[tric][0] = p;
            trigrams[tric][1] = p + 3;
            tric++;
        }
        p++;
    }
    return tric;
}

// Check if key passes branch filter
static b8 BEGrepBranchOk(u8cs branch_filter, uri *ku) {
    if (!($ok(branch_filter) && !$empty(branch_filter))) return 1;
    u8cs q = {ku->query[0], ku->query[1]};
    if ($empty(q)) return 0;
    u8cs br = {};
    if (BEQueryParse(NULL, br, q) != OK) return 0;
    return $eq(br, branch_filter);
}

// Lookup each trigram key, decode BASON hashlet objects, intersect into bitset
static ok64 BEGrepBitset(BEp be, u8 bitset[512], u8cs *trigrams, int tric) {
    sane(be != NULL && tric > 0);
    memset(bitset, 0xFF, 512);
    a_cstr(sch_tri, BE_SCHEME_TRI);
    for (int t = 0; t < tric; t++) {
        a_uri(tri_key, sch_tri, 0, be->loc.path, trigrams[t], 0);

        aBpad(u8, tbuf, 4096);
        ok64 go = ROCKGet(&be->db, tbuf, tri_key);
        if (go != OK) {
            done;  // trigram not in index -> no matches
        }

        u8 tri_bits[512];
        memset(tri_bits, 0, sizeof(tri_bits));
        u8cs tval = {tbuf[1], tbuf[2]};
        aBpad(u64, tstk, 32);
        call(BASONOpen, tstk, tval);
        u8 ttype = 0;
        u8cs tkey = {};
        u8cs tvv = {};
        // Enter the Object container
        if (BASONDrain(tstk, tval, &ttype, tkey, tvv) == OK &&
            ttype == 'O') {
            call(BASONInto, tstk, tval, tvv);
            while (BASONDrain(tstk, tval, &ttype, tkey, tvv) == OK) {
                if ($len(tkey) >= 2) {
                    u16 bucket = (u16)(RON64_REV[tkey[0][0]] |
                                       (RON64_REV[tkey[0][1]] << 6));
                    tri_bits[bucket >> 3] |= (1 << (bucket & 7));
                }
            }
            call(BASONOuto, tstk);
        }

        for (int i = 0; i < 512; i++) {
            bitset[i] &= tri_bits[i];
        }
    }
    done;
}

// Scan stat: keys with dedup, optionally filter by bitset
// bitset=NULL means full scan (no trigram filtering)
static ok64 BEGrepScan(BEp be, u8cs prefix, size_t path_off, u8cs query,
                        u8cs branch_filter, u8 *bitset,
                        BEGrepCBf cb, voidp arg) {
    sane(be != NULL && cb != NULL);
    a_pad(u8, prev_pad, 512);
    u8cs prev_frel = {};
    ROCKiter it = {};
    call(ROCKIterOpen, &it, &be->db);
    call(ROCKIterSeek, &it, prefix);
    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(prefix) ||
            memcmp(k[0], prefix[0], $len(prefix)) != 0)
            break;
        uri ku = {};
        if (URIutf8Drain(k, &ku) != OK || !BEGrepBranchOk(branch_filter, &ku)) {
            call(ROCKIterNext, &it);
            continue;
        }
        u8cs frel = {ku.path[0] + path_off, ku.path[1]};
        if ($empty(frel) || ($ok(prev_frel) && $eq(frel, prev_frel))) {
            call(ROCKIterNext, &it);
            continue;
        }
        // Copy relpath to stable buffer (iter keys are transient)
        u8bReset(prev_pad);
        u8sFeed(u8bIdle(prev_pad), frel);
        u8csMv(prev_frel, u8bData(prev_pad));

        if (bitset != NULL) {
            // Compute hashlet, check bitset
            u8 fpbuf[256];
            u8s fps = {fpbuf, fpbuf + sizeof(fpbuf)};
            call(u8sFeed, fps, be->loc.path);
            u8sFeed1(fps, '/');
            call(u8sFeed, fps, frel);
            u8cs full_path = {fpbuf, fps[0]};

            u8 hlbuf[4];
            u8s hls = {hlbuf, hlbuf + sizeof(hlbuf)};
            call(BEHashlet, hls, full_path);
            u16 bucket =
                (u16)(RON64_REV[hlbuf[0]] | (RON64_REV[hlbuf[1]] << 6));
            if (!(bitset[bucket >> 3] & (1 << (bucket & 7)))) {
                call(ROCKIterNext, &it);
                continue;
            }
        }

        call(BEGrepVerify, be, frel, query, cb, arg);
        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);
    done;
}

ok64 BEGrep(BEp be, uricp grep_uri, BEGrepCBf result_cb, voidp arg) {
    sane(be != NULL && grep_uri != NULL && result_cb != NULL);

    u8cs query = {grep_uri->fragment[0], grep_uri->fragment[1]};
    test($ok(query) && !$empty(query), BEBAD);

    // Optional branch filter from URI query part
    u8cs branch_filter = {grep_uri->query[0], grep_uri->query[1]};

    // Build stat: prefix for scanning
    a_cstr(sch_stat, BE_SCHEME_STAT);
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(u8sFeed, pfx, sch_stat);
    u8sFeed1(pfx, ':');
    call(u8sFeed, pfx, be->loc.path);
    u8sFeed1(pfx, '/');
    u8cs prefix = {pfxbuf, pfx[0]};
    size_t path_off = $len(be->loc.path) + 1;

    // Extract trigrams from search pattern
    u8cs trigrams[256];
    int tric = BEGrepTrigrams(query, trigrams);

    if (tric == 0) {
        // No trigrams — full scan
        call(BEGrepScan, be, prefix, path_off, query, branch_filter,
             NULL, result_cb, arg);
    } else {
        // Build bitset from trigram index, then filtered scan
        u8 bitset[512];
        call(BEGrepBitset, be, bitset, trigrams, tric);
        call(BEGrepScan, be, prefix, path_off, query, branch_filter,
             bitset, result_cb, arg);
    }
    done;
}
