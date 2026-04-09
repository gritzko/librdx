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
#include "abc/HEX.h"
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

// --- COMMIT bookmark file ---

static b8 dag_is_hex_sha(char const *s, size_t len) {
    if (len < 40) return NO;
    for (int i = 0; i < 40; i++) {
        u8 c = (u8)s[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return NO;
    }
    return YES;
}

static ok64 dag_commit_read(u32 *count, u8cs dagdir,
                             char shas[][44], u32 maxcount) {
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
            memcpy(shas[*count], p, 40);
            shas[*count][40] = 0;
            (*count)++;
        }
        p = lend;
    }
    FILEUnMap(mapped);
    done;
}

static ok64 dag_commit_write(u8cs dagdir, char const newsha[40]) {
    sane($ok(dagdir));

    char shas[DAG_MAX_SHAS][44];
    u32 sha_count = 0;
    dag_commit_read(&sha_count, dagdir, shas, DAG_MAX_SHAS);

    if (sha_count > 0 && memcmp(shas[sha_count - 1], newsha, 40) == 0) done;

    u32 keep_start = 0;
    if (sha_count >= DAG_MAX_SHAS)
        keep_start = sha_count - DAG_MAX_SHAS + 1;

    a_path(path, dagdir);
    a_cstr(name, "/COMMIT");
    call(u8bFeed, path, name);
    call(PATHu8gTerm, PATHu8gIn(path));

    int fd = -1;
    call(FILECreate, &fd, PATHu8cgIn(path));
    u8cs nl = {(u8cp)"\n", (u8cp)"\n" + 1};
    for (u32 i = keep_start; i < sha_count; i++) {
        u8cs data = {(u8cp)shas[i], (u8cp)shas[i] + 40};
        call(FILEFeedall, fd, data);
        call(FILEFeedall, fd, nl);
    }
    u8cs newdata = {(u8cp)newsha, (u8cp)newsha + 40};
    call(FILEFeedall, fd, newdata);
    call(FILEFeedall, fd, nl);
    close(fd);
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

// --- Path log: append-only NUL-separated file + in-memory hash→id map ---

typedef struct {
    u64 *hashes;   // RAPHash of path string
    u32 *ids;      // path_id = offset into PATHS file at time of insert
    u32 count;
    u32 next_id;   // next path_id to assign
    int fd;        // open PATHS file for appending
} dag_paths;

static ok64 dag_paths_init(dag_paths *p, u8cs dagdir) {
    sane(p && $ok(dagdir));
    memset(p, 0, sizeof(*p));
    p->fd = -1;

    p->hashes = calloc(DAG_MAX_PATHS, sizeof(u64));
    p->ids = calloc(DAG_MAX_PATHS, sizeof(u32));
    if (!p->hashes || !p->ids) {
        free(p->hashes); free(p->ids);
        return DAGFAIL;
    }

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
            u32 slot = (u32)(h >> 4) & (DAG_MAX_PATHS - 1);
            while (p->hashes[slot] != 0) slot = (slot + 1) & (DAG_MAX_PATHS - 1);
            p->hashes[slot] = h ? h : 1;
            p->ids[slot] = id;
            p->count++;
            u32 after = (u32)(nul + 1 - base);
            if (after > p->next_id) p->next_id = after;
            cur = nul + 1;
        }
        FILEUnMap(mapped);
    }

    // Open for append
    p->fd = open((char *)u8bDataHead(path), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (p->fd < 0) {
        free(p->hashes); free(p->ids);
        return DAGFAIL;
    }
    done;
}

static void dag_paths_free(dag_paths *p) {
    if (p->fd >= 0) close(p->fd);
    free(p->hashes); free(p->ids);
    memset(p, 0, sizeof(*p));
    p->fd = -1;
}

// Get or create path_id for a path string.  Returns the id.
static u32 dag_paths_intern(dag_paths *p, char const *str, size_t len) {
    u8csc ps = {(u8cp)str, (u8cp)str + len};
    u64 h = RAPHash(ps);
    u64 hkey = h ? h : 1;
    u32 slot = (u32)(h >> 4) & (DAG_MAX_PATHS - 1);
    while (p->hashes[slot] != 0) {
        if (p->hashes[slot] == hkey) return p->ids[slot];
        slot = (slot + 1) & (DAG_MAX_PATHS - 1);
    }
    // New path: append to file
    u32 id = p->next_id;
    write(p->fd, str, len);
    write(p->fd, "\0", 1);
    p->next_id = id + (u32)len + 1;

    p->hashes[slot] = hkey;
    p->ids[slot] = id;
    p->count++;
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

typedef struct {
    belt128cs runs[MSET_MAX_LEVELS];
    u8bp      maps[MSET_MAX_LEVELS];
    u32       n;
} dag_stack;

static ok64 dag_stack_open(dag_stack *st, u8cs dagdir) {
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

static void dag_stack_close(dag_stack *st) {
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

// --- SHA hex → 40-bit hashlet ---

static u64 dag_hex_to_hashlet(char const *hex40) {
    u8 bin[20] = {};
    u8s sb = {bin, bin + 20};
    u8cs hx = {(u8cp)hex40, (u8cp)hex40 + 40};
    if (HEXu8sDrainSome(sb, hx) != OK) return 0;
    u64 h = 0;
    memcpy(&h, bin, 8);
    // Take low 40 bits (first 5 bytes of SHA-1)
    return h & DAG_HASH_MASK;
}

// --- Read all branch tips via git for-each-ref ---

static u32 dag_read_tips(char tips[][44], u32 maxtips, u8cs reporoot) {
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
            memcpy(tips[count], buf, 40);
            tips[count][40] = 0;
            count++;
        }
    }
    pclose(fp);
    return count;
}

// --- Write all branch tips to COMMIT ---

static ok64 dag_write_tips(u8cs dagdir, char tips[][44], u32 ntips) {
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
        u8cs data = {(u8cp)tips[i], (u8cp)tips[i] + 40};
        call(FILEFeedall, fd, data);
        call(FILEFeedall, fd, nl);
    }
    close(fd);
    done;
}

// --- Build --not clause from saved SHAs (validated against repo) ---

static u32 dag_load_bookmarks(char out[][44], u32 maxout,
                               u8cs dagdir, u8cs reporoot) {
    char shas[DAG_MAX_SHAS][44];
    u32 sha_count = 0;
    dag_commit_read(&sha_count, dagdir, shas, DAG_MAX_SHAS);

    // Validate each saved SHA still exists in the repo
    u32 valid = 0;
    for (u32 i = 0; i < sha_count && valid < maxout; i++) {
        char cmd[1024];
        int n = snprintf(cmd, sizeof(cmd),
                         "git -C %.*s cat-file -t %.40s >/dev/null 2>&1",
                         (int)$len(reporoot), (char *)reporoot[0],
                         shas[i]);
        if (n <= 0 || n >= (int)sizeof(cmd)) continue;
        int rc = system(cmd);
        if (WIFEXITED(rc) && WEXITSTATUS(rc) == 0) {
            memcpy(out[valid], shas[i], 40);
            out[valid][40] = 0;
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
    char old_sha[44];
    char new_sha[44];
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
    memcpy(out->old_sha, p, 40);
    out->old_sha[40] = 0;
    p += 40;
    if (p >= end || *p != ' ') return NO;
    p++;

    // new_sha
    if (p + 40 > end) return NO;
    memcpy(out->new_sha, p, 40);
    out->new_sha[40] = 0;
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
    char tips[DAG_MAX_SHAS][44] = {};
    u32 ntips = dag_read_tips(tips, DAG_MAX_SHAS, reporoot);
    if (ntips == 0) fail(DAGFAIL);

    // Load saved bookmarks (validated)
    char bookmarks[DAG_MAX_SHAS][44] = {};
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

    // Spawn git rev-list --all --not <bookmark1> <bookmark2> ...
    char rlcmd[4096];
    int pos = snprintf(rlcmd, sizeof(rlcmd),
                       "git -C %.*s rev-list --topo-order --reverse --parents"
                       " --all",
                       (int)$len(reporoot), (char *)reporoot[0]);
    if (nbookmarks > 0) {
        pos += snprintf(rlcmd + pos, sizeof(rlcmd) - pos, " --not");
        for (u32 i = 0; i < nbookmarks && pos < (int)sizeof(rlcmd) - 50; i++)
            pos += snprintf(rlcmd + pos, sizeof(rlcmd) - pos,
                            " %.40s", bookmarks[i]);
    }

    FILE *rl = popen(rlcmd, "r");
    if (!rl) {
        free(entries);
        dag_gens_free(&gens);
        dag_paths_free(&paths);
        dag_stack_close(&stack);
        failc(DAGFAIL);
    }

    u32 indexed = 0;
    ok64 result = OK;

    char line[8192];
    while (fgets(line, sizeof(line), rl)) {
        size_t llen = strlen(line);
        while (llen > 0 && (line[llen - 1] == '\n' || line[llen - 1] == '\r'))
            line[--llen] = 0;
        if (llen < 40) continue;

        u64 commit_hash = 0;
        u32 commit_gen = 0;
        u64 parents[16] = {};
        int npar = dag_parse_commit(line, llen, &gens, &stack,
                                    &commit_hash, &commit_gen,
                                    parents, 16);
        if (npar < 0) continue;

        dag_gens_put(&gens, commit_hash, commit_gen);

        // COMMIT_GEN
        dag_emit(entries, &nentries, bufcap,
                 DAG_COMMIT_GEN, commit_gen, commit_hash,
                 DAG_COMMIT_GEN, commit_gen, commit_hash);

        // COMMIT_PARENT (one per parent)
        for (int p = 0; p < npar && p < 16; p++) {
            u32 pgen = dag_gens_get(&gens, parents[p]);
            if (pgen == 0) pgen = dag_stack_gen(&stack, parents[p]);
            dag_emit(entries, &nentries, bufcap,
                     DAG_COMMIT_PARENT, commit_gen, commit_hash,
                     DAG_COMMIT_PARENT, pgen, parents[p]);
        }

        // Run git diff-tree to get file changes for this commit
        {
            char dtcmd[2048];
            if (npar > 0) {
                // Diff against first parent
                char parent_buf[44] = {};
                char const *pp = line + 41; // skip commit sha + space
                memcpy(parent_buf, pp, 40);
                parent_buf[40] = 0;
                snprintf(dtcmd, sizeof(dtcmd),
                         "git -C %.*s diff-tree -r -M --no-commit-id -z %s %.40s",
                         (int)$len(reporoot), (char *)reporoot[0],
                         parent_buf, line);
            } else {
                // Root commit: --root shows all files as added
                snprintf(dtcmd, sizeof(dtcmd),
                         "git -C %.*s diff-tree -r --root --no-commit-id -z %.40s",
                         (int)$len(reporoot), (char *)reporoot[0],
                         line);
            }

            FILE *dt = popen(dtcmd, "r");
            if (!dt) continue;

            // diff-tree -z uses NUL separators
            // Format: ":old_mode new_mode old_sha new_sha status\0path\0"
            // For renames: "...status\0old_path\0new_path\0"
            char dtbuf[65536];
            size_t dtlen = fread(dtbuf, 1, sizeof(dtbuf) - 1, dt);
            pclose(dt);
            dtbuf[dtlen] = 0;

            char *dp = dtbuf;
            char *dend = dtbuf + dtlen;
            while (dp < dend && *dp == ':') {
                // Parse the colon record up to NUL
                char *rec_end = dp;
                while (rec_end < dend && *rec_end != 0) rec_end++;
                size_t reclen = (size_t)(rec_end - dp);

                dag_diff_entry de = {};
                if (dag_parse_diff_rec(dp, reclen, &de)) {
                    dp = rec_end + 1; // past NUL after colon record

                    // Read first path (NUL-terminated)
                    char path1[512] = {};
                    if (dp < dend) {
                        char *p1end = dp;
                        while (p1end < dend && *p1end != 0) p1end++;
                        size_t p1len = (size_t)(p1end - dp);
                        if (p1len >= sizeof(path1)) p1len = sizeof(path1) - 1;
                        memcpy(path1, dp, p1len);
                        path1[p1len] = 0;
                        dp = p1end + 1;
                    }
                    // Second path for renames/copies (NUL-terminated)
                    char path2[512] = {};
                    if ((de.status == 'R' || de.status == 'C') && dp < dend) {
                        char *p2end = dp;
                        while (p2end < dend && *p2end != 0) p2end++;
                        size_t p2len = (size_t)(p2end - dp);
                        if (p2len >= sizeof(path2)) p2len = sizeof(path2) - 1;
                        memcpy(path2, dp, p2len);
                        path2[p2len] = 0;
                        dp = p2end + 1;
                    }

                    // Effective path: new path for renames, otherwise path1
                    char const *epath = path1;
                    size_t eplen = strlen(path1);
                    if ((de.status == 'R' || de.status == 'C') && path2[0]) {
                        epath = path2;
                        eplen = strlen(path2);
                    }

                    if (eplen == 0) continue;

                    // PREV_BLOB: new_blob → old_blob
                    if (de.status != 'A' && de.status != 'D') {
                        u64 old_bh = dag_hex_to_hashlet(de.old_sha);
                        u64 new_bh = dag_hex_to_hashlet(de.new_sha);
                        if (old_bh && new_bh) {
                            // old_blob gen: use parent commit gen
                            u32 pgen = (npar > 0) ? dag_gens_get(&gens, parents[0]) : 0;
                            if (pgen == 0 && npar > 0)
                                pgen = dag_stack_gen(&stack, parents[0]);
                            dag_emit(entries, &nentries, bufcap,
                                     DAG_PREV_BLOB, commit_gen, new_bh,
                                     DAG_PREV_BLOB, pgen, old_bh);
                        }
                    }

                    // PATH_VER: (gen, path_id) → (gen, blob)
                    if (de.status != 'D') {
                        u64 new_bh = dag_hex_to_hashlet(de.new_sha);
                        u32 pid = dag_paths_intern(&paths, epath, eplen);
                        dag_emit(entries, &nentries, bufcap,
                                 DAG_PATH_VER, commit_gen, (u64)pid,
                                 DAG_PATH_VER, commit_gen, new_bh);
                    }
                } else {
                    // Skip malformed record
                    dp = rec_end + 1;
                    // Skip path field(s)
                    while (dp < dend && *dp != ':' && *dp != 0) dp++;
                    if (dp < dend && *dp == 0) dp++;
                }
            }
        }

        indexed++;

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
    pclose(rl);

    // Flush remaining
    if (result == OK && nentries > 0) {
        belt128s d = {entries, entries + nentries};
        belt128sSort(d);
        belt128sDedup(d);
        nentries = (size_t)(d[1] - d[0]);
        belt128cs run = {entries, entries + nentries};
        result = dag_index_write(dagdir, run, seqno);
    }

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
