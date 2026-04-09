#ifndef GRAF_DAG_H
#define GRAF_DAG_H

//  DAG: graf's git object-graph index.
//
//  An LSM-style index of belt128 records (16 bytes each) covering
//  commit parentage, commit→tree, blob→prev_blob, and path→blob
//  mappings.  Kept under <reporoot>/.dogs/graf/.  Used by graf to
//  walk history (merge-base, ancestor, file lineage) cheaply during
//  diff and merge.  Stores no object content.
//
//  Layout:
//      .dogs/graf/0000000001.idx   sorted belt128 runs (LSM)
//      .dogs/graf/COMMIT           past HEAD shas, oldest first
//      .dogs/graf/PATHS            append-only NUL-separated path log
//
//  Entry format (belt128 = 2 × u64):
//      a = type[4] | gen[20] | hashlet[40]   (or path_id for PATH_VER)
//      b = type[4] | gen[20] | hashlet[40]
//
//  Entry types (low 4 bits of .a):
//      1  COMMIT_PARENT   commit → parent commit
//      2  COMMIT_TREE     commit → root tree hashlet
//      3  COMMIT_GEN      commit → self (gen is the payload)
//      4  PREV_BLOB       blob → predecessor blob
//      5  PATH_VER        (gen, path_id) → (gen, blob)

#include "abc/INT.h"

con ok64 DAGFAIL    = 0x14a83ca495;
con ok64 DAGNOROOM  = 0x14a85d86d8616;

// --- belt128: 16-byte index record ---

typedef struct belt128 {
    u64 a;
    u64 b;
} belt128;

typedef belt128 const belt128c;
typedef belt128 *belt128p;
typedef belt128 const *belt128cp;

// Entry types (low 4 bits of .a)
#define DAG_COMMIT_PARENT  1
#define DAG_COMMIT_TREE    2
#define DAG_COMMIT_GEN     3
#define DAG_PREV_BLOB      4
#define DAG_PATH_VER       5

#define DAG_TYPE_MASK     0xfULL
#define DAG_GEN_SHIFT     40
#define DAG_GEN_BITS      20
#define DAG_GEN_MASK      ((1ULL << DAG_GEN_BITS) - 1)
#define DAG_HASH_BITS     40
#define DAG_HASH_MASK     ((1ULL << DAG_HASH_BITS) - 1)

// --- Pack/unpack ---

fun u64 DAGPack(u8 type, u32 gen, u64 hashlet) {
    return ((u64)type & DAG_TYPE_MASK) |
           (((u64)gen & DAG_GEN_MASK) << DAG_GEN_SHIFT) |
           ((hashlet & DAG_HASH_MASK) << 4);
}

fun u8  DAGType(u64 v)    { return (u8)(v & DAG_TYPE_MASK); }
fun u32 DAGGen(u64 v)     { return (u32)((v >> DAG_GEN_SHIFT) & DAG_GEN_MASK); }
fun u64 DAGHashlet(u64 v) { return (v >> 4) & DAG_HASH_MASK; }

fun belt128 DAGEntry(u8 atype, u32 agen, u64 ahash,
                     u8 btype, u32 bgen, u64 bhash) {
    return (belt128){
        .a = DAGPack(atype, agen, ahash),
        .b = DAGPack(btype, bgen, bhash)
    };
}

// --- Comparator: sort by (a, b) ---

fun int belt128cmp(belt128cp x, belt128cp y) {
    if (x->a < y->a) return -1;
    if (x->a > y->a) return 1;
    if (x->b < y->b) return -1;
    if (x->b > y->b) return 1;
    return 0;
}

fun b8 belt128Z(belt128cp x, belt128cp y) {
    return x->a < y->a || (x->a == y->a && x->b < y->b);
}

//  Incremental object-graph reindex.
ok64 DAGHook(u8cs reporoot);

#endif
