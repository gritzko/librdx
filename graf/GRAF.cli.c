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
#include "dog/CLI.h"
#include "dog/HUNK.h"

static void GRAFUsage(void) {
    fprintf(stderr,
        "Usage: graf [flags] [URI|files...]\n"
        "\n"
        "  graf -d old new               token-level colored diff\n"
        "  graf --gitdiff                git external diff driver\n"
        "  graf --merge base ours theirs token-level 3-way merge\n"
        "  graf --merge base ours theirs -o f  merge to file\n"
        "  graf -n | --install           install as git diff/merge driver\n"
        "  graf -i | --index             index git object graph\n"
        "  graf -s | --status            show index stats\n"
        "  graf --blame file             token-level blame\n"
        "  graf --weave file [--from c] [--to c]  weave diff\n"
        "\n"
        "Git integration (manual, or use graf -n):\n"
        "  git config diff.graf.command \"graf --gitdiff\"\n"
        "  git config merge.graf.driver \"graf --merge %%O %%A %%B -o %%A\"\n"
    );
}

con char *graf_val_flags = "-o\0--from\0--to\0";

ok64 grafcli() {
    sane(1);
    call(FILEInit);

    cli c = {};
    call(CLIParse, &c, NULL, graf_val_flags);

    u8cs reporoot = {};
    u8csMv(reporoot, c.repo);

    // Producers that emit hunks: select output fd + serializer.
    b8 do_diff    = CLIHas(&c, "-d") || CLIHas(&c, "--diff");
    b8 do_gitdiff = CLIHas(&c, "--gitdiff");
    b8 do_merge   = CLIHas(&c, "--merge");
    b8 do_install = CLIHas(&c, "-n") || CLIHas(&c, "--install");
    b8 do_index   = CLIHas(&c, "-i") || CLIHas(&c, "--index");
    b8 do_status  = CLIHas(&c, "-s") || CLIHas(&c, "--status");
    b8 do_blame   = CLIHas(&c, "--blame") || CLIHas(&c, "-b");
    b8 do_weave   = CLIHas(&c, "--weave") || CLIHas(&c, "-w");

    u8cs merge_out = {};
    CLIFlag(merge_out, &c, "-o");
    u8cs weave_from = {};
    CLIFlag(weave_from, &c, "--from");
    u8cs weave_to = {};
    CLIFlag(weave_to, &c, "--to");

    // URI args become trail (file paths etc)
    pid_t bro_pid = -1;
    b8 produces_hunks = (do_diff || do_gitdiff || do_blame || do_weave) &&
                        !do_merge && !do_install;
    if (produces_hunks) {
        if (c.tty_out) {
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

    // Extract file paths from URIs (path component or full data)
    u8cs trail[16] = {};
    u32 ntrail = 0;
    for (u32 i = 0; i < c.nuris && ntrail < 16; i++) {
        if (!u8csEmpty(c.uris[i].path))
            u8csMv(trail[ntrail], c.uris[i].path);
        else
            u8csMv(trail[ntrail], c.uris[i].data);
        ntrail++;
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
        call(GRAFWeaveDiff, trail[0], reporoot, weave_from, weave_to);
    } else if (do_blame) {
        if (reporoot[0] == NULL) {
            fprintf(stderr, "graf: --blame requires a git repo\n");
            return FAILSANITY;
        }
        if (ntrail < 1) {
            fprintf(stderr, "graf: --blame requires a file path\n");
            return FAILSANITY;
        }
        call(GRAFBlame, trail[0], reporoot);
    } else if (do_status) {
        graf g = {};
        call(GRAFOpen, &g, reporoot);
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
        call(DAGHook, reporoot);
    } else if (do_install) {
        if (reporoot[0] == NULL) {
            fprintf(stderr, "graf: --install requires a git repo\n");
            return FAILSANITY;
        }
        call(GRAFInstall, reporoot);
    } else if (do_gitdiff) {
        if (ntrail < 7) {
            fprintf(stderr, "graf: --gitdiff expects 7 args from git\n");
            return FAILSANITY;
        }
        call(GRAFDiff, trail[1], trail[4], trail[0], trail[3], trail[6]);
    } else if (do_diff) {
        if (ntrail < 2) {
            fprintf(stderr, "graf: --diff requires 2 files: old new\n");
            return FAILSANITY;
        }
        u8cs nomode = {};
        call(GRAFDiff, trail[0], trail[1], trail[1], nomode, nomode);
    } else if (do_merge) {
        if (ntrail < 3) {
            fprintf(stderr, "graf: --merge requires 3 files: base ours theirs\n");
            return FAILSANITY;
        }
        call(GRAFMerge, trail[0], trail[1], trail[2], merge_out);
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
