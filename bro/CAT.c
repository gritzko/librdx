#include "BRO.h"

#include <stdio.h>
#include <string.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"

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

        // Map file
        u8bp mapped = NULL;
        ok64 o = FILEMapRO(&mapped, PATHu8cgIn(fpbuf));
        if (o != OK) {
            fprintf(stderr, "bro: cannot open %.*s: %s\n",
                    (int)$len(fpath_s), (char *)fpath_s[0], ok64str(o));
            continue;
        }

        hunk *hk = &bro_hunks[bro_nhunks];
        *hk = (hunk){};

        // Path
        size_t fzl = (size_t)$len(fpath_s);
        u8p pp = BROArenaWrite(fpath_s[0], fzl);
        if (pp) {
            hk->path[0] = pp;
            hk->path[1] = pp + fzl;
        }

        hk->text[0] = u8bDataHead(mapped);
        hk->text[1] = u8bIdleHead(mapped);

        Bu32 toks = {};
        b8 tokenized = BROTokenize(toks, hk, fpath_s);

        BROHunkAdd();
        BRODefer(mapped, tokenized ? toks : (Bu32){});
    }

    if (bro_nhunks > 0)
        BRORun(bro_hunks, bro_nhunks);
    BROArenaCleanup();

    done;
}
