#include "BRO.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"

static void BROUsage(void) {
    fprintf(stderr,
        "Usage: bro [files...]\n"
        "\n"
        "  bro                   read TLV hunks from stdin (pager mode)\n"
        "  bro file.c [...]      syntax-highlighted cat\n"
        "\n"
        "Keys: q quit, space/f page down, b page up, j/k line, g/G top/end,\n"
        "      / search, n/N next/prev, : goto line\n");
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
            // -h / --help
            if (($len(a) == 2 && a[0][1] == 'h') ||
                ($len(a) == 6 && memcmp(a[0], "--help", 6) == 0)) {
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
        u8css cf = {files, files + nf};
        u8cs nomark = {};
        call(BROCat, cf, nomark);
    } else {
        call(BROPipeRun, STDIN_FILENO);
    }
    done;
}

MAIN(brocli);
