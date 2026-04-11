//  BELT: ersatz git repository format
//
#include "BELT.h"

#include "DELT.h"
#include "GIT.h"
#include "PACK.h"
#include "PKT.h"
#include "SHA1.h"
#include "ZINF.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PRO.h"
#include "abc/RON.h"

// Full type instantiation (Bx.h pulls in Sx.h)
#define X(M, name) M##belt128##name
#include "abc/Bx.h"
#undef X

// QSORT: in-buffer sort/dedup of belt128 entries.
#define X(M, name) M##belt128##name
#include "abc/QSORTx.h"
#undef X

// HIT: heap iterator over a stack of sorted runs (used by MSETx).
fun void belt128csSwap(belt128cs *a, belt128cs *b) {
    belt128c *t0 = (*a)[0], *t1 = (*a)[1];
    (*a)[0] = (*b)[0]; (*a)[1] = (*b)[1];
    (*b)[0] = t0; (*b)[1] = t1;
}

#define X(M, name) M##belt128##name
#include "abc/HITx.h"
#undef X

// MSET: search/merge/compact across a stack of sorted run files.
#define X(M, name) M##belt128##name
#include "abc/MSETx.h"
#undef X

static ok64 belt_index_log(u8cs belt_dir, char const *idx_path);

belt128cp BELTLookup(belt128css stack, u64 hashlet) {
    for (size_t r = 0; r < (size_t)$len(stack); r++) {
        belt128cp base = stack[0][r][0];
        u64 len = stack[0][r][1] - stack[0][r][0];
        // binary search for hashlet (masking type bits)
        u64 lo = 0, hi = len;
        while (lo < hi) {
            u64 mid = lo + (hi - lo) / 2;
            u64 mkey = base[mid].a & ~BELT_TYPE_MASK;
            if (mkey < hashlet) lo = mid + 1;
            else hi = mid;
        }
        if (lo < len && (base[lo].a & ~BELT_TYPE_MASK) == hashlet)
            return &base[lo];
    }
    return NULL;
}

static char const *belt_type_str[] = {
    [BELT_COMMIT] = "commit",
    [BELT_TREE] = "tree",
    [BELT_BLOB] = "blob",
    [BELT_TAG] = "tag",
};

u32 BELTCommitGen(u8cp content, u64 sz, belt128css stack) {
    u32 gen = 1;
    u8cs body = {content, content + sz};
    u8cs field = {}, value = {};
    while (GITu8sDrainCommit(body, field, value) == OK) {
        if ($empty(field)) break;
        if ($len(field) != 6 || memcmp(field[0], "parent", 6) != 0)
            continue;
        if ($len(value) < 40) continue;
        u8 psha[20];
        memset(psha, 0, 20);
        u8s pbin = {psha, psha + 20};
        u8cs phex = {value[0], value[0] + 40};
        if (HEXu8sDrainSome(pbin, phex) != OK) continue;
        u64 needle = 0;
        memcpy(&needle, psha, 8);
        needle &= ~BELT_TYPE_MASK;
        belt128cp found = BELTLookup(stack, needle);
        if (found) {
            u32 pgen = BELTGen(*found);
            if (pgen >= gen) gen = pgen + 1;
        }
    }
    return gen;
}

// --- Index I/O ---

ok64 BELTIndexWrite(u8cs idx_dir, belt128cs run, u64 seqno) {
    sane($ok(idx_dir));
    if ($empty(run)) done;

    a_pad(u8, path, 1024);
    call(u8bFeed, path, idx_dir);
    call(u8bFeed1, path, '/');
    call(RONu8sFeedPad, u8bIdle(path), seqno, BELT_SEQNO_WIDTH);
    ((u8 **)path)[2] += BELT_SEQNO_WIDTH;
    a_cstr(ext, BELT_IDX_EXT);
    call(u8bFeed, path, ext);
    call(PATHu8gTerm, PATHu8gIn(path));

    int fd = -1;
    call(FILECreate, &fd, PATHu8cgIn(path));
    size_t bytes = $len(run) * sizeof(belt128);
    u8cs data = {(u8cp)run[0], (u8cp)run[0] + bytes};
    call(FILEFeedAll, fd, data);
    close(fd);
    done;
}

ok64 BELTNextSeqno(u64p seqno, u8cs idx_dir) {
    sane(seqno != NULL && $ok(idx_dir));
    *seqno = 1;

    a_pad(u8, pat, 1024);
    call(PATHu8bFeed, pat, idx_dir);

    int dfd = -1;
    ok64 o = FILEOpenDir(&dfd, PATHu8cgIn(pat));
    if (o != OK) done;

    u64 maxseq = 0;
    DIR *d = fdopendir(dfd);
    if (!d) { close(dfd); done; }

    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        size_t nlen = strlen(e->d_name);
        if (nlen < 5) continue;
        if (strcmp(e->d_name + nlen - 4, BELT_IDX_EXT) != 0) continue;
        u64 seq = 0;
        for (size_t i = 0; i < nlen - 4; i++) {
            char c = e->d_name[i];
            if (c < '0' || c > '9') { seq = 0; break; }
            seq = seq * 10 + (c - '0');
        }
        if (seq > maxseq) maxseq = seq;
    }
    closedir(d);
    *seqno = maxseq + 1;
    done;
}

// --- Delta resolution ---

ok64 BELTResolve(u8cp pack, u64 packlen, belt128css stack,
                 u64 off, u8p out_type,
                 u8p buf1, u8p buf2, u64 bufsz,
                 u8pp result, u64p outsz) {
    sane(pack && buf1 && buf2 && result && outsz);

    u64 chain[256];
    int depth = 0;
    u64 cur = off;

    for (;;) {
        pack_obj obj = {};
        u8cs from = {pack + cur, pack + packlen};
        call(PACKDrainObjHdr, from, &obj);

        if (obj.type >= 1 && obj.type <= 4) {
            *out_type = obj.type;
            if (obj.size > bufsz) return NOROOM;
            u8s into = {buf1, buf1 + bufsz};
            call(PACKInflate, from, into, obj.size);
            *result = buf1;
            *outsz = obj.size;
            break;
        }

        if (depth >= 256) return BELTFAIL;
        chain[depth++] = cur;

        if (obj.type == PACK_OBJ_OFS_DELTA) {
            cur = cur - obj.ofs_delta;
        } else if (obj.type == PACK_OBJ_REF_DELTA) {
            u64 sha_key = 0;
            memcpy(&sha_key, obj.ref_delta[0], 8);
            u64 needle = sha_key & ~BELT_TYPE_MASK;

            // look up base in MSET stack by hashlet
            belt128cp found = BELTLookup(stack, needle);
            if (!found) return BELTNONE;
            cur = BELTOffset(*found);
        } else {
            return BELTFAIL;
        }
    }

    u8p src = buf1;
    u8p dst = buf2;

    for (int i = depth - 1; i >= 0; i--) {
        pack_obj dobj = {};
        u8cs from = {pack + chain[i], pack + packlen};
        call(PACKDrainObjHdr, from, &dobj);

        u8p dinst = dst + bufsz / 2;
        if (dobj.size > bufsz / 2) return NOROOM;
        u8s dinto = {dinst, dinst + bufsz / 2};
        call(PACKInflate, from, dinto, dobj.size);

        u8cs delta = {dinst, dinst + dobj.size};
        u8cs base = {src, src + *outsz};
        u8g out = {dst, dst, dst + bufsz / 2};
        call(DELTApply, delta, base, out);
        *outsz = u8gLeftLen(out);

        u8p tmp = src; src = dst; dst = tmp;
    }

    *result = src;
    done;
}

// --- Clone ---

#define BELT_BATCH (1 << 21)  // 2M entries per index batch (32MB)
#define BELT_BUFSZ (1 << 26)  // 64 MB working buffer

ok64 BELTClone(u8cs repo_path, u8cs belt_dir) {
    sane($ok(repo_path) && $ok(belt_dir));

    // create .belt/ and .belt/objects.idx/
    {
        a_pad(u8, p, 1024);
        call(u8bFeed, p, belt_dir);
        call(PATHu8gTerm, PATHu8gIn(p));
        call(FILEMakeDirP, PATHu8cgIn(p));
    }
    char idx_path[1024];
    {
        a_pad(u8, p, 1024);
        call(u8bFeed, p, belt_dir);
        a_cstr(sub, "/objects.idx");
        call(u8bFeed, p, sub);
        call(PATHu8gTerm, PATHu8gIn(p));
        call(FILEMakeDirP, PATHu8cgIn(p));
        snprintf(idx_path, sizeof(idx_path), "%.*s",
                 (int)u8bDataLen(p), (char *)u8bDataHead(p));
    }

    // pipe to git-upload-pack
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "git-upload-pack '%.*s'",
             (int)$len(repo_path), (char *)repo_path[0]);

    int to_child[2], from_child[2];
    if (pipe(to_child) != 0) failc(BELTFAIL);
    if (pipe(from_child) != 0) failc(BELTFAIL);

    pid_t pid = fork();
    if (pid < 0) failc(BELTFAIL);

    if (pid == 0) {
        close(to_child[1]);
        close(from_child[0]);
        dup2(to_child[0], 0);
        dup2(from_child[1], 1);
        close(to_child[0]);
        close(from_child[1]);
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        _exit(127);
    }

    close(to_child[0]);
    close(from_child[1]);
    int wfd = to_child[1];
    int rfd = from_child[0];

    // ref advertisement
    u8 advbuf[1 << 20];
    u64 advlen = 0;
    {
        ssize_t n = read(rfd, advbuf, sizeof(advbuf));
        if (n <= 0) fail(BELTFAIL);
        advlen = (u64)n;
    }

    u8cs adv = {advbuf, advbuf + advlen};
    u8cs line = {};
    call(PKTu8sDrain, adv, line);
    if ($len(line) < 40) fail(PKTBADFMT);

    u8 head_hex[41];
    memcpy(head_hex, line[0], 40);
    head_hex[40] = 0;

    for (;;) {
        try(PKTu8sDrain, adv, line);
        on(PKTFLUSH) break;
        on(NODATA) {
            ssize_t n = read(rfd, advbuf + advlen, sizeof(advbuf) - advlen);
            if (n <= 0) fail(BELTFAIL);
            advlen += (u64)n;
            adv[1] = advbuf + advlen;
            continue;
        }
        nedo fail(__);
    }

    fprintf(stderr, "HEAD=%.40s (adv %lu bytes)\n", head_hex, (unsigned long)advlen);

    // want + done
    {
        u8 wbuf[512];
        u8s ws = {wbuf, wbuf + sizeof(wbuf)};

        u8 pktpay[256];
        int plen = snprintf((char *)pktpay, sizeof(pktpay),
                            "want %.40s no-progress\n", head_hex);
        u8cs payload = {pktpay, pktpay + plen};
        call(PKTu8sFeed, ws, payload);
        call(PKTu8sFeedFlush, ws);

        u8 donepay[] = "done\n";
        u8cs donecs = {donepay, donepay + 5};
        call(PKTu8sFeed, ws, donecs);

        u64 wlen = ws[0] - wbuf;
        if (write(wfd, wbuf, wlen) != (ssize_t)wlen) fail(BELTFAIL);
    }
    close(wfd);
    wfd = -1;

    // stream packfile to objects.log
    int logfd = -1;
    {
        a_pad(u8, lp, 1024);
        call(u8bFeed, lp, belt_dir);
        a_cstr(sub, "/objects.log");
        call(u8bFeed, lp, sub);
        call(PATHu8gTerm, PATHu8gIn(lp));
        call(FILECreate, &logfd, PATHu8cgIn(lp));
    }

    // NAK
    u8 nackbuf[64];
    u64 nacklen = 0;
    {
        ssize_t n = read(rfd, nackbuf, sizeof(nackbuf));
        if (n <= 0) fail(BELTFAIL);
        nacklen = (u64)n;
    }
    u8cs nack = {nackbuf, nackbuf + nacklen};
    call(PKTu8sDrain, nack, line);
    if ($len(line) < 3 || memcmp(line[0], "NAK", 3) != 0) fail(BELTFAIL);

    fprintf(stderr, "NAK ok, nack remainder: %lu bytes\n",
            (unsigned long)(nack[1] - nack[0]));

    u64 loglen = 0;
    if (nack[0] < nack[1]) {
        u64 rem = nack[1] - nack[0];
        if (write(logfd, nack[0], rem) != (ssize_t)rem) fail(BELTFAIL);
        loglen += rem;
    }
    fprintf(stderr, "streaming pack...\n");
    {
        u8 iobuf[1 << 16];
        for (;;) {
            ssize_t n = read(rfd, iobuf, sizeof(iobuf));
            if (n <= 0) break;
            if (write(logfd, iobuf, n) != n) fail(BELTFAIL);
            loglen += (u64)n;
            if (loglen < (1 << 20) || (loglen % (1 << 26)) < (1 << 16))
                fprintf(stderr, "  %lu bytes\r", (unsigned long)loglen);
        }
    }
    close(rfd);
    call(FILEClose, &logfd);
    fprintf(stderr, "objects.log: %lu bytes\n", (unsigned long)loglen);

    call(belt_index_log, belt_dir, idx_path);

    // save HEAD
    {
        char hpath[1024];
        snprintf(hpath, sizeof(hpath), "%.*s/HEAD",
                 (int)$len(belt_dir), (char *)belt_dir[0]);
        FILE *f = fopen(hpath, "w");
        if (!f) failc(BELTFAIL);
        fprintf(f, "%.40s\n", head_hex);
        fclose(f);
    }

    { int status; waitpid(pid, &status, 0); }
    done;
}

// --- Index a packfile in objects.log ---

static ok64 belt_index_log(u8cs belt_dir, char const *idx_path) {
    sane($ok(belt_dir));

    u8bp logmap = NULL;
    {
        a_pad(u8, lp, 1024);
        call(u8bFeed, lp, belt_dir);
        a_cstr(sub, "/objects.log");
        call(u8bFeed, lp, sub);
        call(PATHu8gTerm, PATHu8gIn(lp));
        call(FILEMapRO, &logmap, PATHu8cgIn(lp));
    }

    u8cp pack = u8bDataHead(logmap);
    u64 packlen = u8bDataLen(logmap);

    u8cs phdr = {pack, pack + packlen};
    pack_hdr hdr = {};
    call(PACKDrainHdr, phdr, &hdr);
    if (hdr.version != 2) fail(BELTBADFMT);
    fprintf(stderr, "pack: %u objects\n", hdr.count);

    // scan: record offsets
    u64 *offsets = malloc(hdr.count * sizeof(u64));
    u8  *otypes  = malloc(hdr.count);
    if (!offsets || !otypes) fail(BELTFAIL);

    u8p scr = malloc(BELT_BUFSZ);
    if (!scr) fail(BELTFAIL);

    u8cs sc = {phdr[0], pack + packlen};
    for (u32 i = 0; i < hdr.count; i++) {
        offsets[i] = sc[0] - pack;
        pack_obj obj = {};
        call(PACKDrainObjHdr, sc, &obj);
        otypes[i] = obj.type;
        u64 consumed = 0, produced = 0;
        u64 cap = obj.size < BELT_BUFSZ ? obj.size : BELT_BUFSZ;
        ZINFInflate(sc[0], $size(sc), scr, cap, &consumed, &produced);
        sc[0] += consumed;
    }

    fprintf(stderr, "scan done\n");

    // --- MSET-based indexing ---
    u8p buf1 = malloc(BELT_BUFSZ);
    u8p buf2 = malloc(BELT_BUFSZ);
    u8p objbuf = malloc(BELT_BUFSZ);
    if (!buf1 || !buf2 || !objbuf) fail(BELTFAIL);

    // write buffer: 512K entries = 8MB
    belt128 *wbacking = malloc(BELT_BATCH * sizeof(belt128));
    if (!wbacking) fail(BELTFAIL);
    belt128g wbuf = {wbacking, wbacking, wbacking + BELT_BATCH};

    // run stack + mmapped run storage
    belt128cs runs[BELT_MAX_LEVELS] = {};
    belt128css stack = {runs, runs};
    u8bp maps[BELT_MAX_LEVELS] = {};
    u32 nmaps = 0;
    u64 seqno = 1;

    u32 total_indexed = 0;
    u32 skipped = 0;

    // Sort + dedup the in-memory write buffer in place.  No-op if empty.
    #define BELT_SORT_DEDUP() do {                                          \
        if (belt128gLeftLen(wbuf) > 0) {                                    \
            belt128s d = {wbuf[0], wbuf[1]};                                \
            belt128sSort(d);                                                \
            belt128sDedup(d);                                               \
            wbuf[1] = d[1];                                                 \
        }                                                                   \
    } while(0)

    // Write the current (assumed already sorted+deduped) wbuf to disk as
    // a new sorted run, mmap it back, push it onto the LSM stack, reset
    // the write buffer.  No-op if empty.
    #define BELT_WRITE_RUN() do {                                           \
        if (belt128gLeftLen(wbuf) > 0) {                                    \
            belt128cs flushed = {wbuf[0], wbuf[1]};                         \
            u8cs idxdir = $u8str(idx_path);                                 \
            BELTIndexWrite(idxdir, flushed, seqno);                         \
            a_pad(u8, fp, 1024);                                            \
            u8bFeed(fp, idxdir);                                            \
            u8bFeed1(fp, '/');                                              \
            RONu8sFeedPad(u8bIdle(fp), seqno, BELT_SEQNO_WIDTH);           \
            ((u8 **)fp)[2] += BELT_SEQNO_WIDTH;                            \
            a_cstr(ext, BELT_IDX_EXT);                                      \
            u8bFeed(fp, ext);                                               \
            PATHu8gTerm(PATHu8gIn(fp));                                     \
            FILEMapRO(&maps[nmaps], PATHu8cgIn(fp));                        \
            belt128cp base = (belt128cp)u8bDataHead(maps[nmaps]);           \
            u64 len = u8bDataLen(maps[nmaps]) / sizeof(belt128);            \
            stack[1][-1][0] = base;                                         \
            stack[1][-1][1] = base + len;                                   \
            nmaps++;                                                        \
            seqno++;                                                        \
            wbuf[0] = wbuf[1] = wbacking;                                  \
        }                                                                   \
    } while(0)

    // Per-iteration check: when wbuf fills, sort+dedup; if dedup didn't
    // recover at least half the buffer, flush it as a new on-disk run.
    #define BELT_MAYBE_FLUSH() do {                                         \
        if (belt128gRestLen(wbuf) == 0) {                                   \
            BELT_SORT_DEDUP();                                              \
            size_t _left = belt128gLeftLen(wbuf);                           \
            size_t _cap = (size_t)(wbuf[2] - wbuf[0]);                      \
            if (_left * 2 > _cap) BELT_WRITE_RUN();                         \
        }                                                                   \
    } while(0)

    // End-of-pass flush: always sort+dedup whatever remains and write.
    #define BELT_FINAL_FLUSH() do {                                         \
        BELT_SORT_DEDUP();                                                  \
        BELT_WRITE_RUN();                                                   \
    } while(0)

    // pass 1: index base objects (type 1-4)
    for (u32 i = 0; i < hdr.count; i++) {
        if (otypes[i] < 1 || otypes[i] > 4) continue;

        pack_obj obj = {};
        u8cs from = {pack + offsets[i], pack + packlen};
        try(PACKDrainObjHdr, from, &obj);
        nedo { skipped++; continue; }
        if (obj.size > BELT_BUFSZ) { skipped++; continue; }
        u8s into = {buf1, buf1 + BELT_BUFSZ};
        try(PACKInflate, from, into, obj.size);
        nedo { skipped++; continue; }

        char hbuf[64];
        int hlen = snprintf(hbuf, sizeof(hbuf), "%s %lu",
                            belt_type_str[obj.type],
                            (unsigned long)obj.size);
        memcpy(objbuf, hbuf, hlen);
        objbuf[hlen] = 0;
        memcpy(objbuf + hlen + 1, buf1, obj.size);

        u8 sha[20];
        SHA1Sum(sha, objbuf, hlen + 1 + obj.size);

        u32 gen = 0;
        if (obj.type == PACK_OBJ_COMMIT)
            gen = BELTCommitGen(buf1, obj.size, stack);
        belt128 entry = BELTEntry(
            BELTSha1Key(sha, obj.type), obj.type, offsets[i], gen);
        *wbuf[1]++ = entry;
        total_indexed++;

        BELT_MAYBE_FLUSH();
    }
    BELT_FINAL_FLUSH();
    fprintf(stderr, "base: %u objects, %u runs\n", total_indexed, nmaps);

    // pass 2+: resolve deltas using MSET stack for lookups
    b8 *done_flag = calloc(hdr.count, 1);
    if (!done_flag) fail(BELTFAIL);
    for (u32 i = 0; i < hdr.count; i++)
        if (otypes[i] >= 1 && otypes[i] <= 4) done_flag[i] = 1;

    for (int pass = 0; pass < 64; pass++) {
        u32 progress = 0;

        for (u32 i = 0; i < hdr.count; i++) {
            if (done_flag[i]) continue;

            u8 obj_type = 0;
            u8p content = NULL;
            u64 content_sz = 0;

            try(BELTResolve, pack, packlen, stack,
                offsets[i], &obj_type,
                buf1, buf2, BELT_BUFSZ,
                &content, &content_sz);
            nedo continue;

            if (obj_type < 1 || obj_type > 4) { done_flag[i] = 1; continue; }

            char hbuf[64];
            int hlen = snprintf(hbuf, sizeof(hbuf), "%s %lu",
                                belt_type_str[obj_type],
                                (unsigned long)content_sz);
            memcpy(objbuf, hbuf, hlen);
            objbuf[hlen] = 0;
            memcpy(objbuf + hlen + 1, content, content_sz);

            u8 sha[20];
            SHA1Sum(sha, objbuf, hlen + 1 + content_sz);

            u32 gen = 0;
            if (obj_type == BELT_COMMIT)
                gen = BELTCommitGen(content, content_sz, stack);
            belt128 entry = BELTEntry(
                BELTSha1Key(sha, obj_type), obj_type, offsets[i], gen);
            *wbuf[1]++ = entry;
            total_indexed++;

            BELT_MAYBE_FLUSH();

            done_flag[i] = 1;
            progress++;
        }

        BELT_FINAL_FLUSH();

        fprintf(stderr, "pass %d: +%u (%u/%u) %u runs\n",
                pass, progress, total_indexed, hdr.count, nmaps);
        if (progress == 0) break;
    }
    free(done_flag);
    #undef BELT_SORT_DEDUP
    #undef BELT_WRITE_RUN
    #undef BELT_MAYBE_FLUSH
    #undef BELT_FINAL_FLUSH

    fprintf(stderr, "indexed %u, skipped %u\n", total_indexed, skipped);

    for (u32 i = 0; i < nmaps; i++) FILEUnMap(maps[i]);
    free(wbacking);
    free(objbuf);
    free(buf2);
    free(buf1);
    free(scr);
    free(otypes);
    free(offsets);
    call(FILEUnMap, logmap);

    fprintf(stderr, "done\n");
    done;
}

// --- Import: stream packfile into belt ---

ok64 BELTImport(u8cs pack_path, u8cs belt_dir) {
    sane($ok(pack_path) && $ok(belt_dir));

    // create dirs
    {
        a_pad(u8, p, 1024);
        call(u8bFeed, p, belt_dir);
        call(PATHu8gTerm, PATHu8gIn(p));
        call(FILEMakeDirP, PATHu8cgIn(p));
    }
    char idx_path[1024];
    {
        a_pad(u8, p, 1024);
        call(u8bFeed, p, belt_dir);
        a_cstr(sub, "/objects.idx");
        call(u8bFeed, p, sub);
        call(PATHu8gTerm, PATHu8gIn(p));
        call(FILEMakeDirP, PATHu8cgIn(p));
        snprintf(idx_path, sizeof(idx_path), "%.*s",
                 (int)u8bDataLen(p), (char *)u8bDataHead(p));
    }

    // open source pack
    int srcfd = -1;
    {
        a_pad(u8, sp, 1024);
        call(u8bFeed, sp, pack_path);
        call(PATHu8gTerm, PATHu8gIn(sp));
        call(FILEOpen, &srcfd, PATHu8cgIn(sp), O_RDONLY);
    }

    // stream into objects.log
    int logfd = -1;
    {
        a_pad(u8, lp, 1024);
        call(u8bFeed, lp, belt_dir);
        a_cstr(sub, "/objects.log");
        call(u8bFeed, lp, sub);
        call(PATHu8gTerm, PATHu8gIn(lp));
        call(FILECreate, &logfd, PATHu8cgIn(lp));
    }

    u64 loglen = 0;
    {
        u8 iobuf[1 << 16];
        for (;;) {
            ssize_t n = read(srcfd, iobuf, sizeof(iobuf));
            if (n <= 0) break;
            if (write(logfd, iobuf, n) != n) fail(BELTFAIL);
            loglen += (u64)n;
        }
    }
    close(srcfd);
    call(FILEClose, &logfd);
    fprintf(stderr, "objects.log: %lu bytes\n", (unsigned long)loglen);

    call(belt_index_log, belt_dir, idx_path);
    done;
}

// --- Get ---

ok64 BELTGet(u8cs belt_dir, u8cs hex_hash, u8g out, u8p out_type) {
    sane($ok(belt_dir) && $ok(hex_hash) && u8gOK(out) && out_type);

    // parse hex to binary SHA-1
    u8 sha_bin[20];
    memset(sha_bin, 0, 20);
    u64 hexlen = $len(hex_hash);
    if (hexlen < 8 || hexlen > 40) return BELTFAIL;
    for (u64 i = 0; i < hexlen; i++) {
        u8 c = hex_hash[0][i];
        u8 v = 0;
        if (c >= '0' && c <= '9') v = c - '0';
        else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
        else return BELTFAIL;
        sha_bin[i / 2] |= (i & 1) ? v : (v << 4);
    }

    u64 needle = 0;
    memcpy(&needle, sha_bin, 8);
    needle &= ~BELT_TYPE_MASK;

    // load index files, find matching entry
    char idx_path[1024];
    snprintf(idx_path, sizeof(idx_path), "%.*s/objects.idx",
             (int)$len(belt_dir), (char *)belt_dir[0]);

    // load index runs into MSET stack
    u8bp maps[BELT_MAX_LEVELS] = {};
    belt128cs runs[BELT_MAX_LEVELS] = {};
    belt128css stack = {runs, runs};
    u32 nfiles = 0;

    {
        DIR *d = opendir(idx_path);
        if (!d) return BELTNONE;

        char names[BELT_MAX_LEVELS][64];
        u32 count = 0;
        struct dirent *e;
        while ((e = readdir(d)) != NULL && count < BELT_MAX_LEVELS) {
            size_t nlen = strlen(e->d_name);
            if (nlen < 5 || nlen > 63) continue;
            if (strcmp(e->d_name + nlen - 4, BELT_IDX_EXT) != 0) continue;
            memcpy(names[count], e->d_name, nlen + 1);
            count++;
        }
        closedir(d);

        for (u32 i = 0; i < count; i++) {
            a_pad(u8, fp, 1024);
            {
                a_cstr(ip, idx_path);
                u8bFeed(fp, ip);
                u8bFeed1(fp, '/');
                a_cstr(nm, names[i]);
                u8bFeed(fp, nm);
                PATHu8gTerm(PATHu8gIn(fp));
            }
            if (FILEMapRO(&maps[nfiles], PATHu8cgIn(fp)) != OK) continue;
            belt128cp base = (belt128cp)u8bDataHead(maps[nfiles]);
            u64 len = u8bDataLen(maps[nfiles]) / sizeof(belt128);
            runs[nfiles][0] = base;
            runs[nfiles][1] = base + len;
            nfiles++;
            stack[1] = runs + nfiles;
        }
    }

    // lookup by hashlet
    belt128cp hit = BELTLookup(stack, needle);
    if (!hit) {
        for (u32 i = 0; i < nfiles; i++) FILEUnMap(maps[i]);
        return BELTNONE;
    }
    u64 found_off = BELTOffset(*hit);

    // mmap log, resolve
    u8bp logmap = NULL;
    {
        a_pad(u8, lp, 1024);
        call(u8bFeed, lp, belt_dir);
        a_cstr(sub, "/objects.log");
        call(u8bFeed, lp, sub);
        call(PATHu8gTerm, PATHu8gIn(lp));
        call(FILEMapRO, &logmap, PATHu8cgIn(lp));
    }

    u8cp pk = u8bDataHead(logmap);
    u64 pklen = u8bDataLen(logmap);

    u8p b1 = malloc(BELT_BUFSZ);
    u8p b2 = malloc(BELT_BUFSZ);
    if (!b1 || !b2) {
        free(b2); free(b1);
        for (u32 i = 0; i < nfiles; i++) FILEUnMap(maps[i]);
        FILEUnMap(logmap);
        fail(BELTFAIL);
    }

    u8 obj_type = 0;
    u8p content = NULL;
    u64 outsz = 0;

    ok64 o = BELTResolve(pk, pklen, stack,
                           found_off, &obj_type,
                           b1, b2, BELT_BUFSZ,
                           &content, &outsz);
    if (o != OK) {
        free(b2); free(b1);
        for (u32 i = 0; i < nfiles; i++) FILEUnMap(maps[i]);
        FILEUnMap(logmap);
        return o;
    }

    if (outsz > (u64)u8gRestLen(out)) {
        free(b2); free(b1);
        for (u32 i = 0; i < nfiles; i++) FILEUnMap(maps[i]);
        FILEUnMap(logmap);
        return NOROOM;
    }

    memcpy(out[1], content, outsz);
    out[1] += outsz;
    *out_type = obj_type;

    free(b2);
    free(b1);
    for (u32 i = 0; i < nfiles; i++) FILEUnMap(maps[i]);
    call(FILEUnMap, logmap);
    done;
}
