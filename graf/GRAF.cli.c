#include "GRAF.h"
#include "DAG.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/HOME.h"
#include "dog/HUNK.h"

static void GRAFUsage(void) {
    fprintf(stderr,
        "Usage: graf [options] [files...]\n"
        "\n"
        "  graf -d | --diff old new           token-level colored diff\n"
        "  graf --gitdiff                     git external diff driver\n"
        "  graf --merge base ours theirs      token-level 3-way merge\n"
        "  graf --merge base ours theirs -o f merge to file\n"
        "  graf -n | --install                install as git diff/merge driver\n"
        "  graf -i | --index                  index git object graph\n"
        "  graf -s | --status                 show index stats\n"
        "  graf --blame file                  token-level blame\n"
        "  graf --weave file [--from c] [--to c]  weave diff\n"
        "\n"
        "Git integration (manual, or use graf -n):\n"
        "  git config diff.graf.command \"graf --gitdiff\"\n"
        "  git config merge.graf.driver \"graf --merge %%O %%A %%B -o %%A\"\n"
    );
}

static b8 argeq(u8cs a, const char *b) {
    size_t blen = strlen(b);
    return $len(a) == blen && memcmp(a[0], b, blen) == 0;
}

ok64 grafcli() {
    sane(1);
    call(FILEInit);

    b8 tty_out = isatty(STDOUT_FILENO) ? YES : NO;

    // Find workspace root for --install
    a_path(root);
    ok64 rho = HOMEFind(root);
    a_dup(u8c, reporoot, u8bDataC(root));
    if (rho != OK) {
        // Not in a git repo: still allow --diff/--merge with explicit paths
        reporoot[0] = NULL;
        reporoot[1] = NULL;
    }

    // Parse args
    b8 do_diff = NO;
    b8 do_gitdiff = NO;
    b8 do_merge = NO;
    b8 do_install = NO;
    b8 do_index = NO;
    b8 do_status = NO;
    b8 do_blame = NO;
    b8 do_weave = NO;
    u8c *merge_out[2] = {};
    u8c *weave_from[2] = {};
    u8c *weave_to[2] = {};
    u8c *trail[16][2] = {};
    int ntrail = 0;
    int argn = (int)$arglen;

    for (int i = 1; i < argn; i++) {
        u8c *a[2] = {};
        $mv(a, $arg(i));
        if (argeq(a, "-h") || argeq(a, "--help")) {
            GRAFUsage();
            done;
        } else if (argeq(a, "-d") || argeq(a, "--diff")) {
            do_diff = YES;
        } else if (argeq(a, "--gitdiff")) {
            do_gitdiff = YES;
        } else if (argeq(a, "--merge")) {
            do_merge = YES;
        } else if (argeq(a, "-o") && i + 1 < argn) {
            i++;
            $mv(merge_out, $arg(i));
        } else if (argeq(a, "-n") || argeq(a, "--install")) {
            do_install = YES;
        } else if (argeq(a, "-i") || argeq(a, "--index")) {
            do_index = YES;
        } else if (argeq(a, "-s") || argeq(a, "--status")) {
            do_status = YES;
        } else if (argeq(a, "--blame") || argeq(a, "-b")) {
            do_blame = YES;
        } else if (argeq(a, "--weave") || argeq(a, "-w")) {
            do_weave = YES;
        } else if (argeq(a, "--from") && i + 1 < argn) {
            i++;
            $mv(weave_from, $arg(i));
        } else if (argeq(a, "--to") && i + 1 < argn) {
            i++;
            $mv(weave_to, $arg(i));
        } else {
            if (ntrail < 16) { $mv(trail[ntrail], a); ntrail++; }
        }
    }

    // Producers that emit hunks: select output fd + serializer.
    // Skip pager for --merge (writes resolved file) and --install.
    pid_t bro_pid = -1;
    b8 produces_hunks = (do_diff || do_gitdiff || do_blame || do_weave) && !do_merge && !do_install;
    if (produces_hunks) {
        if (tty_out) {
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
            graf_out_fd = pfd[1];
            graf_emit   = HUNKu8sFeed;
            signal(SIGPIPE, SIG_IGN);
        } else {
            graf_out_fd = STDOUT_FILENO;
            graf_emit   = HUNKu8sFeedText;
        }
    }

    if (do_weave) {
        if (reporoot[0] == NULL) {
            fprintf(stderr, "graf: --weave requires a git repo\n");
            return FAILSANITY;
        }
        if (ntrail < 1) {
            fprintf(stderr, "graf: --weave requires a file path\n");
            return FAILSANITY;
        }
        u8cs fp = {trail[0][0], trail[0][1]};
        a_dup(u8c, rr, reporoot);
        a_dup(u8c, wf, weave_from);
        a_dup(u8c, wt, weave_to);
        call(GRAFWeaveDiff, fp, rr, wf, wt);
    } else if (do_blame) {
        if (reporoot[0] == NULL) {
            fprintf(stderr, "graf: --blame requires a git repo\n");
            return FAILSANITY;
        }
        if (ntrail < 1) {
            fprintf(stderr, "graf: --blame requires a file path\n");
            return FAILSANITY;
        }
        u8cs fp = {trail[0][0], trail[0][1]};
        a_dup(u8c, rr, reporoot);
        call(GRAFBlame, fp, rr);
    } else if (do_status) {
        graf g = {};
        a_dup(u8c, rr, reporoot);
        call(GRAFOpen, &g, rr);
        u64 total_entries = 0;
        for (u32 i = 0; i < g.idx.n; i++)
            total_entries += (u64)(g.idx.runs[i][1] - g.idx.runs[i][0]);
        fprintf(stdout, "graf: %u index run(s), %llu entries\n",
                g.idx.n, (unsigned long long)total_entries);
        GRAFClose(&g);
    } else if (do_index) {
        if (reporoot[0] == NULL) {
            fprintf(stderr, "graf: --index requires a git repo\n");
            return FAILSANITY;
        }
        a_dup(u8c, rr, reporoot);
        call(DAGHook, rr);
    } else if (do_install) {
        if (reporoot[0] == NULL) {
            fprintf(stderr, "graf: --install requires a git repo\n");
            return FAILSANITY;
        }
        a_dup(u8c, rr, reporoot);
        call(GRAFInstall, rr);
    } else if (do_gitdiff) {
        // git diff driver: path old-file old-hex old-mode new-file new-hex new-mode
        if (ntrail < 7) {
            fprintf(stderr, "graf: --gitdiff expects 7 args from git\n");
            return FAILSANITY;
        }
        u8cs nm = {trail[0][0], trail[0][1]};
        u8cs op = {trail[1][0], trail[1][1]};
        u8cs om = {trail[3][0], trail[3][1]};
        u8cs np = {trail[4][0], trail[4][1]};
        u8cs nwm = {trail[6][0], trail[6][1]};
        call(GRAFDiff, op, np, nm, om, nwm);
    } else if (do_diff) {
        if (ntrail < 2) {
            fprintf(stderr, "graf: --diff requires 2 files: old new\n");
            return FAILSANITY;
        }
        u8cs op = {trail[0][0], trail[0][1]};
        u8cs np = {trail[1][0], trail[1][1]};
        u8cs nomode = {};
        call(GRAFDiff, op, np, np, nomode, nomode);
    } else if (do_merge) {
        if (ntrail < 3) {
            fprintf(stderr, "graf: --merge requires 3 files: base ours theirs\n");
            return FAILSANITY;
        }
        u8cs bp = {trail[0][0], trail[0][1]};
        u8cs op = {trail[1][0], trail[1][1]};
        u8cs tp = {trail[2][0], trail[2][1]};
        a_dup(u8c, mo, merge_out);
        call(GRAFMerge, bp, op, tp, mo);
    } else {
        GRAFUsage();
        return FAILSANITY;
    }

    // Close pipe to bro and reap.
    if (graf_out_fd >= 0 && graf_out_fd != STDOUT_FILENO) {
        close(graf_out_fd);
        graf_out_fd = -1;
    }
    if (bro_pid > 0) {
        int st = 0;
        waitpid(bro_pid, &st, 0);
        if (WIFEXITED(st) && WEXITSTATUS(st) == 127)
            fprintf(stderr, "graf: bro pager not found\n");
    }
    done;
}

MAIN(grafcli);
