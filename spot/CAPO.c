#include "CAPO.h"

#include <dirent.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

b8 CAPO_COLOR = NO;
b8 CAPO_TERM = NO;

// Guard against assert() crashes in library code
static sigjmp_buf capo_jmpbuf;
static volatile sig_atomic_t capo_in_match = 0;

static void capo_abrt_handler(int sig) {
    (void)sig;
    if (capo_in_match) siglongjmp(capo_jmpbuf, 1);
    _exit(134);
}

#include "abc/ANSI.h"
#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/SORT.h"
#include "spot/LESS.h"
#include "spot/NEIL.h"
#include "spot/SPOT.h"
#include "tok/DEF.h"
#include "tok/JOIN.h"

// --- LESS pager arena: shared across grep/diff/spot ---
#define LESS_ARENA_SIZE (1UL << 27)   // 128MB
#define LESS_MAX_HUNKS 4096
#define LESS_MAX_MAPS 1024
static Bu8 less_arena = {};
static LESShunk less_hunks[LESS_MAX_HUNKS];
static u8bp less_maps[LESS_MAX_MAPS];
static Bu32 less_toks[LESS_MAX_MAPS];
static u32 less_nhunks = 0;
static u32 less_nmaps = 0;

static ok64 LESSArenaInit(void) {
    less_nhunks = 0;
    less_nmaps = 0;
    memset(less_hunks, 0, sizeof(less_hunks));
    memset(less_maps, 0, sizeof(less_maps));
    memset(less_toks, 0, sizeof(less_toks));
    if (less_arena[0] != NULL) {
        // Reset idle pointer to start
        ((u8 **)less_arena)[2] = less_arena[1];
        return OK;
    }
    return u8bMap(less_arena, LESS_ARENA_SIZE);
}

static void LESSArenaCleanup(void) {
    for (u32 i = 0; i < less_nmaps; i++) {
        if (less_toks[i][0] != NULL) u32bUnMap(less_toks[i]);
        if (less_maps[i] != NULL) FILEUnMap(less_maps[i]);
    }
    less_nhunks = 0;
    less_nmaps = 0;
}

// Write bytes into the arena, return pointer to start
static u8p LESSArenaWrite(void const *data, size_t len) {
    if (u8bIdleLen(less_arena) < len) return NULL;
    u8p p = u8bIdleHead(less_arena);
    memcpy(p, data, len);
    ((u8 **)less_arena)[2] = p + len;
    return p;
}

// Reserve len bytes in the arena (zeroed), return pointer
static u8p LESSArenaAlloc(size_t len) {
    if (u8bIdleLen(less_arena) < len) return NULL;
    u8p p = u8bIdleHead(less_arena);
    memset(p, 0, len);
    ((u8 **)less_arena)[2] = p + len;
    return p;
}

// Defer file+toks cleanup until after LESSRun
static void LESSDefer(u8bp mapped, Bu32 toks) {
    if (less_nmaps >= LESS_MAX_MAPS) return;
    less_maps[less_nmaps] = mapped;
    memcpy(less_toks[less_nmaps], toks, sizeof(Bu32));
    less_nmaps++;
}

// MSET for u64 (trigram entries)
#define X(M, name) M##u64##name
#include "abc/MSETx.h"
#undef X

// Myers diff for u64 hash sequences (used by CAPODiff)
#define X(M, name) M##u64##name
#include "abc/DIFFx.h"
#undef X

// --- Language detection via tok/ ---

b8 CAPOKnownExt(u8csc ext);
b8 CAPOKnownExt(u8csc ext) {
    if ($empty(ext)) return NO;
    u8cs nodot = {};
    if (ext[0][0] == '.') {
        nodot[0] = ext[0] + 1;
        nodot[1] = ext[1];
    } else {
        nodot[0] = ext[0]; nodot[1] = ext[1];
    }
    return TOKKnownExt(nodot);
}

// Simple ext-to-name for display (replaces BASTCodec)
static void CAPOCodecName(u8csp codec, u8csc ext) {
    if ($empty(ext)) {
        codec[0] = (u8cp)"text";
        codec[1] = (u8cp)"text" + 4;
        return;
    }
    // Strip dot, use extension as codec name
    if (ext[0][0] == '.') {
        codec[0] = ext[0] + 1;
        codec[1] = ext[1];
    } else {
        $mv(codec, ext);
    }
}

// --- Resolve spot index directory (handles worktrees) ---

ok64 CAPOResolveDir(path8b out, u8csc reporoot) {
    sane($ok(reporoot) && out != NULL);
    a_pad(u8, gitpath, FILE_PATH_MAX_LEN);
    call(PATHu8bFeed, gitpath, reporoot);
    a_cstr(gitname, ".git");
    call(PATHu8bPush, gitpath, gitname);

    ok64 isdir = FILEisdir(PATHu8cgIn(gitpath));
    if (isdir == OK) {
        call(PATHu8bFeed, out, reporoot);
        a_cstr(dotgit, ".git");
        call(PATHu8bPush, out, dotgit);
        a_cstr(spotname, "spot");
        call(PATHu8bPush, out, spotname);
    } else {
        u8bp mapped = NULL;
        call(FILEMapRO, &mapped, PATHu8cgIn(gitpath));
        a_dup(u8c, content, u8bDataC(mapped));
        a_cstr(prefix, "gitdir: ");
        test($len(content) > $len(prefix), PATHBAD);
        u8cs pfxslice = {content[0], content[0] + $len(prefix)};
        test($eq(pfxslice, prefix), PATHBAD);
        u8cp start = content[0] + $len(prefix);
        u8cp end = content[1];
        while (end > start && (*(end - 1) == '\n' || *(end - 1) == '\r'))
            end--;
        u8cs gitdir = {start, end};
        test(!$empty(gitdir), PATHBAD);
        if (gitdir[0][0] == '/') {
            call(PATHu8bFeed, out, gitdir);
        } else {
            call(PATHu8bFeed, out, reporoot);
            call(PATHu8bPush, out, gitdir);
        }
        FILEUnMap(mapped);
        a_cstr(caponame2, "spot");
        call(PATHu8bPush, out, caponame2);
    }
    done;
}

// --- Trigram extraction from tok/ tokens ---

typedef struct {
    u64 **idle;
    u64 *end;
    u32 path_hash;
} CAPOTriCtx;

static ok64 CAPOTriCB(voidp arg, u8cs tri) {
    CAPOTriCtx *ctx = (CAPOTriCtx *)arg;
    if (*ctx->idle >= ctx->end) return CAPONOROOM;
    u64 entry = CAPOTriPack(tri) | (u64)ctx->path_hash;
    **ctx->idle = entry;
    (*ctx->idle)++;
    return OK;
}

// Walk packed tokens, extract RON64 trigrams from source text
static ok64 CAPOTriExtractToks(u32cs toks, u8cp base,
                                ok64 (*cb)(voidp, u8cs), voidp arg) {
    sane(cb != NULL);
    int len = (int)$len(toks);
    for (int i = 0; i < len; i++) {
        u8cs val = {}; tok32Val(val,toks,base,i);
        if ($len(val) < 3) continue;
        u8cp p = val[0];
        u8cp end = val[1] - 2;
        while (p <= end) {
            if (CAPOTriChar(p[0]) && CAPOTriChar(p[1]) &&
                CAPOTriChar(p[2])) {
                u8cs tri = {p, p + 3};
                ok64 r = cb(arg, tri);
                if (r != OK) return r;
            }
            p++;
        }
    }
    return OK;
}

ok64 CAPOIndexFile(u64bp entries, u8csc source, u8csc ext, u8csc path) {
    sane(entries != NULL && $ok(source) && $ok(ext) && $ok(path));
    if ($empty(source)) done;

    // Tokenize
    Bu32 toks = {};
    size_t maxlen = $len(source) + 1;  // at most 1 token per byte
    vcall("mmap toks", u32bMap, toks, maxlen);
    ok64 o = SPOTTokenize(toks, source, ext);
    if (o != OK) {
        u32bUnMap(toks);
        return o;
    }

    u32cp td = u32bDataHead(toks);
    u32cp ti = u32bIdleHead(toks);
    u32cs tokslice = {(u32cp)td, (u32cp)ti};

    CAPOTriCtx ctx = {
        .idle = u64bIdle(entries),
        .end = entries[3],
        .path_hash = CAPOPathHash(path),
    };
    o = CAPOTriExtractToks(tokslice, source[0], CAPOTriCB, &ctx);

    u32bUnMap(toks);
    return o;
}

// --- File I/O ---

ok64 CAPOIndexWrite(u8csc dir, u64cs run, u64 seqno) {
    sane($ok(dir));
    if ($empty(run)) done;

    a_pad(u8, path, FILE_PATH_MAX_LEN);
    call(u8bFeed, path, dir);
    call(u8bFeed1, path, '/');
    call(RONu8sFeedPad, u8bIdle(path), seqno, CAPO_SEQNO_WIDTH);
    ((u8 **)path)[2] += CAPO_SEQNO_WIDTH;
    a_cstr(idxext, CAPO_IDX_EXT);
    call(u8bFeed, path, idxext);
    call(PATHu8gTerm, PATHu8gIn(path));

    int fd = -1;
    call(FILECreate, &fd, PATHu8cgIn(path));
    size_t bytes = $len(run) * sizeof(u64);
    u8cs data = {(u8cp)run[0], (u8cp)run[0] + bytes};
    call(FILEFeedall, fd, data);
    close(fd);
    done;
}

ok64 CAPONextSeqno(u64p seqno, u8csc dir) {
    sane(seqno != NULL && $ok(dir));
    *seqno = 1;

    a_pad(u8, pat, FILE_PATH_MAX_LEN);
    call(PATHu8bFeed, pat, dir);

    int dfd = -1;
    ok64 o = FILEOpenDir(&dfd, PATHu8cgIn(pat));
    if (o != OK) done;

    u64 maxseq = 0;
    DIR *d = fdopendir(dfd);
    if (d == NULL) { close(dfd); done; }

    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        size_t nlen = strlen(e->d_name);
        if (nlen < 5) continue;
        if (strcmp(e->d_name + nlen - 4, CAPO_IDX_EXT) != 0) continue;
        size_t numlen = nlen - 4;
        u8cs numslice = {(u8cp)e->d_name, (u8cp)e->d_name + numlen};
        ok64 val = 0;
        ok64 r = RONutf8sDrain(&val, numslice);
        if (r == OK && val > maxseq) maxseq = val;
    }
    closedir(d);
    *seqno = maxseq + 1;
    done;
}

// --- Stack management ---

ok64 CAPOStackOpen(u64css stack, u8bp *maps, u32p nfiles, u8csc dir) {
    sane($ok(stack) && maps != NULL && nfiles != NULL && $ok(dir));
    *nfiles = 0;

    a_pad(u8, dpat, FILE_PATH_MAX_LEN);
    call(PATHu8bFeed, dpat, dir);

    int dfd = -1;
    ok64 o = FILEOpenDir(&dfd, PATHu8cgIn(dpat));
    if (o != OK) done;

    char names[CAPO_MAX_LEVELS][64];
    u32 count = 0;

    DIR *d = fdopendir(dfd);
    if (d == NULL) { close(dfd); done; }

    struct dirent *e;
    while ((e = readdir(d)) != NULL && count < CAPO_MAX_LEVELS) {
        size_t nlen = strlen(e->d_name);
        if (nlen < 5 || nlen > 63) continue;
        if (strcmp(e->d_name + nlen - 4, CAPO_IDX_EXT) != 0) continue;
        memcpy(names[count], e->d_name, nlen + 1);
        count++;
    }
    closedir(d);

    for (u32 i = 0; i + 1 < count; i++)
        for (u32 j = i + 1; j < count; j++)
            if (strcmp(names[i], names[j]) > 0) {
                char tmp[64];
                memcpy(tmp, names[i], 64);
                memcpy(names[i], names[j], 64);
                memcpy(names[j], tmp, 64);
            }

    for (u32 i = 0; i < count; i++) {
        a_pad(u8, fpath, FILE_PATH_MAX_LEN);
        call(PATHu8bFeed, fpath, dir);
        u8cs fn = {(u8cp)names[i], (u8cp)names[i] + strlen(names[i])};
        call(PATHu8bPush, fpath, fn);

        u8bp mapped = NULL;
        call(FILEMapRO, &mapped, PATHu8cgIn(fpath));

        size_t nbytes = u8bIdleHead(mapped) - u8bDataHead(mapped);
        size_t nentries = nbytes / sizeof(u64);
        u64cp base = (u64cp)u8bDataHead(mapped);

        stack[0][*nfiles][0] = base;
        stack[0][*nfiles][1] = base + nentries;
        maps[*nfiles] = mapped;
        (*nfiles)++;
    }
    done;
}

ok64 CAPOStackClose(u8bp *maps, u32 nfiles) {
    for (u32 i = 0; i < nfiles; i++) {
        if (maps[i] != NULL) FILEUnMap(maps[i]);
    }
    return OK;
}

// --- Compaction ---

ok64 CAPOCompact(u8csc dir) {
    sane($ok(dir));

    u64cs runs[CAPO_MAX_LEVELS] = {};
    u64css stack = {runs, runs};
    u8bp mmaps[CAPO_MAX_LEVELS] = {};
    u32 nfiles = 0;
    call(CAPOStackOpen, stack, mmaps, &nfiles, dir);
    stack[1] = stack[0] + nfiles;

    if (nfiles < 2 || MSETu64IsCompact(stack)) {
        CAPOStackClose(mmaps, nfiles);
        done;
    }

    size_t total = 0;
    for (u32 i = 0; i < nfiles; i++) total += $len(runs[i]);

    u64 *buf = (u64 *)malloc(total * sizeof(u64));
    test(buf != NULL, FAILSANITY);
    u64s into = {buf, buf + total};

    size_t n = $len(stack);
    size_t m = 1;
    size_t mtotal = $len(stack[0][n - 1]);
    while (m < n && mtotal * 8 > $len(stack[0][n - 1 - m])) {
        mtotal += $len(stack[0][n - 1 - m]);
        m++;
    }
    if (m < 2) {
        free(buf);
        CAPOStackClose(mmaps, nfiles);
        done;
    }

    call(MSETu64Compact, stack, into);

    u64 seqno = 0;
    call(CAPONextSeqno, &seqno, dir);
    u64cs merged = {(u64cp)buf, (u64cp)(into[0])};
    call(CAPOIndexWrite, dir, merged, seqno);

    a_pad(u8, dpat, FILE_PATH_MAX_LEN);
    call(PATHu8bFeed, dpat, dir);

    char fnames[CAPO_MAX_LEVELS][64];
    u32 fcount = 0;
    int dfd = -1;
    ok64 o = FILEOpenDir(&dfd, PATHu8cgIn(dpat));
    if (o == OK) {
        DIR *dd = fdopendir(dfd);
        if (dd) {
            struct dirent *e;
            while ((e = readdir(dd)) != NULL && fcount < CAPO_MAX_LEVELS) {
                size_t nlen = strlen(e->d_name);
                if (nlen < 5 || nlen > 63) continue;
                if (strcmp(e->d_name + nlen - 4, CAPO_IDX_EXT) != 0) continue;
                memcpy(fnames[fcount], e->d_name, nlen + 1);
                fcount++;
            }
            closedir(dd);
        }
    }

    for (u32 i = 0; i + 1 < fcount; i++)
        for (u32 j = i + 1; j < fcount; j++)
            if (strcmp(fnames[i], fnames[j]) > 0) {
                char tmp[64];
                memcpy(tmp, fnames[i], 64);
                memcpy(fnames[i], fnames[j], 64);
                memcpy(fnames[j], tmp, 64);
            }

    CAPOStackClose(mmaps, nfiles);

    u32 unlinked = 0;
    for (u32 i = fcount; i > 0 && unlinked < m; i--) {
        if (i == fcount) continue;
        a_pad(u8, ulpath, FILE_PATH_MAX_LEN);
        call(PATHu8bFeed, ulpath, dir);
        u8cs ulfn = {(u8cp)fnames[i - 1],
                     (u8cp)fnames[i - 1] + strlen(fnames[i - 1])};
        call(PATHu8bPush, ulpath, ulfn);
        unlink((char *)u8bDataHead(ulpath));
        unlinked++;
    }

    free(buf);
    done;
}

// --- Helper: find file extension ---

#define CAPOFindExt(ext, path, len)                      \
    do {                                                 \
        (ext)[0] = NULL; (ext)[1] = NULL;                \
        for (size_t _i = (len); _i > 0; _i--) {         \
            if ((path)[_i - 1] == '/') break;            \
            if ((path)[_i - 1] == '.') {                 \
                (ext)[0] = (u8cp)(path) + _i - 1;       \
                (ext)[1] = (u8cp)(path) + (len);         \
                break;                                   \
            }                                            \
        }                                                \
    } while (0)

// --- Reindex ---

static ok64 CAPOReindexWork(u8csc reporoot, u8csc dirslice, u64bp entries) {
    sane($ok(reporoot) && $ok(dirslice) && entries != NULL);

    char cmdbuf[FILE_PATH_MAX_LEN + 32];
    int n = snprintf(cmdbuf, sizeof(cmdbuf), "git -C %.*s ls-files",
                     (int)$len(reporoot), (char *)reporoot[0]);
    test(n > 0 && n < (int)sizeof(cmdbuf), FAILSANITY);

    FILE *fp = popen(cmdbuf, "r");
    test(fp != NULL, FAILSANITY);

    u32 indexed = 0, skipped = 0, failed = 0;
    u64 seqno = 1;
    size_t total_entries = 0;

    char line[FILE_PATH_MAX_LEN];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = 0;
        if (len == 0) continue;

        u8cs ext = {};
        CAPOFindExt(ext, line, len);
        if ($empty(ext)) { skipped++; continue; }
        if (!CAPOKnownExt(ext)) { skipped++; continue; }

        char fpath[FILE_PATH_MAX_LEN * 2];
        int pn = snprintf(fpath, sizeof(fpath), "%.*s/%s",
                          (int)$len(reporoot), (char *)reporoot[0], line);
        if (pn <= 0 || pn >= (int)sizeof(fpath)) { skipped++; continue; }

        a_pad(u8, fpbuf, FILE_PATH_MAX_LEN);
        u8cs fps = {(u8cp)fpath, (u8cp)fpath + pn};
        call(PATHu8bFeed, fpbuf, fps);

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, PATHu8cgIn(fpbuf));
        if (o != OK) {
            fprintf(stderr, "FAIL\t%s\t%s\t(open %s)\n",
                    ok64str(o), line, fpath);
            failed++;
            continue;
        }

        a_dup(u8c, source, u8bDataC(mapped));
        u8cs relpath = {(u8cp)line, (u8cp)line + len};
        o = CAPOIndexFile(entries, source, ext, relpath);
        FILEUnMap(mapped);
        if (o != OK) {
            fprintf(stderr, "FAIL\t%s\t%s\t(src=%zu)\n",
                    ok64str(o), line, $len(source));
            failed++;
            continue;
        }
        u8c *codec[2] = {};
        CAPOCodecName(codec, ext);
        if (CAPO_TERM)
            fprintf(stderr, "\033[%dmOK\t%.*s\t%s\033[0m\n",
                    GRAY, (int)$len(codec), (char *)codec[0], line);
        else
            fprintf(stderr, "OK\t%.*s\t%s\n",
                    (int)$len(codec), (char *)codec[0], line);
        indexed++;

        size_t pending = u64bDataLen(entries);
        if (pending >= CAPO_FLUSH_AT) {
            u64s data = {u64bDataHead(entries), u64bIdleHead(entries)};
            $sort(data, u64cmp);
            u64cs run = {(u64cp)data[0], (u64cp)data[1]};
            call(CAPOIndexWrite, dirslice, run, seqno++);
            total_entries += pending;
            fprintf(stderr, "spot: flushed %zu entries\n", pending);
            u64bReset(entries);
        }
    }
    pclose(fp);

    size_t pending = u64bDataLen(entries);
    if (pending > 0) {
        u64s data = {u64bDataHead(entries), u64bIdleHead(entries)};
        $sort(data, u64cmp);
        u64cs run = {(u64cp)data[0], (u64cp)data[1]};
        call(CAPOIndexWrite, dirslice, run, seqno++);
        total_entries += pending;
    }

    fprintf(stderr, "spot: indexed %u files, %zu entries, skipped %u, failed %u\n",
            indexed, total_entries, skipped, failed);

    if (seqno > 2) {
        fprintf(stderr, "spot: compacting %" PRIu64 " runs\n",
                (u64)(seqno - 1));
        call(CAPOCompact, dirslice);
    }

    done;
}

ok64 CAPOReindex(u8csc reporoot) {
    sane($ok(reporoot));

    fprintf(stderr, "spot: repo root %.*s\n",
            (int)$len(reporoot), (char *)reporoot[0]);

    a_pad(u8, capodir, FILE_PATH_MAX_LEN);
    call(CAPOResolveDir, capodir, reporoot);
    a_dup(u8c, dirslice, u8bDataC(capodir));
    vcall("mkdir", FILEMakeDirP, PATHu8cgIn(capodir));
    fprintf(stderr, "spot: index dir %s\n", (char *)u8bDataHead(capodir));

    Bu64 entries = {};
    vcall("mmap scratch", u64bMap, entries, CAPO_SCRATCH_LEN);

    ok64 o = CAPOReindexWork(reporoot, dirslice, entries);

    u64bUnMap(entries);
    if (o == OK) CAPOCommitWrite(reporoot, dirslice);
    return o;
}

// --- Parallel reindex: single proc ---

static ok64 CAPOReindexProcWork(u8csc reporoot, u8csc dirslice,
                                u64bp entries, u32 nprocs, u32 proc) {
    sane($ok(reporoot) && $ok(dirslice) && entries != NULL);

    char cmdbuf[FILE_PATH_MAX_LEN + 32];
    int n = snprintf(cmdbuf, sizeof(cmdbuf), "git -C %.*s ls-files",
                     (int)$len(reporoot), (char *)reporoot[0]);
    test(n > 0 && n < (int)sizeof(cmdbuf), FAILSANITY);

    FILE *fp = popen(cmdbuf, "r");
    test(fp != NULL, FAILSANITY);

    u32 indexed = 0, skipped = 0, failed = 0;
    u32 batch = 0;
    size_t total_entries = 0;
    u32 fileno = 0;

    char line[FILE_PATH_MAX_LEN];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = 0;
        if (len == 0) continue;

        if (fileno++ % nprocs != proc) continue;

        u8cs ext = {};
        CAPOFindExt(ext, line, len);
        if ($empty(ext)) { skipped++; continue; }
        if (!CAPOKnownExt(ext)) { skipped++; continue; }

        char fpath[FILE_PATH_MAX_LEN * 2];
        int pn = snprintf(fpath, sizeof(fpath), "%.*s/%s",
                          (int)$len(reporoot), (char *)reporoot[0], line);
        if (pn <= 0 || pn >= (int)sizeof(fpath)) { skipped++; continue; }

        a_pad(u8, fpbuf, FILE_PATH_MAX_LEN);
        u8cs fps = {(u8cp)fpath, (u8cp)fpath + pn};
        call(PATHu8bFeed, fpbuf, fps);

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, PATHu8cgIn(fpbuf));
        if (o != OK) {
            fprintf(stderr, "FAIL\t%s\t%s\t(open %s)\n",
                    ok64str(o), line, fpath);
            failed++;
            continue;
        }

        a_dup(u8c, source, u8bDataC(mapped));
        u8cs relpath = {(u8cp)line, (u8cp)line + len};
        o = CAPOIndexFile(entries, source, ext, relpath);
        FILEUnMap(mapped);
        if (o != OK) {
            fprintf(stderr, "FAIL\t%s\t%s\t(src=%zu)\n",
                    ok64str(o), line, $len(source));
            failed++;
            continue;
        }
        u8c *codec[2] = {};
        CAPOCodecName(codec, ext);
        if (CAPO_TERM)
            fprintf(stderr, "\033[%dmOK\t%.*s\t%s\t(%zu entries)\033[0m\n",
                    GRAY, (int)$len(codec), (char*)codec[0], line, u64bDataLen(entries));
        else
            fprintf(stderr, "OK\t%.*s\t%s\t(%zu entries)\n",
                    (int)$len(codec), (char*)codec[0], line, u64bDataLen(entries));
        indexed++;

        size_t pending = u64bDataLen(entries);
        if (pending >= CAPO_FLUSH_AT) {
            fprintf(stderr, "spot[%u/%u]: flushing %zu entries\n",
                    proc, nprocs, pending);
            u64s data = {u64bDataHead(entries), u64bIdleHead(entries)};
            $sort(data, u64cmp);
            u64 seqno = (u64)nprocs * batch + proc + 1;
            u64cs run = {(u64cp)data[0], (u64cp)data[1]};
            call(CAPOIndexWrite, dirslice, run, seqno);
            total_entries += pending;
            batch++;
            u64bReset(entries);
        }
    }
    pclose(fp);

    size_t pending = u64bDataLen(entries);
    if (pending > 0) {
        u64s data = {u64bDataHead(entries), u64bIdleHead(entries)};
        $sort(data, u64cmp);
        u64 seqno = (u64)nprocs * batch + proc + 1;
        u64cs run = {(u64cp)data[0], (u64cp)data[1]};
        call(CAPOIndexWrite, dirslice, run, seqno);
        total_entries += pending;
    }

    fprintf(stderr, "spot[%u/%u]: indexed %u files, %zu entries, skipped %u, failed %u\n",
            proc, nprocs, indexed, total_entries, skipped, failed);
    done;
}

ok64 CAPOReindexProc(u8csc reporoot, u32 nprocs, u32 proc) {
    sane($ok(reporoot) && proc < nprocs && nprocs > 0);

    fprintf(stderr, "spot[%u/%u]: starting\n", proc, nprocs);

    a_pad(u8, capodir, FILE_PATH_MAX_LEN);
    call(CAPOResolveDir, capodir, reporoot);
    a_dup(u8c, dirslice, u8bDataC(capodir));
    vcall("mkdir", FILEMakeDirP, PATHu8cgIn(capodir));

    Bu64 entries = {};
    vcall("mmap scratch", u64bMap, entries, CAPO_SCRATCH_LEN);

    ok64 o = CAPOReindexProcWork(reporoot, dirslice, entries, nprocs, proc);

    u64bUnMap(entries);
    return o;
}

// --- Compact all into one ---

ok64 CAPOCompactAll(u8csc dir) {
    sane($ok(dir));

    for (;;) {
        u64cs runs[CAPO_MAX_LEVELS] = {};
        u64css stack = {runs, runs};
        u8bp mmaps[CAPO_MAX_LEVELS] = {};
        u32 nfiles = 0;
        call(CAPOStackOpen, stack, mmaps, &nfiles, dir);
        stack[1] = stack[0] + nfiles;

        if (nfiles <= 1) {
            CAPOStackClose(mmaps, nfiles);
            done;
        }

        size_t total = 0;
        for (u32 i = 0; i < nfiles; i++) total += $len(runs[i]);
        fprintf(stderr, "spot: merging %u runs, %zu total entries\n",
                nfiles, total);

        Bu64 mbuf = {};
        call(u64bMap, mbuf, total);
        u64s into = {u64bIdleHead(mbuf), mbuf[3]};

        MSETu64Start(stack);
        size_t merged_count = 0;
        while (!$empty(stack)) {
            if (into[0] >= into[1]) {
                fprintf(stderr, "spot: merge buffer full at %zu\n", merged_count);
                break;
            }
            *into[0] = ****stack;
            into[0]++;
            merged_count++;
            MSETu64Next(stack);
        }

        u64 seqno = 0;
        call(CAPONextSeqno, &seqno, dir);
        fprintf(stderr, "spot: next seqno = %" PRIu64 " (dir = '%.*s')\n",
                seqno,
                (int)$len(dir), (char *)dir[0]);
        u64cs merged = {(u64cp)mbuf[0], (u64cp)into[0]};
        fprintf(stderr, "spot: writing %zu deduplicated entries (seqno %" PRIu64 ")\n",
                $len(merged), seqno);
        call(CAPOIndexWrite, dir, merged, seqno);

        CAPOStackClose(mmaps, nfiles);
        u64bUnMap(mbuf);

        a_pad(u8, dpat, FILE_PATH_MAX_LEN);
        call(PATHu8bFeed, dpat, dir);
        int dfd = -1;
        ok64 o = FILEOpenDir(&dfd, PATHu8cgIn(dpat));
        if (o == OK) {
            char fnames[CAPO_MAX_LEVELS][64];
            u32 fcount = 0;
            DIR *dd = fdopendir(dfd);
            if (dd) {
                struct dirent *e;
                while ((e = readdir(dd)) != NULL && fcount < CAPO_MAX_LEVELS) {
                    size_t nlen = strlen(e->d_name);
                    if (nlen < 5 || nlen > 63) continue;
                    if (strcmp(e->d_name + nlen - 4, CAPO_IDX_EXT) != 0) continue;
                    memcpy(fnames[fcount], e->d_name, nlen + 1);
                    fcount++;
                }
                closedir(dd);

                for (u32 i = 0; i + 1 < fcount; i++)
                    for (u32 j = i + 1; j < fcount; j++)
                        if (strcmp(fnames[i], fnames[j]) > 0) {
                            char tmp[64];
                            memcpy(tmp, fnames[i], 64);
                            memcpy(fnames[i], fnames[j], 64);
                            memcpy(fnames[j], tmp, 64);
                        }

                for (u32 i = 0; i + 1 < fcount; i++) {
                    a_pad(u8, ulpath, FILE_PATH_MAX_LEN);
                    call(PATHu8bFeed, ulpath, dir);
                    u8cs ulfn = {(u8cp)fnames[i],
                                 (u8cp)fnames[i] + strlen(fnames[i])};
                    call(PATHu8bPush, ulpath, ulfn);
                    unlink((char *)u8bDataHead(ulpath));
                }
            }
        }

        break;
    }
    done;
}

// --- Commit tracking ---

ok64 CAPOCommitWrite(u8csc reporoot, u8csc capodir) {
    sane($ok(reporoot) && $ok(capodir));

    // get current HEAD
    char cmdbuf[FILE_PATH_MAX_LEN + 64];
    int n = snprintf(cmdbuf, sizeof(cmdbuf),
                     "git -C %.*s rev-parse HEAD",
                     (int)$len(reporoot), (char *)reporoot[0]);
    test(n > 0 && n < (int)sizeof(cmdbuf), FAILSANITY);

    FILE *fp = popen(cmdbuf, "r");
    test(fp != NULL, FAILSANITY);

    char newsha[64];
    char *got = fgets(newsha, sizeof(newsha), fp);
    pclose(fp);
    test(got != NULL, FAILSANITY);

    size_t slen = strlen(newsha);
    if (slen > 0 && newsha[slen - 1] == '\n') newsha[--slen] = 0;
    test(slen >= 40, FAILSANITY);
    newsha[40] = 0;

    // read existing SHAs
    char shas[CAPO_MAX_SHAS][44];
    u32 sha_count = 0;
    CAPOCommitRead(&sha_count, capodir, shas, CAPO_MAX_SHAS);

    // skip if newest already matches
    if (sha_count > 0 && memcmp(shas[sha_count - 1], newsha, 40) == 0) done;

    // keep at most CAPO_MAX_SHAS - 1 most recent old entries
    u32 keep_start = 0;
    if (sha_count >= CAPO_MAX_SHAS) keep_start = sha_count - CAPO_MAX_SHAS + 1;

    a_pad(u8, path, FILE_PATH_MAX_LEN);
    call(u8bFeed, path, capodir);
    a_cstr(commit_name, "/COMMIT");
    call(u8bFeed, path, commit_name);
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

static b8 CAPOIsHexSha(const char *s, size_t len) {
    if (len < 40) return NO;
    for (int i = 0; i < 40; i++) {
        u8 c = (u8)s[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return NO;
    }
    return YES;
}

ok64 CAPOCommitRead(u32p count, u8csc capodir,
                    char shas[][44], u32 maxcount) {
    sane(count != NULL && $ok(capodir) && shas != NULL && maxcount > 0);
    *count = 0;

    a_pad(u8, path, FILE_PATH_MAX_LEN);
    call(u8bFeed, path, capodir);
    a_cstr(commit_name, "/COMMIT");
    call(u8bFeed, path, commit_name);
    call(PATHu8gTerm, PATHu8gIn(path));

    u8bp mapped = NULL;
    ok64 o = FILEMapRO(&mapped, PATHu8cgIn(path));
    if (o != OK) done;

    a_dup(u8c, content, u8bDataC(mapped));
    u8cp p = content[0];
    u8cp end = content[1];

    while (p < end && *count < maxcount) {
        // skip leading whitespace/newlines
        while (p < end && (*p == '\n' || *p == '\r' || *p == ' ')) p++;
        if (p >= end) break;
        // find end of line
        u8cp lend = p;
        while (lend < end && *lend != '\n') lend++;
        size_t llen = (size_t)(lend - p);
        if (CAPOIsHexSha((const char *)p, llen)) {
            memcpy(shas[*count], p, 40);
            shas[*count][40] = 0;
            (*count)++;
        }
        p = lend;
    }

    FILEUnMap(mapped);
    done;
}

// --- Hook (incremental) ---

static b8 CAPOHookDiffCmd(char *cmdbuf, size_t cmdsz,
                           u8csc reporoot, u8csc dirslice) {
    char shas[CAPO_MAX_SHAS][44];
    u32 sha_count = 0;
    CAPOCommitRead(&sha_count, dirslice, shas, CAPO_MAX_SHAS);

    if (sha_count == 0) {
        fprintf(stderr, "spot: no saved commit, full reindex\n");
        return NO;
    }

    // try newest first (last in file)
    char chkbuf[FILE_PATH_MAX_LEN + 128];
    for (u32 i = sha_count; i > 0; i--) {
        int n = snprintf(chkbuf, sizeof(chkbuf),
                         "git -C %.*s merge-base --is-ancestor %.40s HEAD",
                         (int)$len(reporoot), (char *)reporoot[0],
                         shas[i - 1]);
        if (n <= 0 || n >= (int)sizeof(chkbuf)) continue;
        int rc = system(chkbuf);
        if (WIFEXITED(rc) && WEXITSTATUS(rc) == 0) {
            if (CAPO_COLOR)
                fprintf(stderr, "\033[%dmChanges since %.40s\033[0m\n",
                        GRAY, shas[i - 1]);
            else
                fprintf(stderr, "Changes since %.40s\n", shas[i - 1]);
            n = snprintf(cmdbuf, cmdsz,
                         "git -C %.*s diff --name-only %.40s",
                         (int)$len(reporoot), (char *)reporoot[0],
                         shas[i - 1]);
            if (n <= 0 || n >= (int)cmdsz) return NO;
            return YES;
        }
    }

    fprintf(stderr, "spot: saved commits unreachable, full reindex\n");
    return NO;
}

static ok64 CAPOHookDiff(u8csc reporoot, u8csc dirslice,
                          u64bp entries, const char *cmdbuf) {
    sane($ok(reporoot) && $ok(dirslice) && entries != NULL);

    FILE *fp = popen(cmdbuf, "r");
    test(fp != NULL, FAILSANITY);

    int indexed = 0;
    char line[FILE_PATH_MAX_LEN];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = 0;
        if (len == 0) continue;

        u8cs ext = {};
        CAPOFindExt(ext, line, len);
        if ($empty(ext)) continue;
        if (!CAPOKnownExt(ext)) continue;

        char fpath[FILE_PATH_MAX_LEN * 2];
        int pn = snprintf(fpath, sizeof(fpath), "%.*s/%s",
                          (int)$len(reporoot), (char *)reporoot[0], line);
        if (pn <= 0 || pn >= (int)sizeof(fpath)) continue;

        a_pad(u8, fpbuf, FILE_PATH_MAX_LEN);
        u8cs fps = {(u8cp)fpath, (u8cp)fpath + pn};
        call(PATHu8bFeed, fpbuf, fps);

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, PATHu8cgIn(fpbuf));
        if (o != OK) continue;

        a_dup(u8c, source, u8bDataC(mapped));
        u8cs relpath = {(u8cp)line, (u8cp)line + len};
        o = CAPOIndexFile(entries, source, ext, relpath);
        FILEUnMap(mapped);
        if (o != OK) continue;

        u8c *codec[2] = {};
        CAPOCodecName(codec, ext);
        fprintf(stderr, "OK\t%.*s\t%s\n",
                (int)$len(codec), (char *)codec[0], line);
        indexed++;
    }
    pclose(fp);
    fprintf(stderr, "%d file(s) re-indexed\n", indexed);

    size_t pending = u64bDataLen(entries);
    if (pending > 0) {
        u64s data = {u64bDataHead(entries), u64bIdleHead(entries)};
        $sort(data, u64cmp);
        u64 seqno = 0;
        call(CAPONextSeqno, &seqno, dirslice);
        u64cs run = {(u64cp)data[0], (u64cp)data[1]};
        call(CAPOIndexWrite, dirslice, run, seqno);
    }

    call(CAPOCompact, dirslice);
    done;
}

ok64 CAPOHook(u8csc reporoot) {
    sane($ok(reporoot));

    a_pad(u8, capodir, FILE_PATH_MAX_LEN);
    call(CAPOResolveDir, capodir, reporoot);
    a_dup(u8c, dirslice, u8bDataC(capodir));
    call(FILEMakeDirP, PATHu8cgIn(capodir));

    char cmdbuf[FILE_PATH_MAX_LEN + 128];
    ok64 o = OK;

    Bu64 entries = {};
    call(u64bMap, entries, CAPO_SCRATCH_LEN);

    if (CAPOHookDiffCmd(cmdbuf, sizeof(cmdbuf), reporoot, dirslice)) {
        o = CAPOHookDiff(reporoot, dirslice, entries, cmdbuf);
    } else {
        o = CAPOReindexWork(reporoot, dirslice, entries);
    }

    u64bUnMap(entries);

    if (o == OK) CAPOCommitWrite(reporoot, dirslice);
    return o;
}

// --- Trigram query helpers ---

static ok64 CAPOCollectPaths(u64css iter, u64 tri_prefix, u32 *hashes,
                             u32p nhashes, u32 maxhashes) {
    ok64 o = MSETu64Seek(iter, tri_prefix);
    if (o != OK) return OK;

    while (!$empty(iter)) {
        u64 entry = ****iter;
        if (CAPOTriOf(entry) != tri_prefix) break;
        if (*nhashes < maxhashes) {
            hashes[*nhashes] = (u32)entry;
            (*nhashes)++;
        }
        MSETu64Next(iter);
    }
    return OK;
}

static u32 CAPOIntersect(u32 *a, u32 na, u32 *b, u32 nb, u32 *out) {
    u32 i = 0, j = 0, k = 0;
    while (i < na && j < nb) {
        if (a[i] < b[j]) i++;
        else if (a[i] > b[j]) j++;
        else { out[k++] = a[i]; i++; j++; }
    }
    return k;
}

static int CAPOu32cmp(const void *a, const void *b) {
    u32 x = *(const u32 *)a, y = *(const u32 *)b;
    return (x > y) - (x < y);
}

static void CAPOProgress(const char *line) {
    if (!CAPO_TERM) return;
    fprintf(stderr, "\r\033[K");
    if (line != NULL)
        fprintf(stderr, "\033[%dm%s\033[0m", GRAY, line);
}

// --- Function name finder (heuristic) ---

// Check if ext (with dot) matches one of the given suffixes.
static b8 CAPOExtIs(u8csc ext, const char *a, const char *b) {
    u8cs nodot = {};
    if ($empty(ext)) return NO;
    if (ext[0][0] == '.') { nodot[0] = ext[0] + 1; nodot[1] = ext[1]; }
    else { nodot[0] = ext[0]; nodot[1] = ext[1]; }
    size_t n = (size_t)$len(nodot);
    size_t al = strlen(a);
    if (n == al && memcmp(nodot[0], a, al) == 0) return YES;
    if (b != NULL) {
        size_t bl = strlen(b);
        if (n == bl && memcmp(nodot[0], b, bl) == 0) return YES;
    }
    return NO;
}

// Walk backward from `pos` looking for a section header line.
// Heuristic depends on file extension:
//   md/markdown/rst/txt: line starting with '#'
//   py: col-0 'def ' or 'class '
//   default (C-like): col-0 identifier + '('
static void CAPOFindFunc(u8csc source, u32 pos, u8csc ext,
                          char *out, size_t outsz) {
    out[0] = 0;
    if ($empty(source) || pos == 0 || outsz < 2) return;
    u8cp base = source[0];
    u32 slen = (u32)$len(source);
    if (pos > slen) pos = slen;

    b8 is_md = CAPOExtIs(ext, "md", "markdown") ||
               CAPOExtIs(ext, "rst", "txt");
    b8 is_py = CAPOExtIs(ext, "py", NULL);

    // Find start of current line
    u32 ls = pos;
    while (ls > 0 && base[ls - 1] != '\n') ls--;

    // Walk backward up to 200 lines looking for a header line
    for (int tries = 0; tries < 200 && ls > 0; tries++) {
        // Move to previous line
        ls--;
        while (ls > 0 && base[ls - 1] != '\n') ls--;

        // Skip empty lines
        u32 le = ls;
        while (le < slen && base[le] != '\n') le++;
        if (le == ls) continue;

        u8 ch = base[ls];
        u32 linelen = le - ls;

        if (is_md) {
            // Markdown: line starting with '#'
            if (ch != '#') continue;
        } else if (is_py) {
            // Python: col-0 'def ' or 'class '
            if (linelen >= 4 && memcmp(base + ls, "def ", 4) == 0) {}
            else if (linelen >= 6 && memcmp(base + ls, "class ", 6) == 0) {}
            else continue;
        } else {
            // C-like: col-0 identifier + '('
            // Skip comment lines
            if (ch == '/' || ch == '*' || ch == '#') continue;
            // Must start with identifier char at column 0
            if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                  ch == '_'))
                continue;
            // Must contain '('
            b8 has_paren = NO;
            for (u32 j = ls; j < le; j++)
                if (base[j] == '(') { has_paren = YES; break; }
            if (!has_paren) continue;
        }

        // Copy line, trim trailing { / : and whitespace
        u32 copylen = linelen;
        if (copylen >= outsz) copylen = (u32)(outsz - 1);
        memcpy(out, base + ls, copylen);
        out[copylen] = 0;
        while (copylen > 0 &&
               (out[copylen - 1] == '{' || out[copylen - 1] == ':' ||
                out[copylen - 1] == ' ' || out[copylen - 1] == '\t' ||
                out[copylen - 1] == '\r'))
            out[--copylen] = 0;
        return;
    }
}

// --- Hunk title formatter ---

// Format "--- path :: func ---" with smart truncation to 64 visible chars.
// filepath may be NULL, funcname may be empty ("").
// Returns formatted length (0 if nothing to format).
#define CAPO_MAX_HLS 64
#define HUNK_MAX 64
static int CAPOFormatTitle(char *out, size_t outsz,
                            const char *filepath, const char *funcname) {
    int hlen = 0;
    if (filepath && funcname && funcname[0])
        hlen = snprintf(out, outsz, "--- %s :: %s ---", filepath, funcname);
    else if (filepath)
        hlen = snprintf(out, outsz, "--- %s ---", filepath);
    else if (funcname && funcname[0])
        hlen = snprintf(out, outsz, "--- %s ---", funcname);
    else
        return 0;

    if (hlen > HUNK_MAX && filepath && funcname && funcname[0]) {
        size_t plen = strlen(filepath);
        int budget = HUNK_MAX - 12 - (int)plen;
        if (budget < 1) budget = 1;
        hlen = snprintf(out, outsz, "--- %s :: %.*s ---",
                         filepath, budget, funcname);
    }
    if (hlen > HUNK_MAX && filepath) {
        const char *p = filepath + strlen(filepath);
        int budget = HUNK_MAX - 12 - 3;
        if (funcname && funcname[0]) {
            size_t flen = strlen(funcname);
            if (flen > 20) flen = 20;
            budget -= (int)flen + 4;
        }
        if (budget < 8) budget = 8;
        while (p > filepath && (int)(filepath + strlen(filepath) - p) < budget)
            p--;
        if (funcname && funcname[0]) {
            int favail = HUNK_MAX - 12 - 3 -
                         (int)(filepath + strlen(filepath) - p);
            if (favail < 1) favail = 1;
            hlen = snprintf(out, outsz, "--- ...%s :: %.*s ---",
                             p, favail, funcname);
        } else {
            hlen = snprintf(out, outsz, "--- ...%s ---", p);
        }
    }
    return hlen;
}

// --- Grep: substring search in source text (no tree) ---

static void CAPOGrepCtx(u8csc source, u32 match_pos, u32 nctx,
                         u32 *lo, u32 *hi) {
    u32 slen = (u32)$len(source);
    if (match_pos > slen) match_pos = slen;
    u32 ls = match_pos;
    while (ls > 0 && source[0][ls - 1] != '\n') ls--;
    *lo = ls;
    for (u32 i = 0; i < nctx && *lo > 0; i++) {
        (*lo)--;
        while (*lo > 0 && source[0][*lo - 1] != '\n') (*lo)--;
    }
    u32 le = match_pos;
    while (le < slen && source[0][le] != '\n') le++;
    if (le < slen) le++;
    *hi = le;
    for (u32 i = 0; i < nctx && *hi < slen; i++) {
        while (*hi < slen && source[0][*hi] != '\n') (*hi)++;
        if (*hi < slen) (*hi)++;
    }
}

ok64 CAPOGrep(u8csc substring, u8csc ext, u8csc reporoot, u32 ctx_lines,
              u8css files) {
    sane($ok(substring) && !$empty(substring) && $ok(reporoot));
    int nfiles = (int)$len(files);

    // Language filter: match file extension literally
    u8cs target_ext = {};
    if (!$empty(ext)) { $mv(target_ext, ext); }

    // --- Trigram filtering (skip when explicit files given) ---
    u32 maxhashes = 64 * 1024;
    u32 *hashbuf1 = NULL;
    u32 *hashbuf2 = NULL;
    u32 nhashes = 0;
    b8 has_trigrams = NO;

    if (nfiles == 0) {
        a_pad(u8, capodir, FILE_PATH_MAX_LEN);
        call(CAPOResolveDir, capodir, reporoot);
        a_dup(u8c, dirslice, u8bDataC(capodir));

        hashbuf1 = (u32 *)malloc(maxhashes * sizeof(u32));
        hashbuf2 = (u32 *)malloc(maxhashes * sizeof(u32));
        test(hashbuf1 != NULL && hashbuf2 != NULL, FAILSANITY);

        u64cs runs[CAPO_MAX_LEVELS] = {};
        u64css stack = {runs, runs};
        u8bp mmaps[CAPO_MAX_LEVELS] = {};
        u32 nidxfiles = 0;
        call(CAPOStackOpen, stack, mmaps, &nidxfiles, dirslice);
        stack[1] = stack[0] + nidxfiles;

        if (nidxfiles == 0)
            fprintf(stderr,
                    "spot: warning: no index, run `spot` or `spot --fork N` first\n");

        if (nidxfiles > 0) {
            u8cp p = substring[0];
            u8cp end = substring[1] - 2;
            while (p <= end) {
                if (CAPOTriChar(p[0]) && CAPOTriChar(p[1]) &&
                    CAPOTriChar(p[2])) {
                    u8cs tri = {p, p + 3};
                    u64 tri_prefix = CAPOTriPack(tri);

                    u64cs seek_runs[CAPO_MAX_LEVELS];
                    for (u32 i = 0; i < nidxfiles; i++) {
                        seek_runs[i][0] = runs[i][0];
                        seek_runs[i][1] = runs[i][1];
                    }
                    u64css seek_iter = {seek_runs, seek_runs + nidxfiles};
                    MSETu64Start(seek_iter);

                    u32 tri_nhashes = 0;
                    CAPOCollectPaths(seek_iter, tri_prefix, hashbuf2,
                                     &tri_nhashes, maxhashes);

                    if (!has_trigrams) {
                        memcpy(hashbuf1, hashbuf2, tri_nhashes * sizeof(u32));
                        nhashes = tri_nhashes;
                        has_trigrams = YES;
                    } else {
                        qsort(hashbuf2, tri_nhashes, sizeof(u32), CAPOu32cmp);
                        qsort(hashbuf1, nhashes, sizeof(u32), CAPOu32cmp);
                        nhashes = CAPOIntersect(hashbuf1, nhashes, hashbuf2,
                                                tri_nhashes, hashbuf1);
                    }
                }
                p++;
            }
        }

        CAPOStackClose(mmaps, nidxfiles);

        if (has_trigrams && nhashes > 0)
            qsort(hashbuf1, nhashes, sizeof(u32), CAPOu32cmp);
    }

    call(LESSArenaInit);

    FILE *fp = NULL;
    if (nfiles == 0) {
        char cmdbuf[FILE_PATH_MAX_LEN + 32];
        int cn = snprintf(cmdbuf, sizeof(cmdbuf), "git -C %.*s ls-files",
                          (int)$len(reporoot), (char *)reporoot[0]);
        test(cn > 0 && cn < (int)sizeof(cmdbuf), FAILSANITY);
        fp = popen(cmdbuf, "r");
        test(fp != NULL, FAILSANITY);
    }

    size_t ndl_len = (size_t)$len(substring);

    char line[FILE_PATH_MAX_LEN];
    int fi = 0;
    while (nfiles > 0
           ? fi < nfiles
           : fgets(line, sizeof(line), fp) != NULL) {
        size_t len;
        if (nfiles > 0) {
            u8cs *fp = u8cssAtP(files, fi);
            len = (size_t)$len(*fp);
            if (len >= sizeof(line)) { fi++; continue; }
            u8s lns = {(u8p)line, (u8p)line + len};
            u8sCopy(lns, *fp);
            line[len] = 0;
            fi++;
        } else {
            len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') line[--len] = 0;
            if (len == 0) continue;
        }

        u8cs relpath = {(u8cp)line, (u8cp)line + len};

        if (nfiles == 0) {
            if (has_trigrams && nhashes > 0) {
                u32 phash = CAPOPathHash(relpath);
                if (!bsearch(&phash, hashbuf1, nhashes, sizeof(u32), CAPOu32cmp))
                    continue;
            } else if (has_trigrams) {
                continue;
            }
        }

        u8cs file_ext = {};
        CAPOFindExt(file_ext, line, len);
        if ($empty(file_ext)) continue;
        if (!CAPOKnownExt(file_ext)) continue;
        if (nfiles == 0 && !$empty(target_ext)) {
            if ($len(file_ext) != $len(target_ext) ||
                memcmp(file_ext[0], target_ext[0], $len(target_ext)) != 0)
                continue;
        }

        CAPOProgress(line);

        char fpath[FILE_PATH_MAX_LEN * 2];
        int pn = snprintf(fpath, sizeof(fpath), "%.*s/%s",
                          (int)$len(reporoot), (char *)reporoot[0], line);
        if (pn <= 0 || pn >= (int)sizeof(fpath)) continue;

        a_pad(u8, fpbuf, FILE_PATH_MAX_LEN);
        u8cs fps = {(u8cp)fpath, (u8cp)fpath + pn};
        __ = PATHu8bFeed(fpbuf, fps);
        if (__ != OK) continue;

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, PATHu8cgIn(fpbuf));
        if (o != OK) continue;

        a_dup(u8c, source, u8bDataC(mapped));

        // Try to tokenize for syntax highlighting
        b8 tokenized = NO;
        Bu32 gtoks = {};
        if (!$empty(file_ext) && CAPOKnownExt(file_ext)) {
            size_t maxlen = $len(source) + 1;
            ok64 to = u32bMap(gtoks, maxlen);
            if (to == OK) {
                to = SPOTTokenize(gtoks, source, file_ext);
                if (to == OK) {
                    u32 *dts[2] = {u32bDataHead(gtoks), u32bIdleHead(gtoks)};
                    u8cs dext = {file_ext[0], file_ext[1]};
                    if (!$empty(dext) && dext[0][0] == '.') dext[0]++;
                    DEFMark(dts, source, dext);
                    tokenized = YES;
                } else
                    u32bUnMap(gtoks);
            }
        }
        u32cs gts = {};
        if (tokenized) {
            gts[0] = (u32cp)u32bDataHead(gtoks);
            gts[1] = (u32cp)u32bIdleHead(gtoks);
        }

        // Search source text directly for substring matches
        u32 prev_hi = 0;
        b8 found_any = NO;
        b8 first_hunk = YES;

        u8cp sp = source[0];
        u8cp se = source[1];
        if ((size_t)$len(source) >= ndl_len) {
            u8cp send = se - ndl_len;
            while (sp <= send) {
                if (memcmp(sp, substring[0], ndl_len) == 0) {
                    u32 match_pos = (u32)(sp - source[0]);
                    u32 ctx_lo = 0, ctx_hi = 0;
                    CAPOGrepCtx(source, match_pos, ctx_lines, &ctx_lo, &ctx_hi);

                    if (!found_any) {
                        CAPOProgress(NULL);
                        found_any = YES;
                    }

                    // Collect all matches within this context block
                    range32 hls[CAPO_MAX_HLS];
                    int nhl = 0;
                    hls[nhl++] = (range32){match_pos, match_pos + (u32)ndl_len};
                    u8cp sp2 = sp + 1;
                    while (sp2 <= send && nhl < CAPO_MAX_HLS) {
                        if (memcmp(sp2, substring[0], ndl_len) == 0) {
                            u32 mp2 = (u32)(sp2 - source[0]);
                            if (mp2 >= ctx_hi) break;
                            hls[nhl++] = (range32){mp2, mp2 + (u32)ndl_len};
                            u32 lo2 = 0, hi2 = 0;
                            CAPOGrepCtx(source, mp2, ctx_lines, &lo2, &hi2);
                            if (hi2 > ctx_hi) ctx_hi = hi2;
                        }
                        sp2++;
                    }

                    b8 contiguous = (ctx_lo <= prev_hi);
                    if (ctx_lo < prev_hi) ctx_lo = prev_hi;
                    if (ctx_lo < ctx_hi &&
                        less_nhunks < LESS_MAX_HUNKS &&
                        u8bIdleLen(less_arena) > (ctx_hi - ctx_lo + 512)) {
                        LESShunk *hk = &less_hunks[less_nhunks];
                        memset(hk, 0, sizeof(*hk));

                        // Title
                        if (!contiguous || first_hunk) {
                            char funcname[256];
                            CAPOFindFunc(source, ctx_lo, file_ext,
                                         funcname, sizeof(funcname));
                            char hdr[512];
                            int tlen = CAPOFormatTitle(hdr, sizeof(hdr),
                                                       line, funcname);
                            if (tlen > 0) {
                                u8p tp = LESSArenaWrite(hdr, (size_t)tlen);
                                if (tp != NULL) {
                                    hk->title[0] = tp;
                                    hk->title[1] = tp + tlen;
                                }
                            }
                        }

                        hk->text[0] = source[0] + ctx_lo;
                        hk->text[1] = source[0] + ctx_hi;
                        hk->toks[0] = gts[0];
                        hk->toks[1] = gts[1];

                        // Lits
                        if (CAPO_COLOR) {
                            u32 region_len = ctx_hi - ctx_lo;
                            u8p lp = LESSArenaAlloc(region_len);
                            if (lp != NULL) {
                                int ntoks = (int)$len(gts);
                                for (int ti = 0; ti < ntoks; ti++) {
                                    u32 tlo = (ti > 0) ? tok32Offset(gts[0][ti-1]) : 0;
                                    u32 thi = tok32Offset(gts[0][ti]);
                                    if (thi <= ctx_lo || tlo >= ctx_hi) continue;
                                    u32 clo = tlo < ctx_lo ? ctx_lo : tlo;
                                    u32 chi = thi > ctx_hi ? ctx_hi : thi;
                                    u8 tag = tok32Tag(gts[0][ti]) - 'A';
                                    memset(lp + (clo - ctx_lo), tag, chi - clo);
                                }
                                for (int h = 0; h < nhl; h++) {
                                    u32 hlo = hls[h].lo < ctx_lo ? ctx_lo : hls[h].lo;
                                    u32 hhi = hls[h].hi > ctx_hi ? ctx_hi : hls[h].hi;
                                    for (u32 b = hlo; b < hhi; b++)
                                        lp[b - ctx_lo] |= LESS_INS;
                                }
                                hk->lits[0] = lp;
                                hk->lits[1] = lp + region_len;
                            }
                        }

                        less_nhunks++;
                        first_hunk = NO;
                    }
                    prev_hi = ctx_hi;
                    sp = sp2 - 1;
                }
                sp++;
            }
        }

        if (found_any)
            LESSDefer(mapped, tokenized ? gtoks : (Bu32){});
        else {
            if (tokenized) u32bUnMap(gtoks);
            FILEUnMap(mapped);
        }
    }
    if (fp != NULL) pclose(fp);
    CAPOProgress(NULL);

    if (less_nhunks > 0)
        LESSRun(less_hunks, less_nhunks);
    LESSArenaCleanup();

    if (hashbuf1 != NULL) free(hashbuf1);
    if (hashbuf2 != NULL) free(hashbuf2);
    done;
}

// --- Lits builder (for LESS pager) ---

// Fill lits[0..textlen) with tag indices from tokens.
// Walks the token array, sets lits[byte] = tok32Tag(tok) for each byte.
static void CAPOBuildLits(u8p lits, u8cp base, u32 textlen, u32cs toks) {
    memset(lits, 0, textlen);
    int ntoks = (int)$len(toks);
    for (int i = 0; i < ntoks; i++) {
        u32 lo = (i > 0) ? tok32Offset(toks[0][i - 1]) : 0;
        u32 hi = tok32Offset(toks[0][i]);
        if (hi > textlen) hi = textlen;
        if (lo >= hi) continue;
        u8 tag = tok32Tag(toks[0][i]) - 'A';
        memset(lits + lo, tag, hi - lo);
    }
}

// Mark a byte range in lits with INS or DEL flag
static void CAPOMarkLits(u8p lits, u32 lo, u32 hi, u8 flag) {
    for (u32 i = lo; i < hi; i++)
        lits[i] |= flag;
}

// --- Cat ---

ok64 CAPOCat(u8css files, u8csc reporoot) {
    sane(!$empty(files) && $ok(reporoot));
    int nfiles = (int)$len(files);

    call(LESSArenaInit);

    for (int fi = 0; fi < nfiles; fi++) {
        if (less_nhunks >= LESS_MAX_HUNKS) break;

        u8cs *fp = u8cssAtP(files, fi);
        u8cs fpath_s = {(*fp)[0], (*fp)[1]};
        if ($empty(fpath_s)) continue;

        // Resolve path against CWD (like cat)
        a_pad(u8, fpbuf, FILE_PATH_MAX_LEN);
        __ = u8bFeed(fpbuf, fpath_s);
        if (__ != OK) continue;
        __ = PATHu8gTerm(PATHu8gIn(fpbuf));
        if (__ != OK) continue;

        // Extract extension
        u8cs ext = {};
        size_t plen = (size_t)(u8bIdleHead(fpbuf) - u8bDataHead(fpbuf));
        CAPOFindExt(ext, u8bDataHead(fpbuf), plen);

        // Map file
        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, PATHu8cgIn(fpbuf));
        if (o != OK) {
            fprintf(stderr, "spot: cannot open %.*s: %s\n",
                    (int)$len(fpath_s), (char *)fpath_s[0], ok64str(o));
            continue;
        }

        u8cp src_head = u8bDataHead(mapped);
        u8cp src_idle = u8bIdleHead(mapped);
        u32 srclen = (u32)(src_idle - src_head);

        LESShunk *hk = &less_hunks[less_nhunks];
        memset(hk, 0, sizeof(*hk));

        // Title
        char fpz[FILE_PATH_MAX_LEN];
        size_t fzl = (size_t)$len(fpath_s);
        if (fzl >= sizeof(fpz)) fzl = sizeof(fpz) - 1;
        memcpy(fpz, fpath_s[0], fzl);
        fpz[fzl] = 0;
        char hdr[512];
        int tlen = CAPOFormatTitle(hdr, sizeof(hdr), fpz, "");
        if (tlen > 0) {
            u8p tp = LESSArenaWrite(hdr, (size_t)tlen);
            if (tp != NULL) {
                hk->title[0] = tp;
                hk->title[1] = tp + tlen;
            }
        }

        hk->text[0] = src_head;
        hk->text[1] = src_idle;

        // Tokenize
        Bu32 toks = {};
        b8 tokenized = NO;
        if (!$empty(ext) && CAPOKnownExt(ext)) {
            size_t maxlen = srclen + 1;
            o = u32bMap(toks, maxlen);
            if (o == OK) {
                u8cs source = {src_head, src_idle};
                o = SPOTTokenize(toks, source, ext);
                if (o == OK) {
                    u32 *dts[2] = {u32bDataHead(toks), u32bIdleHead(toks)};
                    u8cs dext = {ext[0], ext[1]};
                    if (!$empty(dext) && dext[0][0] == '.') dext[0]++;
                    DEFMark(dts, source, dext);
                    tokenized = YES;
                    hk->toks[0] = (u32cp)u32bDataHead(toks);
                    hk->toks[1] = (u32cp)u32bIdleHead(toks);
                } else {
                    u32bUnMap(toks);
                    memset(toks, 0, sizeof(toks));
                }
            }
        }

        // Lits
        if (CAPO_COLOR && tokenized && srclen > 0) {
            u8p lp = LESSArenaAlloc(srclen);
            if (lp != NULL) {
                CAPOBuildLits(lp, src_head, srclen, hk->toks);
                hk->lits[0] = lp;
                hk->lits[1] = lp + srclen;
            }
        }

        less_nhunks++;
        LESSDefer(mapped, tokenized ? toks : (Bu32){});
    }

    if (less_nhunks > 0)
        LESSRun(less_hunks, less_nhunks);
    LESSArenaCleanup();

    done;
}

// --- Merge: token-level 3-way merge ---

static ok64 CAPOMergeRead(u8cs *data, u8bp *mapped, u8csc path_arg) {
    sane(data != NULL && mapped != NULL);
    a_pad(u8, path, FILE_PATH_MAX_LEN);
    call(u8bFeed, path, path_arg);
    call(PATHu8gTerm, PATHu8gIn(path));
    call(FILEMapRO, mapped, PATHu8cgIn(path));
    (*data)[0] = u8bDataHead(*mapped);
    (*data)[1] = u8bIdleHead(*mapped);
    done;
}

ok64 CAPOMerge(u8csc base_path, u8csc ours_path, u8csc theirs_path,
               u8csc outpath) {
    sane($ok(base_path) && $ok(ours_path) && $ok(theirs_path));

    // Detect extension from ours
    u8cs ext = {};
    size_t olen = (size_t)$len(ours_path);
    CAPOFindExt(ext, ours_path[0], olen);
    if (!$empty(ext) && ext[0][0] == '.') {
        ext[0] = ext[0] + 1;  // strip dot for tok/
    }

    // Read three files
    u8bp map_b = NULL, map_o = NULL, map_t = NULL;
    u8cs base_data = {}, ours_data = {}, theirs_data = {};
    call(CAPOMergeRead, &base_data, &map_b, base_path);
    call(CAPOMergeRead, &ours_data, &map_o, ours_path);
    call(CAPOMergeRead, &theirs_data, &map_t, theirs_path);

    // Tokenize
    JOINfile base = {}, ours = {}, theirs = {};
    ok64 o = JOINTokenize(&base, base_data, ext);
    if (o != OK) goto cleanup;
    o = JOINTokenize(&ours, ours_data, ext);
    if (o != OK) goto cleanup;
    o = JOINTokenize(&theirs, theirs_data, ext);
    if (o != OK) goto cleanup;

    // Merge
    {
        u64 outsz = $len(ours_data) + $len(theirs_data) + 4096;
        u8 *out[4] = {};
        o = u8bAlloc(out, outsz);
        if (o != OK) goto cleanup;
        o = JOINMerge(out, &base, &ours, &theirs);
        if (o != OK) { u8bFree(out); goto cleanup; }

        u8cs result = {out[1], out[2]};
        if (!$empty(outpath)) {
            // Write to file
            a_pad(u8, opath, FILE_PATH_MAX_LEN);
            call(u8bFeed, opath, outpath);
            call(PATHu8gTerm, PATHu8gIn(opath));
            int fd = open((char *)u8bDataHead(opath),
                          O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { u8bFree(out); o = FILEFAIL; goto cleanup; }
            o = FILEFeedall(fd, result);
            close(fd);
        } else {
            o = FILEFeedall(STDOUT_FILENO, result);
        }
        u8bFree(out);
    }

cleanup:
    JOINFree(&base);
    JOINFree(&ours);
    JOINFree(&theirs);
    if (map_b) FILEUnMap(map_b);
    if (map_o) FILEUnMap(map_o);
    if (map_t) FILEUnMap(map_t);
    return o;
}

// --- Diff: token-level colored diff ---

// Helper: get token slice from a JOINfile
#define CAPOJoinToks(ts, jf) \
    u32cs ts = {(u32cp)(jf)->toks[1], (u32cp)(jf)->toks[2]}

ok64 CAPODiff(u8csc old_path, u8csc new_path, u8csc name) {
    sane($ok(old_path) && $ok(new_path));

    // Detect extension from logical name
    u8cs ext = {};
    size_t nlen = (size_t)$len(name);
    CAPOFindExt(ext, name[0], nlen);
    if (!$empty(ext) && ext[0][0] == '.') {
        ext[0] = ext[0] + 1;  // strip dot for tok/
    }

    // Read files (either side may be empty for new/deleted files)
    u8bp map_old = NULL, map_new = NULL;
    u8cs old_data = {}, new_data = {};
    ok64 oro = CAPOMergeRead(&old_data, &map_old, old_path);
    ok64 nro = CAPOMergeRead(&new_data, &map_new, new_path);
    if (oro != OK && nro != OK) return oro;  // both failed

    if (LESSArenaInit() != OK) {
        if (map_old) FILEUnMap(map_old);
        if (map_new) FILEUnMap(map_new);
        return NOROOM;
    }

    // Display name (null-terminated) for hunk titles
    char dispname[FILE_PATH_MAX_LEN];
    size_t dlen = (size_t)$len(name);
    if (dlen >= sizeof(dispname)) dlen = sizeof(dispname) - 1;
    memcpy(dispname, name[0], dlen);
    dispname[dlen] = 0;

    // Deleted file: red header + all old content as DEL
    if (nro != OK) {
        JOINfile old_f = {};
        ok64 o = JOINTokenize(&old_f, old_data, ext);
        if (o == OK) {
            CAPOJoinToks(old_ts, &old_f);
            u32 olen = (u32)$len(old_data);
            LESShunk *hk = &less_hunks[less_nhunks];
            memset(hk, 0, sizeof(*hk));
            char hdr[512];
            int tlen = CAPOFormatTitle(hdr, sizeof(hdr), dispname, "");
            if (tlen > 0) {
                u8p tp = LESSArenaWrite(hdr, (size_t)tlen);
                if (tp) { hk->title[0] = tp; hk->title[1] = tp + tlen; }
            }
            u8p txp = LESSArenaWrite(old_data[0], olen);
            if (txp) { hk->text[0] = txp; hk->text[1] = txp + olen; }
            u8p lp = LESSArenaAlloc(olen);
            if (lp) {
                CAPOBuildLits(lp, old_data[0], olen, old_ts);
                CAPOMarkLits(lp, 0, olen, LESS_DEL);
                hk->lits[0] = lp; hk->lits[1] = lp + olen;
            }
            less_nhunks++;
        }
        JOINFree(&old_f);
        LESSRun(less_hunks, less_nhunks);
        LESSArenaCleanup();
        if (map_old) FILEUnMap(map_old);
        done;
    }

    // New file: green header + all new content as INS
    if (oro != OK) {
        JOINfile new_f = {};
        ok64 o = JOINTokenize(&new_f, new_data, ext);
        if (o != OK) {
            JOINFree(&new_f);
            if (map_new) FILEUnMap(map_new);
            LESSArenaCleanup();
            return o;
        }
        CAPOJoinToks(new_ts, &new_f);
        u32 nlen2 = (u32)$len(new_data);
        LESShunk *hk = &less_hunks[less_nhunks];
        memset(hk, 0, sizeof(*hk));
        char hdr[512];
        int tlen = snprintf(hdr, sizeof(hdr), "+++ %s ---", dispname);
        if (tlen > 0) {
            u8p tp = LESSArenaWrite(hdr, (size_t)tlen);
            if (tp) { hk->title[0] = tp; hk->title[1] = tp + tlen; }
        }
        u8p txp = LESSArenaWrite(new_data[0], nlen2);
        if (txp) { hk->text[0] = txp; hk->text[1] = txp + nlen2; }
        u8p lp = LESSArenaAlloc(nlen2);
        if (lp) {
            CAPOBuildLits(lp, new_data[0], nlen2, new_ts);
            CAPOMarkLits(lp, 0, nlen2, LESS_INS);
            hk->lits[0] = lp; hk->lits[1] = lp + nlen2;
        }
        less_nhunks++;
        LESSRun(less_hunks, less_nhunks);
        LESSArenaCleanup();
        JOINFree(&new_f);
        if (map_new) FILEUnMap(map_new);
        done;
    }

    // Tokenize
    JOINfile old_f = {}, new_f = {};
    ok64 o = JOINTokenize(&old_f, old_data, ext);
    if (o != OK) goto diff_cleanup;
    o = JOINTokenize(&new_f, new_data, ext);
    if (o != OK) goto diff_cleanup;

    {
        u64 on = u64bDataLen(old_f.hashes);
        u64 nn = u64bDataLen(new_f.hashes);

        // Allocate diff workspace
        u64 wsize = DIFFWorkSize(on, nn);
        u64 emax = DIFFEdlMaxEntries(on, nn);
        u64 total = wsize * sizeof(i32) + emax * sizeof(e32);
        u8 *mem[4] = {};
        o = u8bAlloc(mem, total);
        if (o != OK) goto diff_cleanup;

        i32p workp = (i32p)mem[1];
        i32s ws = {workp, workp + wsize};
        e32 *edl_buf = (e32 *)(workp + wsize);
        e32g edl = {edl_buf, edl_buf + emax, edl_buf};

        CAPOJoinToks(old_ts, &old_f);
        CAPOJoinToks(new_ts, &new_f);

        u64cs oh = {old_f.hashes[1], old_f.hashes[2]};
        u64cs nh = {new_f.hashes[1], new_f.hashes[2]};
        o = DIFFu64s(edl, ws, oh, nh);
        if (o != OK) { u8bFree(mem); goto diff_cleanup; }

        // Semantic cleanup: remove false short equalities
        NEILCleanup(edl, old_ts, new_ts, old_data, new_data);

        // Lossless shift: align edit boundaries on line breaks
        NEILShift(edl, old_ts, new_ts, old_data, new_data);

        // dispname built above, before deleted/new branches

        // Walk EDL, emit colored output with context trimming.
        // Build u32 visible-line intervals, then emit per-token.

        #define CTX_LINES 3

        u32 nedl = (u32)(edl[0] - edl[2]);

        // Phase 1: scan EDL, track new-side line numbers.
        // For each change, record visible interval [lo, hi] as
        // a u32 pair: vis[2*i]=lo, vis[2*i+1]=hi.
        u32 *vis = NULL;
        u32 nvis = 0;
        if (nedl > 0)
            vis = (u32 *)malloc(2 * nedl * sizeof(u32));
        if (vis != NULL) {
            u64 sni = 0;
            u32 nl = 0;
            for (u32 k = 0; k < nedl; k++) {
                e32 e = edl[2][k];
                u32 elen = DIFF_LEN(e);
                u32 op = DIFF_OP(e);
                if (op == DIFF_EQ) {
                    for (u32 j = 0; j < elen; j++) {
                        u8cs v = {};
                        tok32Val(v,new_ts,new_f.data[0],(int)(sni + j));
                        $for(u8c, cp, v)
                            if (*cp == '\n') nl++;
                    }
                    sni += elen;
                } else if (op == DIFF_INS) {
                    u32 sl = nl;
                    for (u32 j = 0; j < elen; j++) {
                        u8cs v = {};
                        tok32Val(v,new_ts,new_f.data[0],(int)(sni + j));
                        $for(u8c, cp, v)
                            if (*cp == '\n') nl++;
                    }
                    u32 lo = (sl > CTX_LINES) ? sl - CTX_LINES : 0;
                    u32 hi = nl + CTX_LINES;
                    vis[nvis * 2] = lo;
                    vis[nvis * 2 + 1] = hi;
                    nvis++;
                    sni += elen;
                } else {  // DIFF_DEL
                    u32 lo = (nl > CTX_LINES) ? nl - CTX_LINES : 0;
                    u32 hi = nl + CTX_LINES;
                    vis[nvis * 2] = lo;
                    vis[nvis * 2 + 1] = hi;
                    nvis++;
                }
            }
            // Merge overlapping intervals (sorted by lo already)
            u32 m = 0;
            for (u32 i = 0; i < nvis; i++) {
                u32 lo = vis[i * 2], hi = vis[i * 2 + 1];
                if (m > 0 && lo <= vis[(m - 1) * 2 + 1]) {
                    if (hi > vis[(m - 1) * 2 + 1])
                        vis[(m - 1) * 2 + 1] = hi;
                } else {
                    vis[m * 2] = lo;
                    vis[m * 2 + 1] = hi;
                    m++;
                }
            }
            nvis = m;
        }

        // Phase 2: walk EDL, build LESS hunks.
        // Allocate combined text+lits buffers in arena.
        // Worst case: all old bytes (DEL) + all new bytes (INS).
        u32 old_len = (u32)$len(old_data);
        u32 new_len = (u32)$len(new_data);
        u32 arena_need = old_len + new_len;
        u8p diff_text = NULL, diff_lits = NULL;
        u8p dtxp = NULL, dltp = NULL;  // cursors
        LESShunk *cur_hunk = NULL;

        if (arena_need > 0) {
            diff_text = LESSArenaAlloc(arena_need);
            diff_lits = LESSArenaAlloc(arena_need);
            if (diff_text != NULL && diff_lits != NULL) {
                dtxp = diff_text;
                dltp = diff_lits;
            }
        }

        // Helper macros for copying tokens into the LESS buffer
        #define DIFF_COPY_TOK(toks_s, base, idx, flag) do {    \
            u8cs _v = {};                                       \
            tok32Val(_v,toks_s,base,(int)(idx));              \
            u32 _n = (u32)$len(_v);                             \
            u8 _tag = tok32Tag((toks_s)[0][(idx)]) - 'A';       \
            memcpy(dtxp, _v[0], _n);                            \
            memset(dltp, _tag | (flag), _n);                    \
            dtxp += _n;                                         \
            dltp += _n;                                         \
        } while(0)

        // Start a new LESS hunk with title
        #define DIFF_START_HUNK(boff) do {                      \
            if (cur_hunk != NULL) {                             \
                cur_hunk->text[1] = dtxp;                       \
                cur_hunk->lits[1] = dltp;                       \
            }                                                   \
            if (less_nhunks < LESS_MAX_HUNKS) {                 \
                cur_hunk = &less_hunks[less_nhunks];            \
                memset(cur_hunk, 0, sizeof(*cur_hunk));         \
                char _funcname[256];                            \
                CAPOFindFunc(new_data, (boff), ext,             \
                             _funcname, sizeof(_funcname));      \
                char _hdr[512];                                 \
                int _tl = CAPOFormatTitle(_hdr, sizeof(_hdr),   \
                    dispname, _funcname);                       \
                if (_tl > 0) {                                  \
                    u8p _tp = LESSArenaWrite(_hdr, (size_t)_tl);\
                    if (_tp) {                                  \
                        cur_hunk->title[0] = _tp;               \
                        cur_hunk->title[1] = _tp + _tl;         \
                    }                                           \
                }                                               \
                cur_hunk->text[0] = dtxp;                       \
                cur_hunk->lits[0] = dltp;                       \
                less_nhunks++;                                  \
            }                                                   \
        } while(0)

        // Copy leading whitespace on current line into hunk
        #define DIFF_COPY_LINE_PREFIX(boff) do {                \
            u32 _ls = (boff);                                   \
            while (_ls > 0 && new_data[0][_ls-1] != '\n') _ls--;\
            if (_ls < (boff)) {                                 \
                u32 _pn = (boff) - _ls;                         \
                memcpy(dtxp, new_data[0] + _ls, _pn);           \
                memset(dltp, 0, _pn);                           \
                dtxp += _pn;                                    \
                dltp += _pn;                                    \
            }                                                   \
        } while(0)

        u64 oi = 0, ni = 0;
        u32 cur_line = 0, cur_iv = 0;
        b8 in_gap = YES;

        for (u32 k = 0; k < nedl; ) {
            e32 e = edl[2][k];
            if (DIFF_OP(e) == DIFF_EQ) {
                u32 len = DIFF_LEN(e);
                for (u32 j = 0; j < len; j++) {
                    while (cur_iv < nvis &&
                           vis[cur_iv * 2 + 1] < cur_line)
                        cur_iv++;
                    b8 show = (vis == NULL) ||
                        (cur_iv < nvis &&
                         cur_line >= vis[cur_iv * 2] &&
                         cur_line <= vis[cur_iv * 2 + 1]);
                    if (show) {
                        if (in_gap) {
                            u32 boff = (ni > 0) ? tok32Offset(new_ts[0][ni-1]) : 0;
                            DIFF_START_HUNK(boff);
                            DIFF_COPY_LINE_PREFIX(boff);
                            in_gap = NO;
                        }
                        DIFF_COPY_TOK(new_ts, new_f.data[0], ni, 0);
                    } else {
                        in_gap = YES;
                    }
                    u8cs v = {};
                    tok32Val(v,new_ts,new_f.data[0],(int)ni);
                    $for(u8c, cp, v)
                        if (*cp == '\n') cur_line++;
                    oi++; ni++;
                }
                k++;
            } else {
                // Change region: scan to end of consecutive DEL/INS
                u32 kend = k;
                while (kend < nedl && DIFF_OP(edl[2][kend]) != DIFF_EQ)
                    kend++;

                // Count total DEL and INS tokens in this region
                u32 del_total = 0, ins_total = 0;
                b8 ws_only = YES;
                for (u32 kk = k; kk < kend; kk++) {
                    u32 klen = DIFF_LEN(edl[2][kk]);
                    if (DIFF_OP(edl[2][kk]) == DIFF_DEL) {
                        del_total += klen;
                        for (u32 j = 0; j < klen && ws_only; j++)
                            if (!NEILIsWS(old_ts, old_f.data[0],
                                          oi + del_total - klen + j))
                                ws_only = NO;
                    } else {
                        ins_total += klen;
                        for (u32 j = 0; j < klen && ws_only; j++)
                            if (!NEILIsWS(new_ts, new_f.data[0],
                                          ni + ins_total - klen + j))
                                ws_only = NO;
                    }
                }

                // Whitespace-only changes: emit new side as EQ context
                if (ws_only && ins_total > 0) {
                    for (u32 j = 0; j < ins_total; j++) {
                        while (cur_iv < nvis &&
                               vis[cur_iv * 2 + 1] < cur_line)
                            cur_iv++;
                        b8 show = (vis == NULL) ||
                            (cur_iv < nvis &&
                             cur_line >= vis[cur_iv * 2] &&
                             cur_line <= vis[cur_iv * 2 + 1]);
                        if (show) {
                            if (in_gap) {
                                u64 ti = ni + j;
                                u32 boff = (ti > 0)
                                    ? tok32Offset(new_ts[0][ti-1]) : 0;
                                DIFF_START_HUNK(boff);
                                DIFF_COPY_LINE_PREFIX(boff);
                                in_gap = NO;
                            }
                            DIFF_COPY_TOK(new_ts, new_f.data[0], ni + j, 0);
                        } else {
                            in_gap = YES;
                        }
                        u8cs v = {};
                        tok32Val(v,new_ts,new_f.data[0],(int)(ni + j));
                        $for(u8c, cp, v)
                            if (*cp == '\n') cur_line++;
                    }
                    oi += del_total;
                    ni += ins_total;
                    k = kend;
                    continue;
                }

                // Extract common prefix: compare old/new token
                // source bytes to find misaligned EQ tokens.
                u32 prefix = 0;
                {
                    u32 lim = (del_total < ins_total)
                              ? del_total : ins_total;
                    u64 ti = oi, tj = ni;
                    while (prefix < lim) {
                        u8cs ov = {}, nv = {};
                        tok32Val(ov,old_ts,old_f.data[0],(int)ti);
                        tok32Val(nv,new_ts,new_f.data[0],(int)tj);
                        if ($len(ov) != $len(nv)) break;
                        if (memcmp(ov[0], nv[0], (size_t)$len(ov)))
                            break;
                        prefix++; ti++; tj++;
                    }
                }

                // Extract common suffix
                u32 suffix = 0;
                {
                    u32 lim = (del_total < ins_total)
                              ? del_total : ins_total;
                    lim -= prefix;
                    u64 ti = oi + del_total - 1;
                    u64 tj = ni + ins_total - 1;
                    while (suffix < lim) {
                        u8cs ov = {}, nv = {};
                        tok32Val(ov,old_ts,old_f.data[0],(int)ti);
                        tok32Val(nv,new_ts,new_f.data[0],(int)tj);
                        if ($len(ov) != $len(nv)) break;
                        if (memcmp(ov[0], nv[0], (size_t)$len(ov)))
                            break;
                        suffix++; ti--; tj--;
                    }
                }

                // Save base positions before prefix extraction
                u64 base_oi = oi, base_ni = ni;

                // Emit common prefix as EQ context
                for (u32 j = 0; j < prefix; j++) {
                    while (cur_iv < nvis &&
                           vis[cur_iv * 2 + 1] < cur_line)
                        cur_iv++;
                    b8 show = (vis == NULL) ||
                        (cur_iv < nvis &&
                         cur_line >= vis[cur_iv * 2] &&
                         cur_line <= vis[cur_iv * 2 + 1]);
                    if (show) {
                        if (in_gap) {
                            u64 ti = base_ni + j;
                            u32 boff = (ti > 0)
                                ? tok32Offset(new_ts[0][ti-1]) : 0;
                            DIFF_START_HUNK(boff);
                            DIFF_COPY_LINE_PREFIX(boff);
                            in_gap = NO;
                        }
                        DIFF_COPY_TOK(new_ts, new_f.data[0], base_ni + j, 0);
                    } else {
                        in_gap = YES;
                    }
                    u8cs v = {};
                    tok32Val(v,new_ts,new_f.data[0],(int)(base_ni + j));
                    $for(u8c, cp, v)
                        if (*cp == '\n') cur_line++;
                }

                if (in_gap) {
                    u32 boff = (base_ni > 0)
                        ? tok32Offset(new_ts[0][base_ni-1]) : 0;
                    DIFF_START_HUNK(boff);
                    in_gap = NO;
                }

                // Emit DEL tokens (excluding prefix/suffix)
                u64 toi = base_oi;
                for (u32 kk = k; kk < kend; kk++) {
                    if (DIFF_OP(edl[2][kk]) == DIFF_DEL) {
                        u32 dlen = DIFF_LEN(edl[2][kk]);
                        for (u32 j = 0; j < dlen; j++) {
                            u32 pos = (u32)(toi - base_oi);
                            if (pos >= prefix &&
                                pos < del_total - suffix) {
                                DIFF_COPY_TOK(old_ts, old_f.data[0],
                                              toi, LESS_DEL);
                            }
                            toi++;
                        }
                    }
                }

                // Newline separator between DEL and INS output:
                // if DEL text didn't end with '\n', insert one so
                // deletions and insertions don't merge onto one line.
                // Also copy the INS line's indentation so it aligns.
                if (del_total > prefix + suffix &&
                    ins_total > prefix + suffix &&
                    (dtxp == diff_text || *(dtxp - 1) == '\n')) {
                    u64 last_del = base_oi + del_total - suffix - 1;
                    u8cs ldv = {};
                    tok32Val(ldv,old_ts,old_f.data[0],(int)last_del);
                    if (!$empty(ldv) && *(ldv[1] - 1) != '\n') {
                        // Find indentation of first INS token's line
                        u64 ins_ti = base_ni + prefix;
                        u32 ins_boff = (ins_ti > 0)
                            ? tok32Offset(new_ts[0][ins_ti - 1]) : 0;
                        u32 ls = ins_boff;
                        while (ls > 0 && new_data[0][ls - 1] != '\n')
                            ls--;
                        *dtxp++ = '\n';
                        *dltp++ = LESS_DEL;
                        if (ls < ins_boff) {
                            u32 pn = ins_boff - ls;
                            memcpy(dtxp, new_data[0] + ls, pn);
                            memset(dltp, LESS_INS, pn);
                            dtxp += pn;
                            dltp += pn;
                        }
                    }
                }

                // Emit INS tokens (excluding prefix/suffix)
                u64 tni = base_ni;
                for (u32 kk = k; kk < kend; kk++) {
                    if (DIFF_OP(edl[2][kk]) == DIFF_INS) {
                        u32 ilen = DIFF_LEN(edl[2][kk]);
                        for (u32 j = 0; j < ilen; j++) {
                            u32 pos = (u32)(tni - base_ni);
                            if (pos >= prefix &&
                                pos < ins_total - suffix) {
                                DIFF_COPY_TOK(new_ts, new_f.data[0],
                                              tni, LESS_INS);
                                u8cs v = {};
                                tok32Val(v,new_ts,new_f.data[0],(int)tni);
                                $for(u8c, cp, v)
                                    if (*cp == '\n') cur_line++;
                            }
                            tni++;
                        }
                    }
                }

                // Emit common suffix as EQ context
                for (u32 j = 0; j < suffix; j++) {
                    u64 sn = base_ni + ins_total - suffix + j;
                    while (cur_iv < nvis &&
                           vis[cur_iv * 2 + 1] < cur_line)
                        cur_iv++;
                    b8 show = (vis == NULL) ||
                        (cur_iv < nvis &&
                         cur_line >= vis[cur_iv * 2] &&
                         cur_line <= vis[cur_iv * 2 + 1]);
                    if (show) {
                        DIFF_COPY_TOK(new_ts, new_f.data[0], sn, 0);
                    } else {
                        in_gap = YES;
                    }
                    u8cs v = {};
                    tok32Val(v,new_ts,new_f.data[0],(int)sn);
                    $for(u8c, cp, v)
                        if (*cp == '\n') cur_line++;
                }

                oi = toi;
                ni = tni;
                k = kend;
            }
        }

        // Finalize last LESS hunk
        if (cur_hunk != NULL) {
            cur_hunk->text[1] = dtxp;
            cur_hunk->lits[1] = dltp;
        }

        #undef DIFF_COPY_TOK
        #undef DIFF_START_HUNK
        #undef DIFF_COPY_LINE_PREFIX

        if (vis != NULL) free(vis);
        u8bFree(mem);
    }

    if (less_nhunks > 0)
        LESSRun(less_hunks, less_nhunks);
    LESSArenaCleanup();

diff_cleanup:
    JOINFree(&old_f);
    JOINFree(&new_f);
    if (map_old) FILEUnMap(map_old);
    if (map_new) FILEUnMap(map_new);
    return o;
}

// --- SPOT search ---

ok64 CAPOSpot(u8csc needle, u8csc replace, u8csc ext, u8csc reporoot,
              u8css files) {
    sane($ok(needle) && $ok(ext) && $ok(reporoot));
    int nfiles = (int)$len(files);

    if (!CAPOKnownExt(ext)) return SPOTBAD;

    // --- Trigram filtering (skip when explicit files given) ---
    u32 maxhashes = 64 * 1024;
    u32 *hashbuf1 = NULL;
    u32 *hashbuf2 = NULL;
    u32 nhashes = 0;
    b8 has_trigrams = NO;

    if (nfiles == 0) {
        a_pad(u8, capodir, FILE_PATH_MAX_LEN);
        call(CAPOResolveDir, capodir, reporoot);
        a_dup(u8c, dirslice, u8bDataC(capodir));

        hashbuf1 = (u32 *)malloc(maxhashes * sizeof(u32));
        hashbuf2 = (u32 *)malloc(maxhashes * sizeof(u32));
        test(hashbuf1 != NULL && hashbuf2 != NULL, FAILSANITY);

        u64cs runs[CAPO_MAX_LEVELS] = {};
        u64css stack = {runs, runs};
        u8bp mmaps[CAPO_MAX_LEVELS] = {};
        u32 nidxfiles = 0;
        call(CAPOStackOpen, stack, mmaps, &nidxfiles, dirslice);
        stack[1] = stack[0] + nidxfiles;

        if (nidxfiles == 0)
            fprintf(stderr, "spot: warning: no index, run `spot` or `spot --fork N` first\n");

        if (nidxfiles > 0) {
            u8cp p = needle[0];
            u8cp end = needle[1] - 2;
            while (p <= end) {
                if (CAPOTriChar(p[0]) && CAPOTriChar(p[1]) &&
                    CAPOTriChar(p[2])) {
                    u8cs tri = {p, p + 3};
                    u64 tri_prefix = CAPOTriPack(tri);

                    u64cs seek_runs[CAPO_MAX_LEVELS];
                    for (u32 i = 0; i < nidxfiles; i++) {
                        seek_runs[i][0] = runs[i][0];
                        seek_runs[i][1] = runs[i][1];
                    }
                    u64css seek_iter = {seek_runs, seek_runs + nidxfiles};
                    MSETu64Start(seek_iter);

                    u32 tri_nhashes = 0;
                    CAPOCollectPaths(seek_iter, tri_prefix, hashbuf2,
                                     &tri_nhashes, maxhashes);

                    if (!has_trigrams) {
                        memcpy(hashbuf1, hashbuf2, tri_nhashes * sizeof(u32));
                        nhashes = tri_nhashes;
                        has_trigrams = YES;
                    } else {
                        qsort(hashbuf2, tri_nhashes, sizeof(u32), CAPOu32cmp);
                        qsort(hashbuf1, nhashes, sizeof(u32), CAPOu32cmp);
                        nhashes = CAPOIntersect(hashbuf1, nhashes, hashbuf2,
                                                tri_nhashes, hashbuf1);
                    }
                }
                p++;
            }
        }

        CAPOStackClose(mmaps, nidxfiles);

        if (has_trigrams && nhashes > 0)
            qsort(hashbuf1, nhashes, sizeof(u32), CAPOu32cmp);
    }

    if ($empty(replace)) {
        call(LESSArenaInit);
    }

    FILE *fp = NULL;
    if (nfiles == 0) {
        char cmdbuf[FILE_PATH_MAX_LEN + 32];
        int cn = snprintf(cmdbuf, sizeof(cmdbuf), "git -C %.*s ls-files",
                          (int)$len(reporoot), (char *)reporoot[0]);
        test(cn > 0 && cn < (int)sizeof(cmdbuf), FAILSANITY);
        fp = popen(cmdbuf, "r");
        test(fp != NULL, FAILSANITY);
    }

    char line[FILE_PATH_MAX_LEN];
    int fi = 0;
    int total_replacements = 0;
    int total_files_replaced = 0;
    while (nfiles > 0
           ? fi < nfiles
           : fgets(line, sizeof(line), fp) != NULL) {
        size_t len;
        if (nfiles > 0) {
            u8cs *fp = u8cssAtP(files, fi);
            len = (size_t)$len(*fp);
            if (len >= sizeof(line)) { fi++; continue; }
            u8s lns = {(u8p)line, (u8p)line + len};
            u8sCopy(lns, *fp);
            line[len] = 0;
            fi++;
        } else {
            len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') line[--len] = 0;
            if (len == 0) continue;
        }

        u8cs relpath = {(u8cp)line, (u8cp)line + len};

        if (nfiles == 0) {
            if (has_trigrams && nhashes > 0) {
                u32 phash = CAPOPathHash(relpath);
                if (!bsearch(&phash, hashbuf1, nhashes, sizeof(u32), CAPOu32cmp))
                    continue;
            } else if (has_trigrams) {
                continue;
            }
        }

        // Filter by extension match
        u8cs file_ext = {};
        CAPOFindExt(file_ext, line, len);
        if ($empty(file_ext)) continue;
        if (nfiles == 0) {
            if ($len(file_ext) != $len(ext) ||
                memcmp(file_ext[0], ext[0], $len(ext)) != 0) continue;
        }

        CAPOProgress(line);

        char fpath[FILE_PATH_MAX_LEN * 2];
        int pn = snprintf(fpath, sizeof(fpath), "%.*s/%s",
                          (int)$len(reporoot), (char *)reporoot[0], line);
        if (pn <= 0 || pn >= (int)sizeof(fpath)) continue;

        a_pad(u8, fpbuf, FILE_PATH_MAX_LEN);
        u8cs fps = {(u8cp)fpath, (u8cp)fpath + pn};
        call(PATHu8bFeed, fpbuf, fps);

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, PATHu8cgIn(fpbuf));
        if (o != OK) continue;

        a_dup(u8c, source, u8bDataC(mapped));

        // Tokenize
        Bu32 toks = {};
        size_t maxlen = $len(source) + 1;
        o = u32bMap(toks, maxlen);
        if (o != OK) { FILEUnMap(mapped); continue; }
        o = SPOTTokenize(toks, source, file_ext);
        if (o != OK) {
            u32bUnMap(toks);
            FILEUnMap(mapped);
            continue;
        }
        {
            u32 *dts[2] = {u32bDataHead(toks), u32bIdleHead(toks)};
            u8cs dext = {file_ext[0], file_ext[1]};
            if (!$empty(dext) && dext[0][0] == '.') dext[0]++;
            DEFMark(dts, source, dext);
        }
        u32cp td = u32bDataHead(toks);
        u32cp ti = u32bIdleHead(toks);
        u32cs htoks = {(u32cp)td, (u32cp)ti};

        signal(SIGABRT, capo_abrt_handler);
        capo_in_match = 1;
        if (sigsetjmp(capo_jmpbuf, 1) != 0) {
            capo_in_match = 0;
            signal(SIGABRT, SIG_DFL);
            u32bUnMap(toks);
            FILEUnMap(mapped);
            continue;
        }

        if (!$empty(replace)) {
            // --- Replacement mode ---
            size_t obuflen = $len(source) * 2 + 4096;
            Bu8 obufm = {};
            o = u8bMap(obufm, obuflen);
            if (o == OK) {
                u8s rout = {u8bIdleHead(obufm), obufm[3]};
                int file_matches = 0;
                o = SPOTReplace(rout, source, htoks, needle,
                                replace, file_ext, &file_matches);
                if (o == OK) {
                    u8cs result = {u8bIdleHead(obufm), rout[0]};
                    FILEUnMap(mapped);
                    mapped = NULL;
                    int fd = -1;
                    ok64 wo = FILECreate(&fd, PATHu8cgIn(fpbuf));
                    if (wo == OK) {
                        FILEFeedall(fd, result);
                        close(fd);
                        CAPOProgress(NULL);
                        fprintf(stderr, "replaced: %s (%d)\n",
                                line, file_matches);
                        total_replacements += file_matches;
                        total_files_replaced++;
                    }
                }
                u8bUnMap(obufm);
            }
        } else {
            // --- Search/display mode ---
            aBpad(u32, nbuf, 4096);
            SPOTstate st = {};
            o = SPOTInit(&st, nbuf, needle, file_ext, htoks, source);
            if (o == OK) {
                b8 found_any = NO;
                b8 first_hunk = YES;
                u32 prev_ctx_hi = 0;
                b8 have_pending = NO;
                range32 pending = {};
                for (;;) {
                    u32 slo, shi;
                    if (have_pending) {
                        slo = pending.lo;
                        shi = pending.hi;
                        have_pending = NO;
                    } else {
                        if (SPOTNext(&st) != OK) break;
                        slo = st.src_rng.lo;
                        shi = st.src_rng.hi;
                    }
                    if (shi <= slo || shi > (u32)$len(source)) continue;

                    if (!found_any) {
                        CAPOProgress(NULL);
                        found_any = YES;
                    }

                    // Compute context around match
                    u32 ctx_lo = 0, ctx_hi = 0;
                    CAPOGrepCtx(source, slo, 3, &ctx_lo, &ctx_hi);

                    // Collect subsequent matches within this context
                    range32 hls[CAPO_MAX_HLS];
                    int nhl = 0;
                    hls[nhl++] = (range32){slo, shi};
                    while (nhl < CAPO_MAX_HLS && SPOTNext(&st) == OK) {
                        u32 s2 = st.src_rng.lo;
                        u32 e2 = st.src_rng.hi;
                        if (e2 <= s2 || e2 > (u32)$len(source)) continue;
                        if (s2 >= ctx_hi) {
                            pending = (range32){s2, e2};
                            have_pending = YES;
                            break;
                        }
                        hls[nhl++] = (range32){s2, e2};
                        u32 lo2 = 0, hi2 = 0;
                        CAPOGrepCtx(source, s2, 3, &lo2, &hi2);
                        if (hi2 > ctx_hi) ctx_hi = hi2;
                    }

                    b8 contiguous = (ctx_lo <= prev_ctx_hi);
                    if (ctx_lo < prev_ctx_hi) ctx_lo = prev_ctx_hi;
                    if (ctx_lo < ctx_hi &&
                        less_nhunks < LESS_MAX_HUNKS) {
                        LESShunk *hk = &less_hunks[less_nhunks];
                        memset(hk, 0, sizeof(*hk));

                        // Title
                        if (!contiguous || first_hunk) {
                            char funcname[256];
                            CAPOFindFunc(source, ctx_lo, file_ext,
                                         funcname, sizeof(funcname));
                            char hdr[512];
                            int tlen = CAPOFormatTitle(hdr, sizeof(hdr),
                                                       line, funcname);
                            if (tlen > 0) {
                                u8p tp = LESSArenaWrite(hdr, (size_t)tlen);
                                if (tp != NULL) {
                                    hk->title[0] = tp;
                                    hk->title[1] = tp + tlen;
                                }
                            }
                        }

                        hk->text[0] = source[0] + ctx_lo;
                        hk->text[1] = source[0] + ctx_hi;
                        hk->toks[0] = htoks[0];
                        hk->toks[1] = htoks[1];

                        // Lits
                        if (CAPO_COLOR) {
                            u32 region_len = ctx_hi - ctx_lo;
                            u8p lp = LESSArenaAlloc(region_len);
                            if (lp != NULL) {
                                int ntoks_r = (int)$len(htoks);
                                for (int ti = 0; ti < ntoks_r; ti++) {
                                    u32 tlo = (ti > 0) ? tok32Offset(htoks[0][ti-1]) : 0;
                                    u32 thi = tok32Offset(htoks[0][ti]);
                                    if (thi <= ctx_lo || tlo >= ctx_hi) continue;
                                    u32 clo = tlo < ctx_lo ? ctx_lo : tlo;
                                    u32 chi = thi > ctx_hi ? ctx_hi : thi;
                                    u8 tag = tok32Tag(htoks[0][ti]) - 'A';
                                    memset(lp + (clo - ctx_lo), tag, chi - clo);
                                }
                                for (int h = 0; h < nhl; h++) {
                                    u32 hlo = hls[h].lo < ctx_lo ? ctx_lo : hls[h].lo;
                                    u32 hhi = hls[h].hi > ctx_hi ? ctx_hi : hls[h].hi;
                                    for (u32 b = hlo; b < hhi; b++)
                                        lp[b - ctx_lo] |= LESS_INS;
                                }
                                hk->lits[0] = lp;
                                hk->lits[1] = lp + region_len;
                            }
                        }

                        less_nhunks++;
                        first_hunk = NO;
                    }
                    prev_ctx_hi = ctx_hi;
                }
            }
        }

        capo_in_match = 0;
        signal(SIGABRT, SIG_DFL);

        if ($empty(replace)) {
            LESSDefer(mapped, toks);
        } else {
            u32bUnMap(toks);
            if (mapped != NULL) FILEUnMap(mapped);
        }
    }
    if (fp != NULL) pclose(fp);
    CAPOProgress(NULL);

    if ($empty(replace) && less_nhunks > 0)
        LESSRun(less_hunks, less_nhunks);
    if ($empty(replace))
        LESSArenaCleanup();

    if (!$empty(replace)) {
        fprintf(stderr, "%d replacements in %d files\n",
                total_replacements, total_files_replaced);
    }

    if (hashbuf1 != NULL) free(hashbuf1);
    if (hashbuf2 != NULL) free(hashbuf2);
    done;
}
