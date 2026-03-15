#include "BIFF.h"

#include "ast/HILI.h"
#include "abc/DIFF.h"
#include "abc/PRO.h"
#include "abc/RAP.h"
#include "abc/RON.h"
#include "PASS.h"

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

// --- N-way merge: heap-based ---

#define BIFFN_MAX 256
#define BIFFN_STK 128

typedef struct {
    u64 *stk[4];
    u8c *data[2];
    u8   type;
    u8c *key[2];
    u8c *val[2];
} BIFFcur;

static _Thread_local BIFFcur *_biff_curs;

static b8 BIFFcurZ(u32cp a, u32cp b) {
    BIFFcur *cs = _biff_curs;
    int cmp = $cmp(cs[*a].key, cs[*b].key);
    if (cmp != 0) return cmp < 0;
    return *a < *b;
}

#define X(M, name) M##u32##name
#include "abc/HEAPx.h"
#undef X

// PopZ returns MISS when popping the last element (sDown on empty).
// The value is still extracted correctly; treat MISS as OK.
static ok64 BIFFHeapPop(u32p v, u32bp buf) {
    ok64 o = HEAPu32PopZ(v, buf, BIFFcurZ);
    if (o == MISS) return OK;
    return o;
}

static ok64 u32bBury(u32bp buf) {
    sane(buf != NULL);
    test(u32bIdleLen(buf) >= 1, BNOROOM);
    u32 dl = (u32)u32bDataLen(buf);
    call(u32bFeed1, buf, dl);
    ((u32 **)buf)[1] = buf[2];
    done;
}

static ok64 u32bDigup(u32bp buf) {
    sane(buf != NULL);
    test(u32bPastLen(buf) >= 1, BNODATA);
    u32 dl = *(buf[1] - 1);
    test(u32bPastLen(buf) >= (size_t)dl + 1, BNODATA);
    ((u32 **)buf)[1] -= dl + 1;
    ((u32 **)buf)[2] = buf[1] + dl;
    done;
}

static ok64 BIFFMergeLevelN(u8bp out, u64bp idx, u32bp heap) {
    sane(out != NULL && heap != NULL);
    BIFFcur *cs = _biff_curs;

    while (u32bDataLen(heap) > 0) {
        // Pop minimum and all same-key entries
        u32 popped[BIFFN_MAX];
        u32 npop = 0;
        u32 first;
        call(BIFFHeapPop, &first, heap);
        popped[npop++] = first;

        while (u32bDataLen(heap) > 0) {
            u32cp top = heap[1];
            if ($cmp(cs[first].key, cs[*top].key) != 0) break;
            u32 next;
            call(BIFFHeapPop, &next, heap);
            popped[npop++] = next;
        }

        // Winner = highest index (last input wins)
        u32 wi = popped[npop - 1];

        if (npop == 1) {
            if (!BIFFIsNull(cs[wi].type, cs[wi].val)) {
                call(BIFFCopy, out, idx, cs[wi].type, cs[wi].key,
                     cs[wi].val, cs[wi].stk, cs[wi].data);
            } else {
                call(BIFFSkip, cs[wi].stk, cs[wi].data,
                     cs[wi].type, cs[wi].val);
            }
        } else if (BIFFIsNull(cs[wi].type, cs[wi].val)) {
            for (u32 j = 0; j < npop; j++) {
                u32 i = popped[j];
                call(BIFFSkip, cs[i].stk, cs[i].data,
                     cs[i].type, cs[i].val);
            }
        } else {
            b8 recurse = BASONPlex(cs[wi].type) ? YES : NO;
            u8 wtype = cs[wi].type;
            if (recurse) {
                for (u32 j = 0; j < npop; j++) {
                    if (cs[popped[j]].type != wtype) {
                        recurse = NO;
                        break;
                    }
                }
            }

            if (recurse) {
                call(BASONFeedInto, idx, out, wtype, cs[wi].key);
                for (u32 j = 0; j < npop; j++) {
                    u32 i = popped[j];
                    call(BASONInto, cs[i].stk, cs[i].data, cs[i].val);
                }
                call(u32bBury, heap);
                for (u32 j = 0; j < npop; j++) {
                    u32 i = popped[j];
                    ok64 o = BASONDrain(cs[i].stk, cs[i].data,
                                        &cs[i].type, cs[i].key,
                                        cs[i].val);
                    if (o == OK) {
                        call(HEAPu32Push1Z, heap, i, BIFFcurZ);
                    }
                }
                call(BIFFMergeLevelN, out, idx, heap);
                for (u32 j = 0; j < npop; j++) {
                    call(BASONOuto, cs[popped[j]].stk);
                }
                call(u32bDigup, heap);
                call(BASONFeedOuto, idx, out);
            } else {
                for (u32 j = 0; j < npop; j++) {
                    u32 i = popped[j];
                    if (i == wi) {
                        call(BIFFCopy, out, idx, cs[i].type, cs[i].key,
                             cs[i].val, cs[i].stk, cs[i].data);
                    } else {
                        call(BIFFSkip, cs[i].stk, cs[i].data,
                             cs[i].type, cs[i].val);
                    }
                }
            }
        }

        for (u32 j = 0; j < npop; j++) {
            u32 i = popped[j];
            ok64 o = BASONDrain(cs[i].stk, cs[i].data,
                                &cs[i].type, cs[i].key, cs[i].val);
            if (o == OK) {
                call(HEAPu32Push1Z, heap, i, BIFFcurZ);
            }
        }
    }
    done;
}

ok64 BASONMergeN(u8bp out, u64bp idx, u8css inputs) {
    sane(out != NULL && inputs != NULL);
    u32 n = (u32)$len(inputs);
    if (n == 0) done;
    if (n > BIFFN_MAX) fail(BASONBAD);

    BIFFcur curs[BIFFN_MAX];
    u64 _stks[BIFFN_MAX * BIFFN_STK];
    _biff_curs = curs;

    u32 _heap[4096];
    u32b heap = {_heap, _heap, _heap, _heap + 4096};

    for (u32 i = 0; i < n; i++) {
        u64 *base = _stks + i * BIFFN_STK;
        curs[i].stk[0] = base;
        curs[i].stk[1] = base;
        curs[i].stk[2] = base;
        curs[i].stk[3] = base + BIFFN_STK;
        curs[i].data[0] = inputs[0][i][0];
        curs[i].data[1] = inputs[0][i][1];

        if (curs[i].data[0] >= curs[i].data[1]) continue;

        call(BASONOpen, curs[i].stk, curs[i].data);
        ok64 o = BASONDrain(curs[i].stk, curs[i].data,
                            &curs[i].type, curs[i].key, curs[i].val);
        if (o == OK) {
            call(HEAPu32Push1Z, heap, i, BIFFcurZ);
        }
    }

    call(BIFFMergeLevelN, out, idx, heap);

    _biff_curs = NULL;
    done;
}

ok64 BASONMergeY(u8s into, u8css inputs) {
    sane(u8sOK(into));
    u8b out = {into[0], into[0], into[0], into[1]};
    call(BASONMergeN, out, NULL, inputs);
    into[0] = out[2];
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
#define BIFF_MAX_NESTING 64

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
        if (ptype != 'O' && ptype != 0) {
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


#define BIFF_MAX_KWIDTH 20

// Forward declaration for BIFFDiffArray → BIFFDiffObject recursion
static ok64 BIFFDiffObject(u8bp out, u64bp idx,
                            u64bp ostk, u8csc odata,
                            u64bp nstk, u8csc ndata);

// Hash-guided array diff. Called after BASONInto on both sides.
// Hash gauges oh/nh are positioned at first child's FIRST hash
// (caller has already skipped the OPEN hash).
// Walks hash marks in lockstep with BASON element reads.
static ok64 BIFFDiffArray(u8bp out, u64bp idx,
                           u64bp ostk, u8csc odata,
                           u64bp nstk, u8csc ndata,
                           u64g oh, u64g nh) {
    sane(out != NULL);
    u8 ot = 0, nt = 0;
    u8cs ok_, ov, nk, nv;
    ok64 oo = BASONDrain(ostk, odata, &ot, ok_, ov);
    ok64 no = BASONDrain(nstk, ndata, &nt, nk, nv);

    // Track last surviving old key for BASONFindMid
    a_pad(u8, lk_buf, BIFF_MAX_KWIDTH);
    u8cs left_key = {NULL, NULL};
    a_pad(u8, mk_buf, BIFF_MAX_KWIDTH);

    while (oo == OK && no == OK) {
        b8 oc = (oh[1] < oh[2]) ? BIFF_CHANGED(*oh[1]) : NO;
        b8 nc = (nh[1] < nh[2]) ? BIFF_CHANGED(*nh[1]) : NO;

        if (!oc && !nc) {
            // EQ: skip both, update left_key = old key
            size_t kl = $len(ok_);
            if (kl > BIFF_MAX_KWIDTH) kl = BIFF_MAX_KWIDTH;
            memcpy(_lk_buf, ok_[0], kl);
            left_key[0] = (u8cp)_lk_buf;
            left_key[1] = (u8cp)_lk_buf + kl;
            call(BIFFSkip, ostk, odata, ot, ov);
            call(BIFFSkip, nstk, ndata, nt, nv);
            oh[1]++;
            nh[1]++;
            oo = BASONDrain(ostk, odata, &ot, ok_, ov);
            no = BASONDrain(nstk, ndata, &nt, nk, nv);
        } else if (oc && !nc) {
            // DEL: tombstone with old key
            u8cs empty = {(u8cp)ok_[0], (u8cp)ok_[0]};
            call(BASONFeed, idx, out, 'B', ok_, empty);
            call(BIFFSkip, ostk, odata, ot, ov);
            // Update left_key to deleted key so subsequent INS
            // splice keys sort AFTER all deletions (red before green)
            size_t kl = $len(ok_);
            if (kl > BIFF_MAX_KWIDTH) kl = BIFF_MAX_KWIDTH;
            memcpy(_lk_buf, ok_[0], kl);
            left_key[0] = (u8cp)_lk_buf;
            left_key[1] = (u8cp)_lk_buf + kl;
            oh[1]++;
            oo = BASONDrain(ostk, odata, &ot, ok_, ov);
        } else if (!oc && nc) {
            // INS: generate key after left_key, must sort before ok_
            u8s mid_into = {_mk_buf, _mk_buf + BIFF_MAX_KWIDTH};
            call(BASONFeedInfInc, mid_into, left_key);
            u8cs mid_key = {(u8cp)_mk_buf, (u8cp)mid_into[0]};
            if ($cmp(mid_key, ok_) >= 0) {
                // Overshot right bound: use BASONFindMid
                mid_into[0] = _mk_buf;
                call(BASONFindMid, mid_into, left_key, ok_, 1, 1, 0);
                mid_key[0] = (u8cp)_mk_buf;
                mid_key[1] = (u8cp)mid_into[0];
            }
            call(BIFFCopy, out, idx, nt, mid_key, nv, nstk, ndata);
            // Update left_key to mid_key
            size_t kl = $len(mid_key);
            memcpy(_lk_buf, _mk_buf, kl);
            left_key[0] = (u8cp)_lk_buf;
            left_key[1] = (u8cp)_lk_buf + kl;
            nh[1]++;
            no = BASONDrain(nstk, ndata, &nt, nk, nv);
        } else {
            // REPLACE: both marked — use old key
            if (BASONPlex(ot) && BASONPlex(nt) && ot == nt) {
                // Same-type containers: recurse via key-sorted diff
                // (both sides share same positional key scheme)
                size_t before = u8bDataLen(out);
                call(BASONFeedInto, idx, out, nt, ok_);
                call(BASONInto, ostk, odata, ov);
                call(BASONInto, nstk, ndata, nv);
                call(BIFFDiffObject, out, idx,
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
                // Different types or leaf: emit new with old key
                call(BIFFSkip, ostk, odata, ot, ov);
                call(BIFFCopy, out, idx, nt, ok_, nv, nstk, ndata);
            }
            // Update left_key = old key
            size_t kl = $len(ok_);
            if (kl > BIFF_MAX_KWIDTH) kl = BIFF_MAX_KWIDTH;
            memcpy(_lk_buf, ok_[0], kl);
            left_key[0] = (u8cp)_lk_buf;
            left_key[1] = (u8cp)_lk_buf + kl;
            oh[1]++;
            nh[1]++;
            oo = BASONDrain(ostk, odata, &ot, ok_, ov);
            no = BASONDrain(nstk, ndata, &nt, nk, nv);
        }
    }

    // Trailing old: all tombstones
    while (oo == OK) {
        u8cs empty = {(u8cp)ok_[0], (u8cp)ok_[0]};
        call(BASONFeed, idx, out, 'B', ok_, empty);
        call(BIFFSkip, ostk, odata, ot, ov);
        oh[1]++;
        oo = BASONDrain(ostk, odata, &ot, ok_, ov);
    }

    // Trailing new: all INS via sequential increment
    while (no == OK) {
        u8s mid_into = {_mk_buf, _mk_buf + BIFF_MAX_KWIDTH};
        call(BASONFeedInfInc, mid_into, left_key);
        u8cs mid_key = {(u8cp)_mk_buf, (u8cp)mid_into[0]};
        call(BIFFCopy, out, idx, nt, mid_key, nv, nstk, ndata);
        // Update left_key
        size_t kl = $len(mid_key);
        memcpy(_lk_buf, _mk_buf, kl);
        left_key[0] = (u8cp)_lk_buf;
        left_key[1] = (u8cp)_lk_buf + kl;
        nh[1]++;
        no = BASONDrain(nstk, ndata, &nt, nk, nv);
    }

    done;
}

// Positional diff by key comparison (no hash guidance).
// Used when array containers of same type are replaced — merge recurses
// into same-type containers, so we must produce a recursive patch.
static ok64 BIFFDiffObject(u8bp out, u64bp idx,
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
                call(BIFFDiffObject, out, idx,
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
// Objects use BIFFDiffObject, arrays use BIFFDiffArray.
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
                if (ot == 'A') {
                    oh[1]++;  // skip OPEN hash
                    nh[1]++;
                    call(BIFFDiffArray, out, idx,
                         ostk, odata, nstk, ndata, oh, nh);
                    oh[1]++;  // skip CLOSE hash
                    nh[1]++;
                } else {
                    BIFFSkipHash(oh);
                    BIFFSkipHash(nh);
                    call(BIFFDiffObject, out, idx,
                         ostk, odata, nstk, ndata);
                }
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
                    if (ot == 'A') {
                        oh[1]++;  // skip OPEN hash
                        nh[1]++;
                        call(BIFFDiffArray, out, idx,
                             ostk, odata, nstk, ndata, oh, nh);
                        oh[1]++;  // skip CLOSE hash
                        nh[1]++;
                    } else {
                        call(BIFFDiffObject, out, idx,
                             ostk, odata, nstk, ndata);
                        BIFFSkipHash(oh);
                        BIFFSkipHash(nh);
                    }
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
                    BIFFSkipHash(oh);
                    BIFFSkipHash(nh);
                }
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

// Core diff logic using pre-allocated memory.
// mem layout: [0,hcap) old hashes, [hcap,2*hcap) new hashes,
// [2*hcap,...) Myers work (i32) and edl (e32).
static ok64 BIFFDiffCore(u8bp out, u64bp idx,
                          u64bp ostk, u8csc odata,
                          u64bp nstk, u8csc ndata,
                          u64p mem, size_t hcap) {
    sane(out != NULL && mem != NULL);

    // Step 1: Open both streams
    call(BASONOpen, ostk, odata);
    call(BASONOpen, nstk, ndata);

    // Step 2: Hash both trees
    u64p oh = mem;
    u64p nh = mem + hcap;
    u64b ohb = {oh, oh, oh, oh + hcap};
    u64b nhb = {nh, nh, nh, nh + hcap};

    u64 _hs1[64], _hs2[64];
    u64b hs1 = {_hs1, _hs1, _hs1, _hs1 + 64};
    u64b hs2 = {_hs2, _hs2, _hs2, _hs2 + 64};
    call(BASONOpen, hs1, odata);
    call(BASONOpen, hs2, ndata);
    // Hash overflow → too many elements, skip diff (empty patch)
    if (BIFFHashes(ohb, hs1, odata, 0, 0) != OK) done;
    if (BIFFHashes(nhb, hs2, ndata, 0, 0) != OK) done;

    u64 olen = u64bDataLen(ohb);
    u64 nlen = u64bDataLen(nhb);

    // Step 3: Clear bits 0-1
    for (u64 i = 0; i < olen; i++) oh[i] &= ~3ULL;
    for (u64 i = 0; i < nlen; i++) nh[i] &= ~3ULL;

    // Step 4: Myers diff — work/edl carved from mem after hashes
    u64cs ohs = {(u64cp)oh, (u64cp)(oh + olen)};
    u64cs nhs = {(u64cp)nh, (u64cp)(nh + nlen)};
    u64 wsize = DIFFWorkSize(olen, nlen);
    u64 emax = DIFFEdlMaxEntries(olen, nlen);

    size_t work_u64 = 4 * hcap + 1;
    i32p workp = (i32p)(mem + 2 * hcap);
    size_t work_cap = work_u64 * 2;
    e32 *edlp = (e32 *)(workp + work_cap);
    size_t edl_cap = hcap * 2;

    if (wsize > work_cap || emax > edl_cap) done;
    i32s work = {workp, workp + work_cap};
    e32g edl = {edlp, edlp + edl_cap, edlp};
    ok64 dro = DIFFu64s(edl, work, ohs, nhs);
    if (dro != OK) done;

    // Step 5: Early exit if all-EQ
    i32 edllen = (i32)(edl[0] - edl[2]);
    if (edllen == 1 && DIFF_OP(edl[2][0]) == DIFF_EQ) done;

    // Step 6: Mark hashes
    e32cs edlcs = {edl[2], edl[0]};
    u64s ohm = {oh, oh + olen};
    u64s nhm = {nh, nh + nlen};
    call(BIFFMarkHashes, ohm, nhm, edlcs);

    // Step 7: Scan with hash gauges
    u64g ohg = {oh, oh, oh + olen};
    u64g nhg = {nh, nh, nh + nlen};
    call(BIFFDiffScan, out, idx, ostk, odata, nstk, ndata,
         ohg, nhg);

    done;
}

ok64 BASONDiff(u8bp out, u64bp idx,
               u64bp ostk, u8csc odata,
               u64bp nstk, u8csc ndata,
               u64bp hbuf) {
    sane(out != NULL);

    // Estimate hash capacity from input sizes
    size_t hest = ($len(odata) + $len(ndata)) / 2;
    if (hest < 256) hest = 256;
    // Need: 2*hest (hashes) + 4*hest+1 (work) + hest (edl) = 7*hest+1
    size_t total = 7 * hest + 16;

    u64p mem = NULL;
    b8 need_free = NO;
    if (hbuf && u64bIdleLen(hbuf) >= total) {
        mem = hbuf[2];
    } else {
        mem = (u64p)malloc(total * sizeof(u64));
        if (!mem) done;
        need_free = YES;
    }

    ok64 o = BIFFDiffCore(out, idx, ostk, odata, nstk, ndata,
                           mem, hest);

    if (need_free) free(mem);
    return o;
}

// --- Diff rendering: colored leaf-value output ---

// Emit all leaf values of a BASON subtree (no color).
static ok64 BIFFRenderLeaves(u8s out, u64bp stk, u8csc data,
                              u8 type, u8cs val) {
    sane(u8sOK(out));
    if (BASONPlex(type)) {
        call(BASONInto, stk, data, val);
        u8 ct; u8cs ck, cv;
        while (BASONDrain(stk, data, &ct, ck, cv) == OK) {
            call(BIFFRenderLeaves, out, stk, data, ct, cv);
        }
        call(BASONOuto, stk);
    } else if (type != 'B' || $len(val) > 0) {
        call(u8sFeed, out, val);
    }
    done;
}

// Parallel walk of old + patch at one level, emit colored text.
static ok64 BIFFRenderLevel(u8s out,
                             u64bp ostk, u8csc odata,
                             u64bp pstk, u8csc pdata) {
    sane(u8sOK(out));
    u8cs DEL = $u8str("\033[9;31m");
    u8cs ADD = $u8str("\033[32m");
    u8cs RST = $u8str("\033[0m");

    u8 ot = 0, pt = 0;
    u8cs ok, ov, pk, pv;
    ok64 oo = BASONDrain(ostk, odata, &ot, ok, ov);
    ok64 po = BASONDrain(pstk, pdata, &pt, pk, pv);

    while (oo == OK && po == OK) {
        int cmp = $cmp(ok, pk);
        if (cmp < 0) {
            call(BIFFRenderLeaves, out, ostk, odata, ot, ov);
            oo = BASONDrain(ostk, odata, &ot, ok, ov);
        } else if (cmp > 0) {
            call(u8sFeed, out, ADD);
            call(BIFFRenderLeaves, out, pstk, pdata, pt, pv);
            call(u8sFeed, out, RST);
            po = BASONDrain(pstk, pdata, &pt, pk, pv);
        } else {
            if (BIFFIsNull(pt, pv)) {
                call(u8sFeed, out, DEL);
                call(BIFFRenderLeaves, out, ostk, odata, ot, ov);
                call(u8sFeed, out, RST);
            } else if (BASONPlex(ot) && BASONPlex(pt) && ot == pt) {
                call(BASONInto, ostk, odata, ov);
                call(BASONInto, pstk, pdata, pv);
                call(BIFFRenderLevel, out, ostk, odata, pstk, pdata);
                call(BASONOuto, ostk);
                call(BASONOuto, pstk);
            } else {
                call(u8sFeed, out, DEL);
                call(BIFFRenderLeaves, out, ostk, odata, ot, ov);
                call(u8sFeed, out, RST);
                call(u8sFeed, out, ADD);
                call(BIFFRenderLeaves, out, pstk, pdata, pt, pv);
                call(u8sFeed, out, RST);
            }
            oo = BASONDrain(ostk, odata, &ot, ok, ov);
            po = BASONDrain(pstk, pdata, &pt, pk, pv);
        }
    }

    while (oo == OK) {
        call(BIFFRenderLeaves, out, ostk, odata, ot, ov);
        oo = BASONDrain(ostk, odata, &ot, ok, ov);
    }

    while (po == OK) {
        call(u8sFeed, out, ADD);
        call(BIFFRenderLeaves, out, pstk, pdata, pt, pv);
        call(u8sFeed, out, RST);
        po = BASONDrain(pstk, pdata, &pt, pk, pv);
    }

    done;
}

ok64 BASONDiffRender(u8s out,
                     u64bp ostk, u8csc odata,
                     u64bp pstk, u8csc pdata) {
    sane(u8sOK(out));
    call(BASONOpen, ostk, odata);
    if ($len(pdata) == 0) {
        u8 type; u8cs key, val;
        while (BASONDrain(ostk, odata, &type, key, val) == OK) {
            call(BIFFRenderLeaves, out, ostk, odata, type, val);
        }
        done;
    }
    call(BASONOpen, pstk, pdata);
    call(BIFFRenderLevel, out, ostk, odata, pstk, pdata);
    done;
}

// --- PASS-based colored diff ---

// Render all leaf values of a BASON element (self-contained stack).
// Unlike BIFFRenderLeaves, this allocates its own stack because
// PASS has already advanced past the element.
static ok64 BIFFPrintLeaves(u8s out, u8 type, u8cs val) {
    sane(u8sOK(out));
    if (BASONPlex(type)) {
        aBpad(u64, stk, 64);
        call(BASONOpen, stk, val);
        u8 ct; u8cs ck, cv;
        while (BASONDrain(stk, val, &ct, ck, cv) == OK) {
            call(BIFFPrintLeaves, out, ct, cv);
        }
    } else if (type != 'B' || $len(val) > 0) {
        call(u8sFeed, out, val);
    }
    done;
}

// Emit leaves with syntax highlighting (for equal/context lines in diff).
static ok64 BIFFPrintStyledLeaves(u8s out, u8 type, u8cs val,
                                   u8 *cstk, int depth) {
    sane(u8sOK(out));
    if (BASONPlex(type)) {
        int d = depth < HILI_MAXDEPTH ? depth : HILI_MAXDEPTH - 1;
        cstk[d] = type;
        HILIContainer(out, type);
        aBpad(u64, stk, 64);
        call(BASONOpen, stk, val);
        u8 ct; u8cs ck, cv;
        while (BASONDrain(stk, val, &ct, ck, cv) == OK) {
            call(BIFFPrintStyledLeaves, out, ct, cv, cstk, depth + 1);
        }
        HILIRestore(out, cstk, depth);
    } else if (type != 'B' || $len(val) > 0) {
        b8 styled = HILILeaf(out, type);
        call(u8sFeed, out, val);
        if (styled) HILIRestore(out, cstk, depth);
    }
    done;
}

// Forward declaration
static ok64 BIFFPrintLevel(u8s out, u8s del, u8s add,
                            u8csc odata, u8csc ndata);

// Emit styled text with background overlay.
// Text may contain syntax ESCs (\033[...m).  Background is re-emitted
// at the start of each line and after every \033[0m reset within a line.
static ok64 BIFFPrintColored(u8s out, u8cs text, u8cs bgesc) {
    sane(u8sOK(out));
    u8cs RST = $u8str("\033[0m");
    u8cp p = text[0];
    b8 bg_on = NO;
    while (p < text[1]) {
        if (*p == '\n') {
            if (bg_on) { call(u8sFeed, out, RST); bg_on = NO; }
            u8sFeed1(out, '\n');
            p++;
            continue;
        }
        if (!bg_on) { call(u8sFeed, out, bgesc); bg_on = YES; }
        // Detect \033[0m — re-emit bg after it
        if (*p == '\033' && p + 3 < text[1] &&
            p[1] == '[' && p[2] == '0' && p[3] == 'm') {
            call(u8sFeed, out, RST);
            call(u8sFeed, out, bgesc);
            p += 4;
            continue;
        }
        u8sFeed1(out, *p);
        p++;
    }
    if (bg_on) call(u8sFeed, out, RST);
    done;
}

// Flush buffered del/add segments: all red first, then all green.
// del0/add0 are the base positions (start of buffered data).
// Resets del/add cursors back to their base positions.
static ok64 BIFFPrintFlush(u8s out, u8s del, u8p del0,
                            u8s add, u8p add0) {
    sane(u8sOK(out));
    a_pad(u8, _de, 16);
    escfeedBG256(_de_idle, HILI_DEL_BG);
    u8cs DELESC = {_de[1], _de[2]};
    a_pad(u8, _ae, 16);
    escfeedBG256(_ae_idle, HILI_ADD_BG);
    u8cs ADDESC = {_ae[1], _ae[2]};
    u8cs ds = {(u8cp)del0, (u8cp)del[0]};
    u8cs as = {(u8cp)add0, (u8cp)add[0]};
    if (!$empty(ds)) call(BIFFPrintColored, out, ds, DELESC);
    if (!$empty(as)) call(BIFFPrintColored, out, as, ADDESC);
    del[0] = del0;
    add[0] = add0;
    done;
}

// Parallel walk with red/green buffering.
// del/add are scratch buffers for accumulating deletion/addition text.
// [0] is the write cursor (advances on BIFFPrintLeaves), [1] is capacity end.
// Base pointers (del0/add0) saved at entry; flush reads data from base to cursor.
static ok64 BIFFPrintLevel(u8s out, u8s del, u8s add,
                            u8csc odata, u8csc ndata) {
    sane(u8sOK(out));
    u8p del0 = del[0];
    u8p add0 = add[0];
    aBpad(u64, ostk, 64);
    aBpad(u64, nstk, 64);
    call(BASONOpen, ostk, odata);
    call(BASONOpen, nstk, ndata);

    // Manual parallel walk: drain both, compare keys
    u8 ot = 0, nt = 0;
    u8cs ok, ov, nk, nv;
    ok64 oo = BASONDrain(ostk, odata, &ot, ok, ov);
    ok64 no = BASONDrain(nstk, ndata, &nt, nk, nv);

    while (oo == OK || no == OK) {
        b8 oonly = NO, nonly = NO, both = NO;
        if (oo == OK && no == OK) {
            int cmp = $cmp(ok, nk);
            if (cmp < 0) oonly = YES;
            else if (cmp > 0) nonly = YES;
            else both = YES;
        } else if (oo == OK) {
            oonly = YES;
        } else {
            nonly = YES;
        }

        if (oonly) {
            u8 ds[64] = {};
            call(BIFFPrintStyledLeaves, del, ot, ov, ds, 0);
            call(BIFFSkip, ostk, odata, ot, ov);
            oo = BASONDrain(ostk, odata, &ot, ok, ov);
        } else if (nonly) {
            u8 as[64] = {};
            call(BIFFPrintStyledLeaves, add, nt, nv, as, 0);
            call(BIFFSkip, nstk, ndata, nt, nv);
            no = BASONDrain(nstk, ndata, &nt, nk, nv);
        } else {
            b8 same = NO;
            if (ot == nt && !BASONPlex(ot)) {
                if ($len(ov) == $len(nv) && $len(ov) == 0) same = YES;
                else if ($len(ov) == $len(nv) &&
                         memcmp(ov[0], nv[0], $len(ov)) == 0)
                    same = YES;
            }

            if (same) {
                call(BIFFPrintFlush, out, del, del0, add, add0);
                u8 cstk[64] = {};
                call(BIFFPrintStyledLeaves, out, ot, ov, cstk, 0);
                call(BIFFSkip, ostk, odata, ot, ov);
                call(BIFFSkip, nstk, ndata, nt, nv);
            } else if (BASONPlex(ot) && BASONPlex(nt) && ot == nt) {
                call(BIFFPrintFlush, out, del, del0, add, add0);
                call(BIFFPrintLevel, out, del, add, ov, nv);
                call(BIFFSkip, ostk, odata, ot, ov);
                call(BIFFSkip, nstk, ndata, nt, nv);
            } else {
                u8 ds[64] = {}, as[64] = {};
                call(BIFFPrintStyledLeaves, del, ot, ov, ds, 0);
                call(BIFFPrintStyledLeaves, add, nt, nv, as, 0);
                call(BIFFSkip, ostk, odata, ot, ov);
                call(BIFFSkip, nstk, ndata, nt, nv);
            }
            oo = BASONDrain(ostk, odata, &ot, ok, ov);
            no = BASONDrain(nstk, ndata, &nt, nk, nv);
        }
    }

    call(BIFFPrintFlush, out, del, del0, add, add0);
    done;
}

// Trim colored text to show only ctx lines around changed lines.
// Changed lines are detected by 256-color background ESC (\033[48;).
// Output always <= input, safe for in-place use.
static ok64 BIFFTrimContext(u8s out, u8cs text, u32 ctx, u8cs name) {
    sane(u8sOK(out));
    // Count lines
    u32 nlines = 0;
    {
        u8cp p = text[0];
        while (p < text[1]) {
            if (*p == '\n') nlines++;
            p++;
        }
        if (text[1] > text[0] && *(text[1] - 1) != '\n') nlines++;
    }
    if (nlines == 0) done;

    // Build line start/end array on stack (capped)
    u32 maxlines = nlines < 8192 ? nlines : 8192;
    u8cp starts[maxlines];
    u8cp ends[maxlines];
    b8 changed[maxlines];
    {
        u32 li = 0;
        u8cp p = text[0];
        starts[0] = p;
        changed[0] = NO;
        while (p < text[1] && li < maxlines) {
            if (*p == '\033' && p + 4 < text[1] &&
                p[1] == '[' && p[2] == '4' && p[3] == '8' && p[4] == ';')
                changed[li] = YES;
            if (*p == '\n') {
                ends[li] = p + 1;
                li++;
                if (li < maxlines) {
                    starts[li] = p + 1;
                    changed[li] = NO;
                }
            }
            p++;
        }
        if (li < maxlines && starts[li] < text[1]) {
            ends[li] = text[1];
            li++;
        }
        nlines = li;
    }

    // Mark lines to keep (within ctx of any changed line)
    b8 keep[nlines];
    for (u32 i = 0; i < nlines; i++) keep[i] = NO;
    for (u32 i = 0; i < nlines; i++) {
        if (!changed[i]) continue;
        u32 lo = (i >= ctx) ? i - ctx : 0;
        u32 hi = (i + ctx < nlines) ? i + ctx : nlines - 1;
        for (u32 j = lo; j <= hi; j++) keep[j] = YES;
    }

    // Output kept lines, insert separator between non-adjacent hunks
    // Uses memmove because source and dest may overlap (in-place trim)
    u8cs GREY = $u8str("\033[90m");
    u8cs RST = $u8str("\033[0m");
    u8cs DASHES = $u8str("---");
    u8cs NL = $u8str("\n");
    i32 last_kept = -1;
    for (u32 i = 0; i < nlines; i++) {
        if (!keep[i]) continue;
        if (last_kept >= 0 && i > (u32)last_kept + 1) {
            call(u8sFeed, out, GREY);
            call(u8sFeed, out, DASHES);
            call(u8sFeed, out, name);
            call(u8sFeed, out, DASHES);
            call(u8sFeed, out, RST);
            call(u8sFeed, out, NL);
        }
        size_t ll = (size_t)(ends[i] - starts[i]);
        if ((size_t)(out[1] - out[0]) < ll) fail(SNOROOM);
        memmove(out[0], starts[i], ll);
        out[0] += ll;
        last_kept = (i32)i;
    }
    done;
}

ok64 BASONDiffPrint(u8s out, u8csc odata, u8csc ndata, u32 ctx,
                     u8cs name) {
    sane(u8sOK(out));
    if ($empty(odata) && $empty(ndata)) done;

    // Carve del/add scratch buffers from end of output space
    size_t total = (size_t)(out[1] - out[0]);
    size_t scratch = total / 4;
    u8p del_start = out[1] - 2 * scratch;
    u8p add_start = out[1] - scratch;
    u8s del = {del_start, del_start + scratch};
    u8s add = {add_start, add_start + scratch};
    u8s wout = {out[0], del_start};

    u8p start = wout[0];
    call(BIFFPrintLevel, wout, del, add, odata, ndata);
    out[0] = wout[0];

    if (ctx > 0) {
        u8cs full = {(u8cp)start, (u8cp)out[0]};
        if ($empty(full)) done;
        // Reset cursor, trim in-place (output always <= input)
        out[0] = start;
        call(BIFFTrimContext, out, full, ctx, name);
    }

    done;
}
