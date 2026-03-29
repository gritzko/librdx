//
// DIJK - Dijkstra shortest path on implicit graphs
//
// Uses kv64 HEAP (min by val=cost) and kv64 HASH (keyed by node id).
// Distances are offset by +1 so that val=0 means "unvisited" in the hash.
//

#ifndef ABC_DIJK_H
#define ABC_DIJK_H

#include "HASH.h"
#include "KV.h"

con ok64 DIJKNOROOM = 0x4c2d1d86d8616;
con ok64 DIJKNOPATH = 0x4c2d1db0a7c4;

fun b8 kv64Zval(kv64cp a, kv64cp b) { return a->val < b->val; }

#define X(M, name) M##kv64##name
#include "HEAPx.h"
#undef X

#endif
