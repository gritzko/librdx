#include "abc/URI.h"
#include "dog/CLI.h"
#include "dog/FRAG.h"

#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/HOME.h"

// Distinct codes so the MAIN-wrapper's `Error: <code>` line tells you
// what kind of failure stopped the pipeline — a dog exited non-zero
// (BEDOGEXIT) or died from a signal (BEDOGSIG). Generic BEFAIL is
// reserved for be's own internal slips. RON60 caps names at ~10 chars
// (60-bit base64 encoding) — verify with `abc/ok64 NAME`.
con ok64 BEFAIL    = 0x2ce3ca495;
con ok64 BEDOGEXIT = 0xb38d6103a149d;
con ok64 BEDOGSIG  = 0x2ce35841c490;

// --- Verb table ---

static char const *const BE_VERB_NAMES[] = {
    "get", "post", "put", "delete",
    "diff", "patch", "merge", "sync",
    "status",
    NULL
};

static void BEUsage(void) {
    fprintf(stderr,
        "Usage: be [verb] [--flags] [URI...]\n"
        "\n"
        "Verbs:\n"
        "  get [uri]            view file, search, show log\n"
        "  put [uri]            stage, commit, push\n"
        "  patch [uri]          search & replace, reindex\n"
        "  diff [uri]           token-level diff\n"
        "  merge [uri]          3-way merge\n"
        "  delete [uri]         remove file, branch\n"
        "  sync [uri]           fetch + merge (or push)\n"
        "  status               show repo status\n"
        "\n"
        "URI format: [//remote] [path] [?ref] [#search]\n"
        "\n"
        "Bare `be` = ensure indexes are current, show status.\n"
    );
}

// --- Run a sibling tool ---

static ok64 BERun(const char *tool, char *const argv[], b8 bg) {
    sane(tool != NULL && argv != NULL);
    char path[FILE_PATH_MAX_LEN];
    a$rg(a0, 0);
    HOMEResolveSibling(path, sizeof(path), tool, (char const *)a0[0]);
    pid_t pid = fork();
    test(pid >= 0, FAILSANITY);
    if (pid == 0) {
        execv(path, argv);
        _exit(127);
    }
    if (bg) done;
    int st = 0;
    waitpid(pid, &st, 0);
    if (WIFEXITED(st) && WEXITSTATUS(st) != 0) {
        fprintf(stderr, "be: %s exited %d\n", tool, WEXITSTATUS(st));
        return BEDOGEXIT;  // child told us it failed cleanly
    } else if (WIFSIGNALED(st)) {
        // Surface signal-based deaths (SIGBUS, SIGSEGV, SIGABRT…)
        // so a crashing dog isn't mistaken for a clean run.
        int sig = WTERMSIG(st);
        char const *sname = strsignal(sig);
        fprintf(stderr, "be: %s killed by signal %d (%s)\n",
                tool, sig, sname ? sname : "?");
        return BEDOGSIG;
    }
    done;
}

// --- Read .dogs/DOGS list ---

static u32 BEReadDogs(char out[][64], u32 maxn) {
    a_path(home);
    if (HOMEFindDogs(home) != OK) return 0;
    a_cstr(dogs_dir, ".dogs");
    a_cstr(dogs_file, "DOGS");
    if (PATHu8bPush(home, dogs_dir) != OK) return 0;
    if (PATHu8bPush(home, dogs_file) != OK) return 0;

    u8bp mapped = NULL;
    if (FILEMapRO(&mapped, PATHu8cgIn(home)) != OK) return 0;
    a_dup(u8c, data, u8bDataC(mapped));

    u32 count = 0;
    while (!$empty(data) && count < maxn) {
        u8cp nl = data[0];
        while (nl < data[1] && *nl != '\n') nl++;
        size_t len = (size_t)(nl - data[0]);
        if (len > 0 && data[0][0] != '#') {
            if (len >= 64) len = 63;
            memcpy(out[count], data[0], len);
            out[count][len] = 0;
            count++;
        }
        data[0] = (nl < data[1]) ? nl + 1 : data[1];
    }

    FILEUnMap(mapped);
    return count;
}

// --- Verb dispatch: forward URI to dogs in order ---
//
// Each dog parses the URI and handles its part:
//   get:    keeper (fetch) → sniff (checkout) → spot (index) → graf (index)
//   post:   sniff (build tree) → keeper (push) → spot → graf
//   put:    sniff (commit) → keeper (move ref) → spot → graf
//   delete: sniff (remove) → spot → graf

typedef struct {
    char const *dog;
    char const *verb;
    b8 bg;             // run in background (don't wait)
} dog_step;

static ok64 BEDispatch(cli *c, dog_step const *steps, u32 nsteps,
                        b8 seq) {
    sane(c && steps);
    for (u32 i = 0; i < nsteps; i++) {
        // argv: dog verb [flags...] [URIs...]
        // All slices borrow from process argv — already NUL-terminated.
        char *argv[2 + CLI_MAX_FLAGS * 2 + CLI_MAX_URIS + 1];
        u32 ac = 0;
        argv[ac++] = (char *)steps[i].dog;
        argv[ac++] = (char *)steps[i].verb;
        for (u32 j = 0; j < c->nflags; j++)
            argv[ac++] = (char *)c->flags[j][0];
        for (u32 j = 0; j < c->nuris; j++)
            argv[ac++] = (char *)c->uris[j].data[0];
        argv[ac] = NULL;
        call(BERun, steps[i].dog, argv, seq ? NO : steps[i].bg);
    }
    done;
}

static ok64 BEGet(cli *c, b8 seq) {
    sane(c);
    static dog_step const steps[] = {
        {"keeper", "get",  NO},
        {"sniff",  "get",  NO},
        {"spot",   "get",  NO},
        {"graf",   "get",  NO},  // foreground: surface graf's stderr
                                 // before the next prompt
    };
    // Skip keeper fetch if no remote (no authority)
    uri *u = (c->nuris > 0) ? &c->uris[0] : NULL;
    u32 start = (u != NULL && !$empty(u->authority)) ? 0 : 1;

    // Bootstrap: when cloning into a fresh dir (remote URI, no existing
    // .dogs/ anywhere up to /), create .dogs/ in cwd so each dog can
    // place its own subdir. Without this, every dog fails with
    // KEEPFAIL/SNIFFFAIL/etc. before printing anything useful.
    if (start == 0) {
        a_path(probe);
        ok64 ho = HOMEFindDogs(probe);
        if (ho != OK) {
            a_path(here);
            char cwdbuf[FILE_PATH_MAX_LEN];
            if (getcwd(cwdbuf, sizeof(cwdbuf)) != NULL) {
                a_cstr(cwds, cwdbuf);
                a_cstr(dotdogs, ".dogs");
                call(PATHu8bFeed, here, cwds);
                call(PATHu8bPush, here, dotdogs);
                call(FILEMakeDirP, PATHu8cgIn(here));
            }
        }
    }
    return BEDispatch(c, steps + start, 4 - start, seq);
}

static ok64 BEPost(cli *c, b8 seq) {
    sane(c);
    static dog_step const steps[] = {
        {"sniff",  "post", NO},
        {"keeper", "put",  NO},
        {"spot",   "get",  NO},
        {"graf",   "get",  YES},
    };
    return BEDispatch(c, steps, 4, seq);
}

static ok64 BEPut(cli *c, b8 seq) {
    sane(c);
    static dog_step const steps[] = {
        {"sniff",  "put",  NO},
        {"keeper", "put",  NO},
        {"spot",   "get",  NO},
        {"graf",   "get",  YES},
    };
    return BEDispatch(c, steps, 4, seq);
}

static ok64 BEDelete(cli *c, b8 seq) {
    sane(c);
    static dog_step const steps[] = {
        {"sniff",  "delete", NO},
        {"spot",   "get",    NO},
        {"graf",   "get",    YES},
    };
    return BEDispatch(c, steps, 3, seq);
}

// --- Bare `be`: --update all dogs, then --status each ---

static ok64 BEDefault(void) {
    sane(1);
    char dogs[16][64] = {};
    u32 ndogs = BEReadDogs(dogs, 16);
    if (ndogs == 0) {
        static const char *pack[] = {"spot", "graf", "sniff"};
        for (u32 i = 0; i < 3; i++)
            memcpy(dogs[i], pack[i], strlen(pack[i]) + 1);
        ndogs = 3;
    }
    for (u32 i = 0; i < ndogs; i++) {
        char *argv[] = {dogs[i], "--update", NULL};
        BERun(dogs[i], argv, NO);
    }
    for (u32 i = 0; i < ndogs; i++) {
        char *argv[] = {dogs[i], "--status", NULL};
        BERun(dogs[i], argv, NO);
    }
    done;
}

// --- Main ---

ok64 becli() {
    sane(1);
    call(FILEInit);

    cli c = {};
    call(CLIParse, &c, BE_VERB_NAMES, NULL);

    if (CLIHas(&c, "-h") || CLIHas(&c, "--help")) {
        BEUsage();
        done;
    }

    // No args → default
    if ($empty(c.verb) && c.nuris == 0 && c.nflags == 0) {
        call(BEDefault);
        done;
    }

    // Classify verb
    a_cstr(v_get,    "get");    a_cstr(v_put,    "put");
    a_cstr(v_post,   "post");   a_cstr(v_delete, "delete");
    a_cstr(v_diff,   "diff");   a_cstr(v_patch,  "patch");
    a_cstr(v_merge,  "merge");  a_cstr(v_sync,   "sync");
    a_cstr(v_status, "status");

    u8cs verb = {};
    $mv(verb, c.verb);

    // Get first URI if available
    uri *u = (c.nuris > 0) ? &c.uris[0] : NULL;
    frag fr = {};
    if (u != NULL && !$empty(u->fragment))
        FRAGu8sDrain(u->fragment, &fr);

    b8 seq = CLIHas(&c, "--seq");

    // No verb → view/search mode
    if ($empty(verb)) {
        if (u != NULL && (fr.type == FRAG_SPOT || fr.type == FRAG_PCRE ||
                          fr.type == FRAG_IDENT)) {
            // Search → spot
            char uri_z[FILE_PATH_MAX_LEN] = {};
            snprintf(uri_z, sizeof(uri_z), "%.*s",
                     (int)$len(u->data), (char *)u->data[0]);
            char *argv[] = {"spot", uri_z, NULL};
            call(BERun, "spot", argv, NO);
        } else if (u != NULL && !$empty(u->path)) {
            // View → bro
            char uri_z[FILE_PATH_MAX_LEN] = {};
            snprintf(uri_z, sizeof(uri_z), "%.*s",
                     (int)$len(u->data), (char *)u->data[0]);
            char *argv[] = {"bro", uri_z, NULL};
            call(BERun, "bro", argv, NO);
        } else {
            call(BEDefault);
        }
    } else if ($eq(verb, v_get)) {
        call(BEGet, &c, seq);
    } else if ($eq(verb, v_post)) {
        call(BEPost, &c, seq);
    } else if ($eq(verb, v_put)) {
        call(BEPut, &c, seq);
    } else if ($eq(verb, v_delete)) {
        call(BEDelete, &c, seq);
    } else if ($eq(verb, v_status)) {
        call(BEDefault);
    } else if ($eq(verb, v_diff)) {
        fprintf(stderr, "be: diff dispatch not yet implemented\n");
    } else if ($eq(verb, v_patch)) {
        fprintf(stderr, "be: patch dispatch not yet implemented\n");
    } else {
        fprintf(stderr, "be: verb '%.*s' not yet implemented\n",
                (int)$len(verb), (char *)verb[0]);
    }

    done;
}

MAIN(becli);
