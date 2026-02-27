#ifndef LIBRDX_BASON_H
#define LIBRDX_BASON_H

#include "TLKV.h"
#include "abc/INT.h"

con ok64 BASONEND = 0x1c5584dcf0;
con ok64 BASONBAD = 0x1c5584de8d;

// Container types: Array and Object
fun b8 BASONPlex(u8 type) { return type == 'A' || type == 'O'; }

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
ok64 BASONNext(u64bp stack, u8csc data, u8p type, u8cs key, u8cs val);
ok64 BASONInto(u64bp stack, u8csc data, u8csc val);
ok64 BASONOuto(u64bp stack);

// Seek within current level using page index.
// Must be called right after BASONInto (cursor == children_start).
ok64 BASONSeek(u64bp stack, u8csc data, u8csc target);

// --- Write helpers: TLKV + page index ---
// idx may be NULL (no index written).

ok64 BASONwInto(u64bp idx, u8bp buf, u8 type, u8csc key);
ok64 BASONwFeed(u64bp idx, u8bp buf, u8 type, u8csc key, u8csc val);
ok64 BASONwOuto(u64bp idx, u8bp buf);

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

fun ok64 basonNext(basonp x) {
    x->ptype = x->type;
    u8cs d = {x->data[1], x->data[2]};
    return BASONNext(x->stack, d, &x->type, x->key, x->val);
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

#endif
