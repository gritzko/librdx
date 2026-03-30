#include "CAPO.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"

// Usage:
//   spot                             incremental update (full reindex if first run)
//   spot file.c                      colorful cat (syntax highlight)
//   spot -i | spot --index           full reindex
//   spot --fork N                    parallel reindex on N cores
//   spot --fork N --proc K           worker K of N (internal)
//   spot --hook                      incremental (post-commit)
//   spot -s "return 0;" .c           SPOT search
//   spot -s "f(x,y)" -r "f(y,x)" .c SPOT search + replace
//   spot -g "TODO" .c                grep in all leaves (incl. comments)
//   spot -g "memmem"                 grep all parseable files
//   spot --diff old new              token-level colored diff
//   spot --gitdiff                   git external diff driver
//   spot --merge base ours theirs    token-level 3-way merge (stdout)
//   spot --merge base ours theirs -o out   merge to file
//   git config: diff.spot.command "spot --gitdiff"
//   git config: merge.spot.driver "spot --merge %O %A %B -o %A"

static void SPOTUsage(void) {
    fprintf(stderr,
        "Usage: spot [options] [files...]\n"
        "\n"
        "  spot                               incremental index update\n"
        "  spot file.c                        syntax-highlighted cat\n"
        "  spot -i | --index                  full reindex\n"
        "  spot --fork N                      parallel reindex on N cores\n"
        "  spot --hook                        post-commit incremental update\n"
        "  spot -s \"pattern\" .ext             structural search\n"
        "  spot -s \"pat\" -r \"repl\" .ext       structural search + replace\n"
        "  spot -g \"text\" [.ext]              grep (substring, incl. comments)\n"
        "  spot -g \"text\" -C N [.ext]         grep with N lines of context\n"
        "  spot -d | --diff old new           token-level colored diff\n"
        "  spot --gitdiff                     git external diff driver\n"
        "  spot --merge base ours theirs      token-level 3-way merge\n"
        "  spot --merge base ours theirs -o f merge to file\n"
        "\n"
        "Patterns: single-letter placeholders (a-z match one token/group,\n"
        "A-Z match multiple tokens). Two spaces = skip gap.\n"
        "\n"
        "Git integration:\n"
        "  git config diff.spot.command \"spot --gitdiff\"\n"
        "  git config merge.spot.driver \"spot --merge %%O %%A %%B -o %%A\"\n"
    );
}

static b8 argeq(u8cs a, const char *b) {
    size_t blen = strlen(b);
    return $len(a) == blen && memcmp(a[0], b, blen) == 0;
}

// Check if a trailing arg is a bare .ext filter known to tok/
static b8 argIsExt(u8cs a) {
    if ($len(a) < 2 || a[0][0] != '.') return NO;
    return CAPOKnownExt(a);
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
    b8 do_index = NO;
    b8 do_merge = NO;
    b8 do_diff = NO;
    b8 do_gitdiff = NO;
    u8c *merge_out[2] = {};
    u8c *spot_ndl[2] = {};
    u8c *spot_rep[2] = {};
    u8c *grep_ndl[2] = {};
    u32 grep_ctx = 3;
    u8c *trail[16][2] = {};
    int ntrail = 0;
    int argn = (int)$arglen;

    for (int i = 1; i < argn; i++) {
        u8c *a[2] = {};
        $mv(a, $arg(i));
        char *eqval = NULL;
        if (argeq(a, "-h") || argeq(a, "--help")) {
            SPOTUsage();
            done;
        } else if (argeq(a, "--fork") && i + 1 < argn) {
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
        } else if (argeq(a, "-d") || argeq(a, "--diff")) {
            do_diff = YES;
        } else if (argeq(a, "--gitdiff")) {
            do_gitdiff = YES;
        } else if (argeq(a, "--merge")) {
            do_merge = YES;
        } else if (argeq(a, "-o") && i + 1 < argn) {
            i++;
            $mv(merge_out, $arg(i));
        } else if (argeq(a, "-i") || argeq(a, "--index")) {
            do_index = YES;
        } else if (argeq(a, "--hook")) {
            is_hook = YES;
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
        } else if ((argeq(a, "-g") || argeq(a, "--grep")) && i + 1 < argn) {
            i++;
            $mv(grep_ndl, $arg(i));
        } else if ((eqval = argeqval(a, "--grep"))) {
            grep_ndl[0] = (u8cp)eqval;
            grep_ndl[1] = (u8cp)eqval + strlen(eqval);
        } else if (argeq(a, "-C") && i + 1 < argn) {
            i++;
            u8c *v[2] = {};
            $mv(v, $arg(i));
            grep_ctx = (u32)atoi((char *)v[0]);
        } else if ((eqval = argeqval(a, "--context"))) {
            grep_ctx = (u32)atoi(eqval);
        } else if (argeq(a, "--context") && i + 1 < argn) {
            i++;
            u8c *v[2] = {};
            $mv(v, $arg(i));
            grep_ctx = (u32)atoi((char *)v[0]);
        } else {
            if (ntrail < 16) { $mv(trail[ntrail], a); ntrail++; }
        }
    }

    // Validate: -r only valid with -s
    if (spot_rep[0] != NULL && spot_ndl[0] == NULL) {
        fprintf(stderr, "spot: --replace requires --spot\n");
        return FAILSANITY;
    }

    if (do_gitdiff) {
        // git diff driver: path old-file old-hex old-mode new-file new-hex new-mode
        if (ntrail < 6) {
            fprintf(stderr, "spot: --gitdiff expects 7 args from git\n");
            return FAILSANITY;
        }
        CAPO_COLOR = YES;  // git pager handles ANSI
        u8cs op = {trail[1][0], trail[1][1]};  // old-file
        u8cs np = {trail[4][0], trail[4][1]};  // new-file
        call(CAPODiff, op, np);
    } else if (do_diff) {
        // Diff mode: expects 2 trailing paths (old new)
        if (ntrail < 2) {
            fprintf(stderr, "spot: --diff requires 2 files: old new\n");
            return FAILSANITY;
        }
        u8cs op = {trail[0][0], trail[0][1]};
        u8cs np = {trail[1][0], trail[1][1]};
        call(CAPODiff, op, np);
    } else if (do_merge) {
        // Merge mode: expects 3 trailing paths (base ours theirs)
        if (ntrail < 3) {
            fprintf(stderr, "spot: --merge requires 3 files: base ours theirs\n");
            return FAILSANITY;
        }
        u8cs bp = {trail[0][0], trail[0][1]};
        u8cs op = {trail[1][0], trail[1][1]};
        u8cs tp = {trail[2][0], trail[2][1]};
        u8cs mo = {merge_out[0], merge_out[1]};
        call(CAPOMerge, bp, op, tp, mo);
    } else if (grep_ndl[0] != NULL) {
        // GREP mode: .ext optional, file paths restrict search
        u8cs ext = {};
        u8cs gfiles[16] = {};
        int gnf = 0;
        for (int i = 0; i < ntrail; i++) {
            if (argIsExt(trail[i])) {
                $mv(ext, trail[i]);
            } else if (gnf < 16) {
                $mv(gfiles[gnf], trail[i]);
                gnf++;
            }
        }
        // No bare .ext — extract extension from first file path
        if ($empty(ext) && gnf > 0) {
            u8cs pe = {};
            PATHu8sExt(pe, gfiles[0]);
            if (!$empty(pe)) {
                ext[0] = pe[0] - 1;  // include the dot
                ext[1] = pe[1];
            }
        }
        u8cs ndl = {grep_ndl[0], grep_ndl[1]};
        u8css gf = {gfiles, gfiles + gnf};
        call(CAPOGrep, ndl, ext, reporoot, grep_ctx, gf);
    } else if (is_hook) {
        call(CAPOHook, reporoot);
    } else if (nfork > 0 && proc != UINT32_MAX) {
        // Worker mode: I am proc K of N
        call(CAPOReindexProc, reporoot, nfork, proc);
    } else if (nfork > 0) {
        // Orchestrator: fork N children, wait, compact
        a_pad(u8, capodir, FILE_PATH_MAX_LEN);
        call(CAPOResolveDir, capodir, reporoot);
        a_dup(u8c, dirslice, u8bDataC(capodir));
        call(FILEMakeDirP, PATHu8cgIn(capodir));

        // Get our own executable path
        char self[FILE_PATH_MAX_LEN];
        ssize_t slen = readlink("/proc/self/exe", self, sizeof(self) - 1);
        test(slen > 0, FAILSANITY);
        self[slen] = 0;

        pid_t pids[256];
        u32 n = nfork;
        if (n > 256) n = 256;

        fprintf(stderr, "spot: forking %u workers\n", n);
        for (u32 k = 0; k < n; k++) {
            pid_t pid = fork();
            if (pid == 0) {
                char nstr[16], kstr[16];
                snprintf(nstr, sizeof(nstr), "%u", n);
                snprintf(kstr, sizeof(kstr), "%u", k);
                execl(self, "spot", "--fork", nstr, "--proc", kstr, NULL);
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
                fprintf(stderr, "spot: worker %u failed (status %d)\n",
                        k, status);
                failures++;
            }
        }

        if (failures > 0)
            fprintf(stderr, "spot: %d workers failed\n", failures);

        fprintf(stderr, "spot: compacting all runs\n");
        call(CAPOCompactAll, dirslice);
        call(CAPOCommitWrite, reporoot, dirslice);
        fprintf(stderr, "spot: done\n");
    } else if (spot_ndl[0] != NULL) {
        // SPOT mode: .ext and/or file paths
        u8cs ext = {};
        u8cs sfiles[16] = {};
        int snf = 0;
        for (int i = 0; i < ntrail; i++) {
            if (argIsExt(trail[i])) {
                $mv(ext, trail[i]);
            } else if (snf < 16) {
                $mv(sfiles[snf], trail[i]);
                snf++;
            }
        }
        // No bare .ext — extract extension from first file path
        if ($empty(ext) && snf > 0) {
            u8cs pe = {};
            PATHu8sExt(pe, sfiles[0]);
            if (!$empty(pe)) {
                ext[0] = pe[0] - 1;  // include the dot
                ext[1] = pe[1];
            }
        }
        if ($empty(ext)) {
            fprintf(stderr, "spot: --spot requires a .ext argument\n");
            return FAILSANITY;
        }
        u8cs ndl = {spot_ndl[0], spot_ndl[1]};
        u8cs rep = {spot_rep[0], spot_rep[1]};
        u8css sf = {sfiles, sfiles + snf};
        call(CAPOSpot, ndl, rep, ext, reporoot, sf);
    } else if (do_index) {
        call(CAPOReindex, reporoot);
    } else if (ntrail > 0) {
        // Cat mode: colorful file output
        u8cs files[16] = {};
        int nf = 0;
        for (int i = 0; i < ntrail && nf < 16; i++) {
            files[nf][0] = trail[i][0];
            files[nf][1] = trail[i][1];
            nf++;
        }
        u8css cf = {files, files + nf};
        call(CAPOCat, cf, reporoot);
    } else {
        call(CAPOHook, reporoot);
    }
    done;
}

MAIN(capocli);
