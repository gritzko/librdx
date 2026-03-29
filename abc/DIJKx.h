//
// DIJKx.h - Dijkstra template
//
// Before including, define:
//   X(M, name) - name mangling macro
//   DIJK_NEXT(from, nexts, ctx) - fills nexts (kv64s) with
//       (neighbor_id, edge_cost) pairs. Returns ok64.
//
// Provides: X(DIJK, Run)(cost, src, tgt, heap, dist, ctx)
// Distances offset by +1 (val=0 means unvisited in hash).
// Node key=0 is reserved (hash empty sentinel).
//
// Requires: PRO.h included before this header (for HASHx.h).
//

#include "DIJK.h"

#ifndef ABC_DIJK_HASH
#define ABC_DIJK_HASH
#pragma push_macro("X")
#undef X
#define X(M, name) M##kv64##name
#include "HASHx.h"
#undef X
#pragma pop_macro("X")
#endif

// Run Dijkstra from src to tgt.
//   heap: kv64 buffer for the priority queue (caller provides)
//   dist: kv64 hash table, must be zeroed (caller provides, sized to taste)
//   result: *cost receives the shortest path cost, or DIJKNOPATH if unreachable
static inline ok64 X(DIJK, Run)(u64 *cost, u64 src, u64 tgt,
                                kv64bp heap, kv64s dist, void *ctx) {
    ok64 o;

    // seed: distance 1 means cost 0 (offset by 1)
    kv64 start = {src, 1};
    o = HASHkv64put(dist, &start);
    if (o != OK) return o;
    o = HEAPkv64Push1Z(heap, start, kv64Zval);
    if (o != OK) return o;

    // neighbor scratch space on stack
    kv64 nbuf[16];
    kv64s nexts = {nbuf, nbuf + 16};

    while (!Bempty(heap)) {
        kv64 cur;
        o = HEAPkv64PopZ(&cur, heap, kv64Zval);
        if (o != OK) return o;

        // check if stale
        kv64 probe = {cur.key, 0};
        o = HASHkv64get(&probe, dist);
        if (o != OK) return o;
        if (cur.val > probe.val) continue;

        // reached target
        if (cur.key == tgt) {
            *cost = cur.val - 1;  // undo +1 offset
            return OK;
        }

        // enumerate neighbors
        nexts[0] = nbuf;
        o = DIJK_NEXT(cur, nexts, ctx);
        if (o != OK) return o;

        kv64cs ns = {nbuf, nexts[0]};
        $for(kv64c, np, ns) {
            u64 ncost = cur.val + np->val;
            kv64 nprobe = {np->key, 0};
            o = HASHkv64get(&nprobe, dist);
            if (o == HASHNONE || ncost < nprobe.val) {
                kv64 entry = {np->key, ncost};
                o = HASHkv64put(dist, &entry);
                if (o != OK) return o;
                o = HEAPkv64Push1Z(heap, entry, kv64Zval);
                if (o != OK) return o;
            }
        }
    }

    return DIJKNOPATH;
}
