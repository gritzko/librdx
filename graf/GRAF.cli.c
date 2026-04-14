#include "GRAF.h"
#include "DAG.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "dog/CLI.h"
#include "dog/HUNK.h"

static void GRAFUsage(void) {
    fprintf(stderr,
        "Usage: graf [verb] [flags] [URI...]\n"
        "\n"
        "  Verbs:\n"
        "    diff old new                 token-level colored diff\n"
        "    merge base ours theirs       token-level 3-way merge\n"
        "    blame file                   token-level blame\n"
        "    weave file?from..to          weave diff between refs\n"
        "    index                        index git object graph\n"
        "\n"
        "  Flags:\n"
        "    -s                           show index stats\n"
        "    -n | --install               install git diff/merge driver\n"
        "    -o <file>                    merge output file\n"
        "    --gitdiff                    git external diff driver\n"
    );
}

con char *graf_verbs[] = {
    "diff", "merge", "blame", "weave", "index", NULL
};
con char *graf_val_flags = "-o\0";

ok64 grafcli() {
    sane(1);
    call(FILEInit);

    cli c = {};
    call(CLIParse, &c, graf_verbs, graf_val_flags);

    u8cs reporoot = {};
    u8csMv(reporoot, c.repo);

    // Identify verb
    a_cstr(v_diff, "diff");
    a_cstr(v_merge, "merge");
    a_cstr(v_blame, "blame");
    a_cstr(v_weave, "weave");
    a_cstr(v_index, "index");

    b8 is_diff   = $eq(c.verb, v_diff);
    b8 is_merge  = $eq(c.verb, v_merge);
    b8 is_blame  = $eq(c.verb, v_blame);
    b8 is_weave  = $eq(c.verb, v_weave);
    b8 is_index  = $eq(c.verb, v_index);
    b8 is_gitdiff = CLIHas(&c, "--gitdiff");
    b8 is_install = CLIHas(&c, "-n") || CLIHas(&c, "--install");
    b8 is_status  = CLIHas(&c, "-s");

    // Setup pager for hunk producers
    pid_t bro_pid = -1;
    b8 produces_hunks = (is_diff || is_gitdiff || is_blame || is_weave);
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

    // Dispatch
    if (is_weave) {
        if (!reporoot[0]) {
            fprintf(stderr, "graf: weave requires a git repo\n");
            return FAILSANITY;
        }
        if (c.nuris < 1) {
            fprintf(stderr, "graf: weave requires a file URI\n");
            return FAILSANITY;
        }
        // weave path?from..to — parse range from query
        uri *u = &c.uris[0];
        u8cs wf = {}, wt = {};
        if (!u8csEmpty(u->query)) {
            // Split on ".." → from..to
            a_dup(u8c, q, u->query);
            u8cs dots = {(u8cp)"..", (u8cp)".." + 2};
            if (u8csFindS(q, dots) == OK) {
                wf[0] = u->query[0];
                wf[1] = q[0];
                wt[0] = q[0] + 2;
                wt[1] = u->query[1];
            } else {
                // Single ref = --to
                u8csMv(wt, u->query);
            }
        }
        u8cs path = {};
        if (!u8csEmpty(u->path))
            u8csMv(path, u->path);
        else
            u8csMv(path, u->data);
        call(GRAFWeaveDiff, path, reporoot, wf, wt);

    } else if (is_blame) {
        if (!reporoot[0]) {
            fprintf(stderr, "graf: blame requires a git repo\n");
            return FAILSANITY;
        }
        if (c.nuris < 1) {
            fprintf(stderr, "graf: blame requires a file URI\n");
            return FAILSANITY;
        }
        uri *u = &c.uris[0];
        u8cs path = {};
        if (!u8csEmpty(u->path))
            u8csMv(path, u->path);
        else
            u8csMv(path, u->data);
        call(GRAFBlame, path, reporoot);

    } else if (is_status) {
        graf g = {};
        call(GRAFOpen, &g, reporoot);
        u64 total_entries = 0;
        for (u32 i = 0; i < g.idx.n; i++)
            total_entries += (u64)(g.idx.runs[i][1] - g.idx.runs[i][0]);
        fprintf(stdout, "graf: %u index run(s), %llu entries\n",
                g.idx.n, (unsigned long long)total_entries);
        GRAFClose(&g);

    } else if (is_index) {
        if (!reporoot[0]) {
            fprintf(stderr, "graf: index requires a git repo\n");
            return FAILSANITY;
        }
        call(DAGHook, reporoot);

    } else if (is_install) {
        if (!reporoot[0]) {
            fprintf(stderr, "graf: install requires a git repo\n");
            return FAILSANITY;
        }
        call(GRAFInstall, reporoot);

    } else if (is_gitdiff) {
        // git external diff driver: 7 URI args from git
        if (c.nuris < 7) {
            fprintf(stderr, "graf: --gitdiff expects 7 args from git\n");
            return FAILSANITY;
        }
        u8cs t[7] = {};
        for (int i = 0; i < 7; i++) {
            if (!u8csEmpty(c.uris[i].path))
                u8csMv(t[i], c.uris[i].path);
            else
                u8csMv(t[i], c.uris[i].data);
        }
        call(GRAFDiff, t[1], t[4], t[0], t[3], t[6]);

    } else if (is_diff) {
        if (c.nuris < 2) {
            fprintf(stderr, "graf: diff requires 2 files: old new\n");
            return FAILSANITY;
        }
        u8cs op = {}, np = {};
        if (!u8csEmpty(c.uris[0].path))
            u8csMv(op, c.uris[0].path);
        else
            u8csMv(op, c.uris[0].data);
        if (!u8csEmpty(c.uris[1].path))
            u8csMv(np, c.uris[1].path);
        else
            u8csMv(np, c.uris[1].data);
        u8cs nomode = {};
        call(GRAFDiff, op, np, np, nomode, nomode);

    } else if (is_merge) {
        if (c.nuris < 3) {
            fprintf(stderr, "graf: merge requires 3 files: base ours theirs\n");
            return FAILSANITY;
        }
        u8cs bp = {}, op = {}, tp = {};
        for (int i = 0; i < 3; i++) {
            u8cs *dst = i == 0 ? &bp : i == 1 ? &op : &tp;
            if (!u8csEmpty(c.uris[i].path))
                u8csMv(*dst, c.uris[i].path);
            else
                u8csMv(*dst, c.uris[i].data);
        }
        u8cs merge_out = {};
        CLIFlag(merge_out, &c, "-o");
        call(GRAFMerge, bp, op, tp, merge_out);

    } else {
        GRAFUsage();
        return FAILSANITY;
    }

    // Close pipe to bro and reap
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
