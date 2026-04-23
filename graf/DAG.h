#ifndef GRAF_DAG_H
#define GRAF_DAG_H

//  DAG: graf's object-graph index (event log of path touches).
//
//  An LSM-style index of wh128 records (16 bytes each) covering
//  commit parentage, commit→tree, and path-touch events.  Kept
//  under <reporoot>/.dogs/graf/.  Used by graf to walk history
//  and flag gens where a path may have changed.  Stores no
//  content; actual blobs are retrieved via keeper using the
//  full path at query time.
//
//  Layout:
//      .dogs/graf/0000000001.idx   sorted wh128 runs (LSM)
//      .dogs/graf/COMMIT           last-seen ref tips, oldest first
//
//  Entry format (wh128 = 2 × wh64 = 16 bytes):
//      a = type[4] | gen[20] | hashlet[40]
//      b = type[4] | gen[20] | hashlet[40]
//
//  Entry types (low 4 bits of .a.type):
//      1  COMMIT_GEN     self-entry, gen → commit
//                        a = (1, gen, commit_h), b = (1, gen, commit_h)
//      2  COMMIT_PARENT  commit → parent commit
//                        a = (2, gen, commit_h), b = (2, pgen, parent_h)
//      3  COMMIT_TREE    commit → root tree hashlet
//                        a = (3, gen, commit_h), b = (3, gen, tree_h)
//      4  PATH_VER       path touched at a gen by a commit
//                        a = (4, gen, path_hashlet), b = (4, gen, commit_h)
//
//  path_hashlet = RAPHash(full_path) & WHIFF_OFF_MASK (40 bits).
//  Collisions possible; query-side verifies via keeper tree-walk.

#include "abc/INT.h"
#include "dog/SHA1.h"
#include "dog/WHIFF.h"
#include "keeper/KEEP.h"

con ok64 DAGFAIL     = 0xd2903ca495;
con ok64 DAGNOROOM   = 0xd2905d86d8616;
con ok64 DAGNOPATH   = 0xd29093a0586;

// --- Entry types ---

#define DAG_COMMIT_GEN     1
#define DAG_COMMIT_PARENT  2
#define DAG_COMMIT_TREE    3
#define DAG_PATH_VER       4

// Reserved gen value used to mark "fresh-in-pack blob" in the
// transient owner map.  Real gens start at 1; 0 is safe.
#define DAG_GEN_FRESH      0

// wh128 a/b use the wh64 layout: hashlet[40] | id[20] | type[4]
// (type in low bits; hashlet in high bits — see dog/WHIFF.h).
#define DAGPack    wh64Pack
#define DAGType    wh64Type
#define DAGGen     wh64Id
#define DAGHashlet wh64Off

fun wh128 DAGEntry(u8 atype, u32 agen, u64 ahash,
                   u8 btype, u32 bgen, u64 bhash) {
    return (wh128){
        .key = wh64Pack(atype, agen, ahash),
        .val = wh64Pack(btype, bgen, bhash),
    };
}

// --- sha1 helpers ---

fun u64 DAGsha1Hashlet(sha1 const *s) {
    return WHIFFHashlet40(s);
}

fun ok64 DAGsha1FromHex(sha1 *out, char const *hex40) {
    u8s sb = {out->data, out->data + 20};
    u8cs hx = {(u8cp)hex40, (u8cp)hex40 + 40};
    return HEXu8sDrainSome(sb, hx);
}

fun void DAGsha1ToHex(char *hex41, sha1 const *s) {
    u8 buf[41];
    u8s hx = {buf, buf + 40};
    u8cs bn = {s->data, s->data + 20};
    HEXu8sFeedSome(hx, bn);
    memcpy(hex41, buf, 40);
    hex41[40] = 0;
}

// --- LSM stack for index lookups ---

#include "abc/MSET.h"

typedef struct {
    wh128cs runs[MSET_MAX_LEVELS];
    u8bp    maps[MSET_MAX_LEVELS];
    u32     n;
} dag_stack;

ok64 dag_stack_open(dag_stack *st, u8cs dagdir);
void dag_stack_close(dag_stack *st);

//  Find first entry matching (type, hashlet) in the stack.
//  Returns NULL if not found.  Scans across type-interleaved entries.
fun wh128cp DAGLookup(dag_stack const *st, u8 type, u64 hashlet) {
    u64 key_lo = DAGPack(type, 0, hashlet);
    u64 key_hi = DAGPack(type, WHIFF_ID_MASK, hashlet);
    for (u32 r = 0; r < st->n; r++) {
        wh128cp base = st->runs[r][0];
        size_t len = (size_t)(st->runs[r][1] - base);
        size_t lo = 0, hi = len;
        while (lo < hi) {
            size_t mid = lo + (hi - lo) / 2;
            if (base[mid].key < key_lo) lo = mid + 1;
            else hi = mid;
        }
        while (lo < len && base[lo].key >= key_lo && base[lo].key <= key_hi) {
            if (DAGType(base[lo].key) == type) return &base[lo];
            lo++;
        }
    }
    return NULL;
}

// ==========================================================
// Graph-navigation primitives
// ==========================================================

//  Generation number of a commit.  0 if not indexed.
fun u32 DAGCommitGen(dag_stack const *idx, u64 commit_h) {
    wh128cp rec = DAGLookup(idx, DAG_COMMIT_GEN, commit_h);
    return rec ? DAGGen(rec->key) : 0;
}

//  Root-tree hashlet of a commit.  0 if not indexed.
fun u64 DAGCommitTree(dag_stack const *idx, u64 commit_h) {
    wh128cp rec = DAGLookup(idx, DAG_COMMIT_TREE, commit_h);
    return rec ? DAGHashlet(rec->val) : 0;
}

//  Collect parent hashlets of a commit into out[0..cap).  Returns the
//  total number of parents found; only the first min(count, cap) are
//  written.  Root commits return 0.
u32 DAGParents(dag_stack const *idx, u64 commit_h, u64 *out, u32 cap);

//  BFS from `tip` over COMMIT_PARENT edges; populate `set` with all
//  reachable commit hashlets (tip included).  `cutoff_gen` prunes the
//  descent: commits with gen < cutoff_gen are admitted to the set but
//  not expanded further.  Pass 0 for full walk.
//  `set` must be a pre-allocated, power-of-two-sized Bwh128.  Pass
//  tip=0 for a no-op (leaves set untouched).
ok64 DAGAncestors(Bwh128 set, dag_stack const *idx,
                  u64 tip, u32 cutoff_gen);

//  Union of DAGAncestors across `n` tips into `set`.  Each tip is
//  walked independently; duplicates collapse on the common set.
ok64 DAGAncestorsOfMany(Bwh128 set, dag_stack const *idx,
                        u64 const *tips, u32 n);

//  Membership check on a set populated by DAGAncestors.
b8 DAGAncestorsHas(Bwh128 set, u64 commit_h);

// --- hashlet width bridging ---
//
//  graf stores 40-bit hashlets (top 40 bits of SHA-1); keeper stores
//  60-bit hashlets (top 60 bits) in its LSM keys.  To resolve a graf
//  hashlet in keeper, left-align into the 60-bit space and do a
//  40-bit prefix match (hexlen = 10).  For small repos 40-bit
//  collisions are vanishingly rare; the caller further narrows by
//  checking obj_type.

#define DAG_H40_HEXLEN 10

fun u64 DAGh40ToKeeperPrefix(u64 h40) { return h40 << 20; }

// --- Per-path version walk ---
//
//  One PATH_VER hit: a commit at `gen` touched a path whose 40-bit
//  hashlet matches the caller's `path_h40`.  Used by blame and
//  GRAFGet to enumerate a file's history.

typedef struct {
    u64 commit_hashlet;
    u32 gen;
} graf_pathver;

//  Scan every PATH_VER record whose key's hashlet matches
//  `RAPHash(filepath) & WHIFF_OFF_MASK`.  When `ancestors` is
//  populated (i.e. head != term), commits outside the set are
//  dropped.  Empty `ancestors` = no filter.  Results land in `out`
//  newest-first (gen descending), capped at `maxvers`.  Returns the
//  number written.
u32 DAGPathVers(graf_pathver *out, u32 maxvers,
                dag_stack const *idx,
                u8cs filepath,
                Bwh128 ancestors);

#endif
