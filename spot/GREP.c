#include "CAPOi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abc/NFA.h"
#include "abc/PRO.h"
#include "abc/SORT.h"
#include "spot/SPOT.h"
#include "tok/DEF.h"

// --- Grep: substring search in source text (no tree) ---

void CAPOGrepCtx(u8csc source, u32 match_pos, u32 nctx,
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

        // Tokenization is deferred until first match (lazy)
        b8 tokenized = NO;
        Bu32 gtoks = {};
        u32cs gts = {};

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
                        // Lazy tokenize for syntax highlighting
                        if (!$empty(file_ext) && CAPOKnownExt(file_ext)) {
                            size_t maxlen = $len(source) + 1;
                            ok64 to = u32bMap(gtoks, maxlen);
                            if (to == OK) {
                                to = SPOTTokenize(gtoks, source, file_ext);
                                if (to == OK) {
                                    u32 *dts[2] = {u32bDataHead(gtoks),
                                                   u32bIdleHead(gtoks)};
                                    u8cs dext = {file_ext[0], file_ext[1]};
                                    if (!$empty(dext) && dext[0][0] == '.')
                                        dext[0]++;
                                    DEFMark(dts, source, dext);
                                    tokenized = YES;
                                    gts[0] = (u32cp)u32bDataHead(gtoks);
                                    gts[1] = (u32cp)u32bIdleHead(gtoks);
                                } else
                                    u32bUnMap(gtoks);
                            }
                        }
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

                        LESSHunkEmit();
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

// --- Regex grep: extract literal runs from regex for trigram filtering ---

// Walk a regex pattern, collect runs of literal characters.
// Meta chars and class escapes break a run; backslash-escaped literals stay.
// For each run >= 3 chars, extract trigrams and intersect with the index.
static void CAPORegexLiterals(u8csc pattern,
                               u64css stack, u32 nidxfiles,
                               u64cs *runs,
                               u32 *hashbuf1, u32 *hashbuf2,
                               u32 maxhashes, u32 *nhashes,
                               b8 *has_trigrams) {
    u8cp p = pattern[0];
    u8cp end = pattern[1];

    // Temp buffer for collecting a literal run
    u8 litbuf[1024];
    u32 litlen = 0;

#define FLUSH_LITS()                                                      \
    do {                                                                   \
        if (litlen >= 3) {                                                 \
            for (u32 li = 0; li + 2 < litlen; li++) {                     \
                if (CAPOTriChar(litbuf[li]) && CAPOTriChar(litbuf[li+1]) && \
                    CAPOTriChar(litbuf[li+2])) {                           \
                    u8 _tb[3] = {litbuf[li], litbuf[li+1], litbuf[li+2]}; \
                    u8cs tri = {_tb, _tb + 3};                            \
                    u64 tri_prefix = CAPOTriPack(tri);                    \
                    u64cs seek_runs[CAPO_MAX_LEVELS];                     \
                    for (u32 si = 0; si < nidxfiles; si++) {              \
                        seek_runs[si][0] = runs[si][0];                   \
                        seek_runs[si][1] = runs[si][1];                   \
                    }                                                      \
                    u64css seek_iter = {seek_runs, seek_runs + nidxfiles}; \
                    MSETu64Start(seek_iter);                               \
                    u32 tri_nhashes = 0;                                   \
                    CAPOCollectPaths(seek_iter, tri_prefix, hashbuf2,      \
                                     &tri_nhashes, maxhashes);             \
                    if (!*has_trigrams) {                                   \
                        memcpy(hashbuf1, hashbuf2,                         \
                               tri_nhashes * sizeof(u32));                 \
                        *nhashes = tri_nhashes;                            \
                        *has_trigrams = YES;                                \
                    } else {                                                \
                        qsort(hashbuf2, tri_nhashes, sizeof(u32),          \
                              CAPOu32cmp);                                 \
                        qsort(hashbuf1, *nhashes, sizeof(u32),             \
                              CAPOu32cmp);                                 \
                        *nhashes = CAPOIntersect(hashbuf1, *nhashes,       \
                                                  hashbuf2, tri_nhashes,   \
                                                  hashbuf1);               \
                    }                                                      \
                }                                                          \
            }                                                              \
        }                                                                  \
        litlen = 0;                                                        \
    } while (0)

    while (p < end) {
        u8 ch = *p;
        if (ch == '\\' && p + 1 < end) {
            u8 esc = p[1];
            // Class escapes break the run
            if (esc == 'd' || esc == 'D' || esc == 'w' || esc == 'W' ||
                esc == 's' || esc == 'S') {
                FLUSH_LITS();
                p += 2;
            } else {
                // Escaped literal: the escaped char itself
                if (litlen < sizeof(litbuf)) litbuf[litlen++] = esc;
                p += 2;
            }
        } else if (ch == '[') {
            // Character class — skip to closing ']'
            FLUSH_LITS();
            p++;
            if (p < end && *p == '^') p++;
            if (p < end && *p == ']') p++;
            while (p < end && *p != ']') {
                if (*p == '\\' && p + 1 < end) p++;
                p++;
            }
            if (p < end) p++;  // skip ']'
        } else if (ch == '*' || ch == '+' || ch == '?' || ch == '|' ||
                   ch == '.' || ch == '(' || ch == ')' || ch == '{' ||
                   ch == '}' || ch == '^' || ch == '$') {
            FLUSH_LITS();
            p++;
        } else {
            // Plain literal
            if (litlen < sizeof(litbuf)) litbuf[litlen++] = ch;
            p++;
        }
    }
    FLUSH_LITS();
#undef FLUSH_LITS
}

ok64 CAPOPcreGrep(u8csc pattern, u8csc ext, u8csc reporoot, u32 ctx_lines,
                   u8css files) {
    sane($ok(pattern) && !$empty(pattern) && $ok(reporoot));
    int nfiles = (int)$len(files);

    // Compile regex
    nfau8 prog_buf[512];
    u32 patch_buf[512];
    nfau8g prog = {prog_buf, prog_buf + 512, prog_buf};
    u32 *ws_patch[2] = {patch_buf, patch_buf + 512};
    u8cs pat = {pattern[0], pattern[1]};
    ok64 co = NFAu8Compile(prog, pat, ws_patch);
    if (co != OK) {
        fprintf(stderr, "spot: bad regex: %s\n", ok64str(co));
        return co;
    }
    nfau8cs cprog = {prog[2], prog[0]};
    u16 nstates = NFAu8States(cprog);

    // NFA workspace
    u64 wsz = NFAu8WorkSize(nstates);
    u32 *nfa_ws_buf = (u32 *)malloc(wsz * sizeof(u32));
    test(nfa_ws_buf != NULL, FAILSANITY);
    u32 *nfa_ws[2] = {nfa_ws_buf, nfa_ws_buf + wsz};

    // Language filter
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
        if (hashbuf1 == NULL || hashbuf2 == NULL) {
            free(nfa_ws_buf);
            free(hashbuf1);
            free(hashbuf2);
            return FAILSANITY;
        }

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
            CAPORegexLiterals(pattern, stack, nidxfiles, runs,
                               hashbuf1, hashbuf2,
                               maxhashes, &nhashes, &has_trigrams);
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

    char line[FILE_PATH_MAX_LEN];
    int fi = 0;
    while (nfiles > 0
           ? fi < nfiles
           : fgets(line, sizeof(line), fp) != NULL) {
        size_t len;
        if (nfiles > 0) {
            u8cs *fpp = u8cssAtP(files, fi);
            len = (size_t)$len(*fpp);
            if (len >= sizeof(line)) { fi++; continue; }
            u8s lns = {(u8p)line, (u8p)line + len};
            u8sCopy(lns, *fpp);
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

        // Tokenization is deferred until first match (lazy)
        b8 tokenized = NO;
        Bu32 gtoks = {};
        u32cs gts = {};

        // Search source line by line with NFA
        u32 prev_hi = 0;
        b8 found_any = NO;
        b8 first_hunk = YES;

        u8cp lp = source[0];
        u8cp se = source[1];
        while (lp < se) {
            // Find end of line
            u8cp le = lp;
            while (le < se && *le != '\n') le++;
            u8cs ln = {lp, le};

            if (NFAu8Search(cprog, ln, nfa_ws)) {
                u32 match_pos = (u32)(lp - source[0]);
                u32 ctx_lo = 0, ctx_hi = 0;
                CAPOGrepCtx(source, match_pos, ctx_lines, &ctx_lo, &ctx_hi);

                if (!found_any) {
                    CAPOProgress(NULL);
                    found_any = YES;
                    // Lazy tokenize for syntax highlighting
                    if (!$empty(file_ext) && CAPOKnownExt(file_ext)) {
                        size_t maxlen = $len(source) + 1;
                        ok64 to = u32bMap(gtoks, maxlen);
                        if (to == OK) {
                            to = SPOTTokenize(gtoks, source, file_ext);
                            if (to == OK) {
                                u32 *dts[2] = {u32bDataHead(gtoks),
                                               u32bIdleHead(gtoks)};
                                u8cs dext = {file_ext[0], file_ext[1]};
                                if (!$empty(dext) && dext[0][0] == '.')
                                    dext[0]++;
                                DEFMark(dts, source, dext);
                                tokenized = YES;
                                gts[0] = (u32cp)u32bDataHead(gtoks);
                                gts[1] = (u32cp)u32bIdleHead(gtoks);
                            } else
                                u32bUnMap(gtoks);
                        }
                    }
                }

                // Find match span for highlighting:
                // try each start position, expand with anchored match
                u32 hl_start = match_pos;
                u32 hl_end = (u32)(le - source[0]);
                for (u8cp sp = lp; sp < le; sp++) {
                    for (u8cp ep = sp + 1; ep <= le; ep++) {
                        u8cs sub = {sp, ep};
                        if (NFAu8Match(cprog, sub, nfa_ws)) {
                            hl_start = (u32)(sp - source[0]);
                            hl_end = (u32)(ep - source[0]);
                            goto found_span;
                        }
                    }
                }
                found_span:;

                // Collect nearby matches within context
                range32 hls[CAPO_MAX_HLS];
                int nhl = 0;
                hls[nhl++] = (range32){hl_start, hl_end};

                // Scan subsequent lines within context
                u8cp lp2 = le;
                if (lp2 < se && *lp2 == '\n') lp2++;
                while (lp2 < se && nhl < CAPO_MAX_HLS) {
                    u8cp le2 = lp2;
                    while (le2 < se && *le2 != '\n') le2++;
                    u32 mp2 = (u32)(lp2 - source[0]);
                    if (mp2 >= ctx_hi) break;

                    u8cs ln2 = {lp2, le2};
                    if (NFAu8Search(cprog, ln2, nfa_ws)) {
                        // Find highlight span
                        u32 hs2 = mp2, he2 = (u32)(le2 - source[0]);
                        for (u8cp sp = lp2; sp < le2; sp++) {
                            for (u8cp ep = sp + 1; ep <= le2; ep++) {
                                u8cs sub = {sp, ep};
                                if (NFAu8Match(cprog, sub, nfa_ws)) {
                                    hs2 = (u32)(sp - source[0]);
                                    he2 = (u32)(ep - source[0]);
                                    goto found_span2;
                                }
                            }
                        }
                        found_span2:;
                        hls[nhl++] = (range32){hs2, he2};

                        u32 lo2 = 0, hi2 = 0;
                        CAPOGrepCtx(source, mp2, ctx_lines, &lo2, &hi2);
                        if (hi2 > ctx_hi) ctx_hi = hi2;
                    }
                    lp2 = le2;
                    if (lp2 < se && *lp2 == '\n') lp2++;
                }

                b8 contiguous = (ctx_lo <= prev_hi);
                if (ctx_lo < prev_hi) ctx_lo = prev_hi;
                if (ctx_lo < ctx_hi &&
                    less_nhunks < LESS_MAX_HUNKS &&
                    u8bIdleLen(less_arena) > (ctx_hi - ctx_lo + 512)) {
                    LESShunk *hk = &less_hunks[less_nhunks];
                    memset(hk, 0, sizeof(*hk));

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

                    if (CAPO_COLOR) {
                        u32 region_len = ctx_hi - ctx_lo;
                        u8p rlp = LESSArenaAlloc(region_len);
                        if (rlp != NULL) {
                            int ntoks = (int)$len(gts);
                            for (int ti = 0; ti < ntoks; ti++) {
                                u32 tlo = (ti > 0) ? tok32Offset(gts[0][ti-1]) : 0;
                                u32 thi = tok32Offset(gts[0][ti]);
                                if (thi <= ctx_lo || tlo >= ctx_hi) continue;
                                u32 clo = tlo < ctx_lo ? ctx_lo : tlo;
                                u32 chi = thi > ctx_hi ? ctx_hi : thi;
                                u8 tag = tok32Tag(gts[0][ti]) - 'A';
                                memset(rlp + (clo - ctx_lo), tag, chi - clo);
                            }
                            for (int h = 0; h < nhl; h++) {
                                u32 hlo = hls[h].lo < ctx_lo ? ctx_lo : hls[h].lo;
                                u32 hhi = hls[h].hi > ctx_hi ? ctx_hi : hls[h].hi;
                                for (u32 b = hlo; b < hhi; b++)
                                    rlp[b - ctx_lo] |= LESS_INS;
                            }
                            hk->lits[0] = rlp;
                            hk->lits[1] = rlp + region_len;
                        }
                    }

                    LESSHunkEmit();
                    first_hunk = NO;
                }
                prev_hi = ctx_hi;

                // Skip to end of context
                lp = source[0] + prev_hi;
                continue;
            }

            lp = le;
            if (lp < se && *lp == '\n') lp++;
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

    free(nfa_ws_buf);
    if (hashbuf1 != NULL) free(hashbuf1);
    if (hashbuf2 != NULL) free(hashbuf2);
    done;
}
