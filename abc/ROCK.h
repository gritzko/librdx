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
    const rocksdb_snapshot_t *snap;
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
ok64 ROCKInit(ROCKdbp db, b8 create);       // alloc default options
ok64 ROCKOpenDB(ROCKdbp db, path8cg path);  // open with current options
ok64 ROCKOpen(ROCKdbp db, path8cg path);    // init+open (convenience)
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

// Prefix scan callback: return OK to continue, non-OK to stop
typedef ok64 (*ROCKscanf)(voidp arg, u8cs key, u8cs val);

// Scan all keys with given prefix, calling f(arg, key, val) for each
ok64 ROCKScan(ROCKdbp db, u8cs prefix, ROCKscanf f, voidp arg);

// Iterator
ok64 ROCKIterOpen(ROCKiterp it, ROCKdbp db);
ok64 ROCKIterSeek(ROCKiterp it, u8cs key);
ok64 ROCKIterSeekForPrev(ROCKiterp it, u8cs key);
ok64 ROCKIterSeekFirst(ROCKiterp it);
ok64 ROCKIterNext(ROCKiterp it);
ok64 ROCKIterPrev(ROCKiterp it);
b8 ROCKIterValid(ROCKiterp it);
void ROCKIterKey(ROCKiterp it, u8csp out);
void ROCKIterVal(ROCKiterp it, u8csp out);
ok64 ROCKIterClose(ROCKiterp it);

// Checkpoint (hard-link SSTs for branching)
ok64 ROCKCheckpoint(ROCKdbp db, path8cg dest);

// Snapshot: pin SST files from compaction
ok64 ROCKSnapshotCreate(ROCKdbp db);
ok64 ROCKSnapshotRelease(ROCKdbp db);

// Disable/enable file deletions (pin SSTs for serving)
ok64 ROCKDisableFileDeletions(ROCKdbp db);
ok64 ROCKEnableFileDeletions(ROCKdbp db);

// Live file listing callback: (arg, filename, size)
typedef ok64 (*ROCKfilef)(voidp arg, u8cs filename, u64 size);

// Enumerate live SST/metadata files, calling f for each
ok64 ROCKLiveFiles(ROCKdbp db, ROCKfilef f, voidp arg);

// Get DB directory path (null-terminated) into buffer
ok64 ROCKGetPath(ROCKdbp db, path8g out);

#endif  // ABC_ROCK_H
