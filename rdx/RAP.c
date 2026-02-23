//
// RAP.c - Compute path-dependent u64 hashes for RDX tree
//
// Scans in pre-order depth-first, outputs hash per element.
// Each hash depends on element data + ancestor hashes (as seed).
//
// Hash semantics:
//   FIRST elements: seed + type + id + value
//   LEX containers (LINEAR, EULER, MULTIX): seed + type + id
//   TUPLE: seed + type + id + key_hash (key uses seed 0)
//
// Three modes:
//   0 (FIRST)    - only FIRST elements, plus empty non-root containers
//   1 (PREORDER) - PLEX hash before contents (standard pre-order)
//   2 (BRACKET)  - PLEX hash before AND after contents
//

#include "RDX.h"
#include "abc/PRO.h"
#include "abc/RAP.h"

// Callback for chained rapid hashing
fun ok64 RapidCB(u8cs chunk, voidp ctx) {
    u64* h = ctx;
    *h = RAPHashSeed(chunk, *h);
    return OK;
}

// Hash an element given seed (parent hash)
// FIRST: seed + type + id + value
// LEX (LINEAR, EULER, MULTIX): seed + type + id
// TUPLE: seed + type + id + key_hash (key uses seed 0)
fun u64 rdxRapidHashes1(rdxp x, u64 seed) {
    a_rawc(rtype, x->type);
    a_rawc(rid, x->id);
    u64 h = RAPHashSeed(rtype, seed);
    h = RAPHashSeed(rid, h);

    switch (x->type) {
        // FIRST elements: include value
        case RDX_TYPE_INT:
        case RDX_TYPE_FLOAT: {
            a_rawc(rval, x->f);
            return RAPHashSeed(rval, h);
        }
        case RDX_TYPE_REF: {
            a_rawc(rval, x->r);
            return RAPHashSeed(rval, h);
        }
        case RDX_TYPE_STRING:
        case RDX_TYPE_TERM:
            if ((x->flags & RDX_UTF_ENC_BITS) == RDX_UTF_ENC_UTF8) {
                return RAPHashSeed(x->s, h);
            } else {
                UTFRecodeCB(x->s, x->flags & RDX_UTF_ENC_BITS, UTF8_DECODER_ALL,
                            RapidCB, &h);
                return h;
            }
        // TUPLE: include key hash (key uses seed 0)
        case RDX_TYPE_TUPLE: {
            rdx key = {};
            if (rdxInto(&key, x) == OK && rdxNext(&key) == OK) {
                u64 key_hash = rdxRapidHashes1(&key, 0);  // key uses seed 0
                a_rawc(rkey, key_hash);
                return RAPHashSeed(rkey, h);
            }
            return h;
        }
        // LEX containers (LINEAR, EULER, MULTIX): just seed + type + id
        default:
            return h;
    }
}

// Hash one element and recurse into children
// flags: 0=FIRST only, 1=PREORDER, 2=BRACKET
ok64 rdxRapidHashes1rec(rdxp from, u64s hashes, u64b ancestors, u8 flags) {
    sane(from && u64sOK(hashes) && u64bOK(ancestors));

    // Get parent hash (seed) from ancestor stack, or 0 for root
    u64 seed = u64bDataLen(ancestors) > 0 ? *u64bLast(ancestors) : 0;
    u64 depth = u64bDataLen(ancestors);

    // Compute this element's hash
    u64 h = rdxRapidHashes1(from, seed);

    // Handle PLEX containers
    if (rdxTypePlex(from)) {
        // Output hash based on mode
        // Mode 0: skip PLEX hashes (FIRST-only)
        // Mode 1/2: output PLEX open hash (with marker + depth)
        if (flags == RDX_HASH_PREORDER || flags == RDX_HASH_BRACKET) {
            call(u64sFeed1, hashes, RDX_HASH_AS_OPEN(h, depth));
        }

        // Compute seed for children
        // For root TUPLE, use type+id only (without key) so children
        // are compared position-independently in diff
        u64 child_seed;
        if (from->type == RDX_TYPE_TUPLE && from->ptype == RDX_TYPE_ROOT) {
            a_rawc(rtype, from->type);
            a_rawc(rid, from->id);
            child_seed = RAPHashSeed(rtype, seed);
            child_seed = RAPHashSeed(rid, child_seed);
        } else {
            child_seed = h;
        }
        u64bFeed1(ancestors, child_seed);

        // Recurse into children
        rdx child = {};
        call(rdxInto, &child, from);
        b8 has_children = NO;
        scan(rdxNext, &child) {
            has_children = YES;
            call(rdxRapidHashes1rec, &child, hashes, ancestors, flags);
        }
        seen(END);

        // Mode-specific post-processing
        if (flags == RDX_HASH_FIRST && !has_children && from->ptype != RDX_TYPE_ROOT) {
            // Mode 0: output hash for empty non-root containers (with marker + depth)
            call(u64sFeed1, hashes, RDX_HASH_AS_OPEN(h, depth));
        }
        if (flags == RDX_HASH_BRACKET) {
            // Mode 2: output close hash (with marker + depth)
            call(u64sFeed1, hashes, RDX_HASH_AS_CLOSE(h, depth));
        }

        call(rdxOuto, &child, from);
        call(u64bPop, ancestors);
    } else {
        // FIRST element - always output (with marker + depth)
        call(u64sFeed1, hashes, RDX_HASH_AS_FIRST(h, depth));
    }

    done;
}

// Scan RDX tree, output hashes based on mode
// flags: 0=FIRST only, 1=PREORDER, 2=BRACKET
ok64 rdxRapidHashesF(u64s hashes, rdxp from, u8 flags) {
    sane(u64sOK(hashes) && from);
    a_pad(u64, anc, RDX_MAX_NESTING);
    scan(rdxNext, from) {
        call(rdxRapidHashes1rec, from, hashes, anc, flags);
    }
    seen(END);
    done;
}

// Scan RDX tree, output hashes (default: FIRST only)
ok64 rdxRapidHashes(u64s hashes, rdxp from) {
    return rdxRapidHashesF(hashes, from, RDX_HASH_FIRST);
}
