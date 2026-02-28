#include "BIFF.h"

#include "abc/PRO.h"

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

// --- Diff: recursive parallel walk ---

static ok64 BIFFDiffLevel(u8bp out, u64bp idx,
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
            // old-only: deleted -> null tombstone
            u8cs empty = {(u8cp)ok[0], (u8cp)ok[0]};
            call(BASONFeed, idx, out, 'B', ok, empty);
            call(BIFFSkip, ostk, odata, ot, ov);
            oo = BASONDrain(ostk, odata, &ot, ok, ov);
        } else if (cmp > 0) {
            // new-only: added
            call(BIFFCopy, out, idx, nt, nk, nv, nstk, ndata);
            no = BASONDrain(nstk, ndata, &nt, nk, nv);
        } else {
            // same key
            if (BASONPlex(ot) && BASONPlex(nt) && ot == nt) {
                // both containers of same type: recurse
                size_t before = u8bDataLen(out);
                call(BASONFeedInto, idx, out, nt, nk);
                call(BASONInto, ostk, odata, ov);
                call(BASONInto, nstk, ndata, nv);
                call(BIFFDiffLevel, out, idx, ostk, odata, nstk, ndata);
                call(BASONOuto, ostk);
                call(BASONOuto, nstk);
                call(BASONFeedOuto, idx, out);
                // strip empty container (no diffs inside)
                size_t after = u8bDataLen(out);
                u8cs from = {out[1] + before, out[1] + after};
                u8 ct; u8cs ck, cv;
                ok64 dr = TLKVDrain(from, &ct, ck, cv);
                if (dr == OK && $len(cv) == 0) {
                    ((u8 **)out)[2] = out[1] + before;
                }
            } else if (BIFFValEqual(ot, ov, nt, nv)) {
                // identical: skip both
                call(BIFFSkip, ostk, odata, ot, ov);
                call(BIFFSkip, nstk, ndata, nt, nv);
            } else {
                // different: emit new
                call(BIFFSkip, ostk, odata, ot, ov);
                call(BIFFCopy, out, idx, nt, nk, nv, nstk, ndata);
            }
            oo = BASONDrain(ostk, odata, &ot, ok, ov);
            no = BASONDrain(nstk, ndata, &nt, nk, nv);
        }
    }

    // remaining old: all deleted
    while (oo == OK) {
        u8cs empty = {(u8cp)ok[0], (u8cp)ok[0]};
        call(BASONFeed, idx, out, 'B', ok, empty);
        call(BIFFSkip, ostk, odata, ot, ov);
        oo = BASONDrain(ostk, odata, &ot, ok, ov);
    }

    // remaining new: all added
    while (no == OK) {
        call(BIFFCopy, out, idx, nt, nk, nv, nstk, ndata);
        no = BASONDrain(nstk, ndata, &nt, nk, nv);
    }

    done;
}

ok64 BASONDiff(u8bp out, u64bp idx,
               u64bp ostk, u8csc odata,
               u64bp nstk, u8csc ndata) {
    sane(out != NULL);
    call(BASONOpen, ostk, odata);
    call(BASONOpen, nstk, ndata);
    call(BIFFDiffLevel, out, idx, ostk, odata, nstk, ndata);
    done;
}
