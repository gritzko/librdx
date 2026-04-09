//  DAG: graf's git object-graph index.
//
//  Drives `git rev-list` + `git diff-tree` to enumerate new commits
//  and their file changes, writing belt128 records into LSM sorted
//  runs under <reporoot>/.dogs/graf/.
//
#include "DAG.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "abc/FILE.h"
#include "abc/KV.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/RAP.h"
#include "abc/RON.h"

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
    call(PATHu8gTerm, PATHu8gIn(path));

    u8bp mapped = NULL;
    ok64 o = FILEMapRO(&mapped, PATHu8cgIn(path));
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
    call(PATHu8gTerm, PATHu8gIn(path));

    u8bp mapped = NULL;
    ok64 o = FILEMapRO(&mapped, PATHu8cgIn(path));
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
    call(PATHu8gTerm, PATHu8gIn(path));

    int fd = -1;
    call(FILECreate, &fd, PATHu8cgIn(path));
    size_t bytes = $len(run) * sizeof(belt128);
    u8cs data = {(u8cp)run[0], (u8cp)run[0] + bytes};
    call(FILEFeedall, fd, data);
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
        if (FILEMapRO(&mapped, PATHu8cgIn(fpath)) != OK) continue;
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
    u64 key_a_hi = DAGPack(DAG_COMMIT_GEN, DAG_GEN_MASK, hashlet);
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
        PATHu8bFeed(dpat, dagdir);
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

// --- Read all branch tips via git for-each-ref ---

static u32 dag_read_tips(sha1 tips[], u32 maxtips, u8cs reporoot) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "git -C %.*s for-each-ref --format='%%(objectname)'",
             (int)$len(reporoot), (char *)reporoot[0]);
    FILE *fp = popen(cmd, "r");
    if (!fp) return 0;
    u32 count = 0;
    char buf[64];
    while (fgets(buf, sizeof(buf), fp) && count < maxtips) {
        size_t l = strlen(buf);
        while (l > 0 && (buf[l - 1] == '\n' || buf[l - 1] == '\r'))
            buf[--l] = 0;
        if (dag_is_hex_sha(buf, l)) {
            DAGsha1FromHex(&tips[count], buf);
            count++;
        }
    }
    pclose(fp);
    return count;
}

// --- Write sha1 array to COMMIT as hex lines ---

static ok64 dag_write_tips(u8cs dagdir, sha1 const tips[], u32 ntips) {
    sane($ok(dagdir));
    if (ntips == 0) done;

    a_path(path, dagdir);
    a_cstr(name, "/COMMIT");
    call(u8bFeed, path, name);
    call(PATHu8gTerm, PATHu8gIn(path));

    int fd = -1;
    call(FILECreate, &fd, PATHu8cgIn(path));
    u8cs nl = {(u8cp)"\n", (u8cp)"\n" + 1};
    for (u32 i = 0; i < ntips; i++) {
        char hex[41];
        DAGsha1ToHex(hex, &tips[i]);
        u8cs data = {(u8cp)hex, (u8cp)hex + 40};
        call(FILEFeedall, fd, data);
        call(FILEFeedall, fd, nl);
    }
    close(fd);
    done;
}

// --- Load saved bookmarks, validate they exist in repo ---

static u32 dag_load_bookmarks(sha1 out[], u32 maxout,
                               u8cs dagdir, u8cs reporoot) {
    sha1 shas[DAG_MAX_SHAS] = {};
    u32 sha_count = 0;
    dag_commit_read(&sha_count, dagdir, shas, DAG_MAX_SHAS);

    u32 valid = 0;
    for (u32 i = 0; i < sha_count && valid < maxout; i++) {
        char hex[41];
        DAGsha1ToHex(hex, &shas[i]);
        char cmd[1024];
        int n = snprintf(cmd, sizeof(cmd),
                         "git -C %.*s cat-file -t %s >/dev/null 2>&1",
                         (int)$len(reporoot), (char *)reporoot[0], hex);
        if (n <= 0 || n >= (int)sizeof(cmd)) continue;
        int rc = system(cmd);
        if (WIFEXITED(rc) && WEXITSTATUS(rc) == 0) {
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

// --- Main entry ---

ok64 DAGHook(u8cs reporoot) {
    sane($ok(reporoot));

    // Resolve .dogs/graf/
    a_path(dagpath, reporoot);
    a_cstr(dagrel, "/" DAG_DIR);
    call(u8bFeed, dagpath, dagrel);
    call(PATHu8gTerm, PATHu8gIn(dagpath));
    a_dup(u8c, dagdir, u8bDataC(dagpath));
    call(FILEMakeDirP, PATHu8cgIn(dagpath));

    // Read current branch tips
    sha1 tips[DAG_MAX_SHAS] = {};
    u32 ntips = dag_read_tips(tips, DAG_MAX_SHAS, reporoot);
    if (ntips == 0) fail(DAGFAIL);

    // Load saved bookmarks (validated)
    sha1 bookmarks[DAG_MAX_SHAS] = {};
    u32 nbookmarks = dag_load_bookmarks(bookmarks, DAG_MAX_SHAS,
                                         dagdir, reporoot);
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

    // Single git log stream: commit headers + raw diffs in one pipe.
    char logcmd[4096];
    int pos = snprintf(logcmd, sizeof(logcmd),
                       "git -C %.*s log --all --topo-order --reverse --parents"
                       " --raw -z --no-abbrev --diff-filter=ACMRT",
                       (int)$len(reporoot), (char *)reporoot[0]);
    if (nbookmarks > 0) {
        pos += snprintf(logcmd + pos, sizeof(logcmd) - pos, " --not");
        for (u32 i = 0; i < nbookmarks && pos < (int)sizeof(logcmd) - 50; i++) {
            char hex[41];
            DAGsha1ToHex(hex, &bookmarks[i]);
            pos += snprintf(logcmd + pos, sizeof(logcmd) - pos, " %s", hex);
        }
    }

    FILE *gl = popen(logcmd, "r");
    if (!gl) {
        free(entries);
        dag_gens_free(&gens);
        dag_paths_free(&paths);
        dag_stack_close(&stack);
        failc(DAGFAIL);
    }

    u32 indexed = 0;
    ok64 result = OK;

    // State for the current commit being processed
    u64 commit_hash = 0;
    u32 commit_gen = 0;
    u64 parents[16] = {};
    int npar = -1;  // -1 = no current commit

    // Read stream line by line.  "commit " lines start a new commit;
    // colon lines are raw diff entries; everything else is skipped.
    // With -z, colon records end with NUL, paths are NUL-separated.
    // We read in large chunks and parse in-buffer.
    size_t bufsz = 1 << 18;  // 256KB read buffer
    char *buf = (char *)malloc(bufsz);
    if (!buf) { pclose(gl); free(entries); dag_gens_free(&gens);
                dag_paths_free(&paths); dag_stack_close(&stack); failc(DAGFAIL); }
    size_t buflen = 0;
    b8 eof = NO;

    while (!eof || buflen > 0) {
        // Fill buffer
        if (!eof) {
            size_t room = bufsz - buflen - 1;
            if (room > 0) {
                size_t got = fread(buf + buflen, 1, room, gl);
                if (got == 0) eof = YES;
                buflen += got;
            }
        }
        buf[buflen] = 0;

        char *p = buf;
        char *end = buf + buflen;

        while (p < end) {
            if (*p == ':') {
                // Colon record: parse diff entry
                // Format: ":old_mode new_mode old_sha new_sha status\0path\0"
                char *rec_end = p;
                while (rec_end < end && *rec_end != 0) rec_end++;
                if (rec_end >= end && !eof) break;  // incomplete, need more data
                size_t reclen = (size_t)(rec_end - p);

                dag_diff_entry de = {};
                if (dag_parse_diff_rec(p, reclen, &de) && npar >= 0) {
                    p = rec_end + 1;  // past NUL

                    // Read first path (NUL-terminated)
                    char *p1 = p;
                    while (p < end && *p != 0) p++;
                    if (p >= end && !eof) { p = p1 - reclen - 1; break; }
                    size_t p1len = (size_t)(p - p1);
                    p++;  // past NUL

                    // Second path for renames/copies
                    char *p2 = NULL;
                    size_t p2len = 0;
                    if ((de.status == 'R' || de.status == 'C') && p < end) {
                        p2 = p;
                        while (p < end && *p != 0) p++;
                        if (p >= end && !eof) { p = p1 - reclen - 1; break; }
                        p2len = (size_t)(p - p2);
                        p++;
                    }

                    char const *epath = p1;
                    size_t eplen = p1len;
                    if (p2 && p2len > 0) { epath = p2; eplen = p2len; }
                    if (eplen == 0) continue;

                    // PREV_BLOB
                    if (de.status != 'A' && de.status != 'D') {
                        u64 old_bh = DAGsha1Hashlet(&de.old_sha);
                        u64 new_bh = DAGsha1Hashlet(&de.new_sha);
                        if (old_bh && new_bh) {
                            u32 pgen = (npar > 0) ? dag_gens_get(&gens, parents[0]) : 0;
                            if (pgen == 0 && npar > 0)
                                pgen = dag_stack_gen(&stack, parents[0]);
                            dag_emit(entries, &nentries, bufcap,
                                     DAG_PREV_BLOB, commit_gen, new_bh,
                                     DAG_PREV_BLOB, pgen, old_bh);
                        }
                    }

                    // PATH_VER + BLOB_COMMIT
                    if (de.status != 'D') {
                        u64 new_bh = DAGsha1Hashlet(&de.new_sha);
                        u32 pid = dag_paths_intern(&paths, epath, eplen);
                        dag_emit(entries, &nentries, bufcap,
                                 DAG_PATH_VER, commit_gen, (u64)pid,
                                 DAG_PATH_VER, commit_gen, new_bh);
                        dag_emit(entries, &nentries, bufcap,
                                 DAG_BLOB_COMMIT, commit_gen, new_bh,
                                 DAG_BLOB_COMMIT, commit_gen, commit_hash);
                    }
                } else {
                    p = rec_end + 1;
                    // Skip path(s)
                    while (p < end && *p != ':' && *p != '\n' && *p != 'c') {
                        while (p < end && *p != 0 && *p != '\n') p++;
                        if (p < end && *p == 0) p++;
                    }
                }
            } else if (p + 7 <= end && memcmp(p, "commit ", 7) == 0 && p + 47 <= end) {
                // Commit header: "commit <sha> [<parent>...]\n"
                char *nl = p;
                while (nl < end && *nl != '\n') nl++;
                if (nl >= end && !eof) break;  // incomplete

                p += 7;  // skip "commit "
                size_t llen = (size_t)(nl - p);

                // Parse as rev-list line: "<sha> <p1> <p2> ..."
                npar = dag_parse_commit(p, llen, &gens, &stack,
                                        &commit_hash, &commit_gen,
                                        parents, 16);
                if (npar >= 0) {
                    dag_gens_put(&gens, commit_hash, commit_gen);

                    dag_emit(entries, &nentries, bufcap,
                             DAG_COMMIT_GEN, commit_gen, commit_hash,
                             DAG_COMMIT_GEN, commit_gen, commit_hash);

                    for (int pi = 0; pi < npar && pi < 16; pi++) {
                        u32 pgen = dag_gens_get(&gens, parents[pi]);
                        if (pgen == 0) pgen = dag_stack_gen(&stack, parents[pi]);
                        dag_emit(entries, &nentries, bufcap,
                                 DAG_COMMIT_PARENT, commit_gen, commit_hash,
                                 DAG_COMMIT_PARENT, pgen, parents[pi]);
                    }
                    indexed++;
                }
                p = nl + 1;
            } else if (*p == 0) {
                // NUL separator between diff records / commits
                p++;
            } else {
                // Skip non-commit, non-colon lines (Author, Date, message)
                char *nl = p;
                while (nl < end && *nl != '\n' && *nl != 0) nl++;
                if (nl >= end && !eof) break;
                p = nl + 1;
            }

            // Flush if buffer is getting full
            if (nentries + 64 >= bufcap) {
                belt128s d = {entries, entries + nentries};
                belt128sSort(d);
                belt128sDedup(d);
                nentries = (size_t)(d[1] - d[0]);
                belt128cs run = {entries, entries + nentries};
                result = dag_index_write(dagdir, run, seqno);
                if (result != OK) break;
                seqno++;
                nentries = 0;
            }
        }

        // Shift unconsumed data to front
        size_t consumed = (size_t)(p - buf);
        buflen -= consumed;
        if (buflen > 0) memmove(buf, p, buflen);

        if (result != OK) break;
    }
    pclose(gl);

    // Flush remaining
    if (result == OK && nentries > 0) {
        belt128s d = {entries, entries + nentries};
        belt128sSort(d);
        belt128sDedup(d);
        nentries = (size_t)(d[1] - d[0]);
        belt128cs run = {entries, entries + nentries};
        result = dag_index_write(dagdir, run, seqno);
    }

    free(buf);
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
