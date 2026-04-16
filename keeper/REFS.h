#ifndef KEEPER_REFS_H
#define KEEPER_REFS_H

//  REFS: URI→URI append-only reflog for keeper.
//  See REF.md (next to this header) for the format spec.
//  Resolution: chase from→to iteratively (max REFS_MAX_CHAIN).
//  Compaction: collapse entries with same from-key, keep latest.

#include "abc/INT.h"
#include "abc/URI.h"
#include "abc/RON.h"
#include "abc/FILE.h"

con ok64 REFSFAIL  = 0x6ce3dc3ca495;
con ok64 REFSNONE  = 0x6ce3dc5d85ce;
con ok64 REFSBAD   = 0x1b38f70b28d;

#define REFS_FILE     "refs"
#define REFS_MAX_CHAIN 8
#define REFS_MAX_REFS  1024

// --- ref record ---

#define REF_ALIAS  1  // //name → full URL
#define REF_SHA    2  // ?refname → ?sha
#define REF_TAG    3  // tag ref
#define REF_BRANCH 4  // branch ref

typedef struct {
    ron60 time;
    u8cs  key;   // from-URI (see REF.md)
    u8cs  val;   // to-URI   (see REF.md)
    u8    type;
} ref;

typedef ref *refp;
typedef ref const *refcp;

// Typed slices for ref arrays
typedef refp refs[2];    // mutable ref slice
typedef refcp refcs[2];  // const ref slice
typedef refp *refsp;
typedef refcp *refcsp;

// Match: key equality
fun b8 REFMatch(refcp a, u8csc key) {
    return $len(a->key) == $len(key) &&
           memcmp(a->key[0], key[0], $len(key)) == 0;
}

// Compare by key (for dedup/sort)
fun int REFKeyCmp(refcp a, refcp b) {
    size_t al = $len(a->key), bl = $len(b->key);
    size_t ml = al < bl ? al : bl;
    int c = memcmp(a->key[0], b->key[0], ml);
    if (c != 0) return c;
    return al < bl ? -1 : al > bl ? 1 : 0;
}

// --- Public API ---

//  Append one from→to mapping with current timestamp.
ok64 REFSAppend(u8csc dir, u8csc from_uri, u8csc to_uri);

//  Resolve a URI by chasing from→to chain.
//  Resolves authority (alias) and query (ref) independently.
//  Result parts written into `resolved` uri struct.
//  `arena` provides scratch space for resolved strings.
ok64 REFSResolve(urip resolved, u8bp arena, u8csc dir, u8csc uri);

//  Record refs from a sync: array of ref records.
ok64 REFSSyncRecord(u8csc dir, refcp arr, u32 nrefs);

//  Load all entries into ref array (latest per key).
//  Returns count in *out_n.  Entries point into mmap (keep map alive).
ok64 REFSLoad(refp arr, u32p out_n, u32 max, u8bp *map, u8csc dir);

//  List current (latest) value for each known from-URI.
typedef ok64 (*refs_cb)(refcp r, void *ctx);
ok64 REFSEach(u8csc dir, refs_cb cb, void *ctx);

//  Compact: rewrite REFS keeping only latest entry per from-key.
ok64 REFSCompact(u8csc dir);

#endif
