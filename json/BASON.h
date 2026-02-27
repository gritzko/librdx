#ifndef LIBRDX_BASON_H
#define LIBRDX_BASON_H

#include "TLKV.h"
#include "abc/INT.h"

con ok64 BASONEND = 0x1c5584dcf0;
con ok64 BASONBAD = 0x1c5584de8d;

// Container types: Array and Object
fun b8 BASONPlex(u8 type) { return type == 'A' || type == 'O'; }

// --- API A: stateless ---
// Stack holds offsets into data, sorted descending:
//   [end_outermost, ..., end_innermost, cursor]
// Data buffer is never modified.

// Push initial frame [data_len, 0] onto stack.
ok64 BASONOpen(u64bp stack, u8csc data);

// Read next TLKV record. Returns BASONEND when level exhausted.
ok64 BASONNext(u64bp stack, u8csc data, u8p type, u8cs key, u8cs val);

// Descend into container children. Push val start offset.
ok64 BASONInto(u64bp stack, u8csc data, u8csc val);

// Ascend from container. Pop cursor, parent resumes.
ok64 BASONOuto(u64bp stack);

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

#endif
