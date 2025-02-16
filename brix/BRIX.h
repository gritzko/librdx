#ifndef RDX_BRIX_H
#define RDX_BRIX_H

#include "abc/KV.h"
#include "abc/NACL.h"
#include "abc/SHA.h"
#include "rdx/RDX.h"

con ok64 BRIXnone = 0xa72cf28526cb;

#define BRIX_MAX_SST0_SIZE (1 << 30)
#define BRIX_MAX_SST0_ENTRIES (1 << 20)

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

extern $u8c BRIKext;
extern $u8c BRIXdir;
extern $u8c BRIXindex;

/** BRIX repo structure:

  - `.brix/e3a57c59...91e7c7f77.brik`
  - `.brix/INDEX`
  - `.brix/TAGS`
  - `.brix/KEYS`
  - `.brix/SIGS`

  The struct is not thread safe as all the storage is memory-mapped,
  hence can be shared between processes. For multi-thread use, use
  locks or something.
*/
typedef struct {
    // The home dir path, including .brix/
    Bu8 home;
    // Open bricks, a buffer of buffers (mmaped SSTs)
    BBu8 store;
    // Repo index
    Bkv64 index;
    Bedpub256 keys;
    Bedsig512 sigs;
    // Scratch pad;
    Bu8 pad;
} BRIX;

fun b8 BRIXok(BRIX const* brix) { return brix != nil && Bok(brix->store); }

// init a repo
ok64 BRIXinit(BRIX* brix, $u8c path);
// open the repo
ok64 BRIXopen(BRIX* brix, $u8c path);
// add an SST to the stack, including dependencies
ok64 BRIXpush(BRIX* brix, sha256c* head);
ok64 BRIXpushrev(BRIX* brix, id128 head);
// add SSTs to the stack, including dependencies
fun ok64 BRIXpushall(BRIX* brix, $sha256 heads) {
    ok64 o = OK;
    for (sha256c* p = $head(heads); o == OK && p < $term(heads); ++p)
        o = BRIXpush(brix, p);
    return o;
}
// len of the stack
fun size_t BRIXlen(BRIX const* brix) { return Bdatalen(brix->store); }
// pop one SST from the stack
ok64 BRIXpop(BRIX* brix);
// pop SSTs from the stack
ok64 BRIXpopto(BRIX* brix, size_t len);

// Get a record (TLKV, ZINT u128 key, RDX body).
// `rdt` the expected RDX type; 0 for any
// `rec` will point to the SST record or to the tmp pad if merge was necessary.
ok64 BRIXget(u8c$ rec, BRIX const* brix, u8 rdt, id128 key);
// Produce the current version of an RDX document
ok64 BRIXproduce($u8 into, BRIX const* brix, u8 rdt, id128 key);
ok64 BRIXproduce2(int fd, BRIX const* brix, u8 rdt, id128 key);

// add an RDX patch on top of the current head(s)
ok64 BRIXpatch(sha256* sha, BRIX* brix, $u8c rdx);
// merge the heads
ok64 BRIXmerge(sha256* sha, BRIX* brix, $sha256c shas);
// compacts the stack, maybe
ok64 BRIXpack(BRIX* brix);
// compacts the stack to the specified length
ok64 BRIXpackto(BRIX* brix, size_t len);
// close the repo
ok64 BRIXclose(BRIX* brix);

#endif
