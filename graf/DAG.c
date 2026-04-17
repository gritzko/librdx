//  DAG: graf's object-graph index (keeper-based).
//
//  Walks keeper's kv64 index to enumerate commits, inflates them
//  via KEEPGet, diffs trees to find file changes, writes belt128
//  records into LSM sorted runs under <reporoot>/.dogs/graf/.
//
#include "DAG.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/KV.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/RAP.h"
#include "abc/RON.h"
#include "dog/DPATH.h"
#include "keeper/GIT.h"
#include "keeper/REFS.h"

// --- Template instantiations for belt128 ---

#define X(M, name) M##belt128##name
#include "abc/Bx.h"
#include "abc/QSORTx.h"
#include "abc/MSETx.h"
#undef X

// --- HASHkv64: open-addressed hash table for path interning ---

#define X(M, name) M##kv64##name
#include "abc/HASHx.h"
#undef X

// --- Constants ---

#define DAG_DIR         ".dogs/graf"
#define DAG_IDX_EXT     ".idx"
#define DAG_SEQNO_W     10
#define DAG_MAX_SHAS    16
#define DAG_BATCH       (1 << 18)   // 256K entries per flush
#define DAG_GENS_BITS   18
#define DAG_GENS_SIZE   (1 << DAG_GENS_BITS)
#define DAG_GENS_MASK   (DAG_GENS_SIZE - 1)
#define DAG_MAX_PATHS   (1 << 20)   // 1M paths in-memory map

// --- COMMIT bookmark file (stores sha1 as hex lines) ---

static b8 dag_is_hex_sha(char const *s, size_t len) {
    if (len < 40) return NO;
    for (int i = 0; i < 40; i++) {
        u8 c = (u8)s[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return NO;
    }
    return YES;
}

static ok64 dag_commit_read(u32 *count, u8cs dagdir,
                             sha1 shas[], u32 maxcount) {
    sane(count != NULL && $ok(dagdir) && shas != NULL && maxcount > 0);
    *count = 0;

    a_path(path, dagdir);
    a_cstr(name, "/COMMIT");
    call(u8bFeed, path, name);
    call(PATHu8bTerm, path);

    u8bp mapped = NULL;
    ok64 o = FILEMapRO(&mapped, $path(path));
    if (o != OK) done;

    u8cp p = u8bDataHead(mapped);
    u8cp end = u8bIdleHead(mapped);
    while (p < end && *count < maxcount) {
        while (p < end && (*p == '\n' || *p == '\r' || *p == ' ')) p++;
        if (p >= end) break;
        u8cp lend = p;
        while (lend < end && *lend != '\n') lend++;
        size_t llen = (size_t)(lend - p);
        if (dag_is_hex_sha((char const *)p, llen)) {
            DAGsha1FromHex(&shas[*count], (char const *)p);
            (*count)++;
        }
        p = lend;
    }
    FILEUnMap(mapped);
    done;
}

// --- In-batch hashlet→gen side-table (open-addressed) ---

typedef struct {
    u64 *keys;   // 0 = empty
    u32 *gens;
} dag_gens;

static ok64 dag_gens_init(dag_gens *g) {
    sane(g);
    g->keys = calloc(DAG_GENS_SIZE, sizeof(u64));
    g->gens = calloc(DAG_GENS_SIZE, sizeof(u32));
    if (!g->keys || !g->gens) {
        free(g->gens); free(g->keys);
        g->keys = NULL; g->gens = NULL;
        return DAGFAIL;
    }
    return OK;
}

static void dag_gens_free(dag_gens *g) {
    free(g->gens); free(g->keys);
    g->keys = NULL; g->gens = NULL;
}

static void dag_gens_put(dag_gens *g, u64 hashlet, u32 gen) {
    u64 k = hashlet ? hashlet : 1;
    u32 i = (u32)(hashlet >> 4) & DAG_GENS_MASK;
    for (;;) {
        if (g->keys[i] == 0 || g->keys[i] == k) {
            g->keys[i] = k;
            g->gens[i] = gen;
            return;
        }
        i = (i + 1) & DAG_GENS_MASK;
    }
}

static u32 dag_gens_get(dag_gens *g, u64 hashlet) {
    u64 k = hashlet ? hashlet : 1;
    u32 i = (u32)(hashlet >> 4) & DAG_GENS_MASK;
    for (;;) {
        if (g->keys[i] == 0) return 0;
        if (g->keys[i] == k) return g->gens[i];
        i = (i + 1) & DAG_GENS_MASK;
    }
}

// --- Path log: append-only NUL-separated file + HASHkv64 map ---

typedef struct {
    Bkv64 map;      // kv64 hash table: key=RAPHash, val=path_id
    u32 next_id;     // next path_id to assign (= byte offset in PATHS)
    int fd;          // open PATHS file for appending
} dag_paths;

static ok64 dag_paths_init(dag_paths *p, u8cs dagdir) {
    sane(p && $ok(dagdir));
    memset(p, 0, sizeof(*p));
    p->fd = -1;

    call(kv64bAllocate, p->map, DAG_MAX_PATHS);

    // Open/create PATHS file, read existing entries to populate map
    a_path(path, dagdir);
    a_cstr(name, "/PATHS");
    call(u8bFeed, path, name);
    call(PATHu8bTerm, path);

    u8bp mapped = NULL;
    ok64 o = FILEMapRO(&mapped, $path(path));
    if (o == OK) {
        u8cp base = u8bDataHead(mapped);
        u8cp end = u8bIdleHead(mapped);
        u8cp cur = base;
        while (cur < end) {
            u8cp nul = cur;
            while (nul < end && *nul != 0) nul++;
            if (nul == cur) { cur = nul + 1; continue; }
            u32 id = (u32)(cur - base);
            u8csc ps = {cur, nul};
            u64 h = RAPHash(ps);
            kv64 entry = {.key = h, .val = (u64)id};
            kv64s idle = {kv64bHead(p->map), kv64bTerm(p->map)};
            HASHkv64Put(idle, &entry);
            u32 after = (u32)(nul + 1 - base);
            if (after > p->next_id) p->next_id = after;
            cur = nul + 1;
        }
        FILEUnMap(mapped);
    }

    // Open for append
    p->fd = open((char *)u8bDataHead(path), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (p->fd < 0) {
        kv64bFree(p->map);
        return DAGFAIL;
    }
    done;
}

static void dag_paths_free(dag_paths *p) {
    if (p->fd >= 0) close(p->fd);
    kv64bFree(p->map);
    memset(p, 0, sizeof(*p));
    p->fd = -1;
}

// Get or create path_id for a path string.  Returns the id.
static u32 dag_paths_intern(dag_paths *p, char const *str, size_t len) {
    u8csc ps = {(u8cp)str, (u8cp)str + len};
    u64 h = RAPHash(ps);
    kv64 probe = {.key = h, .val = 0};
    kv64s tab = {kv64bHead(p->map), kv64bTerm(p->map)};
    if (HASHkv64Get(&probe, tab) == OK) return (u32)probe.val;

    // New path: append to file
    u32 id = p->next_id;
    write(p->fd, str, len);
    write(p->fd, "\0", 1);
    p->next_id = id + (u32)len + 1;

    kv64 entry = {.key = h, .val = (u64)id};
    HASHkv64Put(tab, &entry);
    return id;
}

// --- LSM index I/O (belt128 runs) ---

static ok64 dag_index_write(u8cs dagdir, belt128cs run, u64 seqno) {
    sane($ok(dagdir));
    if ($empty(run)) done;

    a_pad(u8, path, FILE_PATH_MAX_LEN);
    call(u8bFeed, path, dagdir);
    call(u8bFeed1, path, '/');
    call(RONu8sFeedPad, u8bIdle(path), seqno, DAG_SEQNO_W);
    ((u8 **)path)[2] += DAG_SEQNO_W;
    a_cstr(idxext, DAG_IDX_EXT);
    call(u8bFeed, path, idxext);
    call(PATHu8bTerm, path);

    int fd = -1;
    call(FILECreate, &fd, $path(path));
    size_t bytes = $len(run) * sizeof(belt128);
    u8cs data = {(u8cp)run[0], (u8cp)run[0] + bytes};
    call(FILEFeedAll, fd, data);
    close(fd);
    done;
}

static ok64 dag_next_seqno(u64 *seqno, u8cs dagdir) {
    sane(seqno && $ok(dagdir));
    *seqno = 1;

    a_path(dpat);
    call(PATHu8bFeed, dpat, dagdir);

    typedef struct { char (*names)[64]; u32 maxn; u32 count; } lctx;
    // Scan directory inline
    DIR *d = opendir((char *)u8bDataHead(dpat));
    if (!d) done;
    u64 maxseq = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        size_t nlen = strlen(e->d_name);
        if (nlen != DAG_SEQNO_W + 4) continue;
        if (memcmp(e->d_name + DAG_SEQNO_W, DAG_IDX_EXT, 4) != 0) continue;
        u8cs numslice = {(u8cp)e->d_name, (u8cp)e->d_name + DAG_SEQNO_W};
        u64 val = 0;
        ok64 r = RONutf8sDrain(&val, numslice);
        if (r == OK && val > maxseq) maxseq = val;
    }
    closedir(d);
    *seqno = maxseq + 1;
    done;
}

// --- Stack management ---

// dag_stack typedef is in DAG.h

ok64 dag_stack_open(dag_stack *st, u8cs dagdir) {
    sane(st && $ok(dagdir));
    memset(st, 0, sizeof(*st));

    a_path(dpat);
    call(PATHu8bFeed, dpat, dagdir);

    DIR *d = opendir((char *)u8bDataHead(dpat));
    if (!d) done;

    char names[MSET_MAX_LEVELS][64];
    char *namep[MSET_MAX_LEVELS];
    u32 count = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL && count < MSET_MAX_LEVELS) {
        size_t nlen = strlen(e->d_name);
        if (nlen != DAG_SEQNO_W + 4) continue;
        if (memcmp(e->d_name + DAG_SEQNO_W, DAG_IDX_EXT, 4) != 0) continue;
        memcpy(names[count], e->d_name, nlen + 1);
        namep[count] = names[count];
        count++;
    }
    closedir(d);

    // Sort by name (oldest first)
    for (u32 i = 0; i + 1 < count; i++)
        for (u32 j = i + 1; j < count; j++)
            if (strcmp(namep[i], namep[j]) > 0) {
                char *t = namep[i]; namep[i] = namep[j]; namep[j] = t;
            }

    for (u32 i = 0; i < count; i++) {
        u8cs fn = {(u8cp)namep[i], (u8cp)namep[i] + strlen(namep[i])};
        a_path(fpath, dagdir, fn);

        u8bp mapped = NULL;
        if (FILEMapRO(&mapped, $path(fpath)) != OK) continue;
        belt128cp base = (belt128cp)u8bDataHead(mapped);
        size_t nentries = (u8bIdleHead(mapped) - u8bDataHead(mapped)) / sizeof(belt128);
        st->runs[st->n][0] = base;
        st->runs[st->n][1] = base + nentries;
        st->maps[st->n] = mapped;
        st->n++;
    }
    done;
}

void dag_stack_close(dag_stack *st) {
    if (!st) return;
    for (u32 i = 0; i < st->n; i++)
        if (st->maps[i]) FILEUnMap(st->maps[i]);
    st->n = 0;
}

// Lookup gen for a hashlet in the MSET stack (any entry type that
// carries gen in .a).  Returns 0 if not found.
static u32 dag_stack_gen(dag_stack *st, u64 hashlet) {
    // Seek for COMMIT_GEN entry
    u64 key_a = DAGPack(DAG_COMMIT_GEN, 0, hashlet);
    u64 key_a_hi = DAGPack(DAG_COMMIT_GEN, WHIFF_ID_MASK, hashlet);
    for (u32 r = 0; r < st->n; r++) {
        belt128cp base = st->runs[r][0];
        size_t len = (size_t)(st->runs[r][1] - st->runs[r][0]);
        // Binary search for first entry with .a >= key_a
        size_t lo = 0, hi = len;
        while (lo < hi) {
            size_t mid = lo + (hi - lo) / 2;
            if (base[mid].a < key_a) lo = mid + 1;
            else hi = mid;
        }
        if (lo < len && base[lo].a >= key_a && base[lo].a <= key_a_hi)
            return DAGGen(base[lo].a);
    }
    return 0;
}

// --- Compaction ---

static ok64 dag_compact(u8cs dagdir) {
    sane($ok(dagdir));

    dag_stack st = {};
    call(dag_stack_open, &st, dagdir);
    if (st.n < 2) { dag_stack_close(&st); done; }

    belt128css stack = {st.runs, st.runs + st.n};
    if (MSETbelt128IsCompact(stack)) { dag_stack_close(&st); done; }

    size_t total = 0;
    for (u32 i = 0; i < st.n; i++)
        total += (size_t)(st.runs[i][1] - st.runs[i][0]);

    Bbelt128 cbuf = {};
    call(belt128bAllocate, cbuf, total);
    belt128s into = {cbuf[0], cbuf[3]};

    size_t n = st.n;
    size_t m = 1;
    size_t mtotal = (size_t)(st.runs[n - 1][1] - st.runs[n - 1][0]);
    while (m < n && mtotal * 8 > (size_t)(st.runs[n - 1 - m][1] - st.runs[n - 1 - m][0])) {
        mtotal += (size_t)(st.runs[n - 1 - m][1] - st.runs[n - 1 - m][0]);
        m++;
    }
    if (m < 2) {
        belt128bFree(cbuf);
        dag_stack_close(&st);
        done;
    }

    // Merge youngest m runs
    {
        belt128cs subruns[MSET_MAX_LEVELS];
        for (size_t i = 0; i < m; i++) {
            subruns[i][0] = st.runs[n - m + i][0];
            subruns[i][1] = st.runs[n - m + i][1];
        }
        belt128css sub = {subruns, subruns + m};
        MSETbelt128Start(sub);
        belt128s out = {cbuf[0], cbuf[3]};
        ok64 mo = MSETbelt128Merge(out, sub);
        (void)mo;
        into[0] = out[0];
    }

    u64 seqno = 0;
    call(dag_next_seqno, &seqno, dagdir);
    belt128cs merged = {(belt128cp)cbuf[0], (belt128cp)(into[0])};
    call(dag_index_write, dagdir, merged, seqno);

    // Collect filenames before closing maps
    char fnames[MSET_MAX_LEVELS][64];
    u32 fcount = 0;
    {
        a_path(dpat);
        call(PATHu8bFeed, dpat, dagdir);
        DIR *d2 = opendir((char *)u8bDataHead(dpat));
        if (d2) {
            struct dirent *e2;
            while ((e2 = readdir(d2)) != NULL && fcount < MSET_MAX_LEVELS) {
                size_t nlen = strlen(e2->d_name);
                if (nlen != DAG_SEQNO_W + 4) continue;
                if (memcmp(e2->d_name + DAG_SEQNO_W, DAG_IDX_EXT, 4) != 0) continue;
                memcpy(fnames[fcount], e2->d_name, nlen + 1);
                fcount++;
            }
            closedir(d2);
        }
    }
    // Sort filenames
    for (u32 i = 0; i + 1 < fcount; i++)
        for (u32 j = i + 1; j < fcount; j++)
            if (strcmp(fnames[i], fnames[j]) > 0) {
                char tmp[64];
                memcpy(tmp, fnames[i], 64);
                memcpy(fnames[i], fnames[j], 64);
                memcpy(fnames[j], tmp, 64);
            }

    dag_stack_close(&st);

    // Unlink the m oldest of the pre-merge files (all except the newest
    // which is the one we just wrote)
    u32 unlinked = 0;
    for (u32 i = 0; i < fcount && unlinked < m; i++) {
        // Skip the file we just wrote (it has seqno = max)
        u8cs numslice = {(u8cp)fnames[i], (u8cp)fnames[i] + DAG_SEQNO_W};
        u64 fseq = 0;
        RONutf8sDrain(&fseq, numslice);
        if (fseq == seqno) continue;

        u8cs fn = {(u8cp)fnames[i], (u8cp)fnames[i] + strlen(fnames[i])};
        a_path(ulpath, dagdir, fn);
        unlink((char *)u8bDataHead(ulpath));
        unlinked++;
    }

    belt128bFree(cbuf);
    done;
}

// --- SHA hex → 40-bit hashlet (via sha1 type) ---

static u64 dag_hex_to_hashlet(char const *hex40) {
    sha1 s = {};
    if (DAGsha1FromHex(&s, hex40) != OK) return 0;
    return DAGsha1Hashlet(&s);
}

// --- Read all ref tips from keeper REFS ---

static u32 dag_read_tips(sha1 tips[], u32 maxtips, keeper *k) {
    a_cstr(keepdir, k->dir);
    u8bp rmap = NULL;
    ref rarr[REFS_MAX_REFS];
    u32 rn = 0;
    if (REFSLoad(rarr, &rn, REFS_MAX_REFS, &rmap, keepdir) != OK)
        return 0;

    u32 count = 0;
    for (u32 i = 0; i < rn && count < maxtips; i++) {
        // Only SHA refs: val starts with '?'
        if (u8csEmpty(rarr[i].val) || *rarr[i].val[0] != '?') continue;
        u8cs val = {rarr[i].val[0] + 1, rarr[i].val[1]};
        if (u8csLen(val) < 40) continue;
        if (!dag_is_hex_sha((char const *)val[0], u8csLen(val))) continue;
        DAGsha1FromHex(&tips[count], (char const *)val[0]);
        count++;
    }
    if (rmap) u8bUnMap(rmap);
    return count;
}

// --- Write sha1 array to COMMIT as hex lines ---

static ok64 dag_write_tips(u8cs dagdir, sha1 const tips[], u32 ntips) {
    sane($ok(dagdir));
    if (ntips == 0) done;

    a_path(path, dagdir);
    a_cstr(name, "/COMMIT");
    call(u8bFeed, path, name);
    call(PATHu8bTerm, path);

    int fd = -1;
    call(FILECreate, &fd, $path(path));
    u8cs nl = {(u8cp)"\n", (u8cp)"\n" + 1};
    for (u32 i = 0; i < ntips; i++) {
        char hex[41];
        DAGsha1ToHex(hex, &tips[i]);
        u8cs data = {(u8cp)hex, (u8cp)hex + 40};
        call(FILEFeedAll, fd, data);
        call(FILEFeedAll, fd, nl);
    }
    close(fd);
    done;
}

// --- Load saved bookmarks, validate they exist in keeper ---

static u32 dag_load_bookmarks(sha1 out[], u32 maxout,
                               u8cs dagdir, keeper *k) {
    sha1 shas[DAG_MAX_SHAS] = {};
    u32 sha_count = 0;
    dag_commit_read(&sha_count, dagdir, shas, DAG_MAX_SHAS);

    u32 valid = 0;
    for (u32 i = 0; i < sha_count && valid < maxout; i++) {
        u64 hashlet = WHIFFHashlet60(&shas[i]);
        if (KEEPHas(k, hashlet, 15) == OK) {
            out[valid] = shas[i];
            valid++;
        }
    }
    return valid;
}

// --- Emit helpers ---

static void dag_emit(belt128p buf, size_t *pos, size_t cap,
                     u8 atype, u32 agen, u64 ahash,
                     u8 btype, u32 bgen, u64 bhash) {
    if (*pos >= cap) return;
    buf[*pos] = DAGEntry(atype, agen, ahash, btype, bgen, bhash);
    (*pos)++;
}

// --- Parse one commit from rev-list output ---
// Line format: "<sha> <p1> <p2> ..."
// Returns parent count; fills hashlet, parent hashlets, and gen.

static int dag_parse_commit(char const *line, size_t llen,
                            dag_gens *gens, dag_stack *st,
                            u64 *out_hash, u32 *out_gen,
                            u64 parents[], int maxpar) {
    if (llen < 40 || !dag_is_hex_sha(line, 40)) return -1;
    *out_hash = dag_hex_to_hashlet(line);

    u32 gen = 1;
    int npar = 0;
    size_t i = 40;
    while (i < llen && line[i] == ' ') {
        i++;
        if (i + 40 > llen) break;
        if (!dag_is_hex_sha(line + i, 40)) break;
        u64 ph = dag_hex_to_hashlet(line + i);

        u32 pg = dag_gens_get(gens, ph);
        if (pg == 0) pg = dag_stack_gen(st, ph);
        if (pg >= gen) gen = pg + 1;

        if (npar < maxpar) parents[npar] = ph;
        npar++;
        i += 40;
    }
    *out_gen = gen;
    return npar;
}

// --- Parse diff-tree -z colon record ---
// With -z, format is: ":old_mode new_mode old_sha new_sha status"
// (NUL-terminated; path(s) follow as separate NUL-terminated strings)

typedef struct {
    sha1 old_sha;
    sha1 new_sha;
    char status;
} dag_diff_entry;

static b8 dag_parse_diff_rec(char const *rec, size_t rlen,
                              dag_diff_entry *out) {
    memset(out, 0, sizeof(*out));
    if (rlen < 2 || rec[0] != ':') return NO;

    char const *p = rec + 1;
    char const *end = rec + rlen;

    // old_mode
    while (p < end && *p != ' ') p++;
    if (p >= end) return NO;
    p++;

    // new_mode
    while (p < end && *p != ' ') p++;
    if (p >= end) return NO;
    p++;

    // old_sha
    if (p + 40 > end) return NO;
    DAGsha1FromHex(&out->old_sha, p);
    p += 40;
    if (p >= end || *p != ' ') return NO;
    p++;

    // new_sha
    if (p + 40 > end) return NO;
    DAGsha1FromHex(&out->new_sha, p);
    p += 40;
    if (p >= end || *p != ' ') return NO;
    p++;

    // status (A, M, D, R###, C###, T)
    out->status = *p;
    return YES;
}

// --- Recursive tree diff: yield (path, old_blob_hash, new_blob_hash) ---
//
// Walks two trees (old and new) from keeper, comparing entries.
// For changed/added/deleted blobs, calls the callback.

typedef ok64 (*tree_diff_cb)(char const *path, size_t pathlen,
                              u64 old_blob_h, u64 new_blob_h, void0p ctx);

static ok64 dag_tree_diff_r(keeper *k, u8cs old_tree, u8cs new_tree,
                             char *pathbuf, size_t pathoff, size_t pathcap,
                             tree_diff_cb cb, void0p ctx) {
    // Collect entries from both trees into parallel arrays
    // git tree entry: "<mode> <name>\0<20-byte-sha>"
    #define TREE_MAX 4096
    typedef struct { u8cs name; u8cs sha; u8cs mode; } tentry;
    tentry *old_ents = malloc(TREE_MAX * sizeof(tentry));
    tentry *new_ents = malloc(TREE_MAX * sizeof(tentry));
    if (!old_ents || !new_ents) { free(old_ents); free(new_ents); return DAGFAIL; }
    u32 on = 0, nn = 0;

    {
        a_dup(u8c, scan, old_tree);
        u8cs file = {}, sha = {};
        while (on < TREE_MAX && GITu8sDrainTree(scan, file, sha) == OK) {
            u8cs dn = {file[0], file[1]};
            if (u8csFind(dn, ' ') == OK) {
                u8cs dname = {dn[0] + 1, file[1]};
                if (DPATHVerify(dname) != OK) {
                    fprintf(stderr, "dag: bad path '%.*s', skip\n",
                            (int)$len(dname), (char *)dname[0]);
                    continue;
                }
            }
            u8csMv(old_ents[on].name, file);
            u8csMv(old_ents[on].sha, sha);
            u8csMv(old_ents[on].mode, file);
            on++;
        }
    }
    {
        a_dup(u8c, scan, new_tree);
        u8cs file = {}, sha = {};
        while (nn < TREE_MAX && GITu8sDrainTree(scan, file, sha) == OK) {
            u8cs dn = {file[0], file[1]};
            if (u8csFind(dn, ' ') == OK) {
                u8cs dname = {dn[0] + 1, file[1]};
                if (DPATHVerify(dname) != OK) {
                    fprintf(stderr, "dag: bad path '%.*s', skip\n",
                            (int)$len(dname), (char *)dname[0]);
                    continue;
                }
            }
            u8csMv(new_ents[nn].name, file);
            u8csMv(new_ents[nn].sha, sha);
            u8csMv(new_ents[nn].mode, file);
            nn++;
        }
    }

    // Both are sorted by name (git guarantees this).
    // Merge-join to find additions, deletions, modifications.
    u32 oi = 0, ni = 0;
    ok64 result = OK;

    while ((oi < on || ni < nn) && result == OK) {
        int cmp;
        if (oi >= on) cmp = 1;
        else if (ni >= nn) cmp = -1;
        else {
            size_t ol = u8csLen(old_ents[oi].name);
            size_t nl = u8csLen(new_ents[ni].name);
            size_t ml = ol < nl ? ol : nl;
            cmp = memcmp(old_ents[oi].name[0], new_ents[ni].name[0], ml);
            if (cmp == 0) cmp = ol < nl ? -1 : ol > nl ? 1 : 0;
        }

        if (cmp == 0) {
            // Same name — check if SHA differs
            if (u8csLen(old_ents[oi].sha) == 20 &&
                u8csLen(new_ents[ni].sha) == 20 &&
                memcmp(old_ents[oi].sha[0], new_ents[ni].sha[0], 20) != 0) {
                b8 is_tree = *old_ents[oi].mode[0] == '4';
                if (is_tree) {
                    size_t nlen = u8csLen(old_ents[oi].name);
                    if (pathoff + nlen + 1 < pathcap) {
                        memcpy(pathbuf + pathoff, old_ents[oi].name[0], nlen);
                        pathbuf[pathoff + nlen] = '/';

                        u64 oh = WHIFFHashlet60((sha1cp)old_ents[oi].sha[0]);
                        u64 nh = WHIFFHashlet60((sha1cp)new_ents[ni].sha[0]);
                        Bu8 ob = {}, nb = {};
                        if (u8bMap(ob, 4UL << 20) != OK ||
                            u8bMap(nb, 4UL << 20) != OK) {
                            u8bUnMap(ob);
                            u8bUnMap(nb);
                            continue;
                        }
                        u8 ot = 0, nt = 0;
                        if (KEEPGet(k, oh, 15, ob, &ot) == OK &&
                            KEEPGet(k, nh, 15, nb, &nt) == OK) {
                            a_dup(u8c, od, u8bDataC(ob));
                            a_dup(u8c, nd, u8bDataC(nb));
                            result = dag_tree_diff_r(k, od, nd, pathbuf,
                                                     pathoff + nlen + 1,
                                                     pathcap, cb, ctx);
                        }
                        u8bUnMap(ob);
                        u8bUnMap(nb);
                    }
                } else {
                    u64 oh = WHIFFHashlet60((sha1cp)old_ents[oi].sha[0]);
                    u64 nh = WHIFFHashlet60((sha1cp)new_ents[ni].sha[0]);
                    size_t nlen = u8csLen(old_ents[oi].name);
                    if (pathoff + nlen < pathcap) {
                        memcpy(pathbuf + pathoff, old_ents[oi].name[0], nlen);
                        result = cb(pathbuf, pathoff + nlen, oh, nh, ctx);
                    }
                }
            }
            oi++; ni++;
        } else if (cmp < 0) {
            oi++;
        } else {
            b8 is_tree = *new_ents[ni].mode[0] == '4';
            if (!is_tree) {
                u64 nh = WHIFFHashlet60((sha1cp)new_ents[ni].sha[0]);
                size_t nlen = u8csLen(new_ents[ni].name);
                if (pathoff + nlen < pathcap) {
                    memcpy(pathbuf + pathoff, new_ents[ni].name[0], nlen);
                    result = cb(pathbuf, pathoff + nlen, 0, nh, ctx);
                }
            }
            ni++;
        }
    }

    free(old_ents);
    free(new_ents);
    return result;
    #undef TREE_MAX
}

// --- Tree diff context for DAGHook ---

typedef struct {
    belt128p entries;
    size_t  *nentries;
    size_t   bufcap;
    dag_gens *gens;
    dag_stack *stack;
    dag_paths *paths;
    u64 commit_hash;
    u32 commit_gen;
    u32 parent_gen;
} dag_diff_ctx;

static ok64 dag_diff_cb(char const *path, size_t pathlen,
                         u64 old_blob_h, u64 new_blob_h, void0p arg) {
    dag_diff_ctx *ctx = (dag_diff_ctx *)arg;
    if (pathlen == 0 || new_blob_h == 0) return OK;

    // PREV_BLOB (modified files only)
    if (old_blob_h != 0) {
        dag_emit(ctx->entries, ctx->nentries, ctx->bufcap,
                 DAG_PREV_BLOB, ctx->commit_gen, new_blob_h,
                 DAG_PREV_BLOB, ctx->parent_gen, old_blob_h);
    }

    // PATH_VER + BLOB_COMMIT
    u32 pid = dag_paths_intern(ctx->paths, path, pathlen);
    dag_emit(ctx->entries, ctx->nentries, ctx->bufcap,
             DAG_PATH_VER, ctx->commit_gen, (u64)pid,
             DAG_PATH_VER, ctx->commit_gen, new_blob_h);
    dag_emit(ctx->entries, ctx->nentries, ctx->bufcap,
             DAG_BLOB_COMMIT, ctx->commit_gen, new_blob_h,
             DAG_BLOB_COMMIT, ctx->commit_gen, ctx->commit_hash);
    return OK;
}

// --- Main entry: walk keeper's index for commits ---

ok64 DAGHook(keeper *k, u8cs reporoot) {
    sane(k && $ok(reporoot));

    // Resolve .dogs/graf/
    a_path(dagpath, reporoot);
    a_cstr(dagrel, "/" DAG_DIR);
    call(u8bFeed, dagpath, dagrel);
    call(PATHu8bTerm, dagpath);
    a_dup(u8c, dagdir, u8bDataC(dagpath));
    call(FILEMakeDirP, $path(dagpath));

    // Read current ref tips from keeper REFS
    sha1 tips[DAG_MAX_SHAS] = {};
    u32 ntips = dag_read_tips(tips, DAG_MAX_SHAS, k);
    if (ntips == 0) {
        fprintf(stderr, "graf-dag: no refs in keeper\n");
        fail(DAGFAIL);
    }

    // Load saved bookmarks (validated against keeper)
    sha1 bookmarks[DAG_MAX_SHAS] = {};
    u32 nbookmarks = dag_load_bookmarks(bookmarks, DAG_MAX_SHAS,
                                         dagdir, k);

    // Build a set of known commit hashlets to skip
    u64 known_commits[DAG_MAX_SHAS];
    for (u32 i = 0; i < nbookmarks; i++) {
        known_commits[i] = WHIFFHashlet60(&bookmarks[i]);
    }

    if (nbookmarks > 0)
        fprintf(stderr, "graf-dag: incremental, %u bookmark(s)\n", nbookmarks);
    else
        fprintf(stderr, "graf-dag: full reindex\n");

    // Open existing stack for gen lookups
    dag_stack stack = {};
    call(dag_stack_open, &stack, dagdir);

    dag_gens gens = {};
    call(dag_gens_init, &gens);

    dag_paths paths = {};
    call(dag_paths_init, &paths, dagdir);

    // Allocate entry buffer
    size_t bufcap = DAG_BATCH;
    belt128 *entries = (belt128 *)malloc(bufcap * sizeof(belt128));
    if (!entries) {
        dag_gens_free(&gens);
        dag_paths_free(&paths);
        dag_stack_close(&stack);
        failc(DAGFAIL);
    }
    size_t nentries = 0;

    u64 seqno = 0;
    dag_next_seqno(&seqno, dagdir);

    // Walk keeper's kv64 index runs for commit objects
    Bu8 commit_buf = {};
    call(u8bMap, commit_buf, 4UL << 20);  // 4 MB scratch for commit content
    Bu8 tree_old_buf = {}, tree_new_buf = {};
    call(u8bMap, tree_old_buf, 4UL << 20);
    call(u8bMap, tree_new_buf, 4UL << 20);
    char pathbuf[4096];

    u32 indexed = 0;
    ok64 result = OK;

    for (u32 ri = 0; ri < k->nruns && result == OK; ri++) {
        wh128cp base = k->runs[ri][0];
        size_t rlen = (size_t)(k->runs[ri][1] - base);

        for (size_t ei = 0; ei < rlen && result == OK; ei++) {
            u8 type = keepKeyType(base[ei].key);
            if (type != DOG_OBJ_COMMIT) continue;

            u64 commit_h = keepKeyHashlet(base[ei].key);

            // Skip if already known (bookmarked)
            b8 skip = NO;
            for (u32 bi = 0; bi < nbookmarks; bi++) {
                if (known_commits[bi] == commit_h) { skip = YES; break; }
            }
            if (skip) continue;

            // Skip if already indexed (check dag_stack for COMMIT_GEN)
            if (dag_stack_gen(&stack, commit_h) > 0) continue;
            // Skip if indexed in current batch
            if (dag_gens_get(&gens, commit_h) > 0) continue;

            // Inflate commit
            u8bReset(commit_buf);
            u8 obj_type = 0;
            if (KEEPGet(k, commit_h, 15, commit_buf, &obj_type) != OK) continue;
            if (obj_type != DOG_OBJ_COMMIT) continue;

            // Parse commit: extract tree SHA and parent SHAs
            a_dup(u8c, scan, u8bDataC(commit_buf));
            u8cs field = {}, value = {};
            sha1 tree_sha = {};
            sha1 parent_shas[16] = {};
            int npar = 0;
            b8 got_tree = NO;

            while (GITu8sDrainCommit(scan, field, value) == OK) {
                if (u8csEmpty(field)) break;
                a_cstr(f_tree, "tree");
                a_cstr(f_parent, "parent");
                if ($eq(field, f_tree) && u8csLen(value) >= 40) {
                    DAGsha1FromHex(&tree_sha, (char const *)value[0]);
                    got_tree = YES;
                } else if ($eq(field, f_parent) && u8csLen(value) >= 40 && npar < 16) {
                    DAGsha1FromHex(&parent_shas[npar], (char const *)value[0]);
                    npar++;
                }
            }
            if (!got_tree) continue;

            // Compute gen from parent gens
            u32 gen = 1;
            u64 parent_hashlets[16] = {};
            for (int pi = 0; pi < npar; pi++) {
                parent_hashlets[pi] = WHIFFHashlet60(&parent_shas[pi]);
                u32 pg = dag_gens_get(&gens, parent_hashlets[pi]);
                if (pg == 0) pg = dag_stack_gen(&stack, parent_hashlets[pi]);
                if (pg >= gen) gen = pg + 1;
            }

            dag_gens_put(&gens, commit_h, gen);

            // Emit COMMIT_GEN
            dag_emit(entries, &nentries, bufcap,
                     DAG_COMMIT_GEN, gen, commit_h,
                     DAG_COMMIT_GEN, gen, commit_h);

            // Emit COMMIT_PARENT for each parent
            for (int pi = 0; pi < npar; pi++) {
                u32 pgen = dag_gens_get(&gens, parent_hashlets[pi]);
                if (pgen == 0) pgen = dag_stack_gen(&stack, parent_hashlets[pi]);
                dag_emit(entries, &nentries, bufcap,
                         DAG_COMMIT_PARENT, gen, commit_h,
                         DAG_COMMIT_PARENT, pgen, parent_hashlets[pi]);
            }

            // Emit COMMIT_TREE
            u64 tree_h = WHIFFHashlet60(&tree_sha);
            dag_emit(entries, &nentries, bufcap,
                     DAG_COMMIT_TREE, gen, commit_h,
                     DAG_COMMIT_TREE, gen, tree_h);

            // Diff tree vs first parent's tree (if parent exists)
            u8bReset(tree_new_buf);
            u8 tt = 0;
            if (KEEPGet(k, tree_h, 15, tree_new_buf, &tt) != OK) goto next;

            if (npar > 0) {
                // Get parent commit → parent tree
                Bu8 parent_commit = {};
                if (u8bMap(parent_commit, 1UL << 20) != OK) goto next;
                u8 pt = 0;
                u32 parent_gen = dag_gens_get(&gens, parent_hashlets[0]);
                if (parent_gen == 0)
                    parent_gen = dag_stack_gen(&stack, parent_hashlets[0]);

                if (KEEPGet(k, parent_hashlets[0], 15, parent_commit, &pt) == OK &&
                    pt == DOG_OBJ_COMMIT) {
                    a_dup(u8c, pscan, u8bDataC(parent_commit));
                    u8cs pf = {}, pv = {};
                    sha1 ptree = {};
                    while (GITu8sDrainCommit(pscan, pf, pv) == OK) {
                        if (u8csEmpty(pf)) break;
                        a_cstr(ft, "tree");
                        if ($eq(pf, ft) && u8csLen(pv) >= 40) {
                            DAGsha1FromHex(&ptree, (char const *)pv[0]);
                            break;
                        }
                    }
                    u64 ptree_h = WHIFFHashlet60(&ptree);

                    u8bReset(tree_old_buf);
                    u8 ott = 0;
                    if (KEEPGet(k, ptree_h, 15, tree_old_buf, &ott) == OK) {
                        dag_diff_ctx dctx = {
                            .entries = entries, .nentries = &nentries,
                            .bufcap = bufcap, .gens = &gens, .stack = &stack,
                            .paths = &paths, .commit_hash = commit_h,
                            .commit_gen = gen, .parent_gen = parent_gen
                        };
                        a_dup(u8c, otree, u8bDataC(tree_old_buf));
                        a_dup(u8c, ntree, u8bDataC(tree_new_buf));
                        dag_tree_diff_r(k, otree, ntree, pathbuf, 0,
                                        sizeof(pathbuf), dag_diff_cb, &dctx);
                    }
                }
                u8bUnMap(parent_commit);
            } else {
                // Root commit: diff against empty tree
                u8cs empty = {};
                dag_diff_ctx dctx = {
                    .entries = entries, .nentries = &nentries,
                    .bufcap = bufcap, .gens = &gens, .stack = &stack,
                    .paths = &paths, .commit_hash = commit_h,
                    .commit_gen = gen, .parent_gen = 0
                };
                a_dup(u8c, ntree, u8bDataC(tree_new_buf));
                dag_tree_diff_r(k, empty, ntree, pathbuf, 0,
                                sizeof(pathbuf), dag_diff_cb, &dctx);
            }
next:
            indexed++;

            // Flush if buffer is getting full
            if (nentries + 64 >= bufcap) {
                belt128s d = {entries, entries + nentries};
                belt128sSort(d);
                belt128sDedup(d);
                nentries = (size_t)(d[1] - d[0]);
                belt128cs run = {entries, entries + nentries};
                result = dag_index_write(dagdir, run, seqno);
                seqno++;
                nentries = 0;
            }
        }
    }

    // Flush remaining
    if (result == OK && nentries > 0) {
        belt128s d = {entries, entries + nentries};
        belt128sSort(d);
        belt128sDedup(d);
        nentries = (size_t)(d[1] - d[0]);
        belt128cs run = {entries, entries + nentries};
        result = dag_index_write(dagdir, run, seqno);
    }

    u8bUnMap(commit_buf);
    u8bUnMap(tree_old_buf);
    u8bUnMap(tree_new_buf);
    free(entries);
    dag_gens_free(&gens);
    dag_paths_free(&paths);
    dag_stack_close(&stack);

    if (result != OK) return result;

    dag_compact(dagdir);
    dag_write_tips(dagdir, tips, ntips);

    fprintf(stderr, "graf-dag: indexed %u commit(s)\n", indexed);
    done;
}
