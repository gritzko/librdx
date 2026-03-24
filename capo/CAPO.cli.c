#include "CAPO.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"

// Usage:
//   capo                       full reindex (single process)
//   capo --fork N              parallel reindex on N cores
//   capo --fork N --proc K     worker K of N (internal)
//   capo --hook                incremental (post-commit)
//   capo '#fn.main'            query with CSS selector

static b8 argeq(u8cs a, const char *b) {
    size_t blen = strlen(b);
    return $len(a) == blen && memcmp(a[0], b, blen) == 0;
}

ok64 capocli() {
    sane(1);
    call(FILEInit);

    // Find repo root via git
    char rootbuf[FILE_PATH_MAX_LEN];
    FILE *gfp = popen("git rev-parse --show-toplevel", "r");
    test(gfp != NULL, FAILsanity);
    test(fgets(rootbuf, sizeof(rootbuf), gfp) != NULL, FAILsanity);
    pclose(gfp);
    size_t rlen = strlen(rootbuf);
    if (rlen > 0 && rootbuf[rlen - 1] == '\n') rootbuf[--rlen] = 0;

    a_pad(u8, root, FILE_PATH_MAX_LEN);
    u8cs rbs = {(u8cp)rootbuf, (u8cp)rootbuf + rlen};
    call(u8bFeed, root, rbs);
    u8cs reporoot = {u8bDataHead(root), u8bIdleHead(root)};

    // Parse args
    u32 nfork = 0, proc = UINT32_MAX;
    b8 is_hook = NO;
    u8c *selector[2] = {};
    int argn = (int)$arglen;

    for (int i = 1; i < argn; i++) {
        u8c *a[2] = {};
        $mv(a, $arg(i));
        if (argeq(a, "--fork") && i + 1 < argn) {
            i++;
            u8c *v[2] = {};
            $mv(v, $arg(i));
            nfork = (u32)atoi((char *)v[0]);
        } else if (argeq(a, "--proc") && i + 1 < argn) {
            i++;
            u8c *v[2] = {};
            $mv(v, $arg(i));
            proc = (u32)atoi((char *)v[0]);
        } else if (argeq(a, "--hook")) {
            is_hook = YES;
        } else {
            $mv(selector, a);
        }
    }

    if (is_hook) {
        call(CAPOHook, reporoot);
    } else if (nfork > 0 && proc != UINT32_MAX) {
        // Worker mode: I am proc K of N
        call(CAPOReindexProc, reporoot, nfork, proc);
    } else if (nfork > 0) {
        // Orchestrator: fork N children, wait, compact
        a_pad(u8, capodir, FILE_PATH_MAX_LEN);
        call(u8bFeed, capodir, reporoot);
        a_cstr(suf, "/" CAPO_DIR);
        call(u8bFeed, capodir, suf);
        u8cs dirslice = {u8bDataHead(capodir), u8bIdleHead(capodir)};
        u8bFeed1(capodir, 0);
        u8bShed1(capodir);
        call(FILEMakeDirP, path8cgIn(capodir));

        // Get our own executable path
        char self[FILE_PATH_MAX_LEN];
        ssize_t slen = readlink("/proc/self/exe", self, sizeof(self) - 1);
        test(slen > 0, FAILsanity);
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
            test(pid > 0, FAILsanity);
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
        fprintf(stderr, "capo: done\n");
    } else if (!$empty(selector)) {
        if (selector[0][0] == '#') selector[0]++;
        u8cs sel = {selector[0], selector[1]};
        call(CAPOQuery, sel, reporoot);
    } else {
        call(CAPOReindex, reporoot);
    }
    done;
}

MAIN(capocli);
