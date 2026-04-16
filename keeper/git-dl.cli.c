//  git-dl: download a git repo via git-upload-pack, store pack + index
//
//  Usage: git-dl <repo-path> <out-dir>
//
//  Receives the full packfile, saves as <out-dir>/pack.bin, builds
//  a kv64 hash index <out-dir>/pack.idx (sha1[0:8] → pack offset).
//  Resolves delta chains (OFS_DELTA + REF_DELTA) to compute SHA-1.
//
#include "keeper/DELT.h"
#include "keeper/GIT.h"
#include "keeper/PACK.h"
#include "keeper/PKT.h"
#include "keeper/SHA1.h"
#include "keeper/ZINF.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/KV.h"
#include "abc/PRO.h"

#define X(M, name) M##kv64##name
#include "abc/HASHx.h"
#undef X

con ok64 DLFAIL = 0x3553ca495;

static char const *type_str[] = {
    [PACK_OBJ_COMMIT] = "commit",
    [PACK_OBJ_TREE] = "tree",
    [PACK_OBJ_BLOB] = "blob",
    [PACK_OBJ_TAG] = "tag",
};

#define DL_BUFSZ (1 << 26)  // 64 MB

// Resolve an object at `off` in the pack to its final content.
// Follows OFS_DELTA / REF_DELTA chains. For REF_DELTA, looks up
// the base via the kv64 index (sha1[0:8] → offset).
// Returns base type and content pointer + size.
static ok64 resolve(u8cp pack, u64 packlen, kv64s idx,
                     u64 off, u8 *out_type,
                     u8p buf1, u8p buf2, u64 bufsz,
                     u8p *result, u64 *outsz) {
    sane(pack && buf1 && buf2 && result && outsz);

    u64 chain[256];
    int depth = 0;
    u64 cur = off;

    // chase delta chain to base
    for (;;) {
        pack_obj obj = {};
        u8cs from = {pack + cur, pack + packlen};
        call(PACKDrainObjHdr, from, &obj);

        if (obj.type >= 1 && obj.type <= 4) {
            // base object
            *out_type = obj.type;
            if (obj.size > bufsz) return NOROOM;
            u8s into = {buf1, buf1 + bufsz};
            call(PACKInflate, from, into, obj.size);
            *result = buf1;
            *outsz = obj.size;
            break;
        }

        if (depth >= 256) return DLFAIL;
        chain[depth++] = cur;

        if (obj.type == PACK_OBJ_OFS_DELTA) {
            cur = cur - obj.ofs_delta;
        } else if (obj.type == PACK_OBJ_REF_DELTA) {
            // look up base by SHA-1 in index
            u64 sha_key = 0;
            memcpy(&sha_key, obj.ref_delta[0], 8);
            kv64 lookup = {.key = sha_key, .val = 0};
            ok64 o = HASHkv64Get(&lookup, idx);
            if (o != OK) return o;  // base not yet indexed
            cur = lookup.val;
        } else {
            return DLFAIL;
        }
    }

    // apply deltas bottom-up
    u8p src = buf1;
    u8p dst = buf2;

    for (int i = depth - 1; i >= 0; i--) {
        pack_obj dobj = {};
        u8cs from = {pack + chain[i], pack + packlen};
        call(PACKDrainObjHdr, from, &dobj);

        // inflate delta instructions into second half of dst
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

// Compute git object SHA-1: hash("<type> <size>\0<content>")
static void git_sha1(u8 sha[20], u8 type, u8cp content, u64 sz, u8p tmp) {
    char hbuf[64];
    int hlen = snprintf(hbuf, sizeof(hbuf), "%s %lu",
                        type_str[type], (unsigned long)sz);
    memcpy(tmp, hbuf, hlen);
    tmp[hlen] = 0;
    memcpy(tmp + hlen + 1, content, sz);
    sha1 _h;
    u8csc _d = {tmp, tmp + hlen + 1 + sz};
    SHA1Sum(&_h, _d);
    memcpy(sha, _h.data, 20);
}

ok64 gitdl() {
    sane(1);

    if ($arglen < 3) fail(DLFAIL);
    a$rg(repo_arg, 1);
    a$rg(outdir_arg, 2);

    char repo[512];
    snprintf(repo, sizeof(repo), "%.*s", (int)$len(repo_arg), repo_arg[0]);
    char outdir[512];
    snprintf(outdir, sizeof(outdir), "%.*s", (int)$len(outdir_arg), outdir_arg[0]);

    mkdir(outdir, 0755);

    u8p rbuf = malloc(DL_BUFSZ);
    u8p buf1 = malloc(DL_BUFSZ);
    u8p buf2 = malloc(DL_BUFSZ);
    u8p objbuf = malloc(DL_BUFSZ);
    if (!rbuf || !buf1 || !buf2 || !objbuf) fail(DLFAIL);

    // --- pipe to git-upload-pack ---
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "git-upload-pack '%s'", repo);

    int to_child[2], from_child[2];
    if (pipe(to_child) != 0) failc(DLFAIL);
    if (pipe(from_child) != 0) failc(DLFAIL);

    pid_t pid = fork();
    if (pid < 0) failc(DLFAIL);

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

    // --- ref advertisement ---
    u64 rlen = 0;
    {
        ssize_t n = read(rfd, rbuf, DL_BUFSZ);
        if (n <= 0) fail(DLFAIL);
        rlen = (u64)n;
    }

    u8cs adv = {rbuf, rbuf + rlen};
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
            ssize_t n = read(rfd, rbuf + rlen, DL_BUFSZ - rlen);
            if (n <= 0) fail(DLFAIL);
            rlen += (u64)n;
            adv[1] = rbuf + rlen;
            continue;
        }
        nedo fail(__);
    }

    fprintf(stderr, "HEAD=%.40s\n", head_hex);

    // --- want + done ---
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
        if (write(wfd, wbuf, wlen) != (ssize_t)wlen) fail(DLFAIL);
    }
    close(wfd);

    // --- read response ---
    rlen = 0;
    for (;;) {
        ssize_t n = read(rfd, rbuf + rlen, DL_BUFSZ - rlen);
        if (n <= 0) break;
        rlen += (u64)n;
    }
    close(rfd);

    u8cs resp = {rbuf, rbuf + rlen};
    call(PKTu8sDrain, resp, line);
    if ($len(line) < 3 || memcmp(line[0], "NAK", 3) != 0) fail(PACKFAIL);

    u8cp packbase = resp[0];
    pack_hdr hdr = {};
    call(PACKDrainHdr, resp, &hdr);
    if (hdr.version != 2) fail(PACKBADFMT);
    u64 packlen = rlen - (packbase - rbuf);

    fprintf(stderr, "pack: %u objects, %lu bytes\n",
            hdr.count, (unsigned long)packlen);

    // --- scan: record offsets ---
    u64 *offsets = calloc(hdr.count, sizeof(u64));
    u8 *types = calloc(hdr.count, 1);
    if (!offsets || !types) fail(DLFAIL);

    u8cs scan = {resp[0], rbuf + rlen};
    for (u32 i = 0; i < hdr.count; i++) {
        offsets[i] = scan[0] - packbase;

        pack_obj obj = {};
        call(PACKDrainObjHdr, scan, &obj);
        types[i] = obj.type;

        a_pad(u8, scratch, 4096);
        ZINFInflate(u8bIdle(scratch), scan);
    }

    // --- save packfile ---
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/pack.bin", outdir);
        FILE *f = fopen(path, "wb");
        if (!f) failc(DLFAIL);
        if (fwrite(packbase, 1, packlen, f) != packlen) fail(DLFAIL);
        fclose(f);
        fprintf(stderr, "wrote %s\n", path);
    }

    // --- build index, multi-pass for delta resolution ---
    u64 tblsz = ((u64)hdr.count * 4 + 15) & ~15ULL;
    if (tblsz < 256) tblsz = 256;

    kv64 *idxmem = calloc(tblsz, sizeof(kv64));
    if (!idxmem) fail(DLFAIL);
    kv64s idx = {idxmem, idxmem + tblsz};

    b8 *done_flag = calloc(hdr.count, 1);
    if (!done_flag) fail(DLFAIL);

    u32 total_indexed = 0;

    // iterate until no more progress
    for (int pass = 0; pass < 32; pass++) {
        u32 progress = 0;

        for (u32 i = 0; i < hdr.count; i++) {
            if (done_flag[i]) continue;

            u8 obj_type = 0;
            u8p content = NULL;
            u64 content_sz = 0;

            if (types[i] >= 1 && types[i] <= 4) {
                // base object
                obj_type = types[i];
                pack_obj obj = {};
                u8cs from = {packbase + offsets[i], packbase + packlen};
                try(PACKDrainObjHdr, from, &obj);
                nedo continue;
                if (obj.size > DL_BUFSZ) { done_flag[i] = 1; continue; }
                u8s into = {buf1, buf1 + DL_BUFSZ};
                try(PACKInflate, from, into, obj.size);
                nedo continue;
                content = buf1;
                content_sz = obj.size;
            } else if (types[i] == PACK_OBJ_OFS_DELTA ||
                       types[i] == PACK_OBJ_REF_DELTA) {
                try(resolve, packbase, packlen, idx,
                    offsets[i], &obj_type,
                    buf1, buf2, DL_BUFSZ,
                    &content, &content_sz);
                nedo continue;  // base not yet indexed
            } else {
                done_flag[i] = 1;
                continue;
            }

            if (obj_type < 1 || obj_type > 4) { done_flag[i] = 1; continue; }

            u8 sha[20];
            git_sha1(sha, obj_type, content, content_sz, objbuf);

            u64 sha_key = 0;
            memcpy(&sha_key, sha, 8);

            kv64 entry = {.key = sha_key, .val = offsets[i]};
            try(HASHkv64Put, idx, &entry);
            nedo { done_flag[i] = 1; continue; }

            done_flag[i] = 1;
            total_indexed++;
            progress++;

            if (pass == 0) {
                u8 shex[40];
                u8s ss = {shex, shex + 40};
                u8cs sb = {sha, sha + 20};
                HEXu8sFeedSome(ss, sb);
                fprintf(stderr, "  %s %6lu  %.40s  @%lu\n",
                        type_str[obj_type], (unsigned long)content_sz,
                        shex, (unsigned long)offsets[i]);
            }
        }

        fprintf(stderr, "pass %d: +%u (total %u/%u)\n",
                pass, progress, total_indexed, hdr.count);

        if (progress == 0) break;
    }

    // --- save index ---
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/pack.idx", outdir);
        FILE *f = fopen(path, "wb");
        if (!f) failc(DLFAIL);
        if (fwrite(idxmem, sizeof(kv64), tblsz, f) != tblsz) fail(DLFAIL);
        fclose(f);
        fprintf(stderr, "wrote %s (%u entries)\n", path, total_indexed);
    }

    // --- save HEAD ---
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/HEAD", outdir);
        FILE *f = fopen(path, "w");
        if (!f) failc(DLFAIL);
        fprintf(f, "%.40s\n", head_hex);
        fclose(f);
    }

    free(done_flag);
    free(offsets);
    free(types);
    free(idxmem);
    free(objbuf);
    free(buf2);
    free(buf1);
    free(rbuf);

    { int status; waitpid(pid, &status, 0); }

    fprintf(stderr, "done: %s\n", outdir);
    done;
}

MAIN(gitdl)
