#include "CAPOi.h"

#include <stdio.h>
#include <string.h>

#include "abc/PRO.h"
#include "spot/SPOT.h"
#include "tok/DEF.h"

// --- Cat: syntax-highlighted file display ---

ok64 CAPOCat(u8css files, u8csc reporoot) {
    sane(!$empty(files) && $ok(reporoot));
    int nfiles = (int)$len(files);

    call(LESSArenaInit);

    for (int fi = 0; fi < nfiles; fi++) {
        if (less_nhunks >= LESS_MAX_HUNKS) break;

        u8cs *fp = u8cssAtP(files, fi);
        a_dup(u8c,fpath_s,(*fp));
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
        *hk = (LESShunk){};

        // Title
        char fpz[FILE_PATH_MAX_LEN];
        size_t fzl = (size_t)$len(fpath_s);
        if (fzl >= sizeof(fpz)) fzl = sizeof(fpz) - 1;
        memcpy(fpz, fpath_s[0], fzl);
        fpz[fzl] = 0;
        u8gp g = u8aOpen(less_arena);
        call(CAPOFormatTitle, u8gRest(g), fpz, "");
        u8cs title = {};
        u8aClose(less_arena, title);
        if (!$empty(title)) {
            hk->title[0] = title[0];
            hk->title[1] = title[1];
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
                    a_dup(u8c,dext,ext);
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

        // No hili for cat (no diff/match highlights)

        LESSHunkEmit();
        LESSDefer(mapped, tokenized ? toks : (Bu32){});
    }

    if (less_nhunks > 0)
        LESSRun(less_hunks, less_nhunks);
    LESSArenaCleanup();

    done;
}
