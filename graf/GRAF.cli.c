//  graf CLI: token-level diff, merge, blame, weave, DAG index.
//
//  Verbs:
//    graf diff old new                token-level colored diff
//    graf merge base ours theirs      3-way merge
//    graf blame file                  token-level blame
//    graf weave file?from..to         weave diff between refs
//    graf index                       index object graph from keeper
//    graf status                      show index stats
//    graf help                        this message
//
//  Flags:
//    -o <file>                        merge output file
//    -t | --tlv                       force TLV output
//
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
#include "keeper/KEEP.h"

// --- Verb table ---

static char const *const graf_verbs[] = {
    "get", "diff", "merge", "blame", "weave", "index",
    "status", "help", NULL
};

static char const graf_val_flags[] = "-o\0";

// --- Usage ---

static void GRAFUsage(void) {
    fprintf(stderr,
        "Usage: graf <verb> [flags] [URI...]\n"
        "\n"
        "  Verbs:\n"
        "    diff old new                 token-level colored diff\n"
        "    merge base ours theirs       3-way merge\n"
        "    blame file                   token-level blame\n"
        "    weave file?from..to          weave diff between refs\n"
        "    index                        index object graph from keeper\n"
        "    status                       show index stats\n"
        "    help                         this message\n"
        "\n"
        "  Flags:\n"
        "    -o <file>                    merge output file\n"
        "    -t | --tlv                   force TLV output\n"
    );
}

// --- Bro pager setup ---

static pid_t graf_start_pager(b8 tty_out) {
    if (!tty_out) {
        graf_out_fd = STDOUT_FILENO;
        graf_emit   = HUNKu8sFeedText;
        return -1;
    }
    char bropath[FILE_PATH_MAX_LEN];
    a$rg(a0, 0);
    HOMEResolveSibling(bropath, sizeof(bropath),
                       "bro", (char const *)a0[0]);
    int pfd[2];
    if (pipe(pfd) != 0) {
        graf_out_fd = STDOUT_FILENO;
        graf_emit   = HUNKu8sFeedText;
        return -1;
    }
    pid_t pid = fork();
    if (pid < 0) {
        close(pfd[0]); close(pfd[1]);
        graf_out_fd = STDOUT_FILENO;
        graf_emit   = HUNKu8sFeedText;
        return -1;
    }
    if (pid == 0) {
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
    return pid;
}

static void graf_stop_pager(pid_t pid) {
    if (graf_out_fd >= 0 && graf_out_fd != STDOUT_FILENO) {
        close(graf_out_fd);
        graf_out_fd = -1;
    }
    if (pid > 0) {
        int st = 0;
        waitpid(pid, &st, 0);
        if (WIFEXITED(st) && WEXITSTATUS(st) == 127)
            fprintf(stderr, "graf: bro pager not found\n");
    }
}

// --- URI path helper ---

static void graf_uri_path(u8cs out, uri *u) {
    if (!u8csEmpty(u->path))
        u8csMv(out, u->path);
    else
        u8csMv(out, u->data);
}

// --- Entry ---

ok64 grafcli() {
    sane(1);
    call(FILEInit);

    cli c = {};
    call(CLIParse, &c, graf_verbs, graf_val_flags);

    a_cstr(v_get,    "get");
    a_cstr(v_diff,   "diff");
    a_cstr(v_merge,  "merge");
    a_cstr(v_blame,  "blame");
    a_cstr(v_weave,  "weave");
    a_cstr(v_index,  "index");
    a_cstr(v_status, "status");
    a_cstr(v_help,   "help");

    if ($eq(c.verb, v_help) || CLIHas(&c, "-h") || CLIHas(&c, "--help")) {
        GRAFUsage(); done;
    }

    if ($empty(c.verb)) {
        GRAFUsage();
        return FAILSANITY;
    }

    u8cs reporoot = {};
    u8csMv(reporoot, c.repo);

    // --- status: no pager, no keeper needed ---

    if ($eq(c.verb, v_status)) {
        graf g = {};
        call(GRAFOpen, &g, reporoot);
        u64 total_entries = 0;
        for (u32 i = 0; i < g.idx.n; i++)
            total_entries += (u64)(g.idx.runs[i][1] - g.idx.runs[i][0]);
        fprintf(stdout, "graf: %u index run(s), %llu entries\n",
                g.idx.n, (unsigned long long)total_entries);
        GRAFClose(&g);
        done;
    }

    // --- diff: file-based, no keeper needed ---

    if ($eq(c.verb, v_diff)) {
        if (c.nuris < 2) {
            fprintf(stderr, "graf: diff requires 2 files: old new\n");
            return FAILSANITY;
        }
        pid_t pager = graf_start_pager(c.tty_out);
        u8cs op = {}, np = {};
        graf_uri_path(op, &c.uris[0]);
        graf_uri_path(np, &c.uris[1]);
        u8cs nomode = {};
        ok64 ret = GRAFDiff(op, np, np, nomode, nomode);
        graf_stop_pager(pager);
        return ret;
    }

    // --- merge: file-based, no keeper needed ---

    if ($eq(c.verb, v_merge)) {
        if (c.nuris < 3) {
            fprintf(stderr, "graf: merge requires 3 files: base ours theirs\n");
            return FAILSANITY;
        }
        u8cs bp = {}, op = {}, tp = {};
        graf_uri_path(bp, &c.uris[0]);
        graf_uri_path(op, &c.uris[1]);
        graf_uri_path(tp, &c.uris[2]);
        u8cs merge_out = {};
        CLIFlag(merge_out, &c, "-o");
        return GRAFMerge(bp, op, tp, merge_out);
    }

    // --- index, blame, weave: require keeper ---

    if (!reporoot[0]) {
        fprintf(stderr, "graf: %.*s requires .dogs/keeper\n",
                (int)$len(c.verb), (char *)c.verb[0]);
        return FAILSANITY;
    }

    keeper k = {};
    call(KEEPOpen, &k, reporoot);
    ok64 ret = OK;

    if ($eq(c.verb, v_get) || $eq(c.verb, v_index)) {
        ret = DAGHook(&k, reporoot);

    } else if ($eq(c.verb, v_blame)) {
        if (c.nuris < 1) {
            fprintf(stderr, "graf: blame requires a file URI\n");
            KEEPClose(&k);
            return FAILSANITY;
        }
        pid_t pager = graf_start_pager(c.tty_out);
        u8cs path = {};
        graf_uri_path(path, &c.uris[0]);
        ret = GRAFBlame(&k, path, reporoot);
        graf_stop_pager(pager);

    } else if ($eq(c.verb, v_weave)) {
        if (c.nuris < 1) {
            fprintf(stderr, "graf: weave requires a file URI\n");
            KEEPClose(&k);
            return FAILSANITY;
        }
        pid_t pager = graf_start_pager(c.tty_out);
        uri *u = &c.uris[0];
        u8cs wf = {}, wt = {};
        if (!u8csEmpty(u->query)) {
            a_dup(u8c, q, u->query);
            u8cs dots = {(u8cp)"..", (u8cp)".." + 2};
            if (u8csFindS(q, dots) == OK) {
                wf[0] = u->query[0];
                wf[1] = q[0];
                wt[0] = q[0] + 2;
                wt[1] = u->query[1];
            } else {
                u8csMv(wt, u->query);
            }
        }
        u8cs path = {};
        graf_uri_path(path, u);
        ret = GRAFWeaveDiff(&k, path, reporoot, wf, wt);
        graf_stop_pager(pager);

    } else {
        fprintf(stderr, "graf: unknown verb '%.*s'\n",
                (int)$len(c.verb), (char *)c.verb[0]);
        ret = FAILSANITY;
    }

    KEEPClose(&k);
    return ret;
}

MAIN(grafcli);
