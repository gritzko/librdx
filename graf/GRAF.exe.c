//  GRAFExec — run a parsed CLI against an open graf state.
//  Same effect as invoking `graf ...` as a separate process.
//
#include "GRAF.h"
#include "DAG.h"

#include <signal.h>
#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/CLI.h"
#include "dog/HOME.h"
#include "dog/HUNK.h"
#include "keeper/KEEP.h"

// --- Verb / flag tables ---

char const *const GRAF_CLI_VERBS[] = {
    "get", "diff", "merge", "blame", "weave", "index",
    "status", "help", NULL
};

char const GRAF_CLI_VAL_FLAGS[] = "-o\0";

// --- Usage ---

static void graf_usage(void) {
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
    a_path(bropath);
    a$rg(a0, 0);
    a_cstr(bro_name, "bro");
    HOMEResolveSibling(NULL, bropath, bro_name, a0);
    u8cs args[] = {u8slit("bro")};
    u8css argv = {args, args + 1};
    pid_t pid = 0;
    int wfd = -1;
    if (FILESpawn($path(bropath), argv, &wfd, NULL, &pid) != OK) {
        graf_out_fd = STDOUT_FILENO;
        graf_emit   = HUNKu8sFeedText;
        return -1;
    }
    graf_out_fd = wfd;
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
        int rc = 0;
        FILEReap(pid, &rc);
        if (rc == 127)
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

ok64 GRAFExec(cli *c) {
    sane(c);
    graf *g = &GRAF;

    a_cstr(v_get,    "get");
    a_cstr(v_diff,   "diff");
    a_cstr(v_merge,  "merge");
    a_cstr(v_blame,  "blame");
    a_cstr(v_weave,  "weave");
    a_cstr(v_index,  "index");
    a_cstr(v_status, "status");
    a_cstr(v_help,   "help");

    if ($eq(c->verb, v_help) || CLIHas(c, "-h") || CLIHas(c, "--help")) {
        graf_usage(); done;
    }

    if ($empty(c->verb)) {
        graf_usage();
        return FAILSANITY;
    }

    u8cs reporoot = {};
    u8csMv(reporoot, c->repo);
    // If CLI parsing didn't supply a repo, fall back to h->root.
    if ($empty(reporoot) && g->h && g->h->root[0]) {
        a_dup(u8c, hs, u8bDataC(g->h->root));
        u8csMv(reporoot, hs);
    }

    // --- status: uses graf state only ---

    if ($eq(c->verb, v_status)) {
        u64 total_entries = 0;
        for (u32 i = 0; i < g->idx.n; i++)
            total_entries += (u64)(g->idx.runs[i][1] - g->idx.runs[i][0]);
        fprintf(stdout, "graf: %u index run(s), %llu entries\n",
                g->idx.n, (unsigned long long)total_entries);
        done;
    }

    // --- diff: file-based, no keeper needed ---

    if ($eq(c->verb, v_diff)) {
        if (c->nuris < 2) {
            fprintf(stderr, "graf: diff requires 2 files: old new\n");
            return FAILSANITY;
        }
        pid_t pager = graf_start_pager(c->tty_out);
        u8cs op = {}, np = {};
        graf_uri_path(op, &c->uris[0]);
        graf_uri_path(np, &c->uris[1]);
        u8cs nomode = {};
        ok64 ret = GRAFDiff(op, np, np, nomode, nomode);
        graf_stop_pager(pager);
        return ret;
    }

    // --- merge: file-based, no keeper needed ---

    if ($eq(c->verb, v_merge)) {
        if (c->nuris < 3) {
            fprintf(stderr, "graf: merge requires 3 files: base ours theirs\n");
            return FAILSANITY;
        }
        u8cs bp = {}, op = {}, tp = {};
        graf_uri_path(bp, &c->uris[0]);
        graf_uri_path(op, &c->uris[1]);
        graf_uri_path(tp, &c->uris[2]);
        u8cs merge_out = {};
        CLIFlag(merge_out, c, "-o");
        return GRAFMerge(bp, op, tp, merge_out);
    }

    // --- index, blame, weave: require keeper ---

    if (!reporoot[0]) {
        fprintf(stderr, "graf: %.*s requires .dogs/keeper\n",
                (int)$len(c->verb), (char *)c->verb[0]);
        return FAILSANITY;
    }

    
    call(KEEPOpen, g->h, YES);
    ok64 ret = OK;

    if ($eq(c->verb, v_get) || $eq(c->verb, v_index)) {
        // Indexing is now driven by keeper calling GRAFUpdate per
        // object.  Stand-alone `graf index` is a no-op here — the
        // index is built incrementally as keeper ingests packs.
        fprintf(stderr,
            "graf: index is built via DOGUpdate; no action\n");
        ret = OK;
    } else if ($eq(c->verb, v_blame)) {
        if (c->nuris < 1) {
            fprintf(stderr, "graf: blame requires a file URI\n");
            KEEPClose();
            return FAILSANITY;
        }
        pid_t pager = graf_start_pager(c->tty_out);
        u8cs path = {};
        graf_uri_path(path, &c->uris[0]);
        // tip_h=0 for now: unscoped blame (no ancestry filter).
        // A ref-scoped entry point can resolve URI ?ref → commit
        // hashlet and pass it here.
        ret = GRAFBlame(&KEEP, path, 0, reporoot);
        graf_stop_pager(pager);

    } else if ($eq(c->verb, v_weave)) {
        if (c->nuris < 1) {
            fprintf(stderr, "graf: weave requires a file URI\n");
            KEEPClose();
            return FAILSANITY;
        }
        pid_t pager = graf_start_pager(c->tty_out);
        uri *u = &c->uris[0];
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
        ret = GRAFWeaveDiff(&KEEP, path, reporoot, wf, wt);
        graf_stop_pager(pager);

    } else {
        fprintf(stderr, "graf: unknown verb '%.*s'\n",
                (int)$len(c->verb), (char *)c->verb[0]);
        ret = FAILSANITY;
    }

    KEEPClose();
    return ret;
}
