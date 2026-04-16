#ifndef XX_BELT_H
#define XX_BELT_H

//  BELT: ersatz git-compatible repository format
//
//  .belt/objects.log   — append-only packfile stream
//  .belt/objects.idx/  — LSM index (sorted belt128 runs)
//  .belt/HEAD          — current head (hex SHA-1)
//
//  Index entry (belt128 = 16 bytes):
//    a = (hashlet << 4) | obj_type     "address" / identity
//    b = (offset  << 4) | flags        "body"    / location
//  obj_type: 1=commit, 2=tree, 3=blob, 4=tag, 0=segment boundary
//  Sorted by (a, b) — full 128-bit ordering.

#include "abc/INT.h"

con ok64 BELTFAIL   = 0x2ce55d3ca495;
con ok64 BELTBADFMT = 0x2ce55d2ca34f59d;
con ok64 BELTNONE   = 0x2ce55d5d85ce;

// --- belt128: the index record ---

typedef struct belt128 {
    u64 a;  // (hashlet << 4) | type
    u64 b;  // (offset  << 4) | flags
} belt128;

typedef belt128 const belt128c;
typedef belt128 *belt128p;
typedef belt128 const *belt128cp;

// Comparator: sort by a, break ties by b
fun int belt128cmp(belt128cp x, belt128cp y) {
    if (x->a < y->a) return -1;
    if (x->a > y->a) return 1;
    if (x->b < y->b) return -1;
    if (x->b > y->b) return 1;
    return 0;
}

// Less-than for HIT/MSET
fun b8 belt128Z(belt128cp x, belt128cp y) {
    return x->a < y->a || (x->a == y->a && x->b < y->b);
}

// Core slice types (hand-declared to avoid Sx.h pollution)
typedef belt128 *belt128s[2];
typedef belt128 const *belt128cs[2];
typedef belt128 const *const belt128csc[2];
typedef belt128cs *belt128css[2];

// Object types (low 4 bits of .a)
#define BELT_COMMIT  1
#define BELT_TREE    2
#define BELT_BLOB    3
#define BELT_TAG     4
#define BELT_SEG     0  // pack segment boundary

#define BELT_TYPE_MASK 0xfULL
#define BELT_GEN_SHIFT 44
#define BELT_OFF_BITS  40
#define BELT_OFF_MASK  (((u64)1 << BELT_OFF_BITS) - 1)
#define BELT_GEN_BITS  20
#define BELT_GEN_MASK  (((u64)1 << BELT_GEN_BITS) - 1)
#define BELT_IDX_EXT ".idx"
#define BELT_SEQNO_WIDTH 10
#define BELT_MAX_LEVELS 128

// --- Accessors ---

fun belt128 BELTEntry(u64 hashlet, u8 type, u64 offset, u32 gen) {
    return (belt128){
        .a = (hashlet & ~BELT_TYPE_MASK) | (type & BELT_TYPE_MASK),
        .b = (((u64)(gen & BELT_GEN_MASK)) << BELT_GEN_SHIFT) |
             ((offset & BELT_OFF_MASK) << 4)
    };
}

fun u64 BELTHashlet(belt128 e) { return e.a & ~BELT_TYPE_MASK; }
fun u8  BELTType(belt128 e)    { return (u8)(e.a & BELT_TYPE_MASK); }
fun u64 BELTOffset(belt128 e)  { return (e.b >> 4) & BELT_OFF_MASK; }
fun u32 BELTGen(belt128 e)     { return (u32)((e.b >> BELT_GEN_SHIFT) & BELT_GEN_MASK); }

// Truncate SHA-1 to hashlet, combine with type
fun u64 BELTSha1Key(u8cp sha, u8 type) {
    u64 h = 0;
    memcpy(&h, sha, 8);
    return (h & ~BELT_TYPE_MASK) | (type & BELT_TYPE_MASK);
}

// --- API ---

ok64 BELTClone(u8cs repo_path, u8cs belt_dir);
ok64 BELTImport(u8cs pack_path, u8cs belt_dir);
ok64 BELTGet(u8cs belt_dir, u8cs hex_hash, u8g out, u8p out_type);
ok64 BELTIndexWrite(u8cs idx_dir, belt128cs run, u64 seqno);
ok64 BELTNextSeqno(u64p seqno, u8cs idx_dir);

//  Lookup by hashlet in sorted run stack (ignores type bits).
belt128cp BELTLookup(belt128css stack, u64 hashlet);

//  Compute generation number for a commit from its content.
//  gen = 1 + max(parent gens).  Root commits get gen=1.
u32 BELTCommitGen(u8cp content, u64 sz, belt128css stack);

//  Resolve object at `off` in pack to final content.
//  Chases OFS_DELTA/REF_DELTA chains via `stack` lookups.
//  Writes base type to `out_type`, content pointer to `result`.
ok64 BELTResolve(u8cp pack, u64 packlen, belt128css stack,
                 u64 off, u8p out_type,
                 u8p buf1, u8p buf2, u64 bufsz,
                 u8pp result, u64p outsz);

#endif
