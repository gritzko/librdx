#include "CAPOi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abc/NFA.h"
#include "abc/PRO.h"
#include "abc/SORT.h"
#include "spot/SPOT.h"
#include "dog/DEF.h"

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
    size_t maxhashes = 1ULL << 28;  // 1G / sizeof(u32)
    Bu32 hashbuf1 = {};
    b8 has_trigrams = NO;

    if (nfiles == 0) {
        a_path(capodir);
        call(CAPOResolveDir, capodir, reporoot);
        a_dup(u8c, dirslice, u8bDataC(capodir));

        call(u32bMap, hashbuf1, maxhashes);

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
                    HITu64Start(seek_iter);

                    if (!has_trigrams) {
                        u32bReset(hashbuf1);
                        CAPOCollectPaths(seek_iter, tri_prefix,
                                         u32bDataIdle(hashbuf1));
                        has_trigrams = YES;
                    } else {
                        u32sSort(u32bData(hashbuf1));
                        CAPOFilterInPlace(hashbuf1, seek_iter, tri_prefix);
                    }
                }
                p++;
            }
        }

        CAPOStackClose(mmaps, nidxfiles);

        if (has_trigrams && u32bDataLen(hashbuf1) > 0)
            u32sSort(u32bData(hashbuf1));
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
            if (has_trigrams && u32bDataLen(hashbuf1) > 0) {
                u32 phash = CAPOPathHash(relpath);
                if (!u32sBsearch(&phash, u32bData(hashbuf1)))
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
            if (!TOKSameLexer(file_ext, target_ext)) continue;
        }

        CAPOProgress(line);

        char fpath[FILE_PATH_MAX_LEN * 2];
        int pn = snprintf(fpath, sizeof(fpath), "%.*s/%s",
                          (int)$len(reporoot), (char *)reporoot[0], line);
        if (pn <= 0 || pn >= (int)sizeof(fpath)) continue;

        a_path(fpbuf);
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
                                    a_dup(u8c,dext,file_ext);
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
                        *hk = (LESShunk){};

                        // Title
                        if (!contiguous || first_hunk) {
                            char funcname[256];
                            CAPOFindFunc(source, ctx_lo, file_ext,
                                         funcname, sizeof(funcname));
                            u8gp _tg = u8aOpen(less_arena);
                            call(CAPOFormatTitle, u8gRest(_tg), line, funcname);
                            u8cs _title = {};
                            u8aClose(less_arena, _title);
                            if (!$empty(_title)) {
                                hk->title[0] = _title[0];
                                hk->title[1] = _title[1];
                            }
                        }

                        hk->text[0] = source[0] + ctx_lo;
                        hk->text[1] = source[0] + ctx_hi;

                        // Clip file-level toks to context region
                        HUNKu32sClip(less_arena, hk->toks, gts,
                                     ctx_lo, ctx_hi);

                        // Build sparse hili from match ranges
                        if (CAPO_COLOR) {
                            a_pad(u32, hbuf, 2 * CAPO_MAX_HLS + 1);
                            u32 prev_end = 0;
                            for (int hi2 = 0; hi2 < nhl; hi2++) {
                                u32 mlo = hls[hi2].lo < ctx_lo
                                              ? 0
                                              : hls[hi2].lo - ctx_lo;
                                u32 mhi = hls[hi2].hi > ctx_hi
                                              ? ctx_hi - ctx_lo
                                              : hls[hi2].hi - ctx_lo;
                                if (mlo > prev_end)
                                    u32bFeed1(hbuf, tok32Pack('A', mlo));
                                u32bFeed1(hbuf, tok32Pack('I', mhi));
                                prev_end = mhi;
                            }
                            u32 region_len = ctx_hi - ctx_lo;
                            if (prev_end < region_len)
                                u32bFeed1(hbuf, tok32Pack('A', region_len));
                            a_dup(u32 const, hd, u32bDataC(hbuf));
                            u8p hp = LESSArenaWrite(hd[0],
                                         $len(hd) * sizeof(u32));
                            if (hp) {
                                hk->hili[0] = (u32cp)hp;
                                hk->hili[1] = (u32cp)(hp +
                                    $len(hd) * sizeof(u32));
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

    if (!BNULL(hashbuf1)) u32bUnMap(hashbuf1);
    done;
}

// --- Regex grep: extract literal runs from regex for trigram filtering ---

// Walk a regex pattern, collect runs of literal characters.
// Meta chars and class escapes break a run; backslash-escaped literals stay.
// For each run >= 3 chars, extract trigrams and intersect with the index.
static void CAPORegexLiterals(u8csc pattern,
                               u64css stack, u32 nidxfiles,
                               u64cs *runs,
                               u32b hashbuf1,
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
                    HITu64Start(seek_iter);                               \
                    if (!*has_trigrams) {                                   \
                        u32bReset(hashbuf1);                               \
                        CAPOCollectPaths(seek_iter, tri_prefix,            \
                                         u32bDataIdle(hashbuf1));          \
                        *has_trigrams = YES;                                \
                    } else {                                                \
                        u32sSort(u32bData(hashbuf1));                      \
                        HITu64Seek(seek_iter, &tri_prefix);               \
                        CAPOFilterInPlace(hashbuf1, seek_iter,             \
                                          tri_prefix);                     \
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
    a_dup(u8c,pat,pattern);
    ok64 co = NFAu8Compile(prog, pat, ws_patch);
    if (co != OK) {
        fprintf(stderr, "spot: bad regex: %s\n", ok64str(co));
        return co;
    }
    nfau8cs cprog = {prog[2], prog[0]};
    u16 nstates = NFAu8States(cprog);

    // NFA workspace
    u64 wsz = NFAu8WorkSize(nstates);
    Bu32 nfa_ws_bb = {};
    call(u32bAlloc, nfa_ws_bb, wsz);
    u32s nfa_ws = {};
    $mv(nfa_ws, u32bIdle(nfa_ws_bb));

    // Language filter
    u8cs target_ext = {};
    if (!$empty(ext)) { $mv(target_ext, ext); }

    // --- Trigram filtering (skip when explicit files given) ---
    size_t maxhashes = 1ULL << 28;  // 1G / sizeof(u32)
    Bu32 hashbuf1 = {};
    b8 has_trigrams = NO;

    if (nfiles == 0) {
        a_path(capodir);
        call(CAPOResolveDir, capodir, reporoot);
        a_dup(u8c, dirslice, u8bDataC(capodir));

        ok64 ao = u32bMap(hashbuf1, maxhashes);
        if (ao != OK) {
            u32bFree(nfa_ws_bb);
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
                               hashbuf1, &has_trigrams);
        }

        CAPOStackClose(mmaps, nidxfiles);

        if (has_trigrams && u32bDataLen(hashbuf1) > 0)
            u32sSort(u32bData(hashbuf1));
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
            if (has_trigrams && u32bDataLen(hashbuf1) > 0) {
                u32 phash = CAPOPathHash(relpath);
                if (!u32sBsearch(&phash, u32bData(hashbuf1)))
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
            if (!TOKSameLexer(file_ext, target_ext)) continue;
        }

        CAPOProgress(line);

        char fpath[FILE_PATH_MAX_LEN * 2];
        int pn = snprintf(fpath, sizeof(fpath), "%.*s/%s",
                          (int)$len(reporoot), (char *)reporoot[0], line);
        if (pn <= 0 || pn >= (int)sizeof(fpath)) continue;

        a_path(fpbuf);
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
                                a_dup(u8c,dext,file_ext);
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
                        u8gp _tg = u8aOpen(less_arena);
                        call(CAPOFormatTitle, u8gRest(_tg), line, funcname);
                        u8cs _title = {};
                        u8aClose(less_arena, _title);
                        if (!$empty(_title)) {
                            hk->title[0] = _title[0];
                            hk->title[1] = _title[1];
                        }
                    }

                    hk->text[0] = source[0] + ctx_lo;
                    hk->text[1] = source[0] + ctx_hi;

                    // Clip file-level toks to context region
                    HUNKu32sClip(less_arena, hk->toks, gts,
                                 ctx_lo, ctx_hi);

                    // Build sparse hili from match ranges
                    if (CAPO_COLOR) {
                        a_pad(u32, hbuf, 2 * CAPO_MAX_HLS + 1);
                        u32 prev_end = 0;
                        for (int hi2 = 0; hi2 < nhl; hi2++) {
                            u32 mlo = hls[hi2].lo < ctx_lo
                                          ? 0
                                          : hls[hi2].lo - ctx_lo;
                            u32 mhi = hls[hi2].hi > ctx_hi
                                          ? ctx_hi - ctx_lo
                                          : hls[hi2].hi - ctx_lo;
                            if (mlo > prev_end)
                                u32bFeed1(hbuf, tok32Pack('A', mlo));
                            u32bFeed1(hbuf, tok32Pack('I', mhi));
                            prev_end = mhi;
                        }
                        u32 region_len = ctx_hi - ctx_lo;
                        if (prev_end < region_len)
                            u32bFeed1(hbuf, tok32Pack('A', region_len));
                        a_dup(u32 const, hd, u32bDataC(hbuf));
                        u8p hp = LESSArenaWrite(hd[0],
                                     $len(hd) * sizeof(u32));
                        if (hp) {
                            hk->hili[0] = (u32cp)hp;
                            hk->hili[1] = (u32cp)(hp +
                                $len(hd) * sizeof(u32));
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

    u32bFree(nfa_ws_bb);
    if (!BNULL(hashbuf1)) u32bUnMap(hashbuf1);
    done;
}
