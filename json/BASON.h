#ifndef LIBRDX_BASON_H
#define LIBRDX_BASON_H

#include "TLKV.h"
#include "abc/INT.h"

con ok64 BASONEND = 0x1c5584dcf0;
con ok64 BASONBAD = 0x1c5584de8d;

// Container types: vowels are containers, consonants are leaves
fun b8 BASONPlex(u8 type) {
    return type == 'A' || type == 'E' || type == 'I' ||
           type == 'O' || type == 'U' || type == 'Y';
}

// Page size for index sampling (one entry per page)
#define BASON_PAGE 512
// Index record TLV tag
#define BASON_IDX 'X'

// Pack 6-byte key prefix into u64 (big-endian for lexicographic compare).
// Low 16 bits reserved for page-local offset when packed as index entry.
fun u64 BASONKeyPrefix(u8csc key) {
    u64 pfx = 0;
    size_t n = $len(key);
    if (n > 6) n = 6;
    for (size_t i = 0; i < n; i++)
        pfx = (pfx << 8) | key[0][i];
    pfx <<= (6 - n) * 8;
    return pfx;
}

// --- Stack layout ---
// [end₀, end₁, ..., endₙ, cursor]
// Open pushes [data_len, 0].  Into overwrites cursor with end, pushes new cursor.
// Outo pops cursor; the end below becomes the new cursor.

// --- API A: stateless ---
// Stack holds u64 offsets into data buffer.
// Data buffer is never modified by the reader.

ok64 BASONOpen(u64bp stack, u8csc data);
ok64 BASONDrain(u64bp stack, u8csc data, u8p type, u8cs key, u8cs val);
ok64 BASONInto(u64bp stack, u8csc data, u8csc val);
ok64 BASONOuto(u64bp stack);

// Seek within current level using page index.
// Must be called right after BASONInto (cursor == children_start).
ok64 BASONSeek(u64bp stack, u8csc data, u8csc target);

// --- Write helpers: TLKV + page index ---
// idx may be NULL (no index written).

ok64 BASONFeedInto(u64bp idx, u8bp buf, u8 type, u8csc key);
ok64 BASONFeed(u64bp idx, u8bp buf, u8 type, u8csc key, u8csc val);
ok64 BASONFeedOuto(u64bp idx, u8bp buf);

// Sort TLKV children by key inside an open container (between TLKVInto and
// TLKVOuto).  Uses idle space as scratch.  Invalidates idx entries if non-NULL.
ok64 BASONSortChildren(u8bp buf, u64bp idx);

// --- Array key helpers ---

// Minimum RON64 width to represent n sequential keys (0..n-1).
fun u8 BASONKeyWidth(u64 n) {
    u8 w = 1;
    u64 cap = 64;
    while (cap < n && w < 10) { cap *= 64; w++; }
    return w;
}

// Increment a RON64 bigint key, feed result into slice.
// Output has same width as orig. Fails on overflow (all '~').
ok64 BASONFeedInc(u8s into, u8cs orig);

// Compute a midpoint key between left and right for array splicing.
// Ensures room for len insertions with collision probability 1/pcoll.
// Empty left/right means no bound (start/end of keyspace).
ok64 BASONFindMid(u8s into, u8cs left, u8cs right,
                  u64 len, u64 pcoll, u64 random);

// Incremental key generator for sequential parsing (unknown count).
// First digit determines increment position P via width thresholds.
// Subsequent digits use full 0..63 range. Empty prev produces "0".
ok64 BASONFeedInfInc(u8s into, u8cs prev);

// --- JSON ↔ BASON ---
ok64 BASONParseJSON(u8bp buf, u64bp idx, u8cs json);
ok64 BASONExportJSON(u8s out, u64bp stack, u8csc data);

// --- API B: cursor struct, delegates to API A ---

typedef struct bason {
    u8    type;
    u8    ptype;
    u8bp  data;
    u64bp stack;
    u8cs  key;
    u8cs  val;
} bason;

typedef bason* basonp;

fun ok64 basonOpen(basonp x) {
    u8cs d = {x->data[1], x->data[2]};
    return BASONOpen(x->stack, d);
}

fun ok64 basonDrain(basonp x) {
    x->ptype = x->type;
    u8cs d = {x->data[1], x->data[2]};
    return BASONDrain(x->stack, d, &x->type, x->key, x->val);
}

fun ok64 basonInto(basonp x) {
    if (!BASONPlex(x->type)) return BASONBAD;
    u8cs d = {x->data[1], x->data[2]};
    ok64 o = BASONInto(x->stack, d, x->val);
    if (o == OK) x->ptype = x->type;
    return o;
}

fun ok64 basonOuto(basonp x) {
    return BASONOuto(x->stack);
}

fun ok64 basonSeek(basonp x, u8csc target) {
    u8cs d = {x->data[1], x->data[2]};
    return BASONSeek(x->stack, d, target);
}

// --- Write path wrappers ---

fun ok64 basonFeedInto(basonp x, u8 type, u8csc key) {
    return BASONFeedInto(x->stack, x->data, type, key);
}

fun ok64 basonFeed(basonp x, u8 type, u8csc key, u8csc val) {
    return BASONFeed(x->stack, x->data, type, key, val);
}

fun ok64 basonFeedOuto(basonp x) {
    return BASONFeedOuto(x->stack, x->data);
}

fun ok64 basonParseJSON(basonp x, u8cs json) {
    return BASONParseJSON(x->data, x->stack, json);
}

#endif
