#include "CAPO.h"

#include <dirent.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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
#include "json/BASON.h"

// MSET for u64 (trigram entries)
#define X(M, name) M##u64##name
#include "abc/MSETx.h"
#undef X

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

    // Parse source to BASON
    size_t buflen = $len(source) * 16;
    if (buflen < 1024 * 1024) buflen = 1024 * 1024;
    u8b bson = {};
    call(u8bMap, bson, buflen);
    size_t idxlen = buflen / BASON_PAGE + 256;
    u64 *_idx = (u64 *)malloc(idxlen * sizeof(u64));
    test(_idx != NULL, FAILsanity);
    u64b idx = {_idx, _idx, _idx, _idx + idxlen};

    ok64 o = BASTParse(bson, idx, source, ext);
    if (o != OK) {
        free(_idx);
        u8bUnMap(bson);
        return o;
    }

    u8cs bdata = {bson[1], bson[2]};
    CAPOTriCtx ctx = {
        .idle = u64bIdle(entries),
        .end = entries[3],
        .path_hash = CAPOPathHash(path),
    };
    o = CAPOTriExtract(bdata, CAPOTriCB, &ctx);

    free(_idx);
    u8bUnMap(bson);
    return o;
}

// --- File I/O ---

ok64 CAPOIndexWrite(u8csc dir, u64cs run, u64 seqno) {
    sane($ok(dir));
    if ($empty(run)) done;

    a_pad(u8, path, FILE_PATH_MAX_LEN);
    call(u8bFeed, path, dir);
    u8bFeed1(path, '/');
    call(RONu8sFeedPad, u8bIdle(path), seqno, CAPO_SEQNO_WIDTH);
    ((u8 **)path)[2] += CAPO_SEQNO_WIDTH;
    a_cstr(ext, CAPO_IDX_EXT);
    call(u8bFeed, path, ext);
    u8bFeed1(path, 0);
    u8bShed1(path);

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
    call(u8bFeed, pat, dir);
    u8bFeed1(pat, 0);

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
    call(u8bFeed, dpat, dir);
    u8bFeed1(dpat, 0);

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
        call(u8bFeed, fpath, dir);
        u8bFeed1(fpath, '/');
        u8cs fn = {(u8cp)names[i], (u8cp)names[i] + strlen(names[i])};
        call(u8bFeed, fpath, fn);
        u8bFeed1(fpath, 0);
        u8bShed1(fpath);

        u8bp mapped = NULL;
        call(FILEMapRO, &mapped, path8cgIn(fpath));

        size_t nbytes = mapped[2] - mapped[1];
        size_t nentries = nbytes / sizeof(u64);
        u64cp base = (u64cp)mapped[1];

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
    test(buf != NULL, FAILsanity);
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
    call(u8bFeed, dpat, dir);
    u8bFeed1(dpat, 0);

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
        call(u8bFeed, ulpath, dir);
        u8bFeed1(ulpath, '/');
        u8cs ulfn = {(u8cp)fnames[i - 1],
                     (u8cp)fnames[i - 1] + strlen(fnames[i - 1])};
        call(u8bFeed, ulpath, ulfn);
        u8bFeed1(ulpath, 0);
        u8bShed1(ulpath);
        unlink((char *)ulpath[1]);
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

ok64 CAPOReindex(u8csc reporoot) {
    sane($ok(reporoot));

    fprintf(stderr, "capo: repo root %.*s\n",
            (int)$len(reporoot), (char *)reporoot[0]);

    a_pad(u8, capodir, FILE_PATH_MAX_LEN);
    call(u8bFeed, capodir, reporoot);
    a_cstr(suf, "/" CAPO_DIR);
    call(u8bFeed, capodir, suf);
    u8cs dirslice = {capodir[1], capodir[2]};
    u8bFeed1(capodir, 0);
    u8bShed1(capodir);
    call(FILEMakeDirP, path8cgIn(capodir));
    fprintf(stderr, "capo: index dir %s\n", (char *)capodir[1]);

    // 1GB scratch buffer via anonymous mmap (pages allocated on demand)
    u64b entries = {};
    call(u64bMap, entries, CAPO_SCRATCH_LEN);

    // git ls-files
    char cmdbuf[FILE_PATH_MAX_LEN + 32];
    int n = snprintf(cmdbuf, sizeof(cmdbuf), "git -C %.*s ls-files",
                     (int)$len(reporoot), (char *)reporoot[0]);
    test(n > 0 && n < (int)sizeof(cmdbuf), FAILsanity);

    FILE *fp = popen(cmdbuf, "r");
    test(fp != NULL, FAILsanity);

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
        call(u8bFeed, fpbuf, fps);
        u8bFeed1(fpbuf, 0);
        u8bShed1(fpbuf);

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, path8cgIn(fpbuf));
        if (o != OK) { failed++; continue; }

        u8cs source = {mapped[1], mapped[2]};
        u8cs relpath = {(u8cp)line, (u8cp)line + len};
        o = CAPOIndexFile(entries, source, ext, relpath);
        FILEUnMap(mapped);
        if (o != OK) {
            fprintf(stderr, "FAIL\t%s\t%s\n", ok64str(o), line);
            failed++;
            continue;
        }
        u8c *codec[2] = {};
        BASTCodec(codec, ext);
        fprintf(stderr, "OK\t%.*s\t%s\n", (int)$len(codec), (char*)codec[0], line);
        indexed++;

        // Flush when scratch buffer is large enough
        size_t pending = u64bDataLen(entries);
        if (pending >= CAPO_FLUSH_AT) {
            u64s data = {entries[1], entries[2]};
            $sort(data, u64cmp);
            u64cs run = {(u64cp)data[0], (u64cp)data[1]};
            CAPOIndexWrite(dirslice, run, seqno++);
            total_entries += pending;
            fprintf(stderr, "capo: flushed %zu entries\n", pending);
            // Reset scratch: data and idle pointers back to start
            ((u64 **)entries)[1] = entries[0];
            ((u64 **)entries)[2] = entries[0];
        }
    }
    pclose(fp);

    // Flush remaining entries
    size_t pending = u64bDataLen(entries);
    if (pending > 0) {
        u64s data = {entries[1], entries[2]};
        $sort(data, u64cmp);
        u64cs run = {(u64cp)data[0], (u64cp)data[1]};
        CAPOIndexWrite(dirslice, run, seqno++);
        total_entries += pending;
    }

    u64bUnMap(entries);

    fprintf(stderr, "capo: indexed %u files, %zu entries, skipped %u, failed %u\n",
            indexed, total_entries, skipped, failed);

    // Compact if we wrote multiple runs
    if (seqno > 2) {
        fprintf(stderr, "capo: compacting %llu runs\n",
                (unsigned long long)(seqno - 1));
        CAPOCompact(dirslice);
    }

    done;
}

// --- Parallel reindex: single proc ---

ok64 CAPOReindexProc(u8csc reporoot, u32 nprocs, u32 proc) {
    sane($ok(reporoot) && proc < nprocs && nprocs > 0);

    fprintf(stderr, "capo[%u/%u]: starting\n", proc, nprocs);

    a_pad(u8, capodir, FILE_PATH_MAX_LEN);
    call(u8bFeed, capodir, reporoot);
    a_cstr(suf, "/" CAPO_DIR);
    call(u8bFeed, capodir, suf);
    u8cs dirslice = {capodir[1], capodir[2]};
    u8bFeed1(capodir, 0);
    u8bShed1(capodir);
    call(FILEMakeDirP, path8cgIn(capodir));

    u64b entries = {};
    call(u64bMap, entries, CAPO_SCRATCH_LEN);

    char cmdbuf[FILE_PATH_MAX_LEN + 32];
    int n = snprintf(cmdbuf, sizeof(cmdbuf), "git -C %.*s ls-files",
                     (int)$len(reporoot), (char *)reporoot[0]);
    test(n > 0 && n < (int)sizeof(cmdbuf), FAILsanity);

    FILE *fp = popen(cmdbuf, "r");
    test(fp != NULL, FAILsanity);

    u32 indexed = 0, skipped = 0, failed = 0;
    u32 batch = 0;  // counts flushes for seqno = nprocs*batch + proc + 1
    size_t total_entries = 0;
    u32 fileno = 0;

    char line[FILE_PATH_MAX_LEN];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = 0;
        if (len == 0) continue;

        // Striped: this proc handles files where fileno % nprocs == proc
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
        call(u8bFeed, fpbuf, fps);
        u8bFeed1(fpbuf, 0);
        u8bShed1(fpbuf);

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, path8cgIn(fpbuf));
        if (o != OK) { failed++; continue; }

        u8cs source = {mapped[1], mapped[2]};
        u8cs relpath = {(u8cp)line, (u8cp)line + len};
        o = CAPOIndexFile(entries, source, ext, relpath);
        FILEUnMap(mapped);
        if (o != OK) {
            fprintf(stderr, "FAIL\t%s\t%s\n", ok64str(o), line);
            failed++;
            continue;
        }
        u8c *codec[2] = {};
        BASTCodec(codec, ext);
        size_t cur = entries[2] - entries[1];
        fprintf(stderr, "OK\t%.*s\t%s\t(%zu entries)\n",
                (int)$len(codec), (char*)codec[0], line, cur);
        indexed++;

        // Flush when large enough
        size_t pending = entries[2] - entries[1];
        if (pending >= CAPO_FLUSH_AT) {
            fprintf(stderr, "capo[%u/%u]: flushing %zu entries\n",
                    proc, nprocs, pending);
            u64s data = {entries[1], entries[2]};
            $sort(data, u64cmp);
            u64 seqno = (u64)nprocs * batch + proc + 1;
            u64cs run = {(u64cp)data[0], (u64cp)data[1]};
            CAPOIndexWrite(dirslice, run, seqno);
            total_entries += pending;
            batch++;
            ((u64 **)entries)[1] = entries[0];
            ((u64 **)entries)[2] = entries[0];
        }
    }
    pclose(fp);

    // Flush remaining
    size_t pending = u64bDataLen(entries);
    if (pending > 0) {
        u64s data = {entries[1], entries[2]};
        $sort(data, u64cmp);
        u64 seqno = (u64)nprocs * batch + proc + 1;
        u64cs run = {(u64cp)data[0], (u64cp)data[1]};
        CAPOIndexWrite(dirslice, run, seqno);
        total_entries += pending;
    }

    u64bUnMap(entries);

    fprintf(stderr, "capo[%u/%u]: indexed %u files, %zu entries, skipped %u, failed %u\n",
            proc, nprocs, indexed, total_entries, skipped, failed);
    done;
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

        u64b mbuf = {};
        call(u64bMap, mbuf, total);
        u64s into = {mbuf[2], mbuf[3]};

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
        CAPONextSeqno(&seqno, dir);
        fprintf(stderr, "capo: next seqno = %llu (dir = '%.*s')\n",
                (unsigned long long)seqno,
                (int)$len(dir), (char *)dir[0]);
        u64cs merged = {(u64cp)mbuf[0], (u64cp)into[0]};
        fprintf(stderr, "capo: writing %zu deduplicated entries (seqno %llu)\n",
                $len(merged), (unsigned long long)seqno);
        CAPOIndexWrite(dir, merged, seqno);

        CAPOStackClose(mmaps, nfiles);
        u64bUnMap(mbuf);

        // Unlink old files (re-scan dir, skip the new one)
        a_pad(u8, dpat, FILE_PATH_MAX_LEN);
        call(u8bFeed, dpat, dir);
        u8bFeed1(dpat, 0);
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
                    call(u8bFeed, ulpath, dir);
                    u8bFeed1(ulpath, '/');
                    u8cs ulfn = {(u8cp)fnames[i],
                                 (u8cp)fnames[i] + strlen(fnames[i])};
                    call(u8bFeed, ulpath, ulfn);
                    u8bFeed1(ulpath, 0);
                    u8bShed1(ulpath);
                    unlink((char *)ulpath[1]);
                }
            }
        }

        break;  // one pass is enough since we merged all
    }
    done;
}

// --- Hook (incremental) ---

ok64 CAPOHook(u8csc reporoot) {
    sane($ok(reporoot));

    a_pad(u8, capodir, FILE_PATH_MAX_LEN);
    call(u8bFeed, capodir, reporoot);
    a_cstr(suf, "/" CAPO_DIR);
    call(u8bFeed, capodir, suf);
    u8cs dirslice = {capodir[1], capodir[2]};
    u8bFeed1(capodir, 0);
    u8bShed1(capodir);
    call(FILEMakeDirP, path8cgIn(capodir));

    char cmdbuf[FILE_PATH_MAX_LEN + 64];
    int n = snprintf(cmdbuf, sizeof(cmdbuf),
                     "git -C %.*s diff --name-only HEAD~1 HEAD",
                     (int)$len(reporoot), (char *)reporoot[0]);
    test(n > 0 && n < (int)sizeof(cmdbuf), FAILsanity);

    FILE *fp = popen(cmdbuf, "r");
    test(fp != NULL, FAILsanity);

    u64b entries = {};
    call(u64bMap, entries, CAPO_SCRATCH_LEN);

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
        call(u8bFeed, fpbuf, fps);
        u8bFeed1(fpbuf, 0);
        u8bShed1(fpbuf);

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, path8cgIn(fpbuf));
        if (o != OK) continue;

        u8cs source = {mapped[1], mapped[2]};
        u8cs relpath = {(u8cp)line, (u8cp)line + len};
        CAPOIndexFile(entries, source, ext, relpath);
        FILEUnMap(mapped);
    }
    pclose(fp);

    size_t pending = u64bDataLen(entries);
    if (pending > 0) {
        u64s data = {entries[1], entries[2]};
        $sort(data, u64cmp);
        u64 seqno = 0;
        CAPONextSeqno(&seqno, dirslice);
        u64cs run = {(u64cp)data[0], (u64cp)data[1]};
        CAPOIndexWrite(dirslice, run, seqno);
    }

    u64bUnMap(entries);
    CAPOCompact(dirslice);
    done;
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

ok64 CAPOQuery(u8csc selector, u8csc reporoot) {
    sane($ok(selector) && $ok(reporoot));

    a_pad(u8, capodir, FILE_PATH_MAX_LEN);
    call(u8bFeed, capodir, reporoot);
    a_cstr(suf, "/" CAPO_DIR);
    call(u8bFeed, capodir, suf);
    u8cs dirslice = {capodir[1], capodir[2]};
    u8bFeed1(capodir, 0);
    u8bShed1(capodir);

    u32 maxhashes = 64 * 1024;
    u32 *hashbuf1 = (u32 *)malloc(maxhashes * sizeof(u32));
    u32 *hashbuf2 = (u32 *)malloc(maxhashes * sizeof(u32));
    test(hashbuf1 != NULL && hashbuf2 != NULL, FAILsanity);

    // Load MSET stack
    u64cs runs[CAPO_MAX_LEVELS] = {};
    u64css stack = {runs, runs};
    u8bp mmaps[CAPO_MAX_LEVELS] = {};
    u32 nfiles = 0;
    CAPOStackOpen(stack, mmaps, &nfiles, dirslice);
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
    u8cs query = {qbuf[1], qbuf[2]};

    // Sort path hashes for binary search
    if (has_trigrams && nhashes > 0)
        qsort(hashbuf1, nhashes, sizeof(u32), CAPOu32cmp);

    // Color only when output is a terminal
    b8 use_color = isatty(STDOUT_FILENO) ? YES : NO;

    // Pager for terminal output (only if $PAGER is set)
    FILE *pager = NULL;
    int saved_stdout = -1;
    char const *pgcmd = getenv("PAGER");
    if (pgcmd != NULL && *pgcmd && use_color) {
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
    test(cn > 0 && cn < (int)sizeof(cmdbuf), FAILsanity);

    FILE *fp = popen(cmdbuf, "r");
    test(fp != NULL, FAILsanity);

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
        call(u8bFeed, fpbuf, fps);
        u8bFeed1(fpbuf, 0);
        u8bShed1(fpbuf);

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, path8cgIn(fpbuf));
        if (o != OK) continue;

        u8cs source = {mapped[1], mapped[2]};
        size_t buflen = $len(source) * 16;
        if (buflen < 1024 * 1024) buflen = 1024 * 1024;
        u8b bson = {};
        o = u8bMap(bson, buflen);
        if (o != OK) { FILEUnMap(mapped); continue; }
        size_t idxlen = buflen / BASON_PAGE + 256;
        u64 *_bidx = (u64 *)malloc(idxlen * sizeof(u64));
        if (!_bidx) { u8bUnMap(bson); FILEUnMap(mapped); continue; }
        u64b bidx = {_bidx, _bidx, _bidx, _bidx + idxlen};

        o = BASTParse(bson, bidx, source, ext);
        if (o != OK) {
            free(_bidx);
            u8bUnMap(bson);
            FILEUnMap(mapped);
            continue;
        }

        u8cs bdata = {bson[1], bson[2]};

        size_t fbuflen = $len(bdata) + 4096;
        u8b fbufm = {};
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
            u8cs filtered = {fbufm[1], fbufm[2]};
            size_t obuflen = $len(filtered) * 4 + 4096;
            u8b obufm = {};
            o = u8bMap(obufm, obuflen);
            if (o == OK) {
                u8s out = {obufm[2], obufm[3]};
                // Path header
                if (use_color) escfeed(out, GRAY);
                a_cstr(hdr_pre, "--- ");
                u8sFeed(out, hdr_pre);
                u8sFeed(out, relpath);
                a_cstr(hdr_post, " ---\n");
                u8sFeed(out, hdr_post);
                if (use_color) escfeed(out, 0);
                if (use_color)
                    o = CSSCat(out, filtered, relpath);
                else
                    o = CSSExport(out, filtered);
                if (o == OK) {
                    u8cs result = {obufm[2], out[0]};
                    FILEFeedall(STDOUT_FILENO, result);
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
