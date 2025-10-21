#ifndef RDX_BRIX_H
#define RDX_BRIX_H

#include "abc/KV.h"
#include "abc/NACL.h"
#include "abc/SHA.h"
#include "abc/SST.h"
#include "rdx/RDX.h"

con ok64 BRIXnone = 0x2db4a1cb3ca9;
con ok64 BRIXbadarg = 0x2db4a19a5a25dab;
con ok64 BRIXnoverb = 0x2db4a1cb3ea9da6;
con ok64 BRIXnotime = 0x2db4a1cb3e2dc69;
con ok64 BRIXmiss = 0x2db4a1c6ddf7;
con ok64 BRIXbad = 0xb6d2866968;

#define BRIX_MAX_SST0_SIZE (1 << 30)
#define BRIX_MAX_SST0_ENTRIES (1 << 20)

enum {
    BRIX_BRANCH_HEAD = 1UL << 60,
};

// a 60 bit hashlet is expected to be unique *within a repo*
typedef u64 h60;

#define u128pack ZINTu128feed
#define u128unpack ZINTu128drain

#define X(M, name) M##kv64##name
#define ABC_HASH_CONVERGE 1
#include "abc/HASHx.h"
#undef X

#define X(M, name) M##u8##name
#include "abc/SKIPx.h"
#undef X

#define X(M, name) M##u128##name
#include "abc/SSTx.h"
#undef X

#define X(M, name) M##edsig512##name
#include "abc/Bx.h"
#undef X

#define X(M, name) M##edpub256##name
#include "abc/Bx.h"
#undef X

extern u8cs BRIKext;
extern u8cs BRIXdir;
extern u8cs BRIXindex;

/** BRIX repo structure:

  - `.rdx/e3a57c59...91e7c7f77.brik`

  The struct is not thread safe as all the storage is memory-mapped,
  hence can be shared between processes. For multi-thread use, use
  locks or something.
*/
typedef struct {
    // The path where the bricks are
    u8cs home;
    // Open bricks, a buffer of buffers (mmaped SSTs)
    Bu8B ssts;
    // Brick hashes
    Bsha256 shas;
    // Scratch pad;
    u8B pad;
} BRIX;

fun b8 BRIXok(BRIX const* brix) {
    return brix != NULL && Bok(brix->ssts) && $ok(brix->home);
}

// close everything previously open, then
// add an SST to the stack, including its dependencies.
ok64 BRIXopenrepo(BRIX* brix, u8cs path);

// open an SST file, including all dependencies (close everything first)
ok64 BRIXopen(BRIX* brix, sha256c* sha);

// add an SST to the stack, including *missing* dependencies
ok64 BRIXadd(BRIX* brix, sha256c* sha);

// @return OK, BRIXnone
ok64 BRIXhave(BRIX const* brix, sha256c* id);

// add SSTs to the stack, including dependencies
fun ok64 BRIXaddall(BRIX* brix, $csha256c heads) {
    ok64 o = OK;
    a$dup(sha256c, dup, heads);
    while (!$empty(dup) && o == OK) o = BRIXadd(brix, $next(dup));
    return o;
}

// length of the stack
fun size_t BRIXlen(BRIX const* brix) { return Bdatalen(brix->ssts); }

// close the added SSTs
ok64 BRIXcloseadded(BRIX* brix);

// close all SSTs
ok64 BRIXcloseall(BRIX* brix);

// close the repo
ok64 BRIXcloserepo(BRIX* brix);

// Merge the added SSTs, so the newly formed SST replaces them.
ok64 BRIXmerge(sha256* newsha, BRIX* brix);

// Get a record (TLKV, ZINT u128 key, RDX body).
//   - `rdt` the expected RDX type; 0 for any
//   - `rec` used for output;
ok64 BRIXget($u8 rec, BRIX const* brix, u8c rdt, id128 key);

// Get a record (TLKV, ZINT u128 key, RDX body).
//   - `rdt` the expected RDX type; 0 for any
//   -  `rec` will point to the original SST record or
//      to the tmp pad if any merging was necessary.
ok64 BRIXgetc(u8c$ rec, BRIX const* brix, u8c rdt, id128 key);

// Same as BRIXget, but recursively produces a document following all
// the references, e.g. `{@rec-1 1 2 [@rec-3] }` rec-1 refers to rec-3
// as an element. if `[@rec-3 "one" "two"]` then the combined result is
// `{@rec-1 1 2 [@rec-3 "one" "two"] }`
ok64 BRIXreget($u8 into, BRIX const* brix, u8c rdt, id128 key);

// Converts a nested RDX document (as produced by BRIXreget) into BRIX
// key-value form.
// Makes a *patch* SST for it (no deps), puts it on the stack.
ok64 BRIXaddpatch(sha256* sha, BRIX* brix, u8cs rdx);

ok64 BRIXfind(sha256* sha, BRIX const* brix, u8cs part);

#endif
