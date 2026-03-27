#include "CAPO.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"

// Usage:
//   capo                             full reindex
//   capo --fork N                    parallel reindex on N cores
//   capo --fork N --proc K           worker K of N (internal)
//   capo --hook                      incremental (post-commit)
//   capo -c "fn.main"                CSS query
//   capo -c "fn:has(malloc)" .c .h   CSS query, filter to .c/.h files
//   capo -s "return 0;" .c           SPOT search
//   capo -s "f(x,y)" -r "f(y,x)" .c SPOT search + replace

static b8 argeq(u8cs a, const char *b) {
    size_t blen = strlen(b);
    return $len(a) == blen && memcmp(a[0], b, blen) == 0;
}

// Match "--flag=value", return pointer to value after '=' or NULL
static char *argeqval(u8cs a, const char *flag) {
    size_t flen = strlen(flag);
    if ($len(a) > flen + 1 && memcmp(a[0], flag, flen) == 0 &&
        a[0][flen] == '=')
        return (char *)a[0] + flen + 1;
    return NULL;
}

ok64 capocli() {
    sane(1);
    call(FILEInit);
    CAPO_TERM = isatty(STDERR_FILENO) ? YES : NO;
    CAPO_COLOR = isatty(STDOUT_FILENO) ? YES : NO;

    // Find repo root via git
    char rootbuf[FILE_PATH_MAX_LEN];
    FILE *gfp = popen("git rev-parse --show-toplevel", "r");
    test(gfp != NULL, FAILSANITY);
    test(fgets(rootbuf, sizeof(rootbuf), gfp) != NULL, FAILSANITY);
    pclose(gfp);
    size_t rlen = strlen(rootbuf);
    if (rlen > 0 && rootbuf[rlen - 1] == '\n') rootbuf[--rlen] = 0;

    a_pad(u8, root, FILE_PATH_MAX_LEN);
    u8cs rbs = {(u8cp)rootbuf, (u8cp)rootbuf + rlen};
    call(u8bFeed, root, rbs);
    a_dup(u8c, reporoot, u8bDataC(root));

    // Parse args
    u32 nfork = 0, proc = UINT32_MAX;
    b8 is_hook = NO;
    u8c *css_sel[2] = {};
    u8c *spot_ndl[2] = {};
    u8c *spot_rep[2] = {};
    u8c *trail[16][2] = {};
    int ntrail = 0;
    int argn = (int)$arglen;

    for (int i = 1; i < argn; i++) {
        u8c *a[2] = {};
        $mv(a, $arg(i));
        char *eqval = NULL;
        if (argeq(a, "--fork") && i + 1 < argn) {
            i++;
            u8c *v[2] = {};
            $mv(v, $arg(i));
            nfork = (u32)atoi((char *)v[0]);
        } else if ((eqval = argeqval(a, "--fork"))) {
            nfork = (u32)atoi(eqval);
        } else if (argeq(a, "--proc") && i + 1 < argn) {
            i++;
            u8c *v[2] = {};
            $mv(v, $arg(i));
            proc = (u32)atoi((char *)v[0]);
        } else if ((eqval = argeqval(a, "--proc"))) {
            proc = (u32)atoi(eqval);
        } else if (argeq(a, "--hook")) {
            is_hook = YES;
        } else if ((argeq(a, "-c") || argeq(a, "--css")) && i + 1 < argn) {
            i++;
            $mv(css_sel, $arg(i));
        } else if ((eqval = argeqval(a, "--css"))) {
            css_sel[0] = (u8cp)eqval;
            css_sel[1] = (u8cp)eqval + strlen(eqval);
        } else if ((argeq(a, "-s") || argeq(a, "--spot")) && i + 1 < argn) {
            i++;
            $mv(spot_ndl, $arg(i));
        } else if ((eqval = argeqval(a, "--spot"))) {
            spot_ndl[0] = (u8cp)eqval;
            spot_ndl[1] = (u8cp)eqval + strlen(eqval);
        } else if ((argeq(a, "-r") || argeq(a, "--replace")) && i + 1 < argn) {
            i++;
            $mv(spot_rep, $arg(i));
        } else if ((eqval = argeqval(a, "--replace"))) {
            spot_rep[0] = (u8cp)eqval;
            spot_rep[1] = (u8cp)eqval + strlen(eqval);
        } else {
            if (ntrail < 16) { $mv(trail[ntrail], a); ntrail++; }
        }
    }

    // Validate: -r only valid with -s
    if (spot_rep[0] != NULL && spot_ndl[0] == NULL) {
        fprintf(stderr, "capo: --replace requires --spot\n");
        return FAILSANITY;
    }

    if (is_hook) {
        call(CAPOHook, reporoot);
    } else if (nfork > 0 && proc != UINT32_MAX) {
        // Worker mode: I am proc K of N
        call(CAPOReindexProc, reporoot, nfork, proc);
    } else if (nfork > 0) {
        // Orchestrator: fork N children, wait, compact
        a_pad(u8, capodir, FILE_PATH_MAX_LEN);
        call(CAPOResolveDir, capodir, reporoot);
        a_dup(u8c, dirslice, u8bDataC(capodir));
        call(FILEMakeDirP, path8cgIn(capodir));

        // Get our own executable path
        char self[FILE_PATH_MAX_LEN];
        ssize_t slen = readlink("/proc/self/exe", self, sizeof(self) - 1);
        test(slen > 0, FAILSANITY);
        self[slen] = 0;

        pid_t pids[256];
        u32 n = nfork;
        if (n > 256) n = 256;

        fprintf(stderr, "capo: forking %u workers\n", n);
        for (u32 k = 0; k < n; k++) {
            pid_t pid = fork();
            if (pid == 0) {
                char nstr[16], kstr[16];
                snprintf(nstr, sizeof(nstr), "%u", n);
                snprintf(kstr, sizeof(kstr), "%u", k);
                execl(self, "capo", "--fork", nstr, "--proc", kstr, NULL);
                _exit(127);
            }
            test(pid > 0, FAILSANITY);
            pids[k] = pid;
        }

        int failures = 0;
        for (u32 k = 0; k < n; k++) {
            int status = 0;
            waitpid(pids[k], &status, 0);
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                fprintf(stderr, "capo: worker %u failed (status %d)\n",
                        k, status);
                failures++;
            }
        }

        if (failures > 0)
            fprintf(stderr, "capo: %d workers failed\n", failures);

        fprintf(stderr, "capo: compacting all runs\n");
        call(CAPOCompactAll, dirslice);
        call(CAPOCommitWrite, reporoot, dirslice);
        fprintf(stderr, "capo: done\n");
    } else if (spot_ndl[0] != NULL) {
        // SPOT mode: find first trailing .ext
        u8cs ext = {};
        for (int i = 0; i < ntrail; i++) {
            if (trail[i][0][0] == '.') {
                ext[0] = trail[i][0];
                ext[1] = trail[i][1];
                break;
            }
        }
        if ($empty(ext)) {
            fprintf(stderr, "capo: --spot requires a .ext argument\n");
            return FAILSANITY;
        }
        u8cs ndl = {spot_ndl[0], spot_ndl[1]};
        u8cs rep = {spot_rep[0], spot_rep[1]};
        call(CAPOSpot, ndl, rep, ext, reporoot);
    } else if (css_sel[0] != NULL) {
        // CSS mode: collect optional ext filter from trailing args
        u8cs sel = {css_sel[0], css_sel[1]};
        u8cs ext = {};
        for (int i = 0; i < ntrail; i++) {
            if (trail[i][0][0] == '.') {
                ext[0] = trail[i][0];
                ext[1] = trail[i][1];
                break;
            }
        }
        call(CAPOQuery, sel, ext, reporoot);
    } else {
        call(CAPOReindex, reporoot);
    }
    done;
}

MAIN(capocli);
