#include "ROCK.h"

#include <stdlib.h>
#include <string.h>

#include "PRO.h"

// Alloc default options (bloom filter, LRU cache)
ok64 ROCKInit(ROCKdbp db, b8 create) {
    sane(db != NULL);
    memset(db, 0, sizeof(ROCKdb));
    db->opt = rocksdb_options_create();
    test(db->opt != NULL, ROCKFAIL);

    if (create) rocksdb_options_set_create_if_missing(db->opt, 1);

    rocksdb_block_based_table_options_t *topt =
        rocksdb_block_based_options_create();
    test(topt != NULL, ROCKFAIL);

    rocksdb_filterpolicy_t *bloom =
        rocksdb_filterpolicy_create_bloom_full(10.0);
    if (bloom) rocksdb_block_based_options_set_filter_policy(topt, bloom);

    db->cache = rocksdb_cache_create_lru(64 * MB);
    if (db->cache)
        rocksdb_block_based_options_set_block_cache(topt, db->cache);

    rocksdb_options_set_block_based_table_factory(db->opt, topt);
    rocksdb_block_based_options_destroy(topt);

    db->ropt = rocksdb_readoptions_create();
    db->wopt = rocksdb_writeoptions_create();
    test(db->ropt != NULL && db->wopt != NULL, ROCKFAIL);
    done;
}

// Internal: null-terminate a path gauge on stack
static ok64 ROCKPath(char *buf, size_t bufsz, path8cg path) {
    size_t len = path[1] - path[0];
    if (len + 1 > bufsz) return ROCKBAD;
    memcpy(buf, path[0], len);
    buf[len] = 0;
    return OK;
}

// Open DB with current options
ok64 ROCKOpenDB(ROCKdbp db, path8cg path) {
    sane(db != NULL && db->opt != NULL && path != NULL && path[0] != NULL);
    char pbuf[4096];
    call(ROCKPath, pbuf, sizeof(pbuf), path);
    char *err = NULL;
    db->db = rocksdb_open(db->opt, pbuf, &err);
    ok64 o = ROCKerr(err);
    if (o != OK || db->db == NULL) {
        ROCKClose(db);
        fail(o != OK ? o : ROCKFAIL);
    }
    done;
}

// Convenience: init defaults + open
ok64 ROCKOpen(ROCKdbp db, path8cg path) {
    sane(db != NULL && path != NULL && path[0] != NULL);
    call(ROCKInit, db, YES);
    return ROCKOpenDB(db, path);
}

ok64 ROCKOpenRO(ROCKdbp db, path8cg path) {
    sane(db != NULL && path != NULL && path[0] != NULL);
    call(ROCKInit, db, NO);
    char pbuf[4096];
    call(ROCKPath, pbuf, sizeof(pbuf), path);
    char *err = NULL;
    db->db = rocksdb_open_for_read_only(db->opt, pbuf, 0, &err);
    ok64 o = ROCKerr(err);
    if (o != OK || db->db == NULL) {
        ROCKClose(db);
        fail(o != OK ? o : ROCKFAIL);
    }
    done;
}

// Merge operator glue: state holds the u8ys function pointer
typedef struct {
    u8ys merge;
} ROCKmerge_state;

static void ROCKmerge_destroy(void *state) { free(state); }

static const char *ROCKmerge_name(void *state) { return "ROCKmerge"; }

static void ROCKmerge_delval(void *state, const char *val, size_t len) {
    (void)state;
    (void)len;
    free((void *)val);
}

// full_merge: existing_value + operands -> merged
static char *ROCKmerge_full(void *state, const char *key, size_t key_length,
                            const char *existing_value,
                            size_t existing_value_length,
                            const char *const *operands_list,
                            const size_t *operands_list_length,
                            int num_operands, unsigned char *success,
                            size_t *new_value_length) {
    ROCKmerge_state *ms = (ROCKmerge_state *)state;
    int total = num_operands + (existing_value != NULL ? 1 : 0);
    if (total == 0) {
        *success = 1;
        *new_value_length = 0;
        return calloc(1, 1);
    }

    // Build u8css from existing_value + operands (heap for large counts)
    u8cs stack_recs[64];
    u8cs *recs = stack_recs;
    if (total > 64) {
        recs = malloc(total * sizeof(u8cs));
        if (recs == NULL) {
            *success = 0;
            return NULL;
        }
    }
    int ri = 0;
    if (existing_value != NULL) {
        recs[ri][0] = (u8cp)existing_value;
        recs[ri][1] = (u8cp)existing_value + existing_value_length;
        ri++;
    }
    for (int i = 0; i < num_operands; i++) {
        recs[ri][0] = (u8cp)operands_list[i];
        recs[ri][1] = (u8cp)operands_list[i] + operands_list_length[i];
        ri++;
    }
    u8css records = {recs, recs + total};

    // Allocate output buffer (sum of inputs as initial estimate)
    size_t cap = 0;
    for (int i = 0; i < total; i++) cap += $len(recs[i]);
    if (cap == 0) cap = 64;
    cap *= 2;

    char *out = malloc(cap);
    if (out == NULL) {
        if (recs != stack_recs) free(recs);
        *success = 0;
        return NULL;
    }
    u8s merged = {(u8p)out, (u8p)out + cap};

    ok64 o = ms->merge(merged, records);
    if (recs != stack_recs) free(recs);
    if (o != OK) {
        free(out);
        *success = 0;
        return NULL;
    }

    *success = 1;
    *new_value_length = (u8p)merged[0] - (u8p)out;
    return out;
}

// partial_merge: operands only (no existing value)
static char *ROCKmerge_partial(void *state, const char *key,
                               size_t key_length,
                               const char *const *operands_list,
                               const size_t *operands_list_length,
                               int num_operands, unsigned char *success,
                               size_t *new_value_length) {
    return ROCKmerge_full(state, key, key_length, NULL, 0, operands_list,
                          operands_list_length, num_operands, success,
                          new_value_length);
}

ok64 ROCKOpenMerge(ROCKdbp db, path8cg path, u8ys merge) {
    sane(db != NULL && path != NULL && path[0] != NULL && merge != NULL);
    call(ROCKInit, db, YES);

    ROCKmerge_state *ms = malloc(sizeof(ROCKmerge_state));
    test(ms != NULL, ROCKFAIL);
    ms->merge = merge;

    db->mop = rocksdb_mergeoperator_create(ms, ROCKmerge_destroy,
                                           ROCKmerge_full, ROCKmerge_partial,
                                           ROCKmerge_delval, ROCKmerge_name);
    test(db->mop != NULL, ROCKFAIL);
    rocksdb_options_set_merge_operator(db->opt, db->mop);

    return ROCKOpenDB(db, path);
}

ok64 ROCKSetMerge(ROCKdbp db, u8ys merge) {
    sane(db != NULL && db->opt != NULL && merge != NULL);
    ROCKmerge_state *ms = malloc(sizeof(ROCKmerge_state));
    test(ms != NULL, ROCKFAIL);
    ms->merge = merge;
    db->mop = rocksdb_mergeoperator_create(ms, ROCKmerge_destroy,
                                           ROCKmerge_full, ROCKmerge_partial,
                                           ROCKmerge_delval, ROCKmerge_name);
    test(db->mop != NULL, ROCKFAIL);
    rocksdb_options_set_merge_operator(db->opt, db->mop);
    done;
}

ok64 ROCKClose(ROCKdbp db) {
    if (db == NULL) return OK;
    if (db->db) {
        if (db->snap) {
            rocksdb_readoptions_set_snapshot(db->ropt, NULL);
            rocksdb_release_snapshot(db->db, db->snap);
            db->snap = NULL;
        }
        rocksdb_cancel_all_background_work(db->db, 1);
        rocksdb_close(db->db);
        db->db = NULL;
    }
    if (db->ropt) {
        rocksdb_readoptions_destroy(db->ropt);
        db->ropt = NULL;
    }
    if (db->wopt) {
        rocksdb_writeoptions_destroy(db->wopt);
        db->wopt = NULL;
    }
    if (db->opt) {
        rocksdb_options_destroy(db->opt);
        db->opt = NULL;
    }
    if (db->cache) {
        rocksdb_cache_destroy(db->cache);
        db->cache = NULL;
    }
    // mop is destroyed by options_destroy
    db->mop = NULL;
    return OK;
}

ok64 ROCKGet(ROCKdbp db, u8bp into, u8cs key) {
    if (db == NULL || db->db == NULL) return ROCKBAD;
    if (into == NULL || key == NULL) return ROCKBAD;
    char *err = NULL;
    size_t vlen = 0;
    char *val =
        rocksdb_get(db->db, db->ropt, (const char *)key[0],
                    $size(key), &vlen, &err);
    ok64 o = ROCKerr(err);
    if (o != OK) return o;
    if (val == NULL) return ROCKnone;

    u8cs data = {(u8cp)val, (u8cp)val + vlen};
    o = u8sFeed(u8bIdle(into), data);
    free(val);
    return o;
}

ok64 ROCKPut(ROCKdbp db, u8cs key, u8cs val) {
    if (db == NULL || db->db == NULL) return ROCKBAD;
    char *err = NULL;
    rocksdb_put(db->db, db->wopt, (const char *)key[0], $size(key),
                (const char *)val[0], $size(val), &err);
    return ROCKerr(err);
}

ok64 ROCKDel(ROCKdbp db, u8cs key) {
    if (db == NULL || db->db == NULL) return ROCKBAD;
    char *err = NULL;
    rocksdb_delete(db->db, db->wopt, (const char *)key[0], $size(key),
                   &err);
    return ROCKerr(err);
}

ok64 ROCKMerge(ROCKdbp db, u8cs key, u8cs val) {
    if (db == NULL || db->db == NULL) return ROCKBAD;
    char *err = NULL;
    rocksdb_merge(db->db, db->wopt, (const char *)key[0], $size(key),
                  (const char *)val[0], $size(val), &err);
    return ROCKerr(err);
}

// Write batch
ok64 ROCKBatchOpen(ROCKbatchp wb) {
    if (wb == NULL) return ROCKBAD;
    wb->wb = rocksdb_writebatch_create();
    if (wb->wb == NULL) return ROCKFAIL;
    return OK;
}

ok64 ROCKBatchPut(ROCKbatchp wb, u8cs key, u8cs val) {
    if (wb == NULL || wb->wb == NULL) return ROCKBAD;
    rocksdb_writebatch_put(wb->wb, (const char *)key[0], $size(key),
                           (const char *)val[0], $size(val));
    return OK;
}

ok64 ROCKBatchDel(ROCKbatchp wb, u8cs key) {
    if (wb == NULL || wb->wb == NULL) return ROCKBAD;
    rocksdb_writebatch_delete(wb->wb, (const char *)key[0], $size(key));
    return OK;
}

ok64 ROCKBatchMerge(ROCKbatchp wb, u8cs key, u8cs val) {
    if (wb == NULL || wb->wb == NULL) return ROCKBAD;
    rocksdb_writebatch_merge(wb->wb, (const char *)key[0], $size(key),
                             (const char *)val[0], $size(val));
    return OK;
}

ok64 ROCKBatchWrite(ROCKdbp db, ROCKbatchp wb) {
    if (db == NULL || db->db == NULL) return ROCKBAD;
    if (wb == NULL || wb->wb == NULL) return ROCKBAD;
    char *err = NULL;
    rocksdb_write(db->db, db->wopt, wb->wb, &err);
    return ROCKerr(err);
}

ok64 ROCKBatchClose(ROCKbatchp wb) {
    if (wb == NULL) return OK;
    if (wb->wb) {
        rocksdb_writebatch_destroy(wb->wb);
        wb->wb = NULL;
    }
    return OK;
}

// Prefix scan with callback
ok64 ROCKScan(ROCKdbp db, u8cs prefix, ROCKscanf f, voidp arg) {
    sane(db != NULL && db->db != NULL && f != NULL);
    ROCKiter it = {};
    call(ROCKIterOpen, &it, db);
    call(ROCKIterSeek, &it, prefix);
    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(prefix) ||
            memcmp(k[0], prefix[0], $len(prefix)) != 0)
            break;
        u8cs v = {};
        ROCKIterVal(&it, v);
        ok64 o = f(arg, k, v);
        if (o != OK) {
            ROCKIterClose(&it);
            return o;
        }
        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);
    done;
}

// Iterator
ok64 ROCKIterOpen(ROCKiterp it, ROCKdbp db) {
    if (it == NULL || db == NULL || db->db == NULL) return ROCKBAD;
    it->db = db;
    it->it = rocksdb_create_iterator(db->db, db->ropt);
    if (it->it == NULL) return ROCKFAIL;
    return OK;
}

ok64 ROCKIterSeek(ROCKiterp it, u8cs key) {
    if (it == NULL || it->it == NULL) return ROCKBAD;
    rocksdb_iter_seek(it->it, (const char *)key[0], $size(key));
    return OK;
}

ok64 ROCKIterSeekForPrev(ROCKiterp it, u8cs key) {
    if (it == NULL || it->it == NULL) return ROCKBAD;
    rocksdb_iter_seek_for_prev(it->it, (const char *)key[0], $size(key));
    return OK;
}

ok64 ROCKIterSeekFirst(ROCKiterp it) {
    if (it == NULL || it->it == NULL) return ROCKBAD;
    rocksdb_iter_seek_to_first(it->it);
    return OK;
}

ok64 ROCKIterNext(ROCKiterp it) {
    if (it == NULL || it->it == NULL) return ROCKBAD;
    rocksdb_iter_next(it->it);
    return OK;
}

ok64 ROCKIterPrev(ROCKiterp it) {
    if (it == NULL || it->it == NULL) return ROCKBAD;
    rocksdb_iter_prev(it->it);
    return OK;
}

b8 ROCKIterValid(ROCKiterp it) {
    if (it == NULL || it->it == NULL) return NO;
    return rocksdb_iter_valid(it->it) ? YES : NO;
}

void ROCKIterKey(ROCKiterp it, u8csp out) {
    size_t len = 0;
    const char *k = rocksdb_iter_key(it->it, &len);
    out[0] = (u8cp)k;
    out[1] = (u8cp)k + len;
}

void ROCKIterVal(ROCKiterp it, u8csp out) {
    size_t len = 0;
    const char *v = rocksdb_iter_value(it->it, &len);
    out[0] = (u8cp)v;
    out[1] = (u8cp)v + len;
}

ok64 ROCKIterClose(ROCKiterp it) {
    if (it == NULL) return OK;
    if (it->it) {
        rocksdb_iter_destroy(it->it);
        it->it = NULL;
    }
    it->db = NULL;
    return OK;
}

ok64 ROCKCheckpoint(ROCKdbp db, path8cg dest) {
    sane(db != NULL && db->db != NULL);
    char pbuf[4096];
    call(ROCKPath, pbuf, sizeof(pbuf), dest);
    char *err = NULL;
    rocksdb_checkpoint_t *cp =
        rocksdb_checkpoint_object_create(db->db, &err);
    ok64 o = ROCKerr(err);
    if (o != OK) return o;
    rocksdb_checkpoint_create(cp, pbuf, 0, &err);
    o = ROCKerr(err);
    rocksdb_checkpoint_object_destroy(cp);
    return o;
}

ok64 ROCKSnapshotCreate(ROCKdbp db) {
    sane(db != NULL && db->db != NULL);
    db->snap = rocksdb_create_snapshot(db->db);
    test(db->snap != NULL, ROCKFAIL);
    rocksdb_readoptions_set_snapshot(db->ropt, db->snap);
    done;
}

ok64 ROCKSnapshotRelease(ROCKdbp db) {
    sane(db != NULL && db->db != NULL);
    if (db->snap != NULL) {
        rocksdb_readoptions_set_snapshot(db->ropt, NULL);
        rocksdb_release_snapshot(db->db, db->snap);
        db->snap = NULL;
    }
    done;
}

ok64 ROCKGetPath(ROCKdbp db, path8g out) {
    sane(db != NULL && db->db != NULL && out != NULL);
    char *path = rocksdb_property_value(db->db, "rocksdb.dbname");
    if (path == NULL) fail(ROCKFAIL);
    size_t plen = strlen(path);
    u8cs pcs = {(u8cp)path, (u8cp)path + plen};
    ok64 o = u8sFeed(out + 1, pcs);
    free(path);
    if (o != OK) return o;
    call(path8gTerm, out);
    done;
}
