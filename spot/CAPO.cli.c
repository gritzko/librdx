#include "CAPO.h"
#include "SPOT_VERSION.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sys/stat.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/HOME.h"
#include "dog/HUNK.h"
#include "spot/LESS.h"

static void SPOTUsage(void) {
    fprintf(stderr,
        "Usage: spot [options] [files...]\n"
        "\n"
        "  spot                               incremental index update\n"
        "  spot file.c                        syntax-highlighted cat\n"
        "  spot -i | --index                  full reindex\n"
        "  spot -f N | --fork N               parallel reindex on N cores\n"
        "  spot -u | --uncommitted            index staged + unstaged changes\n"
        "  spot -U | --untracked              also index untracked (new) files\n"
        "  spot --hook                        post-commit incremental update\n"
        "  spot -s \"pattern\" .ext             code snippet search\n"
        "  spot -s \"pat\" -r \"repl\" .ext       code snippet search + replace\n"
        "  spot -g \"text\" [.ext]              grep (substring, incl. comments)\n"
        "  spot -g \"text\" -C N [.ext]         grep with N lines of context\n"
        "  spot -p \"regex\" [.ext]             regex grep (Thompson NFA)\n"
        "  spot -p \"regex\" -C N [.ext]        regex grep with context\n"
        "\n"
        "Patterns: single-letter placeholders (a-z match one token/group,\n"
        "A-Z match multiple tokens). Two spaces = skip gap.\n"
        "\n"
        "Diff/merge tools live in graf — see `graf --help`.\n"
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

// Match "-fVALUE" short flag with attached value, return value or NULL
static char *argshortval(u8cs a, const char *flag) {
    size_t flen = strlen(flag);
    if ($len(a) > flen && memcmp(a[0], flag, flen) == 0)
        return (char *)a[0] + flen;
    return NULL;
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

    // Open spot state per DOG convention
    spot dog = {};
    call(SPOTOpen, &dog, NO);
    if (getenv("SPOT_COLOR")) { dog.color = YES; CAPO_COLOR = YES; }
    a_dup(u8c, reporoot, u8bDataC(dog.home));

    // Parse args
    u32 nfork = 0, proc = UINT32_MAX;
    b8 is_hook = NO;
    b8 do_index = NO;
    b8 do_update = NO;
    b8 do_status = NO;
    b8 do_uncommitted = NO;
    b8 do_untracked = NO;
    b8 tty_out = isatty(STDOUT_FILENO) ? YES : NO;
    b8 force_tlv = NO;
    u8c *spot_ndl[2] = {};
    u8c *spot_rep[2] = {};
    u8c *grep_ndl[2] = {};
    u8c *pcre_ndl[2] = {};
    u32 grep_ctx = 3;
    u8c *trail[16][2] = {};
    int ntrail = 0;
    int argn = (int)$arglen;

    for (int i = 1; i < argn; i++) {
        u8c *a[2] = {};
        $mv(a, $arg(i));
        char *eqval = NULL;
        if (argeq(a, "-v") || argeq(a, "--version")) {
            fprintf(stderr, "spot %s %s\n", SPOT_GIT_TAG, SPOT_COMMIT_HASH);
            done;
        } else if (argeq(a, "-h") || argeq(a, "--help")) {
            SPOTUsage();
            done;
        } else if ((argeq(a, "-f") || argeq(a, "--fork")) && i + 1 < argn) {
            i++;
            u8c *v[2] = {};
            $mv(v, $arg(i));
            nfork = (u32)atoi((char *)v[0]);
        } else if ((eqval = argeqval(a, "--fork"))) {
            nfork = (u32)atoi(eqval);
        } else if ((eqval = argshortval(a, "-f"))) {
            nfork = (u32)atoi(eqval);
        } else if (argeq(a, "--proc") && i + 1 < argn) {
            i++;
            u8c *v[2] = {};
            $mv(v, $arg(i));
            proc = (u32)atoi((char *)v[0]);
        } else if ((eqval = argeqval(a, "--proc"))) {
            proc = (u32)atoi(eqval);
        } else if (argeq(a, "-i") || argeq(a, "--index")) {
            do_index = YES;
        } else if (argeq(a, "--tlv")) {
            force_tlv = YES;
        } else if (argeq(a, "--hook")) {
            is_hook = YES;
        } else if (argeq(a, "--update")) {
            do_update = YES;
        } else if (argeq(a, "--status")) {
            do_status = YES;
        } else if (argeq(a, "-u") || argeq(a, "--uncommitted")) {
            do_uncommitted = YES;
        } else if (argeq(a, "-U") || argeq(a, "--untracked")) {
            do_uncommitted = YES;
            do_untracked = YES;
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
        } else if ((argeq(a, "-p") || argeq(a, "--pcre")) && i + 1 < argn) {
            i++;
            $mv(pcre_ndl, $arg(i));
        } else if ((eqval = argeqval(a, "--pcre"))) {
            pcre_ndl[0] = (u8cp)eqval;
            pcre_ndl[1] = (u8cp)eqval + strlen(eqval);
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

    // Producers that emit hunks: select output fd + serializer.
    // Skip pager when -r (replace), indexing, or hooks.
    pid_t bro_pid = -1;
    b8 produces_hunks =
        (grep_ndl[0] != NULL || pcre_ndl[0] != NULL ||
         spot_ndl[0] != NULL) &&
        !do_index && !is_hook && spot_rep[0] == NULL;
    if (produces_hunks) {
        if (force_tlv) {
            // TLV to stdout (invoked by bro, no pager fork)
            spot_out_fd = STDOUT_FILENO;
            spot_emit   = HUNKu8sFeed;
            signal(SIGPIPE, SIG_IGN);
        } else if (tty_out) {
            // Fork bro as the pager.  Parent stays the producer.
            char bropath[FILE_PATH_MAX_LEN];
            a$rg(a0, 0);
            HOMEResolveSibling(bropath, sizeof(bropath),
                               "bro", (char const *)a0[0]);
            int pfd[2];
            test(pipe(pfd) == 0, FAILSANITY);
            bro_pid = fork();
            test(bro_pid >= 0, FAILSANITY);
            if (bro_pid == 0) {
                // Child = bro pager: stdin from pipe
                close(pfd[1]);
                dup2(pfd[0], STDIN_FILENO);
                close(pfd[0]);
                execlp(bropath, "bro", (char *)NULL);
                _exit(127);
            }
            close(pfd[0]);
            dog.out_fd = pfd[1];
            dog.emit   = HUNKu8sFeed;
            spot_out_fd = dog.out_fd;
            spot_emit   = dog.emit;
            signal(SIGPIPE, SIG_IGN);
        } else {
            // Plain text mode: write git-style ASCII to stdout.
            dog.out_fd = STDOUT_FILENO;
            dog.emit   = HUNKu8sFeedText;
            spot_out_fd = dog.out_fd;
            spot_emit   = dog.emit;
        }
    }

    if (do_status) {
        // DOG convention: --status = short status report
        a_path(capodir);
        call(CAPOResolveDir, capodir, reporoot);
        a_dup(u8c, dirslice, u8bDataC(capodir));
        // Count index files
        u64cs runs[CAPO_MAX_LEVELS] = {};
        u64css stack = {runs, runs};
        u8bp mmaps[CAPO_MAX_LEVELS] = {};
        u32 nidxfiles = 0;
        call(CAPOStackOpen, stack, mmaps, &nidxfiles, dirslice);
        u64 total = 0;
        for (u32 i = 0; i < nidxfiles; i++)
            total += (u64)$len(runs[i]);
        CAPOStackClose(mmaps, nidxfiles);
        fprintf(stderr, "spot: %u index files, %llu entries\n",
                nidxfiles, (unsigned long long)total);
    } else if (do_update) {
        // DOG convention: --update = incremental index update
        call(CAPOHook, reporoot);
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
        a_dup(u8c,ndl,grep_ndl);
        u8css gf = {gfiles, gfiles + gnf};
        call(CAPOGrep, ndl, ext, reporoot, grep_ctx, gf);
    } else if (pcre_ndl[0] != NULL) {
        // PCRE (regex) mode: .ext optional, file paths restrict search
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
        if ($empty(ext) && gnf > 0) {
            u8cs pe = {};
            PATHu8sExt(pe, gfiles[0]);
            if (!$empty(pe)) {
                ext[0] = pe[0] - 1;
                ext[1] = pe[1];
            }
        }
        a_dup(u8c,ndl,pcre_ndl);
        u8css gf = {gfiles, gfiles + gnf};
        call(CAPOPcreGrep, ndl, ext, reporoot, grep_ctx, gf);
    } else if (do_uncommitted) {
        call(CAPOUncommitted, reporoot, do_untracked);
    } else if (is_hook) {
        call(CAPOHook, reporoot);
    } else if (nfork > 0 && proc != UINT32_MAX) {
        // Worker mode: I am proc K of N
        call(CAPOReindexProc, reporoot, nfork, proc);
    } else if (nfork > 0) {
        // Orchestrator: fork N children, wait, compact
        a_path(capodir);
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
        a_dup(u8c,ndl,spot_ndl);
        a_dup(u8c,rep,spot_rep);
        u8css sf = {sfiles, sfiles + snf};
        call(CAPOSpot, ndl, rep, ext, reporoot, sf);
    } else if (do_index) {
        call(CAPOReindex, reporoot);
    } else if (ntrail > 0) {
        fprintf(stderr, "spot: file display moved to bro; run `bro %s`\n",
                (char *)trail[0][0]);
        return FAILSANITY;
    } else {
        call(CAPOHook, reporoot);
    }

    // Close pipe to bro and reap.
    if (spot_out_fd >= 0 && spot_out_fd != STDOUT_FILENO) {
        close(spot_out_fd);
        spot_out_fd = -1;
    }
    if (bro_pid > 0) {
        int st = 0;
        waitpid(bro_pid, &st, 0);
        if (WIFEXITED(st) && WEXITSTATUS(st) == 127)
            fprintf(stderr, "spot: bro pager not found\n");
    }
    SPOTClose(&dog);
    done;
}

MAIN(capocli);
