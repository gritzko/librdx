#include "BIFF.h"

#include "abc/DIFF.h"
#include "abc/PRO.h"
#include "abc/RAP.h"
#include "abc/RON.h"

#define X(M, name) M##u64##name
#include "abc/DIFFx.h"
#undef X

// Skip a full element in the reader (consume without output).
static ok64 BIFFSkip(u64bp stk, u8csc data,
                     u8 type, u8cs val) {
    sane(stk != NULL);
    if (BASONPlex(type)) {
        call(BASONInto, stk, data, val);
        u8 ct; u8cs ck, cv;
        while (BASONDrain(stk, data, &ct, ck, cv) == OK) {
            call(BIFFSkip, stk, data, ct, cv);
        }
        call(BASONOuto, stk);
    }
    done;
}

// Copy a single BASON element (leaf or subtree) from reader to writer.
// The element must already be drained (type/key/val known).
static ok64 BIFFCopy(u8bp out, u64bp idx,
                     u8 type, u8cs key, u8cs val,
                     u64bp stk, u8csc data) {
    sane(out != NULL);
    if (BASONPlex(type)) {
        call(BASONFeedInto, idx, out, type, key);
        call(BASONInto, stk, data, val);
        u8 ct; u8cs ck, cv;
        while (BASONDrain(stk, data, &ct, ck, cv) == OK) {
            call(BIFFCopy, out, idx, ct, ck, cv, stk, data);
        }
        call(BASONOuto, stk);
        call(BASONFeedOuto, idx, out);
    } else {
        call(BASONFeed, idx, out, type, key, val);
    }
    done;
}

// Compare two BASON values for equality.
static b8 BIFFValEqual(u8 lt, u8cs lval, u8 rt, u8cs rval) {
    if (lt != rt) return NO;
    if ($len(lval) != $len(rval)) return NO;
    if ($len(lval) == 0) return YES;
    return 0 == memcmp(lval[0], rval[0], $len(lval));
}

// Null tombstone: type 'B' with empty value means "delete this key".
static b8 BIFFIsNull(u8 type, u8cs val) {
    return type == 'B' && $len(val) == 0;
}

// --- Merge: recursive parallel walk, right-wins ---

static ok64 BIFFMergeLevel(u8bp out, u64bp idx,
                           u64bp lstk, u8csc ldata,
                           u64bp rstk, u8csc rdata) {
    sane(out != NULL);
    u8 lt = 0, rt = 0;
    u8cs lk, lv, rk, rv;
    ok64 lo = BASONDrain(lstk, ldata, &lt, lk, lv);
    ok64 ro = BASONDrain(rstk, rdata, &rt, rk, rv);

    while (lo == OK && ro == OK) {
        int cmp = $cmp(lk, rk);
        if (cmp < 0) {
            call(BIFFCopy, out, idx, lt, lk, lv, lstk, ldata);
            lo = BASONDrain(lstk, ldata, &lt, lk, lv);
        } else if (cmp > 0) {
            if (!BIFFIsNull(rt, rv)) {
                call(BIFFCopy, out, idx, rt, rk, rv, rstk, rdata);
            } else {
                call(BIFFSkip, rstk, rdata, rt, rv);
            }
            ro = BASONDrain(rstk, rdata, &rt, rk, rv);
        } else {
            // same key
            if (BIFFIsNull(rt, rv)) {
                // null tombstone: delete
                call(BIFFSkip, lstk, ldata, lt, lv);
                call(BIFFSkip, rstk, rdata, rt, rv);
            } else if (BASONPlex(lt) && BASONPlex(rt) && lt == rt) {
                // both containers of same type: recurse
                call(BASONFeedInto, idx, out, rt, rk);
                call(BASONInto, lstk, ldata, lv);
                call(BASONInto, rstk, rdata, rv);
                call(BIFFMergeLevel, out, idx, lstk, ldata, rstk, rdata);
                call(BASONOuto, lstk);
                call(BASONOuto, rstk);
                call(BASONFeedOuto, idx, out);
            } else {
                // right wins
                call(BIFFSkip, lstk, ldata, lt, lv);
                call(BIFFCopy, out, idx, rt, rk, rv, rstk, rdata);
            }
            lo = BASONDrain(lstk, ldata, &lt, lk, lv);
            ro = BASONDrain(rstk, rdata, &rt, rk, rv);
        }
    }

    while (lo == OK) {
        call(BIFFCopy, out, idx, lt, lk, lv, lstk, ldata);
        lo = BASONDrain(lstk, ldata, &lt, lk, lv);
    }

    while (ro == OK) {
        if (!BIFFIsNull(rt, rv)) {
            call(BIFFCopy, out, idx, rt, rk, rv, rstk, rdata);
        } else {
            call(BIFFSkip, rstk, rdata, rt, rv);
        }
        ro = BASONDrain(rstk, rdata, &rt, rk, rv);
    }

    done;
}

ok64 BASONMerge(u8bp out, u64bp idx,
                u64bp lstk, u8csc ldata,
                u64bp rstk, u8csc rdata) {
    sane(out != NULL);
    call(BASONOpen, lstk, ldata);
    call(BASONOpen, rstk, rdata);
    call(BIFFMergeLevel, out, idx, lstk, ldata, rstk, rdata);
    done;
}

// --- Diff: hash + Myers + marker-driven scan ---

// Hash layout: [63:62 type][61:56 depth][55:2 hash][1:0 markers]
#define BIFF_HASH_OPEN  (1ULL << 62)
#define BIFF_HASH_CLOSE (2ULL << 62)
#define BIFF_HASH_FIRST 0ULL
#define BIFF_DEPTH_SHIFT 56
#define BIFF_DEPTH_MASK  (63ULL << BIFF_DEPTH_SHIFT)
#define BIFF_HASH_MASK   (~((3ULL << 62) | BIFF_DEPTH_MASK | 3ULL))
#define BIFF_DEPTH(d)    (((u64)(d) & 63) << BIFF_DEPTH_SHIFT)
#define BIFF_CHANGED(h) ((h) & 1)
#define BIFF_MARK_CHANGED(h) ((h) | 1)
#define BIFF_CHILD_CHANGED(h) ((h) & 2)
#define BIFF_MARK_CHILD_CHANGED(h) ((h) | 2)
#define BIFF_MAX_NESTING 32
#define BIFF_MAX_HASHES 1024

// Hash a BASON subtree into a flat u64 array.
// ptype: parent type ('A' for array children, else BRACKET mode).
// depth: nesting depth, stored in dedicated bits 56-61.
// In array mode, each element gets one FIRST hash (opaque, no recursion).
// In bracket mode, containers get OPEN+children+CLOSE, leaves get FIRST.
static ok64 BIFFHashes(u64bp hashes, u64bp stk, u8csc data,
                       u8 ptype, u32 depth) {
    sane(hashes != NULL && stk != NULL);
    u64 dbits = BIFF_DEPTH(depth);
    u8 ct; u8cs ck, cv;
    while (BASONDrain(stk, data, &ct, ck, cv) == OK) {
        if (ptype == 'A') {
            u64 h = RAPHashSeed(cv, (u64)ct);
            h = (h & BIFF_HASH_MASK) | BIFF_HASH_FIRST | dbits;
            call(u64bFeed1, hashes, h);
        } else if (BASONPlex(ct)) {
            u64 oh = RAPHashSeed(ck, (u64)ct);
            oh = (oh & BIFF_HASH_MASK) | BIFF_HASH_OPEN | dbits;
            call(u64bFeed1, hashes, oh);
            call(BASONInto, stk, data, cv);
            call(BIFFHashes, hashes, stk, data, ct, depth + 1);
            call(BASONOuto, stk);
            u64 ch = (oh & BIFF_HASH_MASK) | BIFF_HASH_CLOSE | dbits;
            call(u64bFeed1, hashes, ch);
        } else {
            u64 h = RAPHashSeed(cv, RAPHashSeed(ck, (u64)ct));
            h = (h & BIFF_HASH_MASK) | BIFF_HASH_FIRST | dbits;
            call(u64bFeed1, hashes, h);
        }
    }
    done;
}

// Mark hashes from Myers EDL: set LSB=1 on changed, propagate
// CHILD_CHANGED (bit 1) to parent OPENs via nesting stack.
static ok64 BIFFMarkHashes(u64s oh, u64s nh, e32cs edl) {
    sane($ok(oh) && $ok(nh));
    u64p ostk[BIFF_MAX_NESTING] = {};
    u64p nstk[BIFF_MAX_NESTING] = {};
    u32 od = 0, nd = 0;

    $for(e32c, ep, edl) {
        u8 kind = DIFF_OP(*ep);
        u32 len = DIFF_LEN(*ep);
        if (kind == DIFF_EQ) {
            for (u32 i = 0; i < len; i++) {
                u64 om = *oh[0] & (3ULL << 62);
                u64 nm = *nh[0] & (3ULL << 62);
                if (om == BIFF_HASH_OPEN) {
                    if (od < BIFF_MAX_NESTING) ostk[od++] = oh[0];
                } else if (om == BIFF_HASH_CLOSE) {
                    if (od > 0) od--;
                }
                if (nm == BIFF_HASH_OPEN) {
                    if (nd < BIFF_MAX_NESTING) nstk[nd++] = nh[0];
                } else if (nm == BIFF_HASH_CLOSE) {
                    if (nd > 0) nd--;
                }
                oh[0]++;
                nh[0]++;
            }
        } else if (kind == DIFF_DEL) {
            for (u32 i = 0; i < len; i++) {
                u64 m = *oh[0] & (3ULL << 62);
                *oh[0] = BIFF_MARK_CHANGED(*oh[0]);
                for (u32 j = 0; j < od; j++)
                    *ostk[j] = BIFF_MARK_CHILD_CHANGED(*ostk[j]);
                if (m == BIFF_HASH_OPEN) {
                    if (od < BIFF_MAX_NESTING) ostk[od++] = oh[0];
                } else if (m == BIFF_HASH_CLOSE) {
                    if (od > 0) od--;
                }
                oh[0]++;
            }
        } else if (kind == DIFF_INS) {
            for (u32 i = 0; i < len; i++) {
                u64 m = *nh[0] & (3ULL << 62);
                *nh[0] = BIFF_MARK_CHANGED(*nh[0]);
                for (u32 j = 0; j < nd; j++)
                    *nstk[j] = BIFF_MARK_CHILD_CHANGED(*nstk[j]);
                if (m == BIFF_HASH_OPEN) {
                    if (nd < BIFF_MAX_NESTING) nstk[nd++] = nh[0];
                } else if (m == BIFF_HASH_CLOSE) {
                    if (nd > 0) nd--;
                }
                nh[0]++;
            }
        }
    }
    done;
}

// Skip one hash entry: if OPEN, skip OPEN+children+CLOSE; else skip FIRST.
static void BIFFSkipHash(u64g h) {
    if (h[1] >= h[2]) return;
    u64 marker = *h[1] & (3ULL << 62);
    if (marker != BIFF_HASH_OPEN) {
        h[1]++;
        return;
    }
    // Skip OPEN
    h[1]++;
    i32 nesting = 1;
    while (h[1] < h[2] && nesting > 0) {
        marker = *h[1] & (3ULL << 62);
        if (marker == BIFF_HASH_OPEN) nesting++;
        else if (marker == BIFF_HASH_CLOSE) nesting--;
        h[1]++;
    }
}


// Positional diff by key comparison (no hash guidance).
// Used when array containers of same type are replaced — merge recurses
// into same-type containers, so we must produce a recursive patch.
static ok64 BIFFDiffPositional(u8bp out, u64bp idx,
                                u64bp ostk, u8csc odata,
                                u64bp nstk, u8csc ndata) {
    sane(out != NULL);
    u8 ot = 0, nt = 0;
    u8cs ok, ov, nk, nv;
    ok64 oo = BASONDrain(ostk, odata, &ot, ok, ov);
    ok64 no = BASONDrain(nstk, ndata, &nt, nk, nv);

    while (oo == OK && no == OK) {
        int cmp = $cmp(ok, nk);
        if (cmp < 0) {
            u8cs empty = {(u8cp)ok[0], (u8cp)ok[0]};
            call(BASONFeed, idx, out, 'B', ok, empty);
            call(BIFFSkip, ostk, odata, ot, ov);
            oo = BASONDrain(ostk, odata, &ot, ok, ov);
        } else if (cmp > 0) {
            call(BIFFCopy, out, idx, nt, nk, nv, nstk, ndata);
            no = BASONDrain(nstk, ndata, &nt, nk, nv);
        } else {
            if (BIFFValEqual(ot, ov, nt, nv)) {
                call(BIFFSkip, ostk, odata, ot, ov);
                call(BIFFSkip, nstk, ndata, nt, nv);
            } else if (BASONPlex(ot) && BASONPlex(nt) && ot == nt) {
                size_t before = u8bDataLen(out);
                call(BASONFeedInto, idx, out, nt, nk);
                call(BASONInto, ostk, odata, ov);
                call(BASONInto, nstk, ndata, nv);
                call(BIFFDiffPositional, out, idx,
                     ostk, odata, nstk, ndata);
                call(BASONOuto, ostk);
                call(BASONOuto, nstk);
                call(BASONFeedOuto, idx, out);
                size_t after = u8bDataLen(out);
                u8cs from2 = {out[1] + before, out[1] + after};
                u8 ct2; u8cs ck2, cv2;
                ok64 dr = TLKVDrain(from2, &ct2, ck2, cv2);
                if (dr == OK && $len(cv2) == 0) {
                    ((u8 **)out)[2] = out[1] + before;
                }
            } else {
                call(BIFFSkip, ostk, odata, ot, ov);
                call(BIFFCopy, out, idx, nt, nk, nv, nstk, ndata);
            }
            oo = BASONDrain(ostk, odata, &ot, ok, ov);
            no = BASONDrain(nstk, ndata, &nt, nk, nv);
        }
    }

    while (oo == OK) {
        u8cs empty = {(u8cp)ok[0], (u8cp)ok[0]};
        call(BASONFeed, idx, out, 'B', ok, empty);
        call(BIFFSkip, ostk, odata, ot, ov);
        oo = BASONDrain(ostk, odata, &ot, ok, ov);
    }

    while (no == OK) {
        call(BIFFCopy, out, idx, nt, nk, nv, nstk, ndata);
        no = BASONDrain(nstk, ndata, &nt, nk, nv);
    }

    done;
}

// Forward declaration for mutual recursion
static ok64 BIFFDiffScan(u8bp out, u64bp idx,
                          u64bp ostk, u8csc odata,
                          u64bp nstk, u8csc ndata,
                          u64g oh, u64g nh);

// Marker-driven parallel walk of both BASON streams + hash gauges.
// Object-mode only: arrays are handled by BIFFDiffPositional.
static ok64 BIFFDiffScan(u8bp out, u64bp idx,
                          u64bp ostk, u8csc odata,
                          u64bp nstk, u8csc ndata,
                          u64g oh, u64g nh) {
    sane(out != NULL);
    u8 ot = 0, nt = 0;
    u8cs ok, ov, nk, nv;
    ok64 oo = BASONDrain(ostk, odata, &ot, ok, ov);
    ok64 no = BASONDrain(nstk, ndata, &nt, nk, nv);

    while (oo == OK || no == OK) {
        if (oo != OK) goto trailing_new;
        if (no != OK) goto trailing_old;

        b8 oc = (oh[1] < oh[2]) ? BIFF_CHANGED(*oh[1]) : NO;
        b8 nc = (nh[1] < nh[2]) ? BIFF_CHANGED(*nh[1]) : NO;

        if (!oc && !nc) {
            // EQ: check CHILD_CHANGED for containers
            b8 occ = oh[1] < oh[2] ? BIFF_CHILD_CHANGED(*oh[1]) : NO;
            b8 ncc = nh[1] < nh[2] ? BIFF_CHILD_CHANGED(*nh[1]) : NO;
            if ((occ || ncc) && BASONPlex(ot) && BASONPlex(nt) && ot == nt) {
                // Container with changed children: descend
                size_t before = u8bDataLen(out);
                call(BASONFeedInto, idx, out, nt, nk);
                call(BASONInto, ostk, odata, ov);
                call(BASONInto, nstk, ndata, nv);
                // Positional diff for children (hash markers may
                // be misaligned due to cross-container matching)
                BIFFSkipHash(oh);
                BIFFSkipHash(nh);
                call(BIFFDiffPositional, out, idx,
                     ostk, odata, nstk, ndata);
                call(BASONOuto, ostk);
                call(BASONOuto, nstk);
                call(BASONFeedOuto, idx, out);
                // Strip empty container
                size_t after = u8bDataLen(out);
                u8cs from2 = {out[1] + before, out[1] + after};
                u8 ct2; u8cs ck2, cv2;
                ok64 dr = TLKVDrain(from2, &ct2, ck2, cv2);
                if (dr == OK && $len(cv2) == 0) {
                    ((u8 **)out)[2] = out[1] + before;
                }
            } else {
                // No child changes: skip both
                call(BIFFSkip, ostk, odata, ot, ov);
                call(BIFFSkip, nstk, ndata, nt, nv);
                BIFFSkipHash(oh);
                BIFFSkipHash(nh);
            }
            oo = BASONDrain(ostk, odata, &ot, ok, ov);
            no = BASONDrain(nstk, ndata, &nt, nk, nv);
        } else if (oc && !nc) {
            // DEL: tombstone old key
            u8cs empty = {(u8cp)ok[0], (u8cp)ok[0]};
            call(BASONFeed, idx, out, 'B', ok, empty);
            call(BIFFSkip, ostk, odata, ot, ov);
            BIFFSkipHash(oh);
            oo = BASONDrain(ostk, odata, &ot, ok, ov);
        } else if (!oc && nc) {
            // INS: emit with original new key
            call(BIFFCopy, out, idx, nt, nk, nv, nstk, ndata);
            BIFFSkipHash(nh);
            no = BASONDrain(nstk, ndata, &nt, nk, nv);
        } else if (oc && nc) {
            // Both changed: interleave by key order
            int cmp = $cmp(ok, nk);
            if (cmp == 0) {
                if (BASONPlex(ot) && BASONPlex(nt) && ot == nt) {
                    // Same key, same container type: recurse
                    size_t before = u8bDataLen(out);
                    call(BASONFeedInto, idx, out, nt, nk);
                    call(BASONInto, ostk, odata, ov);
                    call(BASONInto, nstk, ndata, nv);
                    call(BIFFDiffPositional, out, idx,
                         ostk, odata, nstk, ndata);
                    call(BASONOuto, ostk);
                    call(BASONOuto, nstk);
                    call(BASONFeedOuto, idx, out);
                    size_t after = u8bDataLen(out);
                    u8cs from2 = {out[1] + before, out[1] + after};
                    u8 ct2; u8cs ck2, cv2;
                    ok64 dr = TLKVDrain(from2, &ct2, ck2, cv2);
                    if (dr == OK && $len(cv2) == 0) {
                        ((u8 **)out)[2] = out[1] + before;
                    }
                } else {
                    // Same key, different type or leaf: right-wins
                    call(BIFFSkip, ostk, odata, ot, ov);
                    call(BIFFCopy, out, idx, nt, nk, nv, nstk, ndata);
                }
                BIFFSkipHash(oh);
                BIFFSkipHash(nh);
                oo = BASONDrain(ostk, odata, &ot, ok, ov);
                no = BASONDrain(nstk, ndata, &nt, nk, nv);
            } else if (cmp < 0) {
                // Old key smaller: tombstone old, keep new for next iter
                u8cs empty = {(u8cp)ok[0], (u8cp)ok[0]};
                call(BASONFeed, idx, out, 'B', ok, empty);
                call(BIFFSkip, ostk, odata, ot, ov);
                BIFFSkipHash(oh);
                oo = BASONDrain(ostk, odata, &ot, ok, ov);
            } else {
                // New key smaller: emit new, keep old for next iter
                call(BIFFCopy, out, idx, nt, nk, nv, nstk, ndata);
                BIFFSkipHash(nh);
                no = BASONDrain(nstk, ndata, &nt, nk, nv);
            }
        } else { continue; }
        continue;
        trailing_old: {
            u8cs empty = {(u8cp)ok[0], (u8cp)ok[0]};
            call(BASONFeed, idx, out, 'B', ok, empty);
            call(BIFFSkip, ostk, odata, ot, ov);
            BIFFSkipHash(oh);
            oo = BASONDrain(ostk, odata, &ot, ok, ov);
        } continue;
        trailing_new: {
            call(BIFFCopy, out, idx, nt, nk, nv, nstk, ndata);
            BIFFSkipHash(nh);
            no = BASONDrain(nstk, ndata, &nt, nk, nv);
        }
    }
    done;
}

ok64 BASONDiff(u8bp out, u64bp idx,
               u64bp ostk, u8csc odata,
               u64bp nstk, u8csc ndata) {
    sane(out != NULL);

    // Step 1: Open both streams
    call(BASONOpen, ostk, odata);
    call(BASONOpen, nstk, ndata);

    // Step 2: Hash both trees
    u64 _oh[BIFF_MAX_HASHES], _nh[BIFF_MAX_HASHES];
    u64b ohb = {_oh, _oh, _oh, _oh + BIFF_MAX_HASHES};
    u64b nhb = {_nh, _nh, _nh, _nh + BIFF_MAX_HASHES};

    // Separate stacks for hash pass
    u64 _hs1[64], _hs2[64];
    u64b hs1 = {_hs1, _hs1, _hs1, _hs1 + 64};
    u64b hs2 = {_hs2, _hs2, _hs2, _hs2 + 64};
    call(BASONOpen, hs1, odata);
    call(BASONOpen, hs2, ndata);
    call(BIFFHashes, ohb, hs1, odata, 0, 0);
    call(BIFFHashes, nhb, hs2, ndata, 0, 0);

    u64 olen = u64bDataLen(ohb);
    u64 nlen = u64bDataLen(nhb);

    // Step 3: Clear bits 0-1
    for (u64 i = 0; i < olen; i++) _oh[i] &= ~3ULL;
    for (u64 i = 0; i < nlen; i++) _nh[i] &= ~3ULL;

    // Step 4: Myers diff
    u64cs ohs = {(u64cp)_oh, (u64cp)(_oh + olen)};
    u64cs nhs = {(u64cp)_nh, (u64cp)(_nh + nlen)};
    u64 wsize = DIFFWorkSize(olen, nlen);
    u64 emax = DIFFEdlMaxEntries(olen, nlen);
    i32 workbuf[8192];
    e32 edlbuf[2048];
    if (wsize > 8192 || emax > 2048) done;
    i32s work = {workbuf, workbuf + 8192};
    e32g edl = {edlbuf, edlbuf + 2048, edlbuf};
    ok64 dro = DIFFu64s(edl, work, ohs, nhs);
    if (dro != OK) done;

    // Step 5: Early exit if all-EQ
    i32 edllen = (i32)(edl[0] - edl[2]);
    if (edllen == 1 && DIFF_OP(edl[2][0]) == DIFF_EQ) done;

    // Step 6: Mark hashes
    e32cs edlcs = {edl[2], edl[0]};
    u64s ohm = {_oh, _oh + olen};
    u64s nhm = {_nh, _nh + nlen};
    call(BIFFMarkHashes, ohm, nhm, edlcs);

    // Step 7: Scan with hash gauges
    u64g ohg = {_oh, _oh, _oh + olen};
    u64g nhg = {_nh, _nh, _nh + nlen};
    call(BIFFDiffScan, out, idx, ostk, odata, nstk, ndata,
         ohg, nhg);

    done;
}
