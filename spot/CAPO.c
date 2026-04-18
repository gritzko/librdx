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
#include "dog/DEF.h"
#include "dog/HOME.h"
#include "dog/IGNO.h"
#include "spot/SPOT.h"

// Shell-quote a path slice into buf: 'path' with internal ' → '\''.
// Safe for use inside a `sh -c` command line.
static ok64 capo_sh_quote(u8bp buf, u8csc path) {
    sane(buf != NULL && $ok(path));
    call(u8bFeed1, buf, '\'');
    $for(u8c, p, path) {
        if (*p == '\'') {
            a_cstr(esc, "'\\''");
            call(u8bFeed, buf, esc);
        } else {
            call(u8bFeed1, buf, *p);
        }
    }
    call(u8bFeed1, buf, '\'');
    done;
}

// --- Language detection via tok/ ---

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

// --- Resolve spot index directory ---
// Spot files live in <reporoot>/.dogs/spot/, where <reporoot> is the
// workspace dir containing .git (file or directory). Caller is responsible
// for creating the dir via FILEMakeDirP if it does not yet exist.

ok64 CAPOResolveDir(path8b out, u8csc reporoot) {
    sane($ok(reporoot) && out != NULL);
    // If caller supplied a reporoot, feed it; otherwise walk up via HOME.
    if (!u8csEmpty(reporoot)) {
        a_dup(u8c, r, reporoot);
        call(PATHu8bFeed, out, r);
    } else {
        home rh = {};
        u8cs none = {};
        call(HOMEOpen, &rh, none, NO);
        a_dup(u8c, r, u8bDataC(rh.root));
        ok64 fo = PATHu8bFeed(out, r);
        HOMEClose(&rh);
        if (fo != OK) return fo;
    }
    a_cstr(dotdogs, ".dogs");
    call(PATHu8bPush, out, dotdogs);
    a_cstr(spotname, "spot");
    call(PATHu8bPush, out, spotname);
    done;
}

// --- Trigram extraction from tok/ tokens ---

typedef struct {
    u64 **idle;
    u64 *end;
    u32 path_hash;
} CAPOTriCtx;

static ok64 CAPOTriCB(void0p arg, u8cs tri) {
    CAPOTriCtx *ctx = (CAPOTriCtx *)arg;
    if (*ctx->idle >= ctx->end) return CAPONOROOM;
    u64 entry = CAPOTriPack(tri) | (u64)ctx->path_hash;
    **ctx->idle = entry;
    (*ctx->idle)++;
    return OK;
}

// Walk packed tokens, extract RON64 trigrams from source text
static ok64 CAPOTriExtractToks(u32cs toks, u8cp base,
                                ok64 (*cb)(void0p, u8cs), void0p arg) {
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
    call(u8bFed, path, CAPO_SEQNO_WIDTH);  // RONu8sFeedPad wrote into IDLE
    a_cstr(idxext, CAPO_IDX_EXT);
    call(u8bFeed, path, idxext);
    call(PATHu8bTerm, path);

    int fd = -1;
    call(FILECreate, &fd, $path(path));
    size_t bytes = $len(run) * sizeof(u64);
    u8cs data = {(u8cp)run[0], (u8cp)run[0] + bytes};
    call(FILEFeedAll, fd, data);
    close(fd);
    done;
}

// Callback context for listing .idx files
typedef struct {
    char (*names)[64];
    u32 maxn;
    u32 count;
} CAPOListIdxCtx;

static ok64 CAPOListIdxCB(void0p arg, path8p path) {
    CAPOListIdxCtx *ctx = (CAPOListIdxCtx *)arg;
    if (ctx->count >= ctx->maxn) return OK;
    u8cs base = {};
    a_dup(u8c, pdata, u8bDataC(path));
    PATHu8sBase(base, pdata);
    size_t nlen = (size_t)$len(base);
    if (nlen < 5 || nlen > 63) return OK;
    if (memcmp(base[1] - 4, CAPO_IDX_EXT, 4) != 0) return OK;
    memcpy(ctx->names[ctx->count], base[0], nlen);
    ctx->names[ctx->count][nlen] = 0;
    ctx->count++;
    return OK;
}

// List .idx files in a directory, sorted by name.
// Returns count; names stored in out[0..count)[0..64).
static u32 CAPOListIdx(char out[][64], u32 maxn, u8csc dir) {
    a_path(dpat);
    if (PATHu8bFeed(dpat, dir) != OK) return 0;

    CAPOListIdxCtx ctx = {.names = out, .maxn = maxn, .count = 0};
    FILEScanDir(dpat, CAPOListIdxCB, &ctx);

    // Sort by name
    if (ctx.count > 1)
        qsort(out, ctx.count, 64,
              (int (*)(const void *, const void *))strcmp);
    return ctx.count;
}

ok64 CAPONextSeqno(u64p seqno, u8csc dir) {
    sane(seqno != NULL && $ok(dir));
    *seqno = 1;
    char names[CAPO_MAX_LEVELS][64];
    u32 n = CAPOListIdx(names, CAPO_MAX_LEVELS, dir);
    u64 maxseq = 0;
    for (u32 i = 0; i < n; i++) {
        size_t nlen = strlen(names[i]);
        size_t numlen = nlen - 4;
        u8cs numslice = {(u8cp)names[i], (u8cp)names[i] + numlen};
        ok64 val = 0;
        ok64 r = RONutf8sDrain(&val, numslice);
        if (r == OK && val > maxseq) maxseq = val;
    }
    *seqno = maxseq + 1;
    done;
}

// --- Stack management ---

ok64 CAPOStackOpen(u64css stack, u8bp *maps, u32p nfiles, u8csc dir) {
    sane($ok(stack) && maps != NULL && nfiles != NULL && $ok(dir));
    *nfiles = 0;

    char names[CAPO_MAX_LEVELS][64];
    u32 count = CAPOListIdx(names, CAPO_MAX_LEVELS, dir);

    for (u32 i = 0; i < count; i++) {
        u8cs fn = {(u8cp)names[i], (u8cp)names[i] + strlen(names[i])};
        a_path(fpath, dir, fn);

        u8bp mapped = NULL;
        call(FILEMapRO, &mapped, $path(fpath));

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
        HITu64Start(sub);
        u64p out = into[0];
        HITu64Merge(sub, &out);
        into[0] = out;
    }

    u64 seqno = 0;
    call(CAPONextSeqno, &seqno, dir);
    u64cs merged = {(u64cp)cbuf[0], (u64cp)(into[0])};
    call(CAPOIndexWrite, dir, merged, seqno);

    char fnames[CAPO_MAX_LEVELS][64];
    u32 fcount = CAPOListIdx(fnames, CAPO_MAX_LEVELS, dir);

    CAPOStackClose(mmaps, nfiles);

    u32 unlinked = 0;
    for (u32 i = fcount; i > 0 && unlinked < m; i--) {
        if (i == fcount) continue;
        u8cs ulfn = {(u8cp)fnames[i - 1],
                     (u8cp)fnames[i - 1] + strlen(fnames[i - 1])};
        a_path(ulpath, dir, ulfn);
        unlink((char *)u8bDataHead(ulpath));
        unlinked++;
    }

    u64bFree(cbuf);
    done;
}

// --- Reindex ---

static ok64 CAPOReindexWork(u8csc reporoot, u8csc dirslice, u64bp entries) {
    sane($ok(reporoot) && $ok(dirslice) && entries != NULL);

    // Pick a file-enumerator. In a real git working tree we ask git
    // (cheap, respects .gitignore, follows submodules). In a
    // keeper-cloned dir with no .git we fall back to `find`, which
    // walks the actual filesystem and skips the .dogs/ store itself.
    a_path(gitp);
    call(PATHu8bFeed, gitp, reporoot);
    a_cstr(dotgit, ".git");
    call(PATHu8bPush, gitp, dotgit);
    struct stat gitsb = {};
    b8 has_git = (FILEStat(&gitsb, $path(gitp)) == OK);

    a_pad(u8, cmdbuf, FILE_PATH_MAX_LEN * 2 + 512);
    if (has_git) {
        a_cstr(p1, "git -C ");
        a_cstr(p2, " ls-files && git -C ");
        a_cstr(p3, " submodule foreach --quiet --recursive "
                   "'git ls-files | sed \"s|^|$displaypath/|\"'");
        call(u8bFeed, cmdbuf, p1);
        call(capo_sh_quote, cmdbuf, reporoot);
        call(u8bFeed, cmdbuf, p2);
        call(capo_sh_quote, cmdbuf, reporoot);
        call(u8bFeed, cmdbuf, p3);
    } else {
        // Portable form (busybox find has no -printf): print full
        // ./relative/paths, then strip the leading "./" via sed so
        // output matches git ls-files. Prune .dogs/ so we don't index
        // keeper's pack files etc.
        a_cstr(p1, "cd ");
        a_cstr(p2, " && find . -type d -name .dogs -prune -o "
                   "-type f -print | sed 's|^\\./||'");
        call(u8bFeed, cmdbuf, p1);
        call(capo_sh_quote, cmdbuf, reporoot);
        call(u8bFeed, cmdbuf, p2);
    }
    call(u8bFeed1, cmdbuf, 0);

    FILE *fp = popen((char *)u8bDataHead(cmdbuf), "r");
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

        a_path(fpbuf);
        u8cs lns = {(u8cp)line, (u8cp)line + len};
        if (PATHu8bFeed(fpbuf, reporoot) != OK ||
            PATHu8bPush(fpbuf, lns) != OK) { skipped++; continue; }

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, $path(fpbuf));
        if (o != OK) {
            fprintf(stderr, "FAIL\t%s\t%s\t(open %s)\n",
                    ok64str(o), line, (char *)u8bDataHead(fpbuf));
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
            fprintf(stderr, "\033[%dmOK\t" U8SFMT "\t%s\033[0m\n",
                    GRAY, u8sFmt(codec), line);
        else
            fprintf(stderr, "OK\t" U8SFMT "\t%s\n", u8sFmt(codec), line);
        indexed++;

        size_t pending = u64bDataLen(entries);
        if (pending >= CAPO_FLUSH_AT) {
            u64sp data = u64bData(entries);
            u64sSort(data);
            u64sDedup(data);
            size_t unique = $len(data);
            if (unique * 2 <= pending) {
                fprintf(stderr, "spot: dedup %zu -> %zu, delaying flush\n",
                        pending, unique);
            } else {
                u64cs run = {(u64cp)data[0], (u64cp)data[1]};
                call(CAPOIndexWrite, dirslice, run, seqno++);
                total_entries += $len(run);
                fprintf(stderr, "spot: flushed %zu entries (%zu deduped)\n",
                        $len(run), pending - $len(run));
                u64bReset(entries);
            }
        }
    }
    pclose(fp);

    size_t pending = u64bDataLen(entries);
    if (pending > 0) {
        u64sp data = u64bData(entries);
        u64sSort(data);
        u64sDedup(data);
        u64cs run = {(u64cp)data[0], (u64cp)data[1]};
        call(CAPOIndexWrite, dirslice, run, seqno++);
        total_entries += $len(run);
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

    a_path(capodir);
    call(CAPOResolveDir, capodir, reporoot);
    a_dup(u8c, dirslice, u8bDataC(capodir));
    vcall("mkdir", FILEMakeDirP, $path(capodir));
    fprintf(stderr, "spot: index dir %s\n", (char *)u8bDataHead(capodir));

    Bu64 entries = {};
    vcall("mmap scratch", u64bMap, entries, CAPO_SCRATCH_LEN);

    ok64 o = CAPOReindexWork(reporoot, dirslice, entries);

    u64bUnMap(entries);
    if (o == OK) {
        CAPOCompactAll(dirslice);
        CAPOCommitWrite(reporoot, dirslice);
    }
    return o;
}

// --- Parallel reindex: single proc ---

static ok64 CAPOReindexProcWork(u8csc reporoot, u8csc dirslice,
                                u64bp entries, u32 nprocs, u32 proc) {
    sane($ok(reporoot) && $ok(dirslice) && entries != NULL);

    a_pad(u8, cmdbuf, FILE_PATH_MAX_LEN * 2 + 512);
    {
        a_cstr(p1, "git -C ");
        a_cstr(p2, " ls-files && git -C ");
        a_cstr(p3, " submodule foreach --quiet --recursive "
                   "'git ls-files | sed \"s|^|$displaypath/|\"'");
        call(u8bFeed, cmdbuf, p1);
        call(capo_sh_quote, cmdbuf, reporoot);
        call(u8bFeed, cmdbuf, p2);
        call(capo_sh_quote, cmdbuf, reporoot);
        call(u8bFeed, cmdbuf, p3);
        call(u8bFeed1, cmdbuf, 0);
    }

    FILE *fp = popen((char *)u8bDataHead(cmdbuf), "r");
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

        a_path(fpbuf);
        u8cs lns = {(u8cp)line, (u8cp)line + len};
        if (PATHu8bFeed(fpbuf, reporoot) != OK ||
            PATHu8bPush(fpbuf, lns) != OK) { skipped++; continue; }

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, $path(fpbuf));
        if (o != OK) {
            fprintf(stderr, "FAIL\t%s\t%s\t(open %s)\n",
                    ok64str(o), line, (char *)u8bDataHead(fpbuf));
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
            u64sp data = u64bData(entries);
            u64sSort(data);
            u64sDedup(data);
            size_t unique = $len(data);
            if (unique * 2 <= pending) {
                fprintf(stderr, "spot[%u/%u]: dedup %zu -> %zu, delaying flush\n",
                        proc, nprocs, pending, unique);
            } else {
                u64 seqno = (u64)nprocs * batch + proc + 1;
                u64cs run = {(u64cp)data[0], (u64cp)data[1]};
                fprintf(stderr, "spot[%u/%u]: flushed %zu entries (%zu deduped)\n",
                        proc, nprocs, $len(run), pending - $len(run));
                call(CAPOIndexWrite, dirslice, run, seqno);
                total_entries += $len(run);
                batch++;
                u64bReset(entries);
            }
        }
    }
    int rc = pclose(fp);
    if (rc != 0)
        fprintf(stderr, "spot[%u/%u]: `%s` failed (status %d)\n",
                proc, nprocs, (char *)u8bDataHead(cmdbuf), rc);

    size_t pending = u64bDataLen(entries);
    if (pending > 0) {
        u64sp data = u64bData(entries);
        u64sSort(data);
        u64sDedup(data);
        u64 seqno = (u64)nprocs * batch + proc + 1;
        u64cs run = {(u64cp)data[0], (u64cp)data[1]};
        call(CAPOIndexWrite, dirslice, run, seqno);
        total_entries += $len(run);
    }

    fprintf(stderr, "spot[%u/%u]: indexed %u files, %zu entries, skipped %u, failed %u\n",
            proc, nprocs, indexed, total_entries, skipped, failed);
    done;
}

ok64 CAPOReindexProc(u8csc reporoot, u32 nprocs, u32 proc) {
    sane($ok(reporoot) && proc < nprocs && nprocs > 0);

    fprintf(stderr, "spot[%u/%u]: starting\n", proc, nprocs);

    a_path(capodir);
    call(CAPOResolveDir, capodir, reporoot);
    a_dup(u8c, dirslice, u8bDataC(capodir));
    vcall("mkdir", FILEMakeDirP, $path(capodir));

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

        HITu64Start(stack);
        u64p out = into[0];
        HITu64Merge(stack, &out);
        into[0] = out;

        u64 seqno = 0;
        call(CAPONextSeqno, &seqno, dir);
        fprintf(stderr, "spot: next seqno = %" PRIu64 " (dir = '" U8SFMT "')\n",
                seqno, u8sFmt(dir));
        u64cs merged = {(u64cp)mbuf[0], (u64cp)into[0]};
        fprintf(stderr, "spot: writing %zu deduplicated entries (seqno %" PRIu64 ")\n",
                $len(merged), seqno);
        call(CAPOIndexWrite, dir, merged, seqno);

        CAPOStackClose(mmaps, nfiles);
        u64bUnMap(mbuf);

        {
            char fnames[CAPO_MAX_LEVELS][64];
            u32 fcount = CAPOListIdx(fnames, CAPO_MAX_LEVELS, dir);
            for (u32 i = 0; i + 1 < fcount; i++) {
                u8cs ulfn = {(u8cp)fnames[i],
                             (u8cp)fnames[i] + strlen(fnames[i])};
                a_path(ulpath, dir, ulfn);
                unlink((char *)u8bDataHead(ulpath));
            }
        }

        break;
    }
    done;
}

// --- Commit tracking ---

ok64 CAPOCommitWrite(u8csc reporoot, u8csc capodir) {
    sane($ok(reporoot) && $ok(capodir));

    // Resolve current HEAD. Prefer .dogs/sniff/HEAD (canonical for
    // keeper-cloned dirs and always present after sniff checkout); fall
    // back to `git rev-parse HEAD` for traditional git working trees.
    char newsha[64] = {};
    {
        a_path(snipath);
        a_cstr(snirel, ".dogs/sniff/HEAD");
        call(PATHu8bFeed, snipath, reporoot);
        call(PATHu8bPush, snipath, snirel);
        FILE *sf = fopen((char *)u8bDataHead(snipath), "r");
        if (sf != NULL) {
            char *got = fgets(newsha, sizeof(newsha), sf);
            fclose(sf);
            if (got == NULL) newsha[0] = 0;
        }
    }
    if (newsha[0] == 0) {
        a_cstr(gitp, "/usr/bin/git");
        u8cs gargs[] = {
            u8slit("git"),
            u8slit("-C"),
            {reporoot[0], reporoot[1]},
            u8slit("rev-parse"),
            u8slit("HEAD"),
        };
        u8css gargv = {gargs, gargs + 5};
        pid_t pid = 0;
        int rfd = -1;
        call(FILESpawn, gitp, gargv, NULL, &rfd, &pid);
        ssize_t n = 0;
        while (n < (ssize_t)sizeof(newsha) - 1) {
            ssize_t r = read(rfd, newsha + n, sizeof(newsha) - 1 - (size_t)n);
            if (r <= 0) break;
            n += r;
        }
        close(rfd);
        newsha[n > 0 ? n : 0] = 0;
        int rc = -1;
        FILEReap(pid, &rc);
        test(rc == 0 && n > 0, FAILSANITY);
    }

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
    call(PATHu8bTerm, path);

    int fd = -1;
    call(FILECreate, &fd, $path(path));
    u8cs nl = {(u8cp)"\n", (u8cp)"\n" + 1};
    for (u32 i = keep_start; i < sha_count; i++) {
        u8cs data = {(u8cp)shas[i], (u8cp)shas[i] + 40};
        call(FILEFeedAll, fd, data);
        call(FILEFeedAll, fd, nl);
    }
    u8cs newdata = {(u8cp)newsha, (u8cp)newsha + 40};
    call(FILEFeedAll, fd, newdata);
    call(FILEFeedAll, fd, nl);
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
    call(PATHu8bTerm, path);

    u8bp mapped = NULL;
    ok64 o = FILEMapRO(&mapped, $path(path));
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

// Build "git -C <repo> diff --name-only <sha>" into out.
// Returns OK with out filled when a usable diff command was emitted;
// CAPONODIFF when no saved commit is reachable (caller falls back to
// full reindex); a real error code on internal buffer failure.
static ok64 CAPOHookDiffCmd(u8bp out, u8csc reporoot, u8csc dirslice) {
    sane(out != NULL && $ok(reporoot) && $ok(dirslice));

    char shas[CAPO_MAX_SHAS][44];
    u32 sha_count = 0;
    CAPOCommitRead(&sha_count, dirslice, shas, CAPO_MAX_SHAS);

    if (sha_count == 0) {
        fprintf(stderr, "spot: no saved commit, full reindex\n");
        return CAPONODIFF;
    }

    a_cstr(gitp, "/usr/bin/git");

    // try newest first (last in file)
    for (u32 i = sha_count; i > 0; i--) {
        u8cs cargs[] = {
            u8slit("git"),
            u8slit("-C"),
            {reporoot[0], reporoot[1]},
            u8slit("merge-base"),
            u8slit("--is-ancestor"),
            u8scstr(shas[i - 1]),
            u8slit("HEAD"),
        };
        u8css cargv = {cargs, cargs + 7};
        pid_t pid = 0;
        ok64 so = FILESpawn(gitp, cargv, NULL, NULL, &pid);
        if (so != OK) continue;
        int rc = -1;
        FILEReap(pid, &rc);
        if (rc == 0) {
            if (CAPO_COLOR)
                fprintf(stderr, "\033[%dmChanges since %.40s\033[0m\n",
                        GRAY, shas[i - 1]);
            else
                fprintf(stderr, "Changes since %.40s\n", shas[i - 1]);
            // Build the diff command for the caller to run.
            // (Pipeline-free, but we still go through CAPOIndexFromCmd
            // which expects a shell string; sh -c with positional args
            // keeps the path quoted-by-construction.)
            a_cstr(diff_pre, "git -C ");
            a_cstr(diff_mid, " diff --name-only ");
            u8cs sha_s = {(u8cp)shas[i - 1], (u8cp)shas[i - 1] + 40};
            call(u8bFeed, out, diff_pre);
            call(capo_sh_quote, out, reporoot);
            call(u8bFeed, out, diff_mid);
            call(u8bFeed, out, sha_s);
            call(u8bFeed1, out, 0);
            done;
        }
    }

    fprintf(stderr, "spot: saved commits unreachable, full reindex\n");
    return CAPONODIFF;
}

// Index files listed by a shell command. Returns count of indexed files.
static ok64 CAPOIndexFromCmd(u8csc reporoot, u64bp entries,
                              const char *cmdbuf, int *indexed) {
    sane($ok(reporoot) && entries != NULL && cmdbuf != NULL);

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
        if (!CAPOKnownExt(ext)) continue;

        a_path(fpbuf);
        u8cs lns = {(u8cp)line, (u8cp)line + len};
        if (PATHu8bFeed(fpbuf, reporoot) != OK ||
            PATHu8bPush(fpbuf, lns) != OK) continue;

        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, $path(fpbuf));
        if (o != OK) continue;

        a_dup(u8c, source, u8bDataC(mapped));
        u8cs relpath = {(u8cp)line, (u8cp)line + len};
        o = CAPOIndexFile(entries, source, ext, relpath);
        FILEUnMap(mapped);
        if (o != OK) continue;

        u8c *codec[2] = {};
        CAPOCodecName(codec, ext);
        fprintf(stderr, "OK\t" U8SFMT "\t%s\n", u8sFmt(codec), line);
        if (indexed) (*indexed)++;
    }
    pclose(fp);
    done;
}

// Flush pending entries to an index run and compact.
static ok64 CAPOFlushEntries(u8csc dirslice, u64bp entries) {
    sane($ok(dirslice) && entries != NULL);
    size_t pending = u64bDataLen(entries);
    if (pending > 0) {
        u64sp data = u64bData(entries);
        u64sSort(data);
        u64sDedup(data);
        u64 seqno = 0;
        call(CAPONextSeqno, &seqno, dirslice);
        u64cs run = {(u64cp)data[0], (u64cp)data[1]};
        call(CAPOIndexWrite, dirslice, run, seqno);
    }
    call(CAPOCompact, dirslice);
    done;
}

static ok64 CAPOHookDiff(u8csc reporoot, u8csc dirslice,
                          u64bp entries, const char *cmdbuf) {
    sane($ok(reporoot) && $ok(dirslice) && entries != NULL);

    int indexed = 0;
    call(CAPOIndexFromCmd, reporoot, entries, cmdbuf, &indexed);
    fprintf(stderr, "%d file(s) re-indexed\n", indexed);

    call(CAPOFlushEntries, dirslice, entries);
    done;
}

ok64 CAPOHook(u8csc reporoot) {
    sane($ok(reporoot));

    a_path(capodir);
    call(CAPOResolveDir, capodir, reporoot);
    a_dup(u8c, dirslice, u8bDataC(capodir));
    call(FILEMakeDirP, $path(capodir));

    a_pad(u8, cmdbuf, FILE_PATH_MAX_LEN + 256);
    ok64 o = OK;

    Bu64 entries = {};
    call(u64bMap, entries, CAPO_SCRATCH_LEN);

    ok64 dco = CAPOHookDiffCmd(cmdbuf, reporoot, dirslice);
    if (dco == OK) {
        o = CAPOHookDiff(reporoot, dirslice, entries,
                         (char *)u8bDataHead(cmdbuf));
    } else if (dco == CAPONODIFF) {
        o = CAPOReindexWork(reporoot, dirslice, entries);
    } else {
        o = dco;  // real error — surface it
    }

    u64bUnMap(entries);

    if (o == OK) CAPOCommitWrite(reporoot, dirslice);
    return o;
}

// --- Uncommitted: index staged + unstaged + untracked ---

ok64 CAPOUncommitted(u8csc reporoot, b8 untracked) {
    sane($ok(reporoot));

    a_path(capodir);
    call(CAPOResolveDir, capodir, reporoot);
    a_dup(u8c, dirslice, u8bDataC(capodir));
    call(FILEMakeDirP, $path(capodir));

    Bu64 entries = {};
    call(u64bMap, entries, CAPO_SCRATCH_LEN);

    int indexed = 0;

    // staged + unstaged changes vs HEAD
    a_pad(u8, cmd1, FILE_PATH_MAX_LEN + 128);
    a_cstr(diff_pfx, "git -C ");
    a_cstr(diff_sfx, " diff --name-only HEAD 2>/dev/null");
    call(u8bFeed, cmd1, diff_pfx);
    call(capo_sh_quote, cmd1, reporoot);
    call(u8bFeed, cmd1, diff_sfx);
    call(u8bFeed1, cmd1, 0);
    call(CAPOIndexFromCmd, reporoot, entries,
         (char *)u8bDataHead(cmd1), &indexed);

    // untracked files (only with -U)
    if (untracked) {
        a_pad(u8, cmd2, FILE_PATH_MAX_LEN + 128);
        a_cstr(ls_pfx, "git -C ");
        a_cstr(ls_sfx, " ls-files --others --exclude-standard");
        call(u8bFeed, cmd2, ls_pfx);
        call(capo_sh_quote, cmd2, reporoot);
        call(u8bFeed, cmd2, ls_sfx);
        call(u8bFeed1, cmd2, 0);
        call(CAPOIndexFromCmd, reporoot, entries,
             (char *)u8bDataHead(cmd2), &indexed);
    }

    fprintf(stderr, "%d uncommitted file(s) indexed\n", indexed);
    call(CAPOFlushEntries, dirslice, entries);

    u64bUnMap(entries);
    done;
}

// --- Trigram query helpers ---

// Seek a HIT to a prefix range [prefix, prefix + 1<<32).
// After this, the HIT only contains entries within the prefix.
static ok64 CAPOSeekPrefix(u64css iter, u64 prefix) {
    u64 lo = prefix;
    u64 hi = prefix + (1ULL << 32);
    return HITu64SeekRange(iter, &lo, &hi);
}

ok64 CAPOCollectPaths(u64css iter, u64 tri_prefix, u32g hashes) {
    ok64 o = CAPOSeekPrefix(iter, tri_prefix);
    if (o != OK) return OK;

    while (!$empty(iter)) {
        u32gFeed1(hashes, (u32)(*(*iter[0])[0]));
        HITu64Step(iter);
    }
    return OK;
}

// In-place intersect hashbuf against a range-seeked HIT's path hashes.
// Both hashbuf data and the HIT output are sorted by path hash.
void CAPOFilterInPlace(u32bp hashbuf, u64css iter, u64 prefix) {
    CAPOSeekPrefix(iter, prefix);
    u32s data = {};
    $mv(data, u32bData(hashbuf));
    u32 *r = data[0], *w = data[0];

    while (r < data[1] && !$empty(iter)) {
        u32 ph = (u32)(*(*iter[0])[0]);
        if (*r < ph) { r++; continue; }
        if (*r > ph) { HITu64Step(iter); continue; }
        u32 matched = *r;
        *w++ = matched;
        while (r < data[1] && *r == matched) r++;
        while (!$empty(iter) && (u32)(*(*iter[0])[0]) == matched)
            HITu64Step(iter);
    }
    u32bShed(hashbuf, (size_t)(data[1] - w));
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

// --- Hunk building ---

ok64 CAPOBuildHunk(u8csc source, u32cs htoks, u32 ctx_lo, u32 ctx_hi,
                   range32 const *hls, int nhl,
                   u8csc file_ext, const char *filepath,
                   b8 needs_title, b8 *first_hunk) {
    sane(less_nhunks < LESS_MAX_HUNKS);
    LESShunk *hk = &less_hunks[less_nhunks];
    *hk = (LESShunk){};

    // Compose URI: path#symbol:lineno
    {
        char funcname[256] = {};
        if (needs_title || *first_hunk)
            CAPOFindFunc(source, ctx_lo, file_ext,
                         funcname, sizeof(funcname));
        u32 ln = 1;
        $for(u8c, ch, source) {
            if (ch >= source[0] + ctx_lo) break;
            if (*ch == '\n') ln++;
        }
        u8cs fp = {};
        if (filepath) {
            fp[0] = (u8cp)filepath;
            fp[1] = (u8cp)filepath + strlen(filepath);
        }
        u8gp ug = u8aOpen(less_arena);
        HUNKu8sMakeURI(u8gRest(ug), fp, funcname, ln);
        u8cs uri_s = {};
        u8aClose(less_arena, uri_s);
        $mv(hk->uri, uri_s);
    }

    hk->text[0] = source[0] + ctx_lo;
    hk->text[1] = source[0] + ctx_hi;

    // Clip file-level toks to context region
    HUNKu32sClip(less_arena, hk->toks, htoks, ctx_lo, ctx_hi);

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
            ok64 wo = FILECreate(&fd, $path(fpbuf));
            if (wo == OK) {
                FILEFeedAll(fd, result);
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

        // Collect subsequent matches within this context.
        // The SPOT engine may report the same logical match multiple
        // times with different uppercase-placeholder anchor positions
        // (a wider span first, then a narrower one).  Dedupe by keeping
        // the narrowest match: when a new entry is contained within the
        // previous one, replace; otherwise append.
        range32 hls[CAPO_MAX_HLS];
        int nhl = 0;
        hls[nhl++] = (range32){slo, shi};
        while (nhl <= CAPO_MAX_HLS && SPOTNext(&st) == OK) {
            u32 s2 = st.src_rng.lo;
            u32 e2 = st.src_rng.hi;
            if (e2 <= s2 || e2 > (u32)$len(source)) continue;
            if (s2 >= ctx_hi) {
                pending = (range32){s2, e2};
                have_pending = YES;
                break;
            }
            // If the new range is contained in the previous one
            // (same end, narrower start), replace — not append.
            range32 *prev = &hls[nhl - 1];
            if (s2 >= prev->lo && e2 <= prev->hi) {
                *prev = (range32){s2, e2};
            } else if (nhl < CAPO_MAX_HLS) {
                hls[nhl++] = (range32){s2, e2};
            }
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

// --- Spot per-file callback ---

typedef struct {
    u8cs needle;
    u8cs replace;
    u8cs ext;
    int  total_replacements;
    int  total_files_replaced;
} capo_spot_ctx;

static ok64 capo_spot_file_cb(void *ctx, u8csc relpath, u8csc source,
                               u8csc file_ext, u8bp mapped, path8p fpbuf) {
    sane(ctx != NULL);
    capo_spot_ctx *sc = ctx;

    Bu32 toks = {};
    ok64 o = u32bMap(toks, $len(source) + 1);
    if (o != OK) { FILEUnMap(mapped); return OK; }
    o = SPOTTokenize(toks, source, file_ext);
    if (o != OK) { u32bUnMap(toks); FILEUnMap(mapped); return OK; }
    {
        u32 *dts[2] = {u32bDataHead(toks), u32bIdleHead(toks)};
        a_dup(u8c,dext,file_ext);
        if (!$empty(dext) && dext[0][0] == '.') dext[0]++;
        DEFMark(dts, source, dext);
    }
    u32cs htoks = {(u32cp)u32bDataHead(toks),
                   (u32cp)u32bIdleHead(toks)};

    char rpz[FILE_PATH_MAX_LEN] = {};
    size_t rlen = (size_t)$len(relpath);
    if (rlen >= sizeof(rpz)) rlen = sizeof(rpz) - 1;
    memcpy(rpz, relpath[0], rlen);

    signal(SIGABRT, capo_abrt_handler);
    capo_in_match = 1;
    if (sigsetjmp(capo_jmpbuf, 1) == 0) {
        if (!$empty(sc->replace))
            CAPOSpotReplace(source, mapped, htoks, sc->needle,
                            sc->replace, file_ext, fpbuf, rpz,
                            &sc->total_replacements,
                            &sc->total_files_replaced);
        else
            CAPOSpotFile(source, htoks, sc->needle, file_ext, rpz);
    }
    capo_in_match = 0;
    signal(SIGABRT, SIG_DFL);

    if ($empty(sc->replace)) {
        LESSDefer(mapped, toks);
    } else {
        u32bUnMap(toks);
        FILEUnMap(mapped);
    }
    return OK;
}

ok64 CAPOSpot(u8csc needle, u8csc replace, u8csc ext, u8csc reporoot,
              u8css files) {
    sane($ok(needle) && $ok(ext) && $ok(reporoot));

    if (!CAPOKnownExt(ext)) return SPOTBAD;

    Bu32 hashbuf1 = {};
    b8 has_trigrams = NO;
    if ($len(files) == 0)
        CAPOTrigramFilter(hashbuf1, &has_trigrams, needle, reporoot);

    if ($empty(replace))
        call(LESSArenaInit);

    capo_spot_ctx sc = {};
    $mv(sc.needle, needle);
    $mv(sc.replace, replace);
    $mv(sc.ext, ext);

    CAPOScanOpts opts = {
        .has_trigrams = has_trigrams,
        .file_fn = capo_spot_file_cb,
        .file_ctx = &sc,
    };
    if (!$empty(ext)) $mv(opts.target_ext, ext);
    if (has_trigrams) $mv(opts.tri_hashes, u32bDataC(hashbuf1));

    if ($len(files) > 0)
        CAPOScanFiles(files, &opts);
    else
        CAPOScan(reporoot, &opts);

    CAPOProgress(NULL);

    if ($empty(replace) && less_nhunks > 0)
        LESSRun(less_hunks, less_nhunks);
    if ($empty(replace))
        LESSArenaCleanup();

    if (!$empty(replace))
        fprintf(stderr, "%d replacements in %d files\n",
                sc.total_replacements, sc.total_files_replaced);

    if (!BNULL(hashbuf1)) u32bUnMap(hashbuf1);
    done;
}

// --- File scan ---

typedef struct {
    CAPOScanOpts const *opts;
    u8cs root;
    size_t root_len;
    igno ig;
    b8   has_igno;
} capo_scan_st;

static ok64 capo_scan_cb(void *arg, path8p path) {
    capo_scan_st *st = arg;
    CAPOScanOpts const *opts = st->opts;
    u8cs data = {};
    $mv(data, u8bDataC(path));
    if ($empty(data)) return OK;

    // Directory: trailing '/' appended by FILEScan
    if (data[1][-1] == '/') {
        // Extract basename (between last two '/' or start)
        u8cp p = data[1] - 2;  // before trailing /
        while (p > data[0] && p[-1] != '/') p--;
        // Skip hidden dirs (.git, .dogs, etc.)
        if (*p == '.') return FILESKIP;
        // Check .gitignore
        if (st->has_igno) {
            u8cs rel = {data[0] + st->root_len, data[1] - 1};
            if (!$empty(rel) && rel[0][0] == '/')
                u8csFed(rel, 1);
            if (!$empty(rel) && IGNOMatch(&st->ig, rel, YES))
                return FILESKIP;
        }
        return OK;
    }

    // File: apply filters
    size_t pathlen = (size_t)$len(data);
    u8cs file_ext = {};
    HUNKu8sExt(file_ext, data[0], pathlen);
    if ($empty(file_ext)) return OK;
    if (!CAPOKnownExt(file_ext)) return OK;
    if (!$empty(opts->target_ext)) {
        if (!TOKSameLexer(file_ext, opts->target_ext)) return OK;
    }

    // Compute relpath
    u8cs relpath = {data[0] + st->root_len, data[1]};
    if (!$empty(relpath) && relpath[0][0] == '/')
        u8csFed(relpath, 1);

    // Check .gitignore
    if (st->has_igno && IGNOMatch(&st->ig, relpath, NO))
        return OK;

    // Trigram filter
    if (opts->has_trigrams && !$empty(opts->tri_hashes)) {
        u32 phash = CAPOPathHash(relpath);
        u32s th = {(u32p)opts->tri_hashes[0], (u32p)opts->tri_hashes[1]};
        if (!u32sBsearch(&phash, th))
            return OK;
    }

    // NUL-terminated relpath for progress
    char relpathz[FILE_PATH_MAX_LEN] = {};
    size_t rlen = (size_t)$len(relpath);
    if (rlen >= sizeof(relpathz)) rlen = sizeof(relpathz) - 1;
    memcpy(relpathz, relpath[0], rlen);
    CAPOProgress(relpathz);

    // Map file
    u8bp mapped = NULL;
    ok64 o = FILEMapRO(&mapped, $path(path));
    if (o != OK) return OK;  // skip unreadable files

    u8cs source = {};
    $mv(source, u8bDataC(mapped));

    o = opts->file_fn(opts->file_ctx, relpath, source,
                      file_ext, mapped, path);
    if (o != OK) {
        FILEUnMap(mapped);
        return o;
    }
    return OK;
}

ok64 CAPOScan(u8csc reporoot, CAPOScanOpts const *opts) {
    sane(!$empty(reporoot) && opts != NULL && opts->file_fn != NULL);

    capo_scan_st st = {
        .opts = opts,
    };
    $mv(st.root, reporoot);
    st.root_len = (size_t)$len(reporoot);

    // Load .gitignore
    u8cs igroot = {(u8cp)reporoot[0], (u8cp)reporoot[1]};
    if (IGNOLoad(&st.ig, igroot) == OK)
        st.has_igno = YES;

    a_path(wp);
    call(PATHu8bFeed, wp, reporoot);

    ok64 o = FILEScan(wp, (FILE_SCAN)(FILE_SCAN_ALL | FILE_SCAN_DEEP),
                      capo_scan_cb, &st);

    CAPOProgress(NULL);
    if (st.has_igno) IGNOFree(&st.ig);

    if (o == FILESKIP) o = OK;  // normal completion
    return o;
}

ok64 CAPOScanFiles(u8css files, CAPOScanOpts const *opts) {
    sane(opts != NULL && opts->file_fn != NULL);
    int nf = (int)$len(files);
    for (int i = 0; i < nf; i++) {
        u8cs *fp = u8cssAtP(files, i);
        size_t flen = (size_t)$len(*fp);
        if (flen == 0) continue;

        char line[FILE_PATH_MAX_LEN] = {};
        if (flen >= sizeof(line)) continue;
        memcpy(line, (*fp)[0], flen);

        u8cs file_ext = {};
        HUNKu8sExt(file_ext, (u8cp)line, flen);
        if ($empty(file_ext)) continue;
        if (!CAPOKnownExt(file_ext)) continue;
        if (!$empty(opts->target_ext)) {
            if (!TOKSameLexer(file_ext, opts->target_ext)) continue;
        }

        CAPOProgress(line);

        a_path(fpbuf);
        u8cs fps = {(u8cp)line, (u8cp)line + flen};
        if (PATHu8bFeed(fpbuf, fps) != OK) continue;

        u8bp mapped = NULL;
        if (FILEMapRO(&mapped, $path(fpbuf)) != OK) continue;

        u8cs source = {};
        $mv(source, u8bDataC(mapped));
        u8cs relpath = {(u8cp)line, (u8cp)line + flen};

        ok64 o = opts->file_fn(opts->file_ctx, relpath, source,
                               file_ext, mapped, fpbuf);
        if (o != OK) {
            FILEUnMap(mapped);
            if (o != OK) return o;
        }
    }
    CAPOProgress(NULL);
    return OK;
}

// --- Trigram filter ---

ok64 CAPOTrigramFilter(Bu32 hashbuf, b8 *has_trigrams,
                        u8csc text, u8csc reporoot) {
    sane(hashbuf != NULL && has_trigrams != NULL);
    *has_trigrams = NO;

    a_path(capodir);
    ok64 ro = CAPOResolveDir(capodir, reporoot);
    if (ro != OK) return OK;  // no index, skip filtering
    a_dup(u8c, dirslice, u8bDataC(capodir));

    size_t maxhashes = 1ULL << 28;
    call(u32bMap, hashbuf, maxhashes);

    u64cs runs[CAPO_MAX_LEVELS] = {};
    u64css stack = {runs, runs};
    u8bp mmaps[CAPO_MAX_LEVELS] = {};
    u32 nidxfiles = 0;
    call(CAPOStackOpen, stack, mmaps, &nidxfiles, dirslice);
    stack[1] = stack[0] + nidxfiles;

    if (nidxfiles == 0) {
        CAPOStackClose(mmaps, nidxfiles);
        return OK;
    }

    u8cp p = text[0];
    u8cp end = text[1] - 2;
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
            HITu64Start(seek_iter);

            if (!*has_trigrams) {
                u32bReset(hashbuf);
                CAPOCollectPaths(seek_iter, tri_prefix,
                                 u32bDataIdle(hashbuf));
                *has_trigrams = YES;
            } else {
                u32sSort(u32bData(hashbuf));
                CAPOFilterInPlace(hashbuf, seek_iter, tri_prefix);
            }
        }
        p++;
    }

    CAPOStackClose(mmaps, nidxfiles);

    if (*has_trigrams && u32bDataLen(hashbuf) > 0)
        u32sSort(u32bData(hashbuf));

    return OK;
}

// --- DOG control struct ---

ok64 SPOTOpen(spotp s, home *h, b8 rw) {
    sane(s != NULL && h != NULL);
    zerop(s);
    s->h = h;
    s->lock_fd = -1;
    s->out_fd = -1;
    s->color = isatty(STDOUT_FILENO) ? YES : NO;
    s->term = (isatty(STDERR_FILENO) && isatty(STDOUT_FILENO)) ? YES : NO;

    // Worktree sharing: `.dogs/spot` may be a symlink.  flock on
    // `.dogs/spot/.lock` serializes writers; readers share.
    {
        a_dup(u8c, root_s, u8bDataC(h->root));
        a_cstr(rel, ".dogs/spot");
        a_path(dir, root_s, rel);
        if (rw) call(FILEMakeDirP, $path(dir));
        a_cstr(lockrel, ".lock");
        a_path(lockpath, $path(dir), lockrel);
        call(FILECreate, &s->lock_fd, $path(lockpath));
        call(FILELock, &s->lock_fd, rw);
    }

    call(u8bMap, s->arena, LESS_ARENA_SIZE);

    less_arena[0] = s->arena[0];
    less_arena[1] = s->arena[1];
    less_arena[2] = s->arena[2];
    less_arena[3] = s->arena[3];
    CAPO_COLOR = s->color;
    CAPO_TERM  = s->term;

    done;
}

void SPOTClose(spotp s) {
    if (s == NULL) return;
    for (u32 i = 0; i < s->nmaps; i++) {
        if (s->toks[i][0] != NULL) u32bUnMap(s->toks[i]);
        if (s->maps[i] != NULL)    FILEUnMap(s->maps[i]);
    }
    if (s->arena[0]) u8bUnMap(s->arena);
    if (s->lock_fd >= 0) FILEClose(&s->lock_fd);
    s->nhunks = 0;
    s->nmaps  = 0;
    less_nhunks = 0;
    less_nmaps  = 0;
}
