#include "BRO.h"

#include <stdio.h>
#include <sys/stat.h>

#include "abc/PRO.h"
#include "dog/CLI.h"

static void BROUsage(void) {
    fprintf(stderr,
        "Usage: bro [URI...]\n"
        "\n"
        "  bro                   read TLV hunks from stdin (pager mode)\n"
        "  bro file.c [...]      syntax-highlighted cat\n"
        "  bro file.c#42         open file at line 42\n"
        "  bro dir/              list directory\n"
        "\n"
        "Keys: q quit, space/f page down, b page up, j/k line, g/G top/end,\n"
        "      / or ' search, n/N next/prev,\n"
        "      : URI prompt (path#line, #grep.ext, #'snippet'.ext),\n"
        "      [ ] { } prev/next hunk, ( ) prev/next change,\n"
        "      Enter/l open file, h back, . list dir,\n"
        "      m toggle mouse (wheel scroll, click to open)\n");
}

ok64 brocli() {
    sane(1);
    call(FILEInit);

    cli c = {};
    call(CLIParse, &c, NULL, NULL);  // no verbs, no val-flags

    BRO_COLOR = c.tty_out;
    if (getenv("BRO_COLOR")) BRO_COLOR = YES;
    if (getenv("NO_COLOR"))  BRO_COLOR = NO;

    if (CLIHas(&c, "-h") || CLIHas(&c, "--help")) {
        BROUsage();
        done;
    }

    if (c.nuris > 0) {
        call(BROArenaInit);
        for (u32 i = 0; i < c.nuris; i++) {
            if (bro_nhunks >= BRO_MAX_HUNKS) break;
            uri *u = &c.uris[i];

            // Use path for file/dir, full URI for hunk
            u8cs file_path = {};
            if (!$empty(u->path)) {
                $mv(file_path, u->path);
            } else if (!$empty(u->data)) {
                $mv(file_path, u->data);
            }
            if ($empty(file_path)) continue;

            // Check if directory
            a_pad(u8, fpbuf, FILE_PATH_MAX_LEN);
            __ = u8bFeed(fpbuf, file_path);
            if (__ != OK) continue;
            __ = PATHu8gTerm(PATHu8gIn(fpbuf));
            if (__ != OK) continue;

            struct stat sb = {};
            if (stat((char *)u8bDataHead(fpbuf), &sb) == 0 &&
                S_ISDIR(sb.st_mode)) {
                BROListDir(file_path);
                continue;
            }

            u8bp mapped = NULL;
            ok64 o = FILEMapRO(&mapped, PATHu8cgIn(fpbuf));
            if (o != OK) {
                fprintf(stderr, "bro: cannot open %.*s: %s\n",
                        (int)$len(file_path), (char *)file_path[0],
                        ok64str(o));
                continue;
            }

            hunk *hk = &bro_hunks[bro_nhunks];
            *hk = (hunk){};
            // Store full original arg as URI
            u8p up = BROArenaWrite(u->data[0], (size_t)$len(u->data));
            if (up) { hk->uri[0] = up; hk->uri[1] = up + $len(u->data); }
            hk->text[0] = u8bDataHead(mapped);
            hk->text[1] = u8bIdleHead(mapped);

            Bu32 toks = {};
            b8 tok = BROTokenize(toks, hk, file_path);
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
