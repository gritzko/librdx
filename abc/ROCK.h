#ifndef ABC_ROCK_H
#define ABC_ROCK_H

#include <rocksdb/c.h>

#include "01.h"
#include "BUF.h"
#include "OK.h"
#include "PATH.h"

// Error codes
con ok64 ROCKBAD = 0x1b60c50b28d;
con ok64 ROCKFAIL = 0x6d83143ca495;
con ok64 ROCKnone = 0x6d8314cb3ca9;

// Convert RocksDB's malloc'd error string to ok64 (frees the string)
fun ok64 ROCKerr(char *err) {
    if (err == NULL) return OK;
    free(err);
    return ROCKFAIL;
}

// Database handle
typedef struct {
    rocksdb_t *db;
    rocksdb_options_t *opt;
    rocksdb_readoptions_t *ropt;
    rocksdb_writeoptions_t *wopt;
    rocksdb_mergeoperator_t *mop;
    rocksdb_cache_t *cache;
} ROCKdb;
typedef ROCKdb *ROCKdbp;

// Iterator
typedef struct {
    rocksdb_iterator_t *it;
    ROCKdbp db;
} ROCKiter;
typedef ROCKiter *ROCKiterp;

// Write batch
typedef struct {
    rocksdb_writebatch_t *wb;
} ROCKbatch;
typedef ROCKbatch *ROCKbatchp;

// Database lifecycle
ok64 ROCKOpen(ROCKdbp db, path8cg path);
ok64 ROCKOpenRO(ROCKdbp db, path8cg path);
ok64 ROCKOpenMerge(ROCKdbp db, path8cg path, u8ys merge);
ok64 ROCKClose(ROCKdbp db);

// Point operations
ok64 ROCKGet(ROCKdbp db, u8bp into, u8cs key);
ok64 ROCKPut(ROCKdbp db, u8cs key, u8cs val);
ok64 ROCKDel(ROCKdbp db, u8cs key);
ok64 ROCKMerge(ROCKdbp db, u8cs key, u8cs val);

// Write batch
ok64 ROCKBatchOpen(ROCKbatchp wb);
ok64 ROCKBatchPut(ROCKbatchp wb, u8cs key, u8cs val);
ok64 ROCKBatchDel(ROCKbatchp wb, u8cs key);
ok64 ROCKBatchMerge(ROCKbatchp wb, u8cs key, u8cs val);
ok64 ROCKBatchWrite(ROCKdbp db, ROCKbatchp wb);
ok64 ROCKBatchClose(ROCKbatchp wb);

// Iterator
ok64 ROCKIterOpen(ROCKiterp it, ROCKdbp db);
ok64 ROCKIterSeek(ROCKiterp it, u8cs key);
ok64 ROCKIterSeekFirst(ROCKiterp it);
ok64 ROCKIterNext(ROCKiterp it);
ok64 ROCKIterPrev(ROCKiterp it);
b8 ROCKIterValid(ROCKiterp it);
void ROCKIterKey(ROCKiterp it, u8csp out);
void ROCKIterVal(ROCKiterp it, u8csp out);
ok64 ROCKIterClose(ROCKiterp it);

#endif  // ABC_ROCK_H
