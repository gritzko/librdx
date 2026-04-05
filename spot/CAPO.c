#include "CAPOi.h"

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
#include "abc/PRO.h"
#include "abc/SORT.h"
#include "spot/SPOT.h"
#include "tok/DEF.h"

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

    // Mark definitions (S→N) before extraction
    {
        u32 *dts[2] = {u32bDataHead(toks), u32bIdleHead(toks)};
        u8cs nodot = {};
        if (!$empty(ext) && ext[0][0] == '.') {
            nodot[0] = ext[0] + 1; nodot[1] = ext[1];
        } else {
            nodot[0] = ext[0]; nodot[1] = ext[1];
        }
        DEFMark(dts, source, nodot);
    }

    u32cp td = u32bDataHead(toks);
    u32cp ti = u32bIdleHead(toks);
    u32cs tokslice = {(u32cp)td, (u32cp)ti};

    u32 phash = CAPOPathHash(path);
    CAPOTriCtx ctx = {
        .idle = u64bIdle(entries),
        .end = entries[3],
        .path_hash = phash,
    };
    o = CAPOTriExtractToks(tokslice, source[0], CAPOTriCB, &ctx);
    if (o != OK) { u32bUnMap(toks); return o; }

    // Emit symbol mention/definition entries
    int ntoks = (int)$len(tokslice);
    for (int i = 0; i < ntoks; i++) {
        u8 tag = tok32Tag(tokslice[0][i]);
        if (tag != 'S' && tag != 'N' && tag != 'C') continue;
        u8cs val = {}; tok32Val(val, tokslice, source[0], i);
        if ($len(val) < 2) continue;
        if (*ctx.idle >= ctx.end) break;
        u64 type = (tag == 'N') ? IDX64_DEF : IDX64_MEN;
        u64 entry = (type << 62) | CAPOSymKey(val) | (u64)phash;
        *(*ctx.idle) = entry;
        (*ctx.idle)++;
    }

    u32bUnMap(toks);
    done;
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

    // Check 1/8 LSM invariant
    b8 is_compact = YES;
    for (u32 i = 0; i + 1 < nfiles; i++) {
        if ($len(runs[i + 1]) * 8 > $len(runs[i])) { is_compact = NO; break; }
    }
    if (nfiles < 2 || is_compact) {
        CAPOStackClose(mmaps, nfiles);
        done;
    }

    size_t total = 0;
    for (u32 i = 0; i < nfiles; i++) total += $len(runs[i]);

    Bu64 cbuf = {};
    call(u64bAlloc, cbuf, total);
    u64s into = {cbuf[0], cbuf[3]};

    size_t n = $len(stack);
    size_t m = 1;
    size_t mtotal = $len(stack[0][n - 1]);
    while (m < n && mtotal * 8 > $len(stack[0][n - 1 - m])) {
        mtotal += $len(stack[0][n - 1 - m]);
        m++;
    }
    if (m < 2) {
        u64bFree(cbuf);
        CAPOStackClose(mmaps, nfiles);
        done;
    }

    // Merge youngest m runs using HIT
    {
        u64css sub = {stack[0] + (n - m), stack[0] + n};
        HITu64csStartZ(sub, u64csHeadZ);
        u64 prev = 0;
        b8 first = YES;
        while (!$empty(sub)) {
            if ($empty(into)) break;
            u64 entry = *(*sub[0])[0];
            if (first || entry != prev) {
                *into[0] = entry;
                ++into[0];
                prev = entry;
                first = NO;
            }
            CAPOHITStep(sub, u64csHeadZ);
        }
    }

    u64 seqno = 0;
    call(CAPONextSeqno, &seqno, dir);
    u64cs merged = {(u64cp)cbuf[0], (u64cp)(into[0])};
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

    u64bFree(cbuf);
    done;
}

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

        HITu64csStartZ(stack, u64csHeadZ);
        size_t merged_count = 0;
        u64 prev = 0;
        b8 first = YES;
        while (!$empty(stack)) {
            if (into[0] >= into[1]) {
                fprintf(stderr, "spot: merge buffer full at %zu\n", merged_count);
                break;
            }
            u64 entry = *(*stack[0])[0];
            if (first || entry != prev) {
                *into[0] = entry;
                into[0]++;
                merged_count++;
                prev = entry;
                first = NO;
            }
            CAPOHITStep(stack, u64csHeadZ);
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

ok64 CAPOCollectPaths(u64css iter, u64 tri_prefix, u32g hashes) {
    ok64 o = CAPOHITSeek(iter, tri_prefix, u64csHeadZ);
    if (o != OK) return OK;

    while (!$empty(iter)) {
        u64 entry = *(*iter[0])[0];
        if (CAPOTriOf(entry) != tri_prefix) break;
        u32gFeed1(hashes, (u32)entry);
        CAPOHITStep(iter, u64csHeadZ);
    }
    return OK;
}

u32 CAPOIntersect(u32s a, u32csc b) {
    u32 na = (u32)$len(a), nb = (u32)$len(b);
    u32 i = 0, j = 0, k = 0;
    while (i < na && j < nb) {
        if (a[0][i] < b[0][j]) i++;
        else if (a[0][i] > b[0][j]) j++;
        else { a[0][k++] = a[0][i]; i++; j++; }
    }
    return k;
}

int CAPOu32cmp(const void *a, const void *b) {
    u32 x = *(const u32 *)a, y = *(const u32 *)b;
    return (x > y) - (x < y);
}

void CAPOProgress(const char *line) {
    if (!CAPO_TERM) return;
    fprintf(stderr, "\r\033[K");
    if (line != NULL)
        fprintf(stderr, "\033[%dm%s\033[0m", GRAY, line);
}

// --- Function name finder (heuristic) ---

// Check if ext (with dot) matches one of the given suffixes.
b8 CAPOExtIs(u8csc ext, const char *a, const char *b) {
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
void CAPOFindFunc(u8csc source, u32 pos, u8csc ext,
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
ok64 CAPOFormatTitle(u8s into,
                     const char *filepath, const char *funcname) {
    sane(into[0] != NULL);
    u8p start = into[0];
    if (filepath && funcname && funcname[0]) {
        call(u8sPrintf, into, "--- %s :: %s ---", filepath, funcname);
    } else if (filepath) {
        call(u8sPrintf, into, "--- %s ---", filepath);
    } else if (funcname && funcname[0]) {
        call(u8sPrintf, into, "--- %s ---", funcname);
    } else {
        done;
    }

    int hlen = (int)(into[0] - start);
    if (hlen > HUNK_MAX && filepath && funcname && funcname[0]) {
        size_t plen = strlen(filepath);
        int budget = HUNK_MAX - 12 - (int)plen;
        if (budget < 1) budget = 1;
        into[0] = start;
        call(u8sPrintf, into, "--- %s :: %.*s ---",
             filepath, budget, funcname);
        hlen = (int)(into[0] - start);
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
        into[0] = start;
        if (funcname && funcname[0]) {
            int favail = HUNK_MAX - 12 - 3 -
                         (int)(filepath + strlen(filepath) - p);
            if (favail < 1) favail = 1;
            call(u8sPrintf, into, "--- ...%s :: %.*s ---",
                 p, favail, funcname);
        } else {
            call(u8sPrintf, into, "--- ...%s ---", p);
        }
    }
    done;
}

// --- Hunk building ---

ok64 CAPOBuildHunk(u8csc source, u32cs htoks, u32 ctx_lo, u32 ctx_hi,
                   range32 const *hls, int nhl,
                   u8csc file_ext, const char *filepath,
                   b8 needs_title, b8 *first_hunk) {
    sane(less_nhunks < LESS_MAX_HUNKS);
    LESShunk *hk = &less_hunks[less_nhunks];
    *hk = (LESShunk){};

    // Title
    if (needs_title || *first_hunk) {
        char funcname[256];
        CAPOFindFunc(source, ctx_lo, file_ext,
                     funcname, sizeof(funcname));
        u8gp g = u8aOpen(less_arena);
        call(CAPOFormatTitle, u8gRest(g), filepath, funcname);
        u8cs title = {};
        u8aClose(less_arena, title);
        if (!$empty(title)) {
            hk->title[0] = title[0];
            hk->title[1] = title[1];
        }
    }

    hk->text[0] = source[0] + ctx_lo;
    hk->text[1] = source[0] + ctx_hi;

    // Clip file-level toks to context region
    LESSClipToks(hk->toks, htoks, source[0], ctx_lo, ctx_hi);

    // Build sparse hili from match ranges
    if (CAPO_COLOR) {
        u32gp g = u32aOpen(less_arena);
        u32 prev_end = 0;
        for (int i = 0; i < nhl; i++) {
            u32 mlo = hls[i].lo < ctx_lo
                          ? 0
                          : hls[i].lo - ctx_lo;
            u32 mhi = hls[i].hi > ctx_hi
                          ? ctx_hi - ctx_lo
                          : hls[i].hi - ctx_lo;
            if (mlo > prev_end)
                u32gFeed1(g, tok32Pack('A', mlo));
            u32gFeed1(g, tok32Pack('I', mhi));
            prev_end = mhi;
        }
        u32 region_len = ctx_hi - ctx_lo;
        if (prev_end < region_len)
            u32gFeed1(g, tok32Pack('A', region_len));
        u32cs hili = {};
        u32aClose(less_arena, hili);
        if (!$empty(hili)) {
            hk->hili[0] = hili[0];
            hk->hili[1] = hili[1];
        }
    }

    LESSHunkEmit();
    *first_hunk = NO;
    done;
}

// --- Per-file replacement ---

static ok64 CAPOSpotReplace(u8csc source, u8bp mapped, u32cs htoks,
                             u8csc needle, u8csc replace, u8csc file_ext,
                             u8bp fpbuf, const char *line,
                             int *total_replacements,
                             int *total_files_replaced) {
    sane(!$empty(replace));
    size_t obuflen = $len(source) * 2 + 4096;
    Bu8 obufm = {};
    ok64 o = u8bMap(obufm, obuflen);
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
                *total_replacements += file_matches;
                (*total_files_replaced)++;
            }
        }
        u8bUnMap(obufm);
    }
    done;
}

// --- Per-file search+display ---

static ok64 CAPOSpotFile(u8csc source, u32cs htoks, u8csc needle,
                          u8csc file_ext, const char *line) {
    sane(!$empty(needle));
    aBpad(u32, nbuf, 4096);
    SPOTstate st = {};
    ok64 o = SPOTInit(&st, nbuf, needle, file_ext, htoks, source);
    if (o != OK) done;
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
        if (shi > slo) {
            u32 lo2 = 0, hi2 = 0;
            CAPOGrepCtx(source, shi - 1, 3, &lo2, &hi2);
            if (hi2 > ctx_hi) ctx_hi = hi2;
        }

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
        if (ctx_lo < ctx_hi) {
            call(CAPOBuildHunk, source, htoks, ctx_lo, ctx_hi,
                 hls, nhl, file_ext, line,
                 !contiguous, &first_hunk);
        }
        prev_ctx_hi = ctx_hi;
    }
    done;
}

// --- SPOT search ---

ok64 CAPOSpot(u8csc needle, u8csc replace, u8csc ext, u8csc reporoot,
              u8css files) {
    sane($ok(needle) && $ok(ext) && $ok(reporoot));
    int nfiles = (int)$len(files);

    if (!CAPOKnownExt(ext)) return SPOTBAD;

    // --- Trigram filtering (skip when explicit files given) ---
    u32 maxhashes = 64 * 1024;
    Bu32 hashbuf1 = {};
    Bu32 hashbuf2 = {};
    b8 has_trigrams = NO;

    if (nfiles == 0) {
        a_pad(u8, capodir, FILE_PATH_MAX_LEN);
        call(CAPOResolveDir, capodir, reporoot);
        a_dup(u8c, dirslice, u8bDataC(capodir));

        call(u32bAlloc, hashbuf1, maxhashes);
        call(u32bAlloc, hashbuf2, maxhashes);

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
                    HITu64csStartZ(seek_iter, u64csHeadZ);

                    u32bReset(hashbuf2);
                    CAPOCollectPaths(seek_iter, tri_prefix,
                                     u32bDataIdle(hashbuf2));

                    if (!has_trigrams) {
                        u32bReset(hashbuf1);
                        u32bFeed(hashbuf1, u32bDataC(hashbuf2));
                        has_trigrams = YES;
                    } else {
                        u32sSort(u32bData(hashbuf2));
                        u32sSort(u32bData(hashbuf1));
                        u32 n = CAPOIntersect(
                            u32bData(hashbuf1), u32bDataC(hashbuf2));
                        u32bShed(hashbuf1, u32bDataLen(hashbuf1) - n);
                    }
                }
                p++;
            }
        }

        // Symbol filtering: intersect with identifier mentions
        if (nidxfiles > 0 && has_trigrams && u32bDataLen(hashbuf1) > CAPO_SYM_THRESH) {
            Bu32 ndl_toks = {};
            size_t ndl_maxlen = $len(needle) + 1;
            if (u32bMap(ndl_toks, ndl_maxlen) == OK) {
                if (SPOTTokenize(ndl_toks, needle, ext) == OK) {
                    u32cp ntd = u32bDataHead(ndl_toks);
                    u32cp nti = u32bIdleHead(ndl_toks);
                    u32cs nts = {(u32cp)ntd, (u32cp)nti};
                    int nn = (int)$len(nts);
                    for (int i = 0; i < nn && u32bDataLen(hashbuf1) > 0; i++) {
                        u8 tag = tok32Tag(nts[0][i]);
                        if (tag != 'S') continue;
                        u8cs val = {};
                        tok32Val(val, nts, needle[0], i);
                        if ($len(val) < 2) continue;
                        u64 symkey = CAPOSymKey(val);

                        // Collect both mentions (S) and definitions (N)
                        u32bReset(hashbuf2);
                        for (u64 t = IDX64_MEN; t <= IDX64_DEF; t++) {
                            u64 pfx = (t << 62) | symkey;
                            u64cs sr[CAPO_MAX_LEVELS];
                            for (u32 j = 0; j < nidxfiles; j++) {
                                sr[j][0] = runs[j][0];
                                sr[j][1] = runs[j][1];
                            }
                            u64css si = {sr, sr + nidxfiles};
                            HITu64csStartZ(si, u64csHeadZ);
                            CAPOCollectPaths(si, pfx,
                                             u32bDataIdle(hashbuf2));
                        }
                        u32sSort(u32bData(hashbuf2));
                        u32sSort(u32bData(hashbuf1));
                        u32 n = CAPOIntersect(
                            u32bData(hashbuf1), u32bDataC(hashbuf2));
                        u32bShed(hashbuf1, u32bDataLen(hashbuf1) - n);
                    }
                }
                u32bUnMap(ndl_toks);
            }
        }

        CAPOStackClose(mmaps, nidxfiles);

        if (has_trigrams && u32bDataLen(hashbuf1) > 0)
            u32sSort(u32bData(hashbuf1));
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
            if (has_trigrams && u32bDataLen(hashbuf1) > 0) {
                u32 phash = CAPOPathHash(relpath);
                if (!u32sBsearch(&phash, u32bData(hashbuf1)))
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
            CAPOSpotReplace(source, mapped, htoks, needle, replace,
                            file_ext, fpbuf, line,
                            &total_replacements, &total_files_replaced);
        } else {
            CAPOSpotFile(source, htoks, needle, file_ext, line);
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

    if (!BNULL(hashbuf1)) u32bFree(hashbuf1);
    if (!BNULL(hashbuf2)) u32bFree(hashbuf2);
    done;
}
