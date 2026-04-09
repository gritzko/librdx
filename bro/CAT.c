#include "BRO.h"

#include <stdio.h>
#include <string.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/DEF.h"
#include "dog/HUNK.h"
#include "dog/TOK.h"

// --- Cat: syntax-highlighted file display ---

ok64 BROCat(u8css files, u8csc reporoot) {
    sane(!$empty(files));
    (void)reporoot;
    int nfiles = (int)$len(files);

    call(BROArenaInit);

    for (int fi = 0; fi < nfiles; fi++) {
        if (bro_nhunks >= BRO_MAX_HUNKS) break;

        u8cs *fp = u8cssAtP(files, fi);
        a_dup(u8c, fpath_s, (*fp));
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
        HUNKu8sExt(ext, u8bDataHead(fpbuf), plen);

        // Map file
        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, PATHu8cgIn(fpbuf));
        if (o != OK) {
            fprintf(stderr, "bro: cannot open %.*s: %s\n",
                    (int)$len(fpath_s), (char *)fpath_s[0], ok64str(o));
            continue;
        }

        u8cp src_head = u8bDataHead(mapped);
        u8cp src_idle = u8bIdleHead(mapped);
        u32 srclen = (u32)(src_idle - src_head);

        BROhunk *hk = &bro_hunks[bro_nhunks];
        *hk = (BROhunk){};

        // Title
        char fpz[FILE_PATH_MAX_LEN];
        size_t fzl = (size_t)$len(fpath_s);
        if (fzl >= sizeof(fpz)) fzl = sizeof(fpz) - 1;
        memcpy(fpz, fpath_s[0], fzl);
        fpz[fzl] = 0;
        u8gp g = u8aOpen(bro_arena);
        call(HUNKu8sFormatTitle, u8gRest(g), fpz, "");
        u8cs title = {};
        u8aClose(bro_arena, title);
        if (!$empty(title)) {
            hk->title[0] = title[0];
            hk->title[1] = title[1];
        }
        // Store the display path as hk->path
        u8p pp = BROArenaWrite(fpz, fzl);
        if (pp) {
            hk->path[0] = pp;
            hk->path[1] = pp + fzl;
        }

        hk->text[0] = src_head;
        hk->text[1] = src_idle;

        // Tokenize
        Bu32 toks = {};
        b8 tokenized = NO;
        u8cs ext_nodot = {};
        if (!$empty(ext) && ext[0][0] == '.') {
            ext_nodot[0] = ext[0] + 1;
            ext_nodot[1] = ext[1];
        }
        if (!$empty(ext_nodot) && TOKKnownExt(ext_nodot)) {
            size_t maxlen = srclen + 1;
            o = u32bMap(toks, maxlen);
            if (o == OK) {
                u8cs source = {src_head, src_idle};
                o = HUNKu32bTokenize(toks, source, ext);
                if (o == OK) {
                    u32 *dts[2] = {u32bDataHead(toks), u32bIdleHead(toks)};
                    DEFMark(dts, source, ext_nodot);
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

        BROHunkAdd();
        BRODefer(mapped, tokenized ? toks : (Bu32){});
    }

    if (bro_nhunks > 0)
        BRORun(bro_hunks, bro_nhunks);
    BROArenaCleanup();

    done;
}
