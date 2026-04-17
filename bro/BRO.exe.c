//  BROExec — run a parsed CLI against an open bro state.
//  Same effect as invoking `bro ...` as a separate process.
//
#include "BRO.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/CLI.h"

// --- Usage ---

static void bro_usage(void) {
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

// --- bro has no persistent state: Open/Close are no-ops ---

ok64 BROOpen(bro *b, u8cs home, b8 rw) {
    sane(b);
    *b = (bro){};
    u8csMv(b->home, home);
    b->rw = rw;
    done;
}

ok64 BROClose(bro *b) {
    sane(b);
    (void)b;
    done;
}

// Bro does not index anything — stub to satisfy the DOG 4-fn contract.
ok64 BROUpdate(bro *b, u8 obj_type, u8cs blob, u8csc path) {
    sane(b);
    (void)obj_type; (void)blob; (void)path;
    done;
}

// --- Entry ---

ok64 BROExec(bro *b, cli *c) {
    sane(b && c);

    BRO_COLOR = c->tty_out;
    if (getenv("BRO_COLOR")) BRO_COLOR = YES;
    if (getenv("NO_COLOR"))  BRO_COLOR = NO;

    if (CLIHas(c, "-h") || CLIHas(c, "--help")) {
        bro_usage();
        done;
    }

    if (c->nuris > 0) {
        call(BROArenaInit);
        for (u32 i = 0; i < c->nuris; i++) {
            if (bro_nhunks >= BRO_MAX_HUNKS) break;
            uri *u = &c->uris[i];

            u8cs file_path = {};
            if (!$empty(u->path)) {
                $mv(file_path, u->path);
            } else if (!$empty(u->data)) {
                $mv(file_path, u->data);
            }
            if ($empty(file_path)) continue;

            a_pad(u8, fpbuf, FILE_PATH_MAX_LEN);
            __ = u8bFeed(fpbuf, file_path);
            if (__ != OK) continue;
            __ = PATHu8gTerm(PATHu8gIn(fpbuf));
            if (__ != OK) continue;

            struct stat sb = {};
            if (FILEStat(&sb, PATHu8cgIn(fpbuf)) == OK &&
                S_ISDIR(sb.st_mode)) {
                BROListDir(file_path);
                continue;
            }

            u8bp mapped = NULL;
            ok64 o = FILEMapRO(&mapped, PATHu8cgIn(fpbuf));
            if (o != OK) {
                fprintf(stderr, "bro: cannot open " U8SFMT ": %s\n",
                        u8sFmt(file_path), ok64str(o));
                continue;
            }

            hunk *hk = &bro_hunks[bro_nhunks];
            *hk = (hunk){};
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
