#include "CAPO.h"
#include "SPOT_VERSION.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/CLI.h"
#include "dog/FRAG.h"
#include "dog/HOME.h"
#include "dog/HUNK.h"
#include "spot/LESS.h"

static void SPOTUsage(void) {
    fprintf(stderr,
        "Usage: spot [--flags] [URI...]\n"
        "\n"
        "  spot                               incremental index update\n"
        "  spot -i | --index                  full reindex\n"
        "  spot -f N | --fork N               parallel reindex on N cores\n"
        "  spot -u | --uncommitted            index staged + unstaged changes\n"
        "  spot -U | --untracked              also index untracked (new) files\n"
        "  spot --hook                        post-commit incremental update\n"
        "  spot -s \"pattern\" .ext             code snippet search\n"
        "  spot -s \"pat\" -r \"repl\" .ext       code snippet search + replace\n"
        "  spot -g \"text\" [.ext]              grep (substring)\n"
        "  spot -p \"regex\" [.ext]             regex grep\n"
        "  spot '#pattern.ext'                URI-style search\n"
        "\n"
        "Patterns: single-letter placeholders (a-z match one token/group,\n"
        "A-Z match multiple tokens). Two spaces = skip gap.\n"
        "\n"
        "Diff/merge tools live in graf — see `graf --help`.\n"
    );
}

// Check if a trailing arg is a bare .ext filter known to tok/
static b8 argIsExt(u8csc a) {
    if ($len(a) < 2 || a[0][0] != '.') return NO;
    return CAPOKnownExt(a);
}

// Spot verb names (shared verbs dispatched by be)
static char const *const SPOT_VERB_NAMES[] = {
    "get", "status", "help", NULL
};

// Spot's val-flags: -f -g -s -r -p -C --fork --proc --grep
// --spot --replace --pcre --context
static char const SPOT_VAL_FLAGS[] =
    "-f\0-g\0-s\0-r\0-p\0-C\0"
    "--fork\0--proc\0--grep\0--spot\0--replace\0--pcre\0--context\0";

ok64 capocli() {
    sane(1);
    call(FILEInit);

    // Open spot state per DOG convention
    spot dog = {};
    call(SPOTOpen, &dog, NO);
    if (getenv("SPOT_COLOR")) { dog.color = YES; CAPO_COLOR = YES; }
    a_dup(u8c, reporoot, dog.home);

    // Parse CLI
    cli c = {};
    call(CLIParse, &c, SPOT_VERB_NAMES, SPOT_VAL_FLAGS);

    // Extract flags
    u8cs v = {};

    if (CLIHas(&c, "-v") || CLIHas(&c, "--version")) {
        fprintf(stderr, "spot %s %s\n", SPOT_GIT_TAG, SPOT_COMMIT_HASH);
        SPOTClose(&dog);
        done;
    }
    if (CLIHas(&c, "-h") || CLIHas(&c, "--help")) {
        SPOTUsage();
        SPOTClose(&dog);
        done;
    }

    // Verb dispatch
    a_cstr(v_get, "get");
    a_cstr(v_status_verb, "status");
    a_cstr(v_help_verb, "help");

    if ($eq(c.verb, v_help_verb)) { SPOTUsage(); SPOTClose(&dog); done; }
    if ($eq(c.verb, v_status_verb)) {
        a_path(capodir);
        CAPOResolveDir(capodir, reporoot);
        a_dup(u8c, dirslice, u8bDataC(capodir));
        u64cs runs[CAPO_MAX_LEVELS] = {};
        u64css stack = {runs, runs};
        u8bp mmaps[CAPO_MAX_LEVELS] = {};
        u32 nidxfiles = 0;
        CAPOStackOpen(stack, mmaps, &nidxfiles, dirslice);
        u64 total = 0;
        for (u32 i = 0; i < nidxfiles; i++)
            total += (u64)$len(runs[i]);
        CAPOStackClose(mmaps, nidxfiles);
        fprintf(stderr, "spot: %u index files, %llu entries\n",
                nidxfiles, (unsigned long long)total);
        SPOTClose(&dog);
        done;
    }
    if ($eq(c.verb, v_get)) {
        call(CAPOHook, reporoot);
        SPOTClose(&dog);
        done;
    }

    b8 do_index = CLIHas(&c, "-i") || CLIHas(&c, "--index");
    b8 do_update = CLIHas(&c, "--update");
    b8 do_status = CLIHas(&c, "--status");
    b8 is_hook = CLIHas(&c, "--hook");
    b8 force_tlv = CLIHas(&c, "-t") || CLIHas(&c, "--tlv");
    b8 do_uncommitted = CLIHas(&c, "-u") || CLIHas(&c, "--uncommitted");
    b8 do_untracked = CLIHas(&c, "-U") || CLIHas(&c, "--untracked");
    if (do_untracked) do_uncommitted = YES;

    u32 nfork = 0;
    CLIFlag(v, &c, "-f");
    if (!$empty(v)) nfork = (u32)atoi((char *)v[0]);
    CLIFlag(v, &c, "--fork");
    if (!$empty(v)) nfork = (u32)atoi((char *)v[0]);

    u32 proc = UINT32_MAX;
    CLIFlag(v, &c, "--proc");
    if (!$empty(v)) proc = (u32)atoi((char *)v[0]);

    u32 grep_ctx = 3;
    CLIFlag(v, &c, "-C");
    if (!$empty(v)) grep_ctx = (u32)atoi((char *)v[0]);
    CLIFlag(v, &c, "--context");
    if (!$empty(v)) grep_ctx = (u32)atoi((char *)v[0]);

    // Search patterns from flags
    u8cs spot_ndl = {}, spot_rep = {}, grep_ndl = {}, pcre_ndl = {};
    CLIFlag(v, &c, "-s");
    if (!$empty(v)) { $mv(spot_ndl, v); }
    CLIFlag(v, &c, "--spot");
    if (!$empty(v)) { $mv(spot_ndl, v); }
    CLIFlag(v, &c, "-r");
    if (!$empty(v)) { $mv(spot_rep, v); }
    CLIFlag(v, &c, "--replace");
    if (!$empty(v)) { $mv(spot_rep, v); }
    CLIFlag(v, &c, "-g");
    if (!$empty(v)) { $mv(grep_ndl, v); }
    CLIFlag(v, &c, "--grep");
    if (!$empty(v)) { $mv(grep_ndl, v); }
    CLIFlag(v, &c, "-p");
    if (!$empty(v)) { $mv(pcre_ndl, v); }
    CLIFlag(v, &c, "--pcre");
    if (!$empty(v)) { $mv(pcre_ndl, v); }

    // Collect trail args: extensions (.c) and file paths from URIs
    u8cs trail[16] = {};
    int ntrail = 0;
    for (u32 ui = 0; ui < c.nuris && ntrail < 16; ui++) {
        uri *u = &c.uris[ui];
        // If URI has a fragment and no explicit -g/-s/-p, dispatch by type
        if ($empty(spot_ndl) && $empty(grep_ndl) && $empty(pcre_ndl) &&
            !$empty(u->fragment)) {
            frag fr = {};
            if (FRAGu8sDrain(u->fragment, &fr) == OK) {
                if (fr.type == FRAG_SPOT && !$empty(fr.body)) {
                    $mv(spot_ndl, fr.body);
                } else if (fr.type == FRAG_PCRE && !$empty(fr.body)) {
                    $mv(pcre_ndl, fr.body);
                } else if (fr.type == FRAG_IDENT && !$empty(fr.body)) {
                    $mv(grep_ndl, fr.body);
                }
                // Collect ext filters from fragment
                for (u8 ei = 0; ei < fr.nexts && ntrail < 16; ei++) {
                    // Reconstruct ".ext" slice
                    // The ext in frag points past the dot; back up one
                    if (!$empty(fr.exts[ei]) && fr.exts[ei][0] > u->data[0]) {
                        trail[ntrail][0] = fr.exts[ei][0] - 1; // include dot
                        trail[ntrail][1] = fr.exts[ei][1];
                        ntrail++;
                    }
                }
            }
            // Path from URI → file filter
            if (!$empty(u->path)) {
                u8cs gpath = {};
                $mv(gpath, u->path);
                if (!$empty(gpath) && *gpath[0] == '/') {
                    u8csFed(gpath, 1);
                }
                if (!$empty(gpath) && ntrail < 16) {
                    $mv(trail[ntrail], gpath);
                    ntrail++;
                }
            }
        } else {
            // Plain URI arg → trail (ext or file path)
            u8cs dat = {};
            if (!$empty(u->path)) {
                $mv(dat, u->path);
            } else if (!$empty(u->data)) {
                $mv(dat, u->data);
            }
            if (!$empty(dat) && ntrail < 16) {
                $mv(trail[ntrail], dat);
                ntrail++;
            }
        }
    }

    // Validate: -r only valid with -s
    if (!$empty(spot_rep) && $empty(spot_ndl)) {
        fprintf(stderr, "spot: --replace requires --spot\n");
        SPOTClose(&dog);
        return FAILSANITY;
    }

    // Output setup
    pid_t bro_pid = -1;
    b8 produces_hunks =
        (!$empty(grep_ndl) || !$empty(pcre_ndl) || !$empty(spot_ndl)) &&
        !do_index && !is_hook && $empty(spot_rep);
    if (produces_hunks) {
        if (force_tlv) {
            spot_out_fd = STDOUT_FILENO;
            spot_emit   = HUNKu8sFeed;
            signal(SIGPIPE, SIG_IGN);
        } else if (c.tty_out) {
            char bropath[FILE_PATH_MAX_LEN];
            a$rg(a0, 0);
            HOMEResolveSibling(bropath, sizeof(bropath),
                               "bro", (char const *)a0[0]);
            int pfd[2];
            test(pipe(pfd) == 0, FAILSANITY);
            bro_pid = fork();
            test(bro_pid >= 0, FAILSANITY);
            if (bro_pid == 0) {
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
            dog.out_fd = STDOUT_FILENO;
            dog.emit   = HUNKu8sFeedText;
            spot_out_fd = dog.out_fd;
            spot_emit   = dog.emit;
        }
    }

    // --- Dispatch ---

    if (do_status) {
        a_path(capodir);
        call(CAPOResolveDir, capodir, reporoot);
        a_dup(u8c, dirslice, u8bDataC(capodir));
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
        call(CAPOHook, reporoot);
    } else if (!$empty(grep_ndl)) {
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
        a_dup(u8c, ndl, grep_ndl);
        u8css gf = {gfiles, gfiles + gnf};
        call(CAPOGrep, ndl, ext, reporoot, grep_ctx, gf);
    } else if (!$empty(pcre_ndl)) {
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
        a_dup(u8c, ndl, pcre_ndl);
        u8css gf = {gfiles, gfiles + gnf};
        call(CAPOPcreGrep, ndl, ext, reporoot, grep_ctx, gf);
    } else if (do_uncommitted) {
        call(CAPOUncommitted, reporoot, do_untracked);
    } else if (is_hook) {
        call(CAPOHook, reporoot);
    } else if (nfork > 0 && proc != UINT32_MAX) {
        call(CAPOReindexProc, reporoot, nfork, proc);
    } else if (nfork > 0) {
        a_path(capodir);
        call(CAPOResolveDir, capodir, reporoot);
        a_dup(u8c, dirslice, u8bDataC(capodir));
        call(FILEMakeDirP, PATHu8cgIn(capodir));

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
    } else if (!$empty(spot_ndl)) {
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
        if ($empty(ext) && snf > 0) {
            u8cs pe = {};
            PATHu8sExt(pe, sfiles[0]);
            if (!$empty(pe)) {
                ext[0] = pe[0] - 1;
                ext[1] = pe[1];
            }
        }
        if ($empty(ext)) {
            fprintf(stderr, "spot: --spot requires a .ext argument\n");
            SPOTClose(&dog);
            return FAILSANITY;
        }
        a_dup(u8c, ndl, spot_ndl);
        a_dup(u8c, rep, spot_rep);
        u8css sf = {sfiles, sfiles + snf};
        call(CAPOSpot, ndl, rep, ext, reporoot, sf);
    } else if (do_index) {
        call(CAPOReindex, reporoot);
    } else if (c.nuris > 0) {
        fprintf(stderr, "spot: file display moved to bro\n");
        SPOTClose(&dog);
        return FAILSANITY;
    } else {
        call(CAPOHook, reporoot);
    }

    // Cleanup
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
