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
//      6  BLOB_COMMIT     blob → commit that introduced it

#include "abc/INT.h"
#include "dog/SHA1.h"
#include "dog/WHIFF.h"
#include "keeper/KEEP.h"

con ok64 DAGFAIL    = 0x14a83ca495;
con ok64 DAGNOROOM  = 0x14a85d86d8616;

// --- belt128: 16-byte index record (two wh64 words) ---

typedef struct belt128 {
    u64 a;
    u64 b;
} belt128;

typedef belt128 const belt128c;
typedef belt128 *belt128p;
typedef belt128 const *belt128cp;
typedef belt128 const *belt128cs[2];

// Entry types (low 4 bits of .a, same position as wh64Type)
#define DAG_COMMIT_PARENT  1
#define DAG_COMMIT_TREE    2
#define DAG_COMMIT_GEN     3
#define DAG_PREV_BLOB      4
#define DAG_PATH_VER       5
#define DAG_BLOB_COMMIT    6

// belt128 a/b use the whiff layout: type[4] | id[20] | hashlet[40]
#define DAGPack    wh64Pack
#define DAGType    wh64Type
#define DAGGen     wh64Id
#define DAGHashlet wh64Off

fun belt128 DAGEntry(u8 atype, u32 agen, u64 ahash,
                     u8 btype, u32 bgen, u64 bhash) {
    return (belt128){
        .a = wh64Pack(atype, agen, ahash),
        .b = wh64Pack(btype, bgen, bhash)
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

fun void DAGHashletToHex(char *out, u64 hashlet) {
    u8s hex = {(u8p)out, (u8p)out + 10};
    WHIFFHexFeed40(hex, hashlet);
    out[10] = 0;
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

// --- LSM stack for index lookups ---

#include "abc/MSET.h"

typedef struct {
    belt128cs runs[MSET_MAX_LEVELS];
    u8bp      maps[MSET_MAX_LEVELS];
    u32       n;
} dag_stack;

ok64 dag_stack_open(dag_stack *st, u8cs dagdir);
void dag_stack_close(dag_stack *st);

//  Find first entry matching (type, hashlet) in the stack.
//  Returns NULL if not found.  Scans across type-interleaved entries.
fun belt128cp DAGLookup(dag_stack const *st, u8 type, u64 hashlet) {
    u64 key_lo = DAGPack(type, 0, hashlet);
    u64 key_hi = DAGPack(type, WHIFF_ID_MASK, hashlet);
    for (u32 r = 0; r < st->n; r++) {
        belt128cp base = st->runs[r][0];
        size_t len = (size_t)(st->runs[r][1] - base);
        size_t lo = 0, hi = len;
        while (lo < hi) {
            size_t mid = lo + (hi - lo) / 2;
            if (base[mid].a < key_lo) lo = mid + 1;
            else hi = mid;
        }
        while (lo < len && base[lo].a >= key_lo && base[lo].a <= key_hi) {
            if (DAGType(base[lo].a) == type) return &base[lo];
            lo++;
        }
    }
    return NULL;
}

//  Incremental object-graph reindex from keeper's index.
ok64 DAGHook(keeper *k, u8cs reporoot);

#endif
