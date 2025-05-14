#ifndef BRIX_TAG_H
#define BRIX_TAG_H

#include "abc/NACL.h"
#include "abc/KV.h"
#include "rdx/RDX.h"

// a tagged/signed version
typedef struct {
    u64 prev;
    u64 ver;
    id128 point;
    sha256 hash;
    edsig512 sig;
} tag1024;

fun u64 tag1024hash(tag1024 const *v) { 
    return mix128(v->point); 
}

fun u64 tag1024cmp(tag1024 const *a, tag1024 const *b) { 
    return u128cmp(&a->point, &b->point);
}

#define X(M, name) M##tag1024##name
#define ABC_HASH_CONVERGE 1
#include "abc/Bx.h"
#include "abc/HASHx.h"
#undef X

typedef Btag1024 TAG;

#endif
