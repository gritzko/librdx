//
// DIFF.c - Compute CRDT patch from doc + stripped new version
//
// Architecture:
// 1. Strip doc to get oud
// 2. Compute BRACKET hashes for oud and neu
// 3. Run Myers diff on hash arrays, mark LSBs: changed=1, unchanged=0
// 4. Two mutually recursive functions iterate in parallel:
//    - diffScanOP: position-ordered (ROOT, TUPLE)
//    - diffScanLEX: key-ordered (EULER, LINEAR, MULTIX)
//
// Result: patch such that strip(merge(doc, patch)) == neu
//

#include "abc/DIFF.h"

#include "RDX.h"
#include "abc/PRO.h"

// Instantiate Myers diff for u64 (hash) sequences
#define X(M, name) M##u64##name
#include "abc/DIFFx.h"

// Error codes
con ok64 DIFFbadhash = 0xd48f3e6968b25dec;

// Forward declaration from RDX.c
b8 rdxIsEmptyTuple(rdxcp x);

// ============================================================================
// Hash LSB helpers - LSB encodes changed (1) vs unchanged (0)
// ============================================================================

#define HASH_CHANGED(h) ((h) & 1)
#define HASH_MARK_CHANGED(h) ((h) | 1)
#define HASH_CLEAR_LSB(h) ((h) & ~1ULL)
// Bit 1: "child changed" - container has marked descendants
#define HASH_CHILD_CHANGED(h) ((h) & 2)
#define HASH_MARK_CHILD_CHANGED(h) ((h) | 2)


// ============================================================================
// Parallel iterator state
// ============================================================================

typedef struct {
    rdx doc;       // Original document (with IDs)
    rdx oud;       // Stripped doc
    rdx neu;       // Stripped new version
    u64g oudhash;  // oud hash gauge [start, cur, end]
    u64g neuhash;  // neu hash gauge [start, cur, end]
    ok64 doc_ok;   // Status of doc iterator
    ok64 oud_ok;   // Status of oud iterator
    ok64 neu_ok;   // Status of neu iterator
    u64 prev_seq;  // Previous doc seq (for LINEAR gap insertion)
    u64 next_seq;  // Next doc seq (for LINEAR gap insertion)
} diff_iter;

typedef diff_iter* diff_iterp;

// ============================================================================
// Parallel iterator functions
// ============================================================================

// Check if hash gauge has remaining elements
fun b8 diffHashOK(u64g h) { return u64gRestLen(h) > 0; }

// Get current hash value (must check diffHashOK first)
fun u64 diffHashCur(u64g h) { return *h[1]; }

// Compute hash nesting depth (number of unclosed OPENs from start to current position)
fun i32 diffHashDepth(u64g h) {
    i32 depth = 0;
    for (u64p p = h[0]; p < h[1]; p++) {
        u64 marker = *p & (3ULL << 62);
        if (marker == (1ULL << 62)) depth++;       // OPEN
        else if (marker == (2ULL << 62)) depth--;  // CLOSE
    }
    return depth;
}

// Debug: verify hash depths match across iterators
#ifdef ABC_TRACE
fun void diffCheckSync(diff_iterp it, const char* where) {
    i32 oud_depth = (it->oud_ok == OK) ? diffHashDepth(it->oudhash) : -1;
    i32 neu_depth = (it->neu_ok == OK) ? diffHashDepth(it->neuhash) : -1;
    if (oud_depth >= 0 && neu_depth >= 0 && oud_depth != neu_depth) {
        fprintf(stderr, "SYNC ERROR at %s: oud_hash_depth=%d neu_hash_depth=%d\n",
                where, oud_depth, neu_depth);
    }
}
#define DIFF_CHECK_SYNC(it, where) diffCheckSync(it, where)
#else
#define DIFF_CHECK_SYNC(it, where) ((void)0)
#endif

// Check if any hash in a subtree is marked as changed (for TUPLE value changes)
// Returns YES if any hash from current position to matching CLOSE has LSB=1
// Uses BRACKET hash depth bits at positions 56-61 to track nesting
fun b8 diffHasMarkedChild(u64g h) {
    if (!diffHashOK(h)) return NO;
    u64 cur_hash = diffHashCur(h);
    // BRACKET markers: OPEN=1<<62, CLOSE=2<<62, FIRST=0
    u64 marker = cur_hash & (3ULL << 62);
    if (marker != (1ULL << 62)) return NO;  // Not a container OPEN
    // Scan children until matching CLOSE
    // Use nesting counter instead of depth bits (more reliable)
    u64p p = h[1] + 1;  // Start after OPEN
    i32 nesting = 1;  // We're inside the current OPEN
    while (p < h[2] && nesting > 0) {
        u64 hval = *p;
        marker = hval & (3ULL << 62);
        if (marker == (1ULL << 62)) {
            nesting++;  // Entering nested container
        } else if (marker == (2ULL << 62)) {
            nesting--;  // Exiting container
            if (nesting == 0) return NO;  // Found matching CLOSE
        }
        if (nesting > 0 && HASH_CHANGED(hval)) {
            return YES;  // Found marked child (only if still inside container)
        }
        p++;
    }
    return NO;  // Shouldn't reach here for valid data
}

// Skip a container's subtree hashes in a hash gauge
// Only advances the hash gauge, not the rdx iterator
// The rdx iterator should already be positioned at the container
fun void diffSkipContainerHash(u64g h) {
    if (!diffHashOK(h)) return;
    u64 cur_hash = diffHashCur(h);
    u64 marker = cur_hash & (3ULL << 62);
    if (marker != (1ULL << 62)) return;  // Not an OPEN - nothing to skip
    // Skip OPEN
    h[1]++;
    // Skip children until matching CLOSE
    i32 nesting = 1;
    while (h[1] < h[2] && nesting > 0) {
        u64 hval = *h[1];
        marker = hval & (3ULL << 62);
        if (marker == (1ULL << 62)) {
            nesting++;
        } else if (marker == (2ULL << 62)) {
            nesting--;
        }
        h[1]++;
    }
}


// Advance doc, optionally skipping tombstones
ok64 diffNextDoc(diff_iterp it, b8 skip_tombs) {
    sane(it);
    if (it->doc_ok != OK) done;
    do {
        it->doc_ok = rdxNext(&it->doc);
    } while (skip_tombs && it->doc_ok == OK && (it->doc.id.seq & 1));
    done;
}

// Advance oud and hash (only if stream active)
ok64 diffNextOud(diff_iterp it) {
    sane(it);
    if (it->oud_ok != OK) done;
    it->oud_ok = rdxNext(&it->oud);
    if (it->oud_ok == OK) u64gFed1(it->oudhash);
    done;
}

// Advance neu and hash (only if stream active)
ok64 diffNextNeu(diff_iterp it) {
    sane(it);
    if (it->neu_ok != OK) done;
    it->neu_ok = rdxNext(&it->neu);
    if (it->neu_ok == OK) u64gFed1(it->neuhash);
    done;
}

// OP (position-ordered): all advance together
// In TUPLE, tombstones in doc have empty tuple replacements in oud - no skipping needed
ok64 diffNextOP(diff_iterp it) {
    sane(it);
    call(diffNextDoc, it, NO);
    call(diffNextOud, it);
    call(diffNextNeu, it);
    done;
}

// LEX: dispatch based on current markers
// In LEX containers, tombstones are removed from oud - doc must skip them
ok64 diffNextLEX(diff_iterp it) {
    sane(it);

    b8 oud_changed = (it->oud_ok == OK && diffHashOK(it->oudhash))
                     ? HASH_CHANGED(diffHashCur(it->oudhash)) : NO;
    b8 neu_changed = (it->neu_ok == OK && diffHashOK(it->neuhash))
                     ? HASH_CHANGED(diffHashCur(it->neuhash)) : NO;

    if (oud_changed && neu_changed) {
        // REPLACE: both changed - advance both
        call(diffNextDoc, it, YES);
        call(diffNextOud, it);
        call(diffNextNeu, it);
    } else if (oud_changed && !neu_changed) {
        // DEL: advance oud + doc (skip tombstones)
        call(diffNextDoc, it, YES);
        call(diffNextOud, it);
    } else if (neu_changed && !oud_changed) {
        // INS: advance neu only
        call(diffNextNeu, it);
    } else if (it->oud_ok == OK && it->neu_ok == OK) {
        // EQ: advance all (skip tombstones in doc)
        call(diffNextDoc, it, YES);
        call(diffNextOud, it);
        call(diffNextNeu, it);
    } else if (it->oud_ok == OK) {
        // Trailing oud (skip tombstones in doc)
        call(diffNextDoc, it, YES);
        call(diffNextOud, it);
    } else if (it->neu_ok == OK) {
        // Trailing neu
        call(diffNextNeu, it);
    }
    done;
}

// Descend into container
ok64 diffInto(diff_iterp child, diff_iterp parent) {
    sane(child && parent);
    *child = (diff_iter){};
    // Descend - treat any rdxInto failure as END (non-container)
    ok64 r;
    r = (parent->doc_ok == OK) ? rdxInto(&child->doc, &parent->doc) : END;
    child->doc_ok = (r == OK) ? OK : END;
    r = (parent->oud_ok == OK) ? rdxInto(&child->oud, &parent->oud) : END;
    child->oud_ok = (r == OK) ? OK : END;
    r = (parent->neu_ok == OK) ? rdxInto(&child->neu, &parent->neu) : END;
    child->neu_ok = (r == OK) ? OK : END;
    // Hash: skip OPEN, child continues from there
    if (parent->oud_ok == OK) u64gFed1(parent->oudhash);
    if (parent->neu_ok == OK) u64gFed1(parent->neuhash);
    u64gMv(child->oudhash, parent->oudhash);
    u64gMv(child->neuhash, parent->neuhash);
    done;
}

// Exit container
ok64 diffOuto(diff_iterp child, diff_iterp parent) {
    sane(child && parent);
    if (parent->doc_ok == OK) call(rdxOuto, &child->doc, &parent->doc);
    if (parent->oud_ok == OK) call(rdxOuto, &child->oud, &parent->oud);
    if (parent->neu_ok == OK) call(rdxOuto, &child->neu, &parent->neu);
    // Hash: restore parent pointer, skip CLOSE
    u64gMv(parent->oudhash, child->oudhash);
    u64gMv(parent->neuhash, child->neuhash);
    if (parent->oud_ok == OK) u64gFed1(parent->oudhash);
    if (parent->neu_ok == OK) u64gFed1(parent->neuhash);
    done;
}

// ============================================================================
// Patch emission helpers
// ============================================================================

// Emit element to patch: copy type/value, set ID
ok64 diffEmit(rdxp patch, rdxcp from, id128 id) {
    sane(patch && from);
    patch->type = from->type;
    patch->flags = from->flags;
    patch->id = id;
    patch->r = from->r;
    call(rdxNext, patch);
    if (rdxTypePlex(from)) {
        // Make a local copy to avoid modifying caller's iterator
        rdx parent = *from;
        rdx cfrom = {};
        rdx cpatch = {};
        call(rdxInto, &cpatch, patch);
        call(rdxInto, &cfrom, &parent);
        call(rdxCopy, &cpatch, &cfrom);
        call(rdxOuto, &cpatch, patch);
    }
    done;
}

// ============================================================================
// Forward declarations for mutual recursion
// ============================================================================

ok64 diffScanOP(diff_iterp it, rdxp patch);
ok64 diffScanLEX(diff_iterp it, rdxp patch);

// ============================================================================
// Mutually recursive scan functions
// ============================================================================

// Scan position-ordered container (ROOT, TUPLE)
// All streams advance in lockstep by position
// Emits ALL positions to patch (TUPLE semantics)
ok64 diffScanOP(diff_iterp it, rdxp patch) {
    sane(it && patch);

    // Track position for TUPLE key handling
    u32 pos = 0;
    // ptype is set by rdxInto - read it before advancing
    u8 ptype = (it->doc_ok == OK) ? it->doc.ptype : patch->ptype;
    // Track if doc uses IDs (for ROOT: use zero ID for INS if doc has no IDs)
    b8 doc_uses_ids = NO;

    // Initial advance to first element (DON'T advance hash - already there)
    if (it->doc_ok == OK) it->doc_ok = rdxNext(&it->doc);
    if (it->oud_ok == OK) it->oud_ok = rdxNext(&it->oud);
    if (it->neu_ok == OK) it->neu_ok = rdxNext(&it->neu);

    while (it->oud_ok == OK || it->neu_ok == OK) {
        // Track if we skipped containers (affects hash advancement at end of loop)
        // Each hash may be skipped independently (e.g. type mismatch: one is container, other is leaf)
        b8 oud_skipped = NO, neu_skipped = NO;

        // For position-ordered containers, compare hashes directly (ignoring marker bit)
        // rather than using marker bits from Myers diff (which can be wrong with duplicates)
        u64 oud_hash_val = diffHashOK(it->oudhash) ? diffHashCur(it->oudhash) : 0;
        u64 neu_hash_val = diffHashOK(it->neuhash) ? diffHashCur(it->neuhash) : 0;
        u64 oud_hash_cmp = oud_hash_val & ~3ULL;  // Strip both marker bits (0=changed, 1=child_changed)
        u64 neu_hash_cmp = neu_hash_val & ~3ULL;

        // TUPLE key (position 0) always has zero ID
        b8 is_tuple_key = (ptype == RDX_TYPE_TUPLE && pos == 0);
        // Track if doc has IDs at current position
        b8 doc_has_id = (it->doc_ok == OK) && (it->doc.id.seq != 0 || it->doc.id.src != 0);
        if (doc_has_id) doc_uses_ids = YES;
        // For EQ case in ROOT with zero-ID doc element, use zero ID
        // For REPLACE/INS, always use computed ID (unless TUPLE key or ROOT without IDs)
        b8 use_zero_id_for_eq = is_tuple_key || (ptype == RDX_TYPE_ROOT && !doc_has_id);

        if (it->oud_ok == OK && it->neu_ok == OK && oud_hash_cmp == neu_hash_cmp) {
            // EQ: both unchanged - emit from doc, descend if container
            b8 is_tombstone = (it->doc_ok == OK && (it->doc.id.seq & 1));
            if (rdxTypePlex(&it->oud) && it->oud.type == it->neu.type &&
                !is_tombstone) {
                // Emit container header from doc
                patch->type = it->doc.type;
                patch->flags = it->doc.flags;
                patch->id = it->doc.id;
                call(rdxNext, patch);
                // Descend into container
                diff_iter child = {};
                rdx cpatch = {};
                u8 ctype = it->oud.type;
                call(diffInto, &child, it);
                call(rdxInto, &cpatch, patch);
                if (ctype == RDX_TYPE_EULER || ctype == RDX_TYPE_LINEAR ||
                    ctype == RDX_TYPE_MULTIX) {
                    call(diffScanLEX, &child, &cpatch);
                } else {
                    call(diffScanOP, &child, &cpatch);
                }
                call(diffOuto, &child, it);
                call(rdxOuto, &cpatch, patch);
                oud_skipped = YES;
                neu_skipped = YES;
            } else {
                // Emit unchanged leaf from doc
                id128 emit_id = use_zero_id_for_eq ? (id128){} : it->doc.id;
                call(diffEmit, patch, &it->doc, emit_id);
            }
        } else if (it->oud_ok == OK && it->neu_ok == OK) {
            // Hashes differ - check if we should descend or replace
            // For containers: CHILD_CHANGED means descend (structure same, children differ)
            // We descend if it's a container with same type
            b8 is_container = rdxTypePlex(&it->oud) && it->oud.type == it->neu.type;
            b8 is_tombstone = (it->doc_ok == OK && (it->doc.id.seq & 1));
            if (is_container && !is_tombstone) {
                // Container with changed children - descend
                patch->type = it->doc.type;
                patch->flags = it->doc.flags;
                patch->id = it->doc.id;
                call(rdxNext, patch);
                // Descend into container
                diff_iter child = {};
                rdx cpatch = {};
                u8 ctype = it->oud.type;
                call(diffInto, &child, it);
                call(rdxInto, &cpatch, patch);
                if (ctype == RDX_TYPE_EULER || ctype == RDX_TYPE_LINEAR ||
                    ctype == RDX_TYPE_MULTIX) {
                    call(diffScanLEX, &child, &cpatch);
                } else {
                    call(diffScanOP, &child, &cpatch);
                }
                call(diffOuto, &child, it);
                call(rdxOuto, &cpatch, patch);
                oud_skipped = YES;
                neu_skipped = YES;
            } else {
                // REPLACE: both present but hashes differ - emit new value with higher seq
                id128 new_id = is_tuple_key ? (id128){}
                             : (id128){.src = it->doc.id.src,
                                       .seq = (it->doc.id.seq & ~1ULL) + 2};
                call(diffEmit, patch, &it->neu, new_id);
                // Skip subtree hashes if container (for doc, oud, and neu)
                // Note: doc skip doesn't affect hash (doc has no hash)
                if (it->doc_ok == OK && rdxTypePlex(&it->doc)) {
                    diff_iter skip = {};
                    call(diffInto, &skip, it);
                    while (skip.doc_ok == OK) call(diffNextDoc, &skip, NO);
                    call(diffOuto, &skip, it);
                }
                if (it->oud_ok == OK && rdxTypePlex(&it->oud)) {
                    diff_iter skip = {};
                    call(diffInto, &skip, it);
                    while (skip.oud_ok == OK) call(diffNextOP, &skip);
                    call(diffOuto, &skip, it);
                    oud_skipped = YES;
                }
                if (it->neu_ok == OK && rdxTypePlex(&it->neu)) {
                    diff_iter skip = {};
                    call(diffInto, &skip, it);
                    while (skip.neu_ok == OK) call(diffNextOP, &skip);
                    call(diffOuto, &skip, it);
                    neu_skipped = YES;
                }
            }
        } else if (it->oud_ok == OK) {
            // DEL: only oud present - emit tombstone (doc value with odd seq)
            // (TUPLE key can't be deleted, so no need for is_tuple_key check here)
            id128 tomb_id = {.src = it->doc.id.src,
                             .seq = it->doc.id.seq | 1};
            call(diffEmit, patch, &it->doc, tomb_id);
            // Skip subtree hashes if container (for doc and oud)
            if (it->doc_ok == OK && rdxTypePlex(&it->doc)) {
                diff_iter skip = {};
                call(diffInto, &skip, it);
                while (skip.doc_ok == OK) call(diffNextDoc, &skip, NO);
                call(diffOuto, &skip, it);
            }
            if (it->oud_ok == OK && rdxTypePlex(&it->oud)) {
                diff_iter skip = {};
                call(diffInto, &skip, it);
                while (skip.oud_ok == OK) call(diffNextOP, &skip);
                call(diffOuto, &skip, it);
                oud_skipped = YES;
            }
        } else {
            // INS: only neu present - emit new element with fresh seq
            // Use zero ID for TUPLE key or ROOT without IDs
            b8 use_zero_id_for_ins = is_tuple_key || (ptype == RDX_TYPE_ROOT && !doc_uses_ids);
            u64 new_seq = (it->doc_ok == OK) ? (it->doc.id.seq & ~1ULL) + 2 : 2;
            id128 new_id = use_zero_id_for_ins ? (id128){} : (id128){.src = 0, .seq = new_seq};
            call(diffEmit, patch, &it->neu, new_id);
            // Skip subtree hashes if container
            if (it->neu_ok == OK && rdxTypePlex(&it->neu)) {
                diff_iter skip = {};
                call(diffInto, &skip, it);
                while (skip.neu_ok == OK) call(diffNextOP, &skip);
                call(diffOuto, &skip, it);
                neu_skipped = YES;
            }
        }

        pos++;
        // Advance to next element
        // Hash advance: skip if we already skipped the container (diffOuto positioned it)
        call(diffNextDoc, it, NO);
        if (it->oud_ok == OK) {
            it->oud_ok = rdxNext(&it->oud);
            if (!oud_skipped && diffHashOK(it->oudhash)) u64gFed1(it->oudhash);
        }
        if (it->neu_ok == OK) {
            it->neu_ok = rdxNext(&it->neu);
            if (!neu_skipped && diffHashOK(it->neuhash)) u64gFed1(it->neuhash);
        }
    }

    // Handle trailing doc elements (empty tuples and tombstones can be interspersed)
    // - Empty tuples: emit to preserve TUPLE positions (rdxStrip removes them from oud)
    // - Tombstones: skip (they remain in doc after merge, don't need to be in patch)
    while (it->doc_ok == OK) {
        b8 is_empty = rdxIsEmptyTuple(&it->doc);
        b8 is_tomb = (it->doc.id.seq & 1);
        if (!is_empty && !is_tomb) break;  // real element - shouldn't happen
        if (is_empty) {
            call(diffEmit, patch, &it->doc, it->doc.id);
        }
        // Skip subtree hashes if container
        if (rdxTypePlex(&it->doc)) {
            diff_iter skip = {};
            call(diffInto, &skip, it);
            while (skip.doc_ok == OK) call(diffNextDoc, &skip, NO);
            call(diffOuto, &skip, it);
        }
        call(diffNextDoc, it, NO);
    }

    // Sanity: all streams must be at END
    test(it->doc_ok != OK, DIFFbadhash);
    test(it->oud_ok != OK, DIFFbadhash);
    test(it->neu_ok != OK, DIFFbadhash);

    done;
}

// Scan key-ordered container (EULER, LINEAR, MULTIX)
// Only emits CHANGED elements (DEL tombstones, INS new values)
ok64 diffScanLEX(diff_iterp it, rdxp patch) {
    sane(it && patch);

    // Track prev seq for LINEAR gap insertion
    u64 prev_seq = 0;

    // Initial advance to first element (DON'T advance hash yet - it's already there)
    if (it->doc_ok == OK) it->doc_ok = rdxNext(&it->doc);
    while (it->doc_ok == OK && (it->doc.id.seq & 1)) {
        it->doc_ok = rdxNext(&it->doc);  // skip tombstones
    }
    if (it->oud_ok == OK) it->oud_ok = rdxNext(&it->oud);
    if (it->neu_ok == OK) it->neu_ok = rdxNext(&it->neu);

    while (it->oud_ok == OK || it->neu_ok == OK) {
        // Check markers BEFORE processing
        b8 oud_changed = (it->oud_ok == OK && diffHashOK(it->oudhash))
                         ? HASH_CHANGED(diffHashCur(it->oudhash)) : NO;
        b8 neu_changed = (it->neu_ok == OK && diffHashOK(it->neuhash))
                         ? HASH_CHANGED(diffHashCur(it->neuhash)) : NO;

        if (oud_changed && neu_changed) {
            // REPLACE: both changed
            // MULTIX: no tombstone, just emit new value with higher seq
            // EULER/LINEAR: tombstone + insert
            u64 new_seq;
            if (it->doc.ptype == RDX_TYPE_MULTIX) {
                new_seq = (it->doc.id.seq & ~1ULL) + 2;
                id128 new_id = {.src = it->doc.id.src, .seq = new_seq};
                call(diffEmit, patch, &it->neu, new_id);
            } else {
                id128 tomb_id = {.src = it->doc.id.src,
                                 .seq = it->doc.id.seq | 1};
                call(diffEmit, patch, &it->doc, tomb_id);
                new_seq = (it->doc.id.seq | 1) + 1;
                id128 new_id = {.src = 0, .seq = new_seq};
                call(diffEmit, patch, &it->neu, new_id);
            }
            prev_seq = new_seq;
            // Skip subtree hashes if containers (independently for each side)
            b8 oud_skipped = NO, neu_skipped = NO;
            if (rdxTypePlex(&it->oud)) {
                diffSkipContainerHash(it->oudhash);
                oud_skipped = YES;
            }
            if (rdxTypePlex(&it->neu)) {
                diffSkipContainerHash(it->neuhash);
                neu_skipped = YES;
            }
            // Advance rdx, but only advance hash if not skipped
            call(diffNextDoc, it, YES);
            if (it->oud_ok == OK) {
                it->oud_ok = rdxNext(&it->oud);
                if (!oud_skipped && diffHashOK(it->oudhash)) u64gFed1(it->oudhash);
            }
            if (it->neu_ok == OK) {
                it->neu_ok = rdxNext(&it->neu);
                if (!neu_skipped && diffHashOK(it->neuhash)) u64gFed1(it->neuhash);
            }
        } else if (oud_changed) {
            // DEL: emit tombstone
            id128 tomb_id = {.src = it->doc.id.src,
                             .seq = it->doc.id.seq | 1};
            call(diffEmit, patch, &it->doc, tomb_id);
            prev_seq = it->doc.id.seq;
            // Skip subtree hashes if container (only oud side - don't touch neu!)
            b8 oud_skipped = NO;
            if (rdxTypePlex(&it->oud)) {
                diffSkipContainerHash(it->oudhash);
                oud_skipped = YES;
            }
            // Advance oud + doc (only advance hash if not skipped)
            call(diffNextDoc, it, YES);
            if (it->oud_ok == OK) {
                it->oud_ok = rdxNext(&it->oud);
                if (!oud_skipped && diffHashOK(it->oudhash)) u64gFed1(it->oudhash);
            }
        } else if (neu_changed) {
            // INS: emit new element
            u64 new_seq;
            if (it->doc.ptype == RDX_TYPE_LINEAR) {
                u64 next_seq = (it->doc_ok == OK) ? it->doc.id.seq : prev_seq + 4;
                new_seq = prev_seq + (next_seq - prev_seq) / 2;
                if (new_seq <= prev_seq) new_seq = prev_seq + 2;
                // Round to even (odd = tombstone). Prefer rounding down for more room.
                if (new_seq & 1) {
                    new_seq--;
                    if (new_seq <= prev_seq) new_seq += 2;  // collision, round up instead
                }
            } else {
                new_seq = prev_seq + 2;
            }
            // MULTIX: preserve src from neu (determines slot)
            u64 new_src = (it->doc.ptype == RDX_TYPE_MULTIX) ? it->neu.id.src : 0;
            id128 new_id = {.src = new_src, .seq = new_seq};
            call(diffEmit, patch, &it->neu, new_id);
            prev_seq = new_seq;
            // Skip subtree hashes if container (only neu side - don't touch oud!)
            b8 neu_skipped = NO;
            if (rdxTypePlex(&it->neu)) {
                diffSkipContainerHash(it->neuhash);
                neu_skipped = YES;
            }
            // Advance neu only (only advance hash if not skipped)
            if (it->neu_ok == OK) {
                it->neu_ok = rdxNext(&it->neu);
                if (!neu_skipped && diffHashOK(it->neuhash)) u64gFed1(it->neuhash);
            }
        } else {
            // EQ: OPEN hashes match - but check if any child is marked
            // Check CHILD_CHANGED bit first (propagated from diffMarkHashes)
            // If not set, fall back to scanning children for marked hashes
            u64 oud_hash_raw = diffHashOK(it->oudhash) ? diffHashCur(it->oudhash) : 0;
            u64 neu_hash_raw = diffHashOK(it->neuhash) ? diffHashCur(it->neuhash) : 0;
            u64 oud_hash = oud_hash_raw & ~3ULL;  // Clear both marker bits
            u64 neu_hash = neu_hash_raw & ~3ULL;
            b8 hashes_match = (oud_hash == neu_hash);
            b8 has_marked_oud = HASH_CHILD_CHANGED(oud_hash_raw);
            b8 has_marked_neu = HASH_CHILD_CHANGED(neu_hash_raw);
            // If CHILD_CHANGED not set, fall back to scanning
            if (!has_marked_oud && !has_marked_neu && hashes_match &&
                it->oud_ok == OK && it->neu_ok == OK &&
                rdxTypePlex(&it->oud) && it->oud.type == it->neu.type) {
                has_marked_oud = diffHasMarkedChild(it->oudhash);
                has_marked_neu = diffHasMarkedChild(it->neuhash);
            }
            if (it->doc_ok == OK) prev_seq = it->doc.id.seq;

            if ((has_marked_oud || has_marked_neu) &&
                it->oud_ok == OK && it->neu_ok == OK &&
                rdxTypePlex(&it->oud) && it->oud.type == it->neu.type) {
                // Container with marked children - emit with SAME ID as doc
                // This tells merge to descend and apply partial updates
                patch->type = it->doc.type;
                patch->flags = it->doc.flags;
                patch->id = it->doc.id;  // Same ID triggers merge descend
                call(rdxNext, patch);

                // Descend and recurse
                diff_iter child = {};
                rdx cpatch = {};
                u8 ctype = it->oud.type;
                call(diffInto, &child, it);
                call(rdxInto, &cpatch, patch);
                if (ctype == RDX_TYPE_EULER || ctype == RDX_TYPE_LINEAR ||
                    ctype == RDX_TYPE_MULTIX) {
                    call(diffScanLEX, &child, &cpatch);
                } else {
                    call(diffScanOP, &child, &cpatch);
                }
                call(diffOuto, &child, it);
                call(rdxOuto, &cpatch, patch);

                // Advance rdx to next element (hash already advanced by diffOuto)
                call(diffNextDoc, it, YES);
                if (it->oud_ok == OK) {
                    it->oud_ok = rdxNext(&it->oud);
                    // DON'T advance hash - diffOuto already positioned it past CLOSE
                }
                if (it->neu_ok == OK) {
                    it->neu_ok = rdxNext(&it->neu);
                    // DON'T advance hash - diffOuto already positioned it past CLOSE
                }
            } else {
                // No marked children - skip subtree hashes if container
                // Note: diffSkipContainerHash advances past CLOSE, so we only advance rdx
                b8 oud_skipped = NO, neu_skipped = NO;
                if (it->oud_ok == OK && rdxTypePlex(&it->oud)) {
                    diffSkipContainerHash(it->oudhash);
                    oud_skipped = YES;
                }
                if (it->neu_ok == OK && rdxTypePlex(&it->neu)) {
                    diffSkipContainerHash(it->neuhash);
                    neu_skipped = YES;
                }
                // Advance rdx iterators, but only advance hash if not skipped
                call(diffNextDoc, it, YES);
                if (it->oud_ok == OK) {
                    it->oud_ok = rdxNext(&it->oud);
                    if (!oud_skipped && diffHashOK(it->oudhash)) u64gFed1(it->oudhash);
                }
                if (it->neu_ok == OK) {
                    it->neu_ok = rdxNext(&it->neu);
                    if (!neu_skipped && diffHashOK(it->neuhash)) u64gFed1(it->neuhash);
                }
            }
        }
    }

    // Sanity: all streams must be at END
    test(it->doc_ok != OK, DIFFbadhash);
    test(it->oud_ok != OK, DIFFbadhash);
    test(it->neu_ok != OK, DIFFbadhash);

    done;
}

// ============================================================================
// Transfer EDL decisions onto hash arrays
// oud: mark deleted elements (LSB=1)
// neu: mark inserted elements (LSB=1)
// ============================================================================

// Helper: mark parent OPENs with CHILD_CHANGED bit
fun void diffMarkParents(u64p* stack, u32 depth) {
    for (u32 i = 0; i < depth; i++) {
        *stack[i] = HASH_MARK_CHILD_CHANGED(*stack[i]);
    }
}

// Debug: dump full EDL to file
#ifdef ABC_TRACE
fun void diffDumpEDL(e32cs edl, u64 oud_len, u64 neu_len) {
    FILE* f = fopen("/tmp/diff-trace.txt", "w");
    if (!f) return;
    u64 oud_pos = 0, neu_pos = 0;
    u64 eq_count = 0, del_count = 0, ins_count = 0;

    u32 const* ep = edl[0];
    while (ep < edl[1]) {
        u8 kind = (*ep) >> 30;
        u32 len = (*ep) & 0x3FFFFFFF;
        char c = (kind == DIFF_EQ) ? '=' : (kind == DIFF_DEL) ? '-' : '+';
        for (u32 i = 0; i < len; i++) fputc(c, f);
        if (kind == DIFF_EQ) { eq_count += len; oud_pos += len; neu_pos += len; }
        else if (kind == DIFF_DEL) { del_count += len; oud_pos += len; }
        else { ins_count += len; neu_pos += len; }
        ep++;
    }
    fprintf(f, "\n\n");
    fprintf(f, "oud_len=%lu neu_len=%lu eq=%lu del=%lu ins=%lu\n",
            oud_len, neu_len, eq_count, del_count, ins_count);
    fprintf(f, "oud_pos=%lu neu_pos=%lu (expected %lu %lu)\n",
            oud_pos, neu_pos, oud_len, neu_len);
    fclose(f);
    fprintf(stderr, "EDL trace written to /tmp/diff-trace.txt\n");
}
#endif

ok64 diffMarkHashes(u64s oud_h, u64s neu_h, e32cs edl) {
    sane($ok(oud_h) && $ok(neu_h));

    // Stacks to track parent OPEN positions
    // Max nesting depth from RDX.h
    u64p oud_stack[RDX_MAX_NESTING] = {};
    u64p neu_stack[RDX_MAX_NESTING] = {};
    u32 oud_depth = 0, neu_depth = 0;

    $for(e32c, ep, edl) {
        u8 kind = DIFF_OP(*ep);
        u32 len = DIFF_LEN(*ep);

        if (kind == DIFF_EQ) {
            // Both match - LSB stays 0, consume both
            // Update parent stacks for both sides
            for (u32 i = 0; i < len; i++) {
                u64 oud_marker = *oud_h[0] & (3ULL << 62);
                u64 neu_marker = *neu_h[0] & (3ULL << 62);
                if (oud_marker == (1ULL << 62)) {  // OPEN
                    if (oud_depth < RDX_MAX_NESTING) oud_stack[oud_depth++] = oud_h[0];
                } else if (oud_marker == (2ULL << 62)) {  // CLOSE
                    if (oud_depth > 0) oud_depth--;
                }
                if (neu_marker == (1ULL << 62)) {  // OPEN
                    if (neu_depth < RDX_MAX_NESTING) neu_stack[neu_depth++] = neu_h[0];
                } else if (neu_marker == (2ULL << 62)) {  // CLOSE
                    if (neu_depth > 0) neu_depth--;
                }
                oud_h[0]++;
                neu_h[0]++;
            }
        } else if (kind == DIFF_DEL) {
            // Removed from oud - mark LSB=1 and propagate to parents
            for (u32 i = 0; i < len; i++) {
                u64 marker = *oud_h[0] & (3ULL << 62);
                *oud_h[0] = HASH_MARK_CHANGED(*oud_h[0]);
                // Mark all parent OPENs with CHILD_CHANGED
                diffMarkParents(oud_stack, oud_depth);
                // Update stack
                if (marker == (1ULL << 62)) {  // OPEN
                    if (oud_depth < RDX_MAX_NESTING) oud_stack[oud_depth++] = oud_h[0];
                } else if (marker == (2ULL << 62)) {  // CLOSE
                    if (oud_depth > 0) oud_depth--;
                }
                oud_h[0]++;
            }
        } else if (kind == DIFF_INS) {
            // Inserted in neu - mark LSB=1 and propagate to parents
            for (u32 i = 0; i < len; i++) {
                u64 marker = *neu_h[0] & (3ULL << 62);
                *neu_h[0] = HASH_MARK_CHANGED(*neu_h[0]);
                // Mark all parent OPENs with CHILD_CHANGED
                diffMarkParents(neu_stack, neu_depth);
                // Update stack
                if (marker == (1ULL << 62)) {  // OPEN
                    if (neu_depth < RDX_MAX_NESTING) neu_stack[neu_depth++] = neu_h[0];
                } else if (marker == (2ULL << 62)) {  // CLOSE
                    if (neu_depth > 0) neu_depth--;
                }
                neu_h[0]++;
            }
        }
    }

    // Ensure both slices fully consumed
    test($empty(oud_h), DIFFbadhash);
    test($empty(neu_h), DIFFbadhash);

    done;
}

// ============================================================================
// Main entry points
// ============================================================================

// rdxDiff2: diff with caller-provided buffers
ok64 rdxDiff2(rdxp patch, rdxp doc, rdxp neu,
              u8g oud_buf, u8g neu_buf, u64g hash_buf, i32s work, e32g edl) {
    sane(patch && doc && neu);

    rdx doc_saved = *doc;

    // Create fake buffers from gauges for TLV writers
    // gauge [data, idle, end] -> buffer [past=data, data=data, idle, end]
    u8b oud_b = {oud_buf[0], oud_buf[0], oud_buf[1], oud_buf[2]};
    u8b neu_b = {neu_buf[0], neu_buf[0], neu_buf[1], neu_buf[2]};

    // Step 1: Strip doc to get oud
    rdx oud_w = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
    oud_w.bulk = oud_b;
    call(rdxStrip, &oud_w, doc);
    u8cs oud_datac = {oud_b[1], oud_b[2]};  // data section

    // Step 2: Compute BRACKET hashes for oud (first half of hash_buf)
    u64 hash_half = (hash_buf[2] - hash_buf[0]) / 2;
    u64p oud_hash_start = hash_buf[0];
    u64s oud_hashes = {oud_hash_start, oud_hash_start + hash_half};
    {
        rdx oud_r = {.format = RDX_FMT_TLV};
        oud_r.next = oud_datac[0];
        oud_r.opt = (u8p)oud_datac[1];
        oud_r.bulk = NULL;
        call(rdxRapidHashesF, oud_hashes, &oud_r, RDX_HASH_BRACKET);
    }
    u64s oud_hashes_data = {oud_hash_start, oud_hashes[0]};

    // Step 3: Strip neu to TLV
    rdx neu_w = {.format = RDX_FMT_TLV | RDX_FMT_WRITE};
    neu_w.bulk = neu_b;
    {
        rdx neu_r = *neu;
        call(rdxStrip, &neu_w, &neu_r);
    }
    u8cs neu_datac = {neu_b[1], neu_b[2]};  // data section

    // Step 4: Compute BRACKET hashes for neu (second half of hash_buf)
    u64p neu_hash_start = hash_buf[0] + hash_half;
    u64s neu_hashes = {neu_hash_start, hash_buf[2]};
    {
        rdx neu_r = {.format = RDX_FMT_TLV};
        neu_r.next = neu_datac[0];
        neu_r.opt = (u8p)neu_datac[1];
        neu_r.bulk = NULL;
        call(rdxRapidHashesF, neu_hashes, &neu_r, RDX_HASH_BRACKET);
    }
    u64s neu_hashes_data = {neu_hash_start, neu_hashes[0]};

    // Step 5: Clear marker bits 0-1 (will be set by Myers marking)
    $for(u64, hp, oud_hashes_data) { *hp = *hp & ~3ULL; }
    $for(u64, hp, neu_hashes_data) { *hp = *hp & ~3ULL; }

    // Step 6: Run Myers diff and mark changed elements
    {
        u64cs ohc = {oud_hashes_data[0], oud_hashes_data[1]};
        u64cs nhc = {neu_hashes_data[0], neu_hashes_data[1]};
        call(DIFFu64s, edl, work, ohc, nhc);
        e32cs edl_cs = {edl[2], edl[0]};

        u64s oud_dup, neu_dup;
        u64sMv(oud_dup, oud_hashes_data);
        u64sMv(neu_dup, neu_hashes_data);
#ifdef ABC_TRACE
        diffDumpEDL(edl_cs, u64sLen(oud_hashes_data), u64sLen(neu_hashes_data));
#endif
        call(diffMarkHashes, oud_dup, neu_dup, edl_cs);
    }

    // Step 7: Initialize parallel iterator at root container
    diff_iter it = {};
    it.doc = doc_saved;
    it.oud.format = RDX_FMT_TLV;
    it.oud.next = oud_datac[0];
    it.oud.opt = (u8p)oud_datac[1];
    it.oud.bulk = NULL;
    it.neu.format = RDX_FMT_TLV;
    it.neu.next = neu_datac[0];
    it.neu.opt = (u8p)neu_datac[1];
    it.neu.bulk = NULL;
    u64gMv(it.oudhash, ((u64g)gauged(oud_hashes_data)));
    u64gMv(it.neuhash, ((u64g)gauged(neu_hashes_data)));

    // Advance to first element to check if it's a container or scalar
    it.doc_ok = rdxNext(&it.doc);
    it.oud_ok = rdxNext(&it.oud);
    it.neu_ok = rdxNext(&it.neu);

    // Check if first element is a container or scalar
    // ROOT containing scalars (e.g., "1,2,3") doesn't have a container to descend into
    b8 is_container = rdxTypePlex(&it.doc);

    if (is_container) {
        // Emit container header to patch
        patch->type = it.doc.type;
        patch->flags = it.doc.flags;
        patch->id = it.doc.id;
        call(rdxNext, patch);

        // Descend into all iterators and patch (diffInto skips OPEN hashes)
        diff_iter child = {};
        rdx cpatch = {};
        call(diffInto, &child, &it);
        call(rdxInto, &cpatch, patch);

        // Invoke appropriate scan function based on container type
        u8 ctype = it.doc.type;
        if (ctype == RDX_TYPE_EULER || ctype == RDX_TYPE_LINEAR ||
            ctype == RDX_TYPE_MULTIX) {
            call(diffScanLEX, &child, &cpatch);
        } else {
            call(diffScanOP, &child, &cpatch);
        }

        // Exit container
        call(diffOuto, &child, &it);
        call(rdxOuto, &cpatch, patch);
    } else {
        // ROOT containing scalars - process at top level using diffScanOP
        // Reset iterator state so diffScanOP's initial advance works correctly
        // We need to "un-advance" by resetting to initial position
        it.doc.type = 0;  // Signal that we need initial advance
        it.oud.type = 0;
        it.neu.type = 0;
        // Reset TLV positions to start
        it.doc.next = doc_saved.next;
        it.doc.opt = doc_saved.opt;
        it.oud.next = oud_datac[0];
        it.oud.opt = (u8p)oud_datac[1];
        it.neu.next = neu_datac[0];
        it.neu.opt = (u8p)neu_datac[1];
        it.doc_ok = OK;
        it.oud_ok = OK;
        it.neu_ok = OK;
        call(diffScanOP, &it, patch);
    }

    done;
}

// rdxDiff: diff with heap-allocated buffers
ok64 rdxDiff(rdxp patch, rdxp doc, rdxp neu) {
    sane(patch && doc && neu);

    // Estimate sizes based on input
    // For BRACKET hashes: each element -> hash + containers add OPEN/CLOSE
    // TLV elements can be as small as 2 bytes, so worst case ~buf_sz/2 elements
    // With OPEN/CLOSE markers, need up to buf_sz hashes per side
    // Compute length based on format: JDR uses next..opt, others use bulk
    u64 doc_len = (doc->format & ~RDX_FMT_WRITE) == RDX_FMT_JDR
                      ? (u64)(doc->opt - (u8p)doc->next)
                      : (u8bOK(doc->bulk) ? u8bDataLen(doc->bulk) : 0);
    u64 neu_len = (neu->format & ~RDX_FMT_WRITE) == RDX_FMT_JDR
                      ? (u64)(neu->opt - (u8p)neu->next)
                      : (u8bOK(neu->bulk) ? u8bDataLen(neu->bulk) : 0);
    u64 buf_sz = (doc_len > neu_len ? doc_len : neu_len) + PAGESIZE;
    u64 hash_sz = buf_sz;  // conservative: 1 hash entry per byte of input
    u64 work_sz = DIFFWorkSize(hash_sz, hash_sz) + 64;
    u64 edl_sz = DIFFEdlMaxEntries(hash_sz, hash_sz) + 64;

    // Allocate buffers with try-then cascade
    Bu8 oud_buf = {};
    Bu8 neu_buf = {};
    Bu64 hash_buf = {};
    Bi32 work_buf = {};
    Bu32 edl_buf = {};

    try(u8bAlloc, oud_buf, buf_sz);
    then try(u8bAlloc, neu_buf, buf_sz);
    then try(u64bAlloc, hash_buf, hash_sz * 2);
    then try(i32bAlloc, work_buf, work_sz);
    then try(u32bAlloc, edl_buf, edl_sz);
    then {
        // DIFFx.h expects gauge layout [cur, end, start]
        u32g edl = {edl_buf[1], edl_buf[3], edl_buf[1]};
        __ = rdxDiff2(patch, doc, neu,
                      u8bDataIdle(oud_buf), u8bDataIdle(neu_buf),
                      u64bDataIdle(hash_buf), i32bIdle(work_buf), edl);
    }

    // Free buffers
    u32bFree(edl_buf);
    i32bFree(work_buf);
    u64bFree(hash_buf);
    u8bFree(neu_buf);
    u8bFree(oud_buf);

    return __;
}
