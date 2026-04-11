#include "BRO.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"

static void BROUsage(void) {
    fprintf(stderr,
        "Usage: bro [files...]\n"
        "\n"
        "  bro                   read TLV hunks from stdin (pager mode)\n"
        "  bro file.c [...]      syntax-highlighted cat\n"
        "\n"
        "Keys: q quit, space/f page down, b page up, j/k line, g/G top/end,\n"
        "      / or ' search, n/N next/prev, : goto line,\n"
        "      # GURI search (text=grep, 'snap'=snippet, /re/=regex, .ext),\n"
        "      [ ] { } prev/next hunk, ( ) prev/next change,\n"
        "      Enter/l open file, h/q back,\n"
        "      m toggle mouse (wheel scroll, click to open)\n");
}

ok64 brocli() {
    sane(1);
    call(FILEInit);
    BRO_COLOR = isatty(STDOUT_FILENO) ? YES : NO;
    if (getenv("BRO_COLOR")) BRO_COLOR = YES;
    if (getenv("NO_COLOR"))  BRO_COLOR = NO;

    int argn = (int)$arglen;

    // Collect non-flag args
    u8cs files[16] = {};
    int nf = 0;
    for (int i = 1; i < argn && nf < 16; i++) {
        u8c *a[2] = {};
        $mv(a, $arg(i));
        if ($len(a) >= 1 && a[0][0] == '-') {
            a_cstr(help_flag, "--help");
            a_cstr(h_flag, "-h");
            if ($eq(a, h_flag) || $eq(a, help_flag)) {
                BROUsage();
                done;
            }
            fprintf(stderr, "bro: unknown flag: %.*s\n",
                    (int)$len(a), (char *)a[0]);
            return FAILSANITY;
        }
        files[nf][0] = a[0];
        files[nf][1] = a[1];
        nf++;
    }

    if (nf > 0) {
        // Cat mode: map files, tokenize, display via BRORun.
        call(BROArenaInit);
        for (int fi = 0; fi < nf; fi++) {
            if (bro_nhunks >= BRO_MAX_HUNKS) break;
            a_dup(u8c, fp, files[fi]);
            if ($empty(fp)) continue;

            a_pad(u8, fpbuf, FILE_PATH_MAX_LEN);
            __ = u8bFeed(fpbuf, fp);
            if (__ != OK) continue;
            __ = PATHu8gTerm(PATHu8gIn(fpbuf));
            if (__ != OK) continue;

            u8bp mapped = NULL;
            ok64 o = FILEMapRO(&mapped, PATHu8cgIn(fpbuf));
            if (o != OK) {
                fprintf(stderr, "bro: cannot open %.*s: %s\n",
                        (int)$len(fp), (char *)fp[0], ok64str(o));
                continue;
            }

            hunk *hk = &bro_hunks[bro_nhunks];
            *hk = (hunk){};
            size_t pl = (size_t)$len(fp);
            u8p pp = BROArenaWrite(fp[0], pl);
            if (pp) { hk->path[0] = pp; hk->path[1] = pp + pl; }
            hk->text[0] = u8bDataHead(mapped);
            hk->text[1] = u8bIdleHead(mapped);

            Bu32 toks = {};
            b8 tok = BROTokenize(toks, hk, fp);
            BROHunkAdd();
            BRODefer(mapped, tok ? toks : (Bu32){});
        }
        if (bro_nhunks > 0)
            BRORun(bro_hunks, bro_nhunks);
        BROArenaCleanup();
    } else {
        call(BROPipeRun, STDIN_FILENO);
    }
    done;
}

MAIN(brocli);
