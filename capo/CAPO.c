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
#include "ast/BAST.h"
#include "ast/CSS.h"
#include "ast/SPOT.h"
#include "json/BASON.h"

// MSET for u64 (trigram entries)
#define X(M, name) M##u64##name
#include "abc/MSETx.h"
#undef X

// --- Resolve capo index directory (handles worktrees) ---

// Resolves the capo index dir: if .git is a directory, uses reporoot/.git/capo.
// If .git is a file (worktree), reads gitdir from it, uses <gitdir>/capo.
ok64 CAPOResolveDir(path8b out, u8csc reporoot) {
    sane($ok(reporoot) && out != NULL);
    a_pad(u8, gitpath, FILE_PATH_MAX_LEN);
    call(path8bFeedS, gitpath, reporoot);
    a_cstr(gitname, ".git");
    call(path8bPush, gitpath, gitname);

    ok64 isdir = FILEisdir(path8cgIn(gitpath));
    if (isdir == OK) {
        // Normal repo: .git is a directory
        call(path8bFeedS, out, reporoot);
        a_cstr(caponame, CAPO_DIR);
        call(path8bPush, out, caponame);
    } else {
        // Worktree: .git is a file with "gitdir: <path>"
        u8bp mapped = NULL;
        call(FILEMapRO, &mapped, path8cgIn(gitpath));
        u8cs content = {u8bDataHead(mapped), u8bIdleHead(mapped)};
        // Parse "gitdir: <path>\n"
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
        // gitdir may be relative to reporoot
        if (gitdir[0][0] == '/') {
            call(path8bFeedS, out, gitdir);
        } else {
            call(path8bFeedS, out, reporoot);
            call(path8bPush, out, gitdir);
        }
        FILEUnMap(mapped);
        a_cstr(caponame2, "capo");
        call(path8bPush, out, caponame2);
    }
    done;
}

// --- Triplet extraction from BASON tree ---

typedef struct {
    u64 **idle;  // points into buf[2] (idle pointer)
    u64 *end;    // buf[3] (end of buffer)
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

// Walk BASON leaves, extract RON64 trigrams, call cb for each
static ok64 CAPOTriExtract(u8csc bason, ok64 (*cb)(voidp, u8cs), voidp arg) {
    sane($ok(bason) && cb != NULL);
    if ($empty(bason)) done;
    aBpad(u64, stk, 64);
    call(BASONOpen, stk, bason);
    int depth = 0;
    for (;;) {
        u8 type = 0;
        u8cs key = {};
        u8cs val = {};
        ok64 o = BASONDrain(stk, bason, &type, key, val);
        if (o != OK) {
            if (depth <= 0) break;
            call(BASONOuto, stk);
            depth--;
            continue;
        }
        if (!BASONCollection(type) && $len(val) >= 3) {
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
        } else if (BASONCollection(type)) {
            call(BASONInto, stk, bason, val);
            depth++;
        }
    }
    done;
}

ok64 CAPOIndexFile(u64bp entries, u8csc source, u8csc ext, u8csc path) {
    sane(entries != NULL && $ok(source) && $ok(ext) && $ok(path));
    if ($empty(source)) done;

    size_t buflen = 1UL << 28;  // 256MB virtual, physical on demand
    Bu8 bson = {};
    vcall("mmap bson", u8bMap, bson, buflen);
    size_t idxlen = 1UL << 20;
    Bu64 idx = {};
    vcall("mmap idx", u64bMap, idx, idxlen);

    ok64 o = BASTParse(bson, idx, source, ext);
    if (o != OK) {
        fprintf(stderr, "capo: parse: %s (src=%zu ext=%.*s)\n",
                ok64str(o), $len(source),
                (int)$len(ext), (char *)ext[0]);
        u64bUnMap(idx);
        u8bUnMap(bson);
        return o;
    }

    u8cs bdata = {u8bDataHead(bson), u8bIdleHead(bson)};
    CAPOTriCtx ctx = {
        .idle = u64bIdle(entries),
        .end = entries[3],
        .path_hash = CAPOPathHash(path),
    };
    o = CAPOTriExtract(bdata, CAPOTriCB, &ctx);

    u64bUnMap(idx);
    u8bUnMap(bson);
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
    a_cstr(ext, CAPO_IDX_EXT);
    call(u8bFeed, path, ext);
    call(path8gTerm, path8gIn(path));

    int fd = -1;
    call(FILECreate, &fd, path8cgIn(path));
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
    call(path8bFeedS, pat, dir);

    int dfd = -1;
    ok64 o = FILEOpenDir(&dfd, path8cgIn(pat));
    if (o != OK) done;

    u64 maxseq = 0;
    DIR *d = fdopendir(dfd);
    if (d == NULL) { close(dfd); done; }

    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        size_t nlen = strlen(e->d_name);
        if (nlen < 5) continue;
        if (strcmp(e->d_name + nlen - 4, CAPO_IDX_EXT) != 0) continue;
        size_t numlen = nlen - 4;  // strip .idx
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
    call(path8bFeedS, dpat, dir);

    int dfd = -1;
    ok64 o = FILEOpenDir(&dfd, path8cgIn(dpat));
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

    // Sort filenames (RON64-padded = sorted by seqno)
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
        call(path8bFeedS, fpath, dir);
        u8cs fn = {(u8cp)names[i], (u8cp)names[i] + strlen(names[i])};
        call(path8bPush, fpath, fn);

        u8bp mapped = NULL;
        call(FILEMapRO, &mapped, path8cgIn(fpath));

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

    // Collect filenames to unlink before closing mmaps
    a_pad(u8, dpat, FILE_PATH_MAX_LEN);
    call(path8bFeedS, dpat, dir);

    char fnames[CAPO_MAX_LEVELS][64];
    u32 fcount = 0;
    int dfd = -1;
    ok64 o = FILEOpenDir(&dfd, path8cgIn(dpat));
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

    // Sort same way
    for (u32 i = 0; i + 1 < fcount; i++)
        for (u32 j = i + 1; j < fcount; j++)
            if (strcmp(fnames[i], fnames[j]) > 0) {
                char tmp[64];
                memcpy(tmp, fnames[i], 64);
                memcpy(fnames[i], fnames[j], 64);
                memcpy(fnames[j], tmp, 64);
            }

    CAPOStackClose(mmaps, nfiles);

    // Unlink the m youngest original files (excluding the one we just wrote)
    // The new file has the highest seqno, so it's last after sort.
    // Original files are the first fcount-1 entries (before re-scan added new).
    // Actually, re-scan happened after write, so new file is included.
    // Unlink from tail, skipping the newest (our merged output).
    u32 unlinked = 0;
    for (u32 i = fcount; i > 0 && unlinked < m; i--) {
        // Skip the file we just wrote (highest seqno = last in sorted order)
        if (i == fcount) continue;
        a_pad(u8, ulpath, FILE_PATH_MAX_LEN);
        call(path8bFeedS, ulpath, dir);
        u8cs ulfn = {(u8cp)fnames[i - 1],
                     (u8cp)fnames[i - 1] + strlen(fnames[i - 1])};
        call(path8bPush, ulpath, ulfn);
        unlink((char *)u8bDataHead(ulpath));
        unlinked++;
    }

    free(buf);
    done;
}

// --- Helper: find file extension ---

// Find file extension in path. Sets ext[0..1] or leaves empty.
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
        if (BASTLanguage(ext) == NULL) { skipped++; continue; }

        char fpath[FILE_PATH_MAX_LEN * 2];
        int pn = snprintf(fpath, sizeof(fpath), "%.*s/%s",
                          (int)$len(reporoot), (char *)reporoot[0], line);
        if (pn <= 0 || pn >= (int)sizeof(fpath)) { skipped++; continue; }

        a_pad(u8, fpbuf, FILE_PATH_MAX_LEN);
        u8cs fps = {(u8cp)fpath, (u8cp)fpath + pn};
        call(path8bFeedS, fpbuf, fps);

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, path8cgIn(fpbuf));
        if (o != OK) {
            fprintf(stderr, "FAIL\t%s\t%s\t(open %s)\n",
                    ok64str(o), line, fpath);
            failed++;
            continue;
        }

        u8cs source = {u8bDataHead(mapped), u8bIdleHead(mapped)};
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
        BASTCodec(codec, ext);
        fprintf(stderr, "OK\t%.*s\t%s\n",
                (int)$len(codec), (char *)codec[0], line);
        indexed++;

        // Flush when scratch buffer is large enough
        size_t pending = u64bDataLen(entries);
        if (pending >= CAPO_FLUSH_AT) {
            u64s data = {u64bDataHead(entries), u64bIdleHead(entries)};
            $sort(data, u64cmp);
            u64cs run = {(u64cp)data[0], (u64cp)data[1]};
            call(CAPOIndexWrite, dirslice, run, seqno++);
            total_entries += pending;
            fprintf(stderr, "capo: flushed %zu entries\n", pending);
            u64bReset(entries);
        }
    }
    pclose(fp);

    // Flush remaining entries
    size_t pending = u64bDataLen(entries);
    if (pending > 0) {
        u64s data = {u64bDataHead(entries), u64bIdleHead(entries)};
        $sort(data, u64cmp);
        u64cs run = {(u64cp)data[0], (u64cp)data[1]};
        call(CAPOIndexWrite, dirslice, run, seqno++);
        total_entries += pending;
    }

    fprintf(stderr, "capo: indexed %u files, %zu entries, skipped %u, failed %u\n",
            indexed, total_entries, skipped, failed);

    if (seqno > 2) {
        fprintf(stderr, "capo: compacting %llu runs\n",
                (unsigned long long)(seqno - 1));
        call(CAPOCompact, dirslice);
    }

    done;
}

ok64 CAPOReindex(u8csc reporoot) {
    sane($ok(reporoot));

    fprintf(stderr, "capo: repo root %.*s\n",
            (int)$len(reporoot), (char *)reporoot[0]);

    a_pad(u8, capodir, FILE_PATH_MAX_LEN);
    call(CAPOResolveDir, capodir, reporoot);
    u8cs dirslice = {u8bDataHead(capodir), u8bIdleHead(capodir)};
    vcall("mkdir", FILEMakeDirP, path8cgIn(capodir));
    fprintf(stderr, "capo: index dir %s\n", (char *)u8bDataHead(capodir));

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
        if (BASTLanguage(ext) == NULL) { skipped++; continue; }

        char fpath[FILE_PATH_MAX_LEN * 2];
        int pn = snprintf(fpath, sizeof(fpath), "%.*s/%s",
                          (int)$len(reporoot), (char *)reporoot[0], line);
        if (pn <= 0 || pn >= (int)sizeof(fpath)) { skipped++; continue; }

        a_pad(u8, fpbuf, FILE_PATH_MAX_LEN);
        u8cs fps = {(u8cp)fpath, (u8cp)fpath + pn};
        call(path8bFeedS, fpbuf, fps);

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, path8cgIn(fpbuf));
        if (o != OK) {
            fprintf(stderr, "FAIL\t%s\t%s\t(open %s)\n",
                    ok64str(o), line, fpath);
            failed++;
            continue;
        }

        u8cs source = {u8bDataHead(mapped), u8bIdleHead(mapped)};
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
        BASTCodec(codec, ext);
        fprintf(stderr, "OK\t%.*s\t%s\t(%zu entries)\n",
                (int)$len(codec), (char*)codec[0], line, u64bDataLen(entries));
        indexed++;

        // Flush when large enough
        size_t pending = u64bDataLen(entries);
        if (pending >= CAPO_FLUSH_AT) {
            fprintf(stderr, "capo[%u/%u]: flushing %zu entries\n",
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

    // Flush remaining
    size_t pending = u64bDataLen(entries);
    if (pending > 0) {
        u64s data = {u64bDataHead(entries), u64bIdleHead(entries)};
        $sort(data, u64cmp);
        u64 seqno = (u64)nprocs * batch + proc + 1;
        u64cs run = {(u64cp)data[0], (u64cp)data[1]};
        call(CAPOIndexWrite, dirslice, run, seqno);
        total_entries += pending;
    }

    fprintf(stderr, "capo[%u/%u]: indexed %u files, %zu entries, skipped %u, failed %u\n",
            proc, nprocs, indexed, total_entries, skipped, failed);
    done;
}

ok64 CAPOReindexProc(u8csc reporoot, u32 nprocs, u32 proc) {
    sane($ok(reporoot) && proc < nprocs && nprocs > 0);

    fprintf(stderr, "capo[%u/%u]: starting\n", proc, nprocs);

    a_pad(u8, capodir, FILE_PATH_MAX_LEN);
    call(CAPOResolveDir, capodir, reporoot);
    u8cs dirslice = {u8bDataHead(capodir), u8bIdleHead(capodir)};
    vcall("mkdir", FILEMakeDirP, path8cgIn(capodir));

    Bu64 entries = {};
    vcall("mmap scratch", u64bMap, entries, CAPO_SCRATCH_LEN);

    ok64 o = CAPOReindexProcWork(reporoot, dirslice, entries, nprocs, proc);

    u64bUnMap(entries);
    return o;
}

// --- Compact all into one ---

ok64 CAPOCompactAll(u8csc dir) {
    sane($ok(dir));

    // Repeatedly compact until only 1 run remains
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

        // Merge ALL runs into one via mmap scratch
        size_t total = 0;
        for (u32 i = 0; i < nfiles; i++) total += $len(runs[i]);
        fprintf(stderr, "capo: merging %u runs, %zu total entries\n",
                nfiles, total);

        Bu64 mbuf = {};
        call(u64bMap, mbuf, total);
        u64s into = {u64bIdleHead(mbuf), mbuf[3]};

        MSETu64Start(stack);
        size_t merged_count = 0;
        while (!$empty(stack)) {
            if (into[0] >= into[1]) {
                fprintf(stderr, "capo: merge buffer full at %zu\n", merged_count);
                break;
            }
            *into[0] = ****stack;
            into[0]++;
            merged_count++;
            MSETu64Next(stack);
        }

        // Find next seqno — must be higher than any existing
        u64 seqno = 0;
        call(CAPONextSeqno, &seqno, dir);
        fprintf(stderr, "capo: next seqno = %llu (dir = '%.*s')\n",
                (unsigned long long)seqno,
                (int)$len(dir), (char *)dir[0]);
        u64cs merged = {(u64cp)mbuf[0], (u64cp)into[0]};
        fprintf(stderr, "capo: writing %zu deduplicated entries (seqno %llu)\n",
                $len(merged), (unsigned long long)seqno);
        call(CAPOIndexWrite, dir, merged, seqno);

        CAPOStackClose(mmaps, nfiles);
        u64bUnMap(mbuf);

        // Unlink old files (re-scan dir, skip the new one)
        a_pad(u8, dpat, FILE_PATH_MAX_LEN);
        call(path8bFeedS, dpat, dir);
        int dfd = -1;
        ok64 o = FILEOpenDir(&dfd, path8cgIn(dpat));
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

                // Sort, unlink all except the last (highest seqno = merged)
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
                    call(path8bFeedS, ulpath, dir);
                    u8cs ulfn = {(u8cp)fnames[i],
                                 (u8cp)fnames[i] + strlen(fnames[i])};
                    call(path8bPush, ulpath, ulfn);
                    unlink((char *)u8bDataHead(ulpath));
                }
            }
        }

        break;  // one pass is enough since we merged all
    }
    done;
}

// --- Commit tracking ---

ok64 CAPOCommitWrite(u8csc reporoot, u8csc capodir) {
    sane($ok(reporoot) && $ok(capodir));

    char cmdbuf[FILE_PATH_MAX_LEN + 64];
    int n = snprintf(cmdbuf, sizeof(cmdbuf),
                     "git -C %.*s rev-parse HEAD",
                     (int)$len(reporoot), (char *)reporoot[0]);
    test(n > 0 && n < (int)sizeof(cmdbuf), FAILSANITY);

    FILE *fp = popen(cmdbuf, "r");
    test(fp != NULL, FAILSANITY);

    char sha[64];
    char *got = fgets(sha, sizeof(sha), fp);
    pclose(fp);
    test(got != NULL, FAILSANITY);

    size_t slen = strlen(sha);
    if (slen > 0 && sha[slen - 1] == '\n') sha[--slen] = 0;
    test(slen >= 40, FAILSANITY);

    a_pad(u8, path, FILE_PATH_MAX_LEN);
    call(u8bFeed, path, capodir);
    a_cstr(commit_name, "/COMMIT");
    call(u8bFeed, path, commit_name);
    call(path8gTerm, path8gIn(path));

    int fd = -1;
    call(FILECreate, &fd, path8cgIn(path));
    u8cs data = {(u8cp)sha, (u8cp)sha + 40};
    call(FILEFeedall, fd, data);
    u8cs nl = {(u8cp)"\n", (u8cp)"\n" + 1};
    call(FILEFeedall, fd, nl);
    close(fd);
    done;
}

ok64 CAPOCommitRead(u32p len, u8csc capodir, u8s buf) {
    sane(len != NULL && $ok(capodir) && $ok(buf));
    *len = 0;

    a_pad(u8, path, FILE_PATH_MAX_LEN);
    call(u8bFeed, path, capodir);
    a_cstr(commit_name, "/COMMIT");
    call(u8bFeed, path, commit_name);
    call(path8gTerm, path8gIn(path));

    u8bp mapped = NULL;
    ok64 o = FILEMapRO(&mapped, path8cgIn(path));
    if (o != OK) done;  // file doesn't exist yet

    u8cs content = {u8bDataHead(mapped), u8bIdleHead(mapped)};
    size_t clen = $len(content);
    if (clen > 40) clen = 40;
    size_t cap = (size_t)$len(buf);
    if (clen > cap) clen = cap;
    if (clen > 0) {
        memcpy(buf[0], content[0], clen);
        *len = (u32)clen;
    }
    FILEUnMap(mapped);
    done;
}

// --- Hook (incremental) ---

// Check saved commit, return YES if incremental diff is possible.
// On YES, cmdbuf contains the diff command. On NO, caller should reindex.
static b8 CAPOHookDiffCmd(char *cmdbuf, size_t cmdsz,
                           u8csc reporoot, u8csc dirslice) {
    char saved_sha[44];
    u8s shabuf = {(u8cp)saved_sha, (u8cp)saved_sha + 40};
    u32 sha_len = 0;
    CAPOCommitRead(&sha_len, dirslice, shabuf);

    if (sha_len != 40) {
        fprintf(stderr, "capo: no saved commit, full reindex\n");
        return NO;
    }
    saved_sha[40] = 0;

    // Check if saved commit is ancestor of HEAD
    char chkbuf[FILE_PATH_MAX_LEN + 128];
    int n = snprintf(chkbuf, sizeof(chkbuf),
                     "git -C %.*s merge-base --is-ancestor %.40s HEAD",
                     (int)$len(reporoot), (char *)reporoot[0], saved_sha);
    if (n <= 0 || n >= (int)sizeof(chkbuf)) return NO;
    int rc = system(chkbuf);
    if (!WIFEXITED(rc) || WEXITSTATUS(rc) != 0) {
        fprintf(stderr, "capo: saved commit unreachable, full reindex\n");
        return NO;
    }

    // Ancestor check passed: diff against saved
    if (CAPO_COLOR)
        fprintf(stderr, "\033[%dmChanges since %.40s\033[0m\n", GRAY, saved_sha);
    else
        fprintf(stderr, "Changes since %.40s\n", saved_sha);
    n = snprintf(cmdbuf, cmdsz,
                 "git -C %.*s diff --name-only %.40s HEAD",
                 (int)$len(reporoot), (char *)reporoot[0], saved_sha);
    if (n <= 0 || n >= (int)cmdsz) return NO;
    return YES;
}

static ok64 CAPOHookDiff(u8csc reporoot, u8csc dirslice,
                          u64bp entries, const char *cmdbuf) {
    sane($ok(reporoot) && $ok(dirslice) && entries != NULL);

    FILE *fp = popen(cmdbuf, "r");
    test(fp != NULL, FAILSANITY);

    char line[FILE_PATH_MAX_LEN];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = 0;
        if (len == 0) continue;

        u8cs ext = {};
        CAPOFindExt(ext, line, len);
        if ($empty(ext)) continue;
        if (BASTLanguage(ext) == NULL) continue;

        char fpath[FILE_PATH_MAX_LEN * 2];
        int pn = snprintf(fpath, sizeof(fpath), "%.*s/%s",
                          (int)$len(reporoot), (char *)reporoot[0], line);
        if (pn <= 0 || pn >= (int)sizeof(fpath)) continue;

        a_pad(u8, fpbuf, FILE_PATH_MAX_LEN);
        u8cs fps = {(u8cp)fpath, (u8cp)fpath + pn};
        call(path8bFeedS, fpbuf, fps);

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, path8cgIn(fpbuf));
        if (o != OK) continue;

        u8cs source = {u8bDataHead(mapped), u8bIdleHead(mapped)};
        u8cs relpath = {(u8cp)line, (u8cp)line + len};
        o = CAPOIndexFile(entries, source, ext, relpath);
        FILEUnMap(mapped);
        if (o != OK) continue;
    }
    pclose(fp);

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
    u8cs dirslice = {u8bDataHead(capodir), u8bIdleHead(capodir)};
    call(FILEMakeDirP, path8cgIn(capodir));

    char cmdbuf[FILE_PATH_MAX_LEN + 128];
    ok64 o = OK;

    Bu64 entries = {};
    call(u64bMap, entries, CAPO_SCRATCH_LEN);

    if (CAPOHookDiffCmd(cmdbuf, sizeof(cmdbuf), reporoot, dirslice)) {
        // Incremental: only diff'd files
        o = CAPOHookDiff(reporoot, dirslice, entries, cmdbuf);
    } else {
        // No saved commit or unreachable: full reindex
        o = CAPOReindexWork(reporoot, dirslice, entries);
    }

    u64bUnMap(entries);

    if (o == OK) CAPOCommitWrite(reporoot, dirslice);
    return o;
}

// --- Query ---

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

// Progress: print filename in grey on stderr, overwriting previous.
// Call with NULL line to erase.
static void CAPOProgress(const char *line) {
    if (!CAPO_TERM) return;
    fprintf(stderr, "\r\033[K");
    if (line != NULL)
        fprintf(stderr, "\033[%dm%s\033[0m", GRAY, line);
}

ok64 CAPOQuery(u8csc selector, u8csc ext, u8csc reporoot) {
    sane($ok(selector) && $ok(reporoot));

    const struct TSLanguage *target_lang = NULL;
    if (!$empty(ext)) target_lang = BASTLanguage(ext);

    a_pad(u8, capodir, FILE_PATH_MAX_LEN);
    call(CAPOResolveDir, capodir, reporoot);
    u8cs dirslice = {u8bDataHead(capodir), u8bIdleHead(capodir)};

    u32 maxhashes = 64 * 1024;
    u32 *hashbuf1 = (u32 *)malloc(maxhashes * sizeof(u32));
    u32 *hashbuf2 = (u32 *)malloc(maxhashes * sizeof(u32));
    test(hashbuf1 != NULL && hashbuf2 != NULL, FAILSANITY);

    // Load MSET stack
    u64cs runs[CAPO_MAX_LEVELS] = {};
    u64css stack = {runs, runs};
    u8bp mmaps[CAPO_MAX_LEVELS] = {};
    u32 nfiles = 0;
    call(CAPOStackOpen, stack, mmaps, &nfiles, dirslice);
    stack[1] = stack[0] + nfiles;

    // Extract searchable text spans from selector:
    //   .name     -> "name"
    //   :has(text) -> "text"
    // Then generate trigrams only from those spans.
    u8cp spans[32][2];
    u32 nspans = 0;
    {
        u8cp p = selector[0];
        u8cp e = selector[1];
        while (p < e && nspans < 32) {
            if (*p == '.' && p + 1 < e) {
                // .name — collect until non-RON64
                u8cp start = p + 1;
                u8cp q = start;
                while (q < e && CAPOTriChar(*q)) q++;
                if (q > start) {
                    spans[nspans][0] = start;
                    spans[nspans][1] = q;
                    nspans++;
                    p = q;
                    continue;
                }
            } else if (*p == ':' && p + 4 < e && memcmp(p, ":has(", 5) == 0) {
                // :has(text) — collect between parens
                u8cp start = p + 5;
                u8cp q = start;
                while (q < e && *q != ')') q++;
                if (q > start) {
                    spans[nspans][0] = start;
                    spans[nspans][1] = q;
                    nspans++;
                    p = (q < e) ? q + 1 : q;
                    continue;
                }
            }
            p++;
        }
    }

    // Generate trigrams from extracted spans, intersect path hashes
    u32 nhashes = 0;
    b8 has_trigrams = NO;

    if (nfiles > 0) {
        for (u32 si = 0; si < nspans; si++) {
            u8cp p = spans[si][0];
            u8cp end = spans[si][1] - 2;
            while (p <= end) {
                if (CAPOTriChar(p[0]) && CAPOTriChar(p[1]) && CAPOTriChar(p[2])) {
                    u8cs tri = {p, p + 3};
                    u64 tri_prefix = CAPOTriPack(tri);

                    u64cs seek_runs[CAPO_MAX_LEVELS];
                    for (u32 i = 0; i < nfiles; i++) {
                        seek_runs[i][0] = runs[i][0];
                        seek_runs[i][1] = runs[i][1];
                    }
                    u64css seek_iter = {seek_runs, seek_runs + nfiles};
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
    }

    CAPOStackClose(mmaps, nfiles);

    // Parse CSS selector
    aBpad(u8, qbuf, 4096);
    aBpad(u64, qidx, 256);
    u8cs sel_mut = {selector[0], selector[1]};
    call(CSSParse, qbuf, qidx, sel_mut);
    u8cs query = {u8bDataHead(qbuf), u8bIdleHead(qbuf)};

    // Sort path hashes for binary search
    if (has_trigrams && nhashes > 0)
        qsort(hashbuf1, nhashes, sizeof(u32), CAPOu32cmp);

    // Pager for terminal output (only if $PAGER is set)
    FILE *pager = NULL;
    int saved_stdout = -1;
    char const *pgcmd = getenv("PAGER");
    if (pgcmd != NULL && *pgcmd && CAPO_COLOR) {
        pager = popen(pgcmd, "w");
        if (pager != NULL) {
            saved_stdout = dup(STDOUT_FILENO);
            dup2(fileno(pager), STDOUT_FILENO);
        }
    }

    // Get file list, filter, parse, match, output
    char cmdbuf[FILE_PATH_MAX_LEN + 32];
    int cn = snprintf(cmdbuf, sizeof(cmdbuf), "git -C %.*s ls-files",
                      (int)$len(reporoot), (char *)reporoot[0]);
    test(cn > 0 && cn < (int)sizeof(cmdbuf), FAILSANITY);

    FILE *fp = popen(cmdbuf, "r");
    test(fp != NULL, FAILSANITY);

    char line[FILE_PATH_MAX_LEN];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = 0;
        if (len == 0) continue;

        u8cs relpath = {(u8cp)line, (u8cp)line + len};

        // Filter by trigram intersection
        if (has_trigrams && nhashes > 0) {
            u32 phash = CAPOPathHash(relpath);
            if (!bsearch(&phash, hashbuf1, nhashes, sizeof(u32), CAPOu32cmp))
                continue;
        } else if (has_trigrams) {
            continue;  // trigrams found but no matches
        }

        u8cs file_ext = {};
        CAPOFindExt(file_ext, line, len);
        if ($empty(file_ext)) continue;
        if (BASTLanguage(file_ext) == NULL) continue;
        if (target_lang && BASTLanguage(file_ext) != target_lang) continue;

        CAPOProgress(line);

        char fpath[FILE_PATH_MAX_LEN * 2];
        int pn = snprintf(fpath, sizeof(fpath), "%.*s/%s",
                          (int)$len(reporoot), (char *)reporoot[0], line);
        if (pn <= 0 || pn >= (int)sizeof(fpath)) continue;

        a_pad(u8, fpbuf, FILE_PATH_MAX_LEN);
        u8cs fps = {(u8cp)fpath, (u8cp)fpath + pn};
        call(path8bFeedS, fpbuf, fps);

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, path8cgIn(fpbuf));
        if (o != OK) continue;

        u8cs source = {u8bDataHead(mapped), u8bIdleHead(mapped)};
        size_t buflen = $len(source) * 16;
        if (buflen < 1024 * 1024) buflen = 1024 * 1024;
        Bu8 bson = {};
        o = u8bMap(bson, buflen);
        if (o != OK) { FILEUnMap(mapped); continue; }
        size_t idxlen = buflen / BASON_PAGE + 256;
        u64 *_bidx = (u64 *)malloc(idxlen * sizeof(u64));
        if (!_bidx) { u8bUnMap(bson); FILEUnMap(mapped); continue; }
        Bu64 bidx = {_bidx, _bidx, _bidx, _bidx + idxlen};

        o = BASTParse(bson, bidx, source, file_ext);
        if (o != OK) {
            free(_bidx);
            u8bUnMap(bson);
            FILEUnMap(mapped);
            continue;
        }

        u8cs bdata = {u8bDataHead(bson), u8bIdleHead(bson)};

        size_t fbuflen = $len(bdata) + 4096;
        Bu8 fbufm = {};
        o = u8bMap(fbufm, fbuflen);
        if (o != OK) { free(_bidx); u8bUnMap(bson); FILEUnMap(mapped); continue; }

        // Guard CSSMatch against assert crashes
        signal(SIGABRT, capo_abrt_handler);
        capo_in_match = 1;
        if (sigsetjmp(capo_jmpbuf, 1) != 0) {
            capo_in_match = 0;
            signal(SIGABRT, SIG_DFL);
            u8bUnMap(fbufm);
            free(_bidx);
            u8bUnMap(bson);
            FILEUnMap(mapped);
            continue;
        }
        o = CSSMatch(fbufm, bdata, query);
        capo_in_match = 0;
        signal(SIGABRT, SIG_DFL);
        if (o == OK && fbufm[1] < fbufm[2]) {
            CAPOProgress(NULL);
            u8cs filtered = {u8bDataHead(fbufm), u8bIdleHead(fbufm)};
            size_t obuflen = $len(filtered) * 4 + 4096;
            Bu8 obufm = {};
            o = u8bMap(obufm, obuflen);
            if (o == OK) {
                u8s out = {u8bIdleHead(obufm), obufm[3]};
                // Path header
                if (CAPO_COLOR) escfeed(out, GRAY);
                a_cstr(hdr_pre, "--- ");
                u8sFeed(out, hdr_pre);
                u8sFeed(out, relpath);
                a_cstr(hdr_post, " ---\n");
                u8sFeed(out, hdr_post);
                if (CAPO_COLOR) escfeed(out, 0);
                if (CAPO_COLOR)
                    o = CSSCat(out, filtered, relpath);
                else
                    o = CSSExport(out, filtered);
                if (o == OK) {
                    u8cs result = {u8bIdleHead(obufm), out[0]};
                    call(FILEFeedall, STDOUT_FILENO, result);
                }
                u8bUnMap(obufm);
            }
        }
        u8bUnMap(fbufm);

        free(_bidx);
        u8bUnMap(bson);
        FILEUnMap(mapped);
    }
    pclose(fp);
    CAPOProgress(NULL);

    if (pager != NULL) {
        fflush(stdout);
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
        pclose(pager);
    }

    free(hashbuf1);
    free(hashbuf2);
    done;
}

// --- SPOT search ---

// Copy one BASON element (leaf or container subtree) to output buffer
static ok64 CAPOCopyElement(u8bp out, u8 type, u8cs key, u8cs val,
                             u64bp stk, u8cs data) {
    sane(out != NULL);
    if (BASONCollection(type)) {
        call(BASONFeedInto, NULL, out, type, key);
        call(BASONInto, stk, data, val);
        u8 ct = 0;
        u8cs ck = {}, cv = {};
        while (BASONDrain(stk, data, &ct, ck, cv) == OK) {
            call(CAPOCopyElement, out, ct, ck, cv, stk, data);
        }
        call(BASONOuto, stk);
        call(BASONFeedOuto, NULL, out);
    } else {
        call(BASONFeed, NULL, out, type, key, val);
    }
    done;
}

// Feed BASON leaves from bdata in source range [slo, shi) into fbufm
// with their original BAST types (for syntax highlighting in CSSCat).
static ok64 CAPOFeedRange(u8bp fbufm, u8csc bdata, u32 slo, u32 shi) {
    sane(fbufm != NULL);
    aBpad(u64, stk, 256);
    call(BASONOpen, stk, bdata);
    int depth = 0;
    u64 src_pos = 0;
    u8 type = 0;
    u8cs key = {}, val = {};
    for (;;) {
        ok64 o = BASONDrain(stk, bdata, &type, key, val);
        if (o != OK) {
            if (depth <= 0) break;
            BASONOuto(stk);
            depth--;
            continue;
        }
        if (BASONCollection(type)) {
            depth++;
            call(BASONInto, stk, bdata, val);
            continue;
        }
        size_t vlen = (size_t)$len(val);
        u32 leaf_lo = (u32)src_pos;
        src_pos += vlen;
        u32 leaf_hi = (u32)src_pos;
        if (leaf_hi <= slo) continue;
        if (leaf_lo >= shi) break;
        call(BASONFeed, NULL, fbufm, type, key, val);
    }
    done;
}

ok64 CAPOSpot(u8csc needle, u8csc replace, u8csc ext, u8csc reporoot) {
    sane($ok(needle) && $ok(ext) && $ok(reporoot));

    const struct TSLanguage *target_lang = BASTLanguage(ext);
    if (!target_lang) return SPOTBAD;

    // --- Trigram filtering ---
    a_pad(u8, capodir, FILE_PATH_MAX_LEN);
    call(CAPOResolveDir, capodir, reporoot);
    u8cs dirslice = {u8bDataHead(capodir), u8bIdleHead(capodir)};

    u32 maxhashes = 64 * 1024;
    u32 *hashbuf1 = (u32 *)malloc(maxhashes * sizeof(u32));
    u32 *hashbuf2 = (u32 *)malloc(maxhashes * sizeof(u32));
    test(hashbuf1 != NULL && hashbuf2 != NULL, FAILSANITY);

    u64cs runs[CAPO_MAX_LEVELS] = {};
    u64css stack = {runs, runs};
    u8bp mmaps[CAPO_MAX_LEVELS] = {};
    u32 nidxfiles = 0;
    call(CAPOStackOpen, stack, mmaps, &nidxfiles, dirslice);
    stack[1] = stack[0] + nidxfiles;

    u32 nhashes = 0;
    b8 has_trigrams = NO;

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

    // Pager for terminal output
    FILE *pager = NULL;
    int saved_stdout = -1;
    char const *pgcmd = getenv("PAGER");
    if (pgcmd != NULL && *pgcmd && CAPO_COLOR) {
        pager = popen(pgcmd, "w");
        if (pager != NULL) {
            saved_stdout = dup(STDOUT_FILENO);
            dup2(fileno(pager), STDOUT_FILENO);
        }
    }

    // Get file list
    char cmdbuf[FILE_PATH_MAX_LEN + 32];
    int cn = snprintf(cmdbuf, sizeof(cmdbuf), "git -C %.*s ls-files",
                      (int)$len(reporoot), (char *)reporoot[0]);
    test(cn > 0 && cn < (int)sizeof(cmdbuf), FAILSANITY);

    FILE *fp = popen(cmdbuf, "r");
    test(fp != NULL, FAILSANITY);

    char line[FILE_PATH_MAX_LEN];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = 0;
        if (len == 0) continue;

        u8cs relpath = {(u8cp)line, (u8cp)line + len};

        // Filter by trigram intersection
        if (has_trigrams && nhashes > 0) {
            u32 phash = CAPOPathHash(relpath);
            if (!bsearch(&phash, hashbuf1, nhashes, sizeof(u32), CAPOu32cmp))
                continue;
        } else if (has_trigrams) {
            continue;
        }

        // Filter by language: file extension must map to same language
        u8cs file_ext = {};
        CAPOFindExt(file_ext, line, len);
        if ($empty(file_ext)) continue;
        if (BASTLanguage(file_ext) != target_lang) continue;

        CAPOProgress(line);

        // Open and parse file
        char fpath[FILE_PATH_MAX_LEN * 2];
        int pn = snprintf(fpath, sizeof(fpath), "%.*s/%s",
                          (int)$len(reporoot), (char *)reporoot[0], line);
        if (pn <= 0 || pn >= (int)sizeof(fpath)) continue;

        a_pad(u8, fpbuf, FILE_PATH_MAX_LEN);
        u8cs fps = {(u8cp)fpath, (u8cp)fpath + pn};
        call(path8bFeedS, fpbuf, fps);

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, path8cgIn(fpbuf));
        if (o != OK) continue;

        u8cs source = {u8bDataHead(mapped), u8bIdleHead(mapped)};
        size_t buflen = $len(source) * 16;
        if (buflen < 1024 * 1024) buflen = 1024 * 1024;
        Bu8 bson = {};
        o = u8bMap(bson, buflen);
        if (o != OK) { FILEUnMap(mapped); continue; }
        size_t idxlen = buflen / BASON_PAGE + 256;
        u64 *_bidx = (u64 *)malloc(idxlen * sizeof(u64));
        if (!_bidx) { u8bUnMap(bson); FILEUnMap(mapped); continue; }
        Bu64 bidx = {_bidx, _bidx, _bidx, _bidx + idxlen};

        o = BASTParse(bson, bidx, source, file_ext);
        if (o != OK) {
            free(_bidx);
            u8bUnMap(bson);
            FILEUnMap(mapped);
            continue;
        }

        u8cs bdata = {u8bDataHead(bson), u8bIdleHead(bson)};

        // SPOT init + match loop
        signal(SIGABRT, capo_abrt_handler);
        capo_in_match = 1;
        if (sigsetjmp(capo_jmpbuf, 1) != 0) {
            capo_in_match = 0;
            signal(SIGABRT, SIG_DFL);
            free(_bidx);
            u8bUnMap(bson);
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
                o = SPOTReplace(rout, source, bdata, needle,
                                replace, file_ext);
                if (o == OK) {
                    u8cs result = {u8bIdleHead(obufm), rout[0]};
                    // Unmap source before writing back
                    FILEUnMap(mapped);
                    mapped = NULL;
                    int fd = -1;
                    ok64 wo = FILECreate(&fd, path8cgIn(fpbuf));
                    if (wo == OK) {
                        FILEFeedall(fd, result);
                        close(fd);
                        CAPOProgress(NULL);
                        fprintf(stderr, "replaced: %s\n", line);
                    }
                }
                u8bUnMap(obufm);
            }
        } else {
            // --- Search/display mode ---
            size_t fbuflen = $len(bdata) + 4096;
            Bu8 fbufm = {};
            o = u8bMap(fbufm, fbuflen);
            if (o != OK) {
                capo_in_match = 0;
                signal(SIGABRT, SIG_DFL);
                free(_bidx);
                u8bUnMap(bson);
                FILEUnMap(mapped);
                continue;
            }

            aBpad(u8, nbuf, 16384);
            aBpad(u64, nidx, 256);
            aBpad(u64, mlog, 1024);
            aBpad(u64, alog, 1024);
            SPOTstate st = {};
            o = SPOTInit(&st, nbuf, nidx, needle, file_ext, bdata);
            if (o == OK) {
                st.mlog[0] = mlog[0]; st.mlog[1] = mlog[1];
                st.mlog[2] = mlog[2]; st.mlog[3] = mlog[3];
                st.alog[0] = alog[0]; st.alog[1] = alog[1];
                st.alog[2] = alog[2]; st.alog[3] = alog[3];

                u32 prev_hi = 0;
                while (SPOTNext(&st) == OK) {
                    // Output entire matched source range
                    u32 slo = st.src_lo;
                    u32 shi = st.src_hi;
                    if (shi <= slo || shi > (u32)$len(source)) continue;
                    if (slo < prev_hi) continue;  // skip overlapping
                    prev_hi = shi;
                    u8cs ekey = {};
                    call(BASONFeedInto, NULL, fbufm, 'A', ekey);
                    call(CAPOFeedRange, fbufm, bdata, slo, shi);
                    call(BASONFeedOuto, NULL, fbufm);
                }
            }

            if (fbufm[1] < fbufm[2]) {
                CAPOProgress(NULL);
                u8cs filtered = {u8bDataHead(fbufm), u8bIdleHead(fbufm)};
                size_t obuflen = $len(filtered) * 4 + 4096;
                Bu8 obufm = {};
                o = u8bMap(obufm, obuflen);
                if (o == OK) {
                    u8s out = {u8bIdleHead(obufm), obufm[3]};
                    if (CAPO_COLOR) escfeed(out, GRAY);
                    a_cstr(hdr_pre, "--- ");
                    u8sFeed(out, hdr_pre);
                    u8sFeed(out, relpath);
                    a_cstr(hdr_post, " ---\n");
                    u8sFeed(out, hdr_post);
                    if (CAPO_COLOR) escfeed(out, 0);
                    if (CAPO_COLOR)
                        o = CSSCat(out, filtered, relpath);
                    else
                        o = CSSExport(out, filtered);
                    a_cstr(trail_nl, "\n");
                    u8sFeed(out, trail_nl);
                    if (o == OK) {
                        u8cs result = {u8bIdleHead(obufm), out[0]};
                        call(FILEFeedall, STDOUT_FILENO, result);
                    }
                    u8bUnMap(obufm);
                }
            }
            u8bUnMap(fbufm);
        }

        free(_bidx);
        u8bUnMap(bson);
        FILEUnMap(mapped);
    }
    pclose(fp);
    CAPOProgress(NULL);

    if (pager != NULL) {
        fflush(stdout);
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
        pclose(pager);
    }

    free(hashbuf1);
    free(hashbuf2);
    done;
}
