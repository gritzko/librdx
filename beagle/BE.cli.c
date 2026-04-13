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

// --- Verb table ---

static char const *const BE_VERB_NAMES[] = {
    "get", "put", "patch", "diff", "merge", "delete", "sync",
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

// --- Run a sibling tool, wait for completion ---

static ok64 BERun(const char *tool, char *const argv[]) {
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
    int st = 0;
    waitpid(pid, &st, 0);
    if (WIFEXITED(st) && WEXITSTATUS(st) != 0)
        fprintf(stderr, "be: %s exited %d\n", tool, WEXITSTATUS(st));
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
        BERun(dogs[i], argv);
    }
    for (u32 i = 0; i < ndogs; i++) {
        char *argv[] = {dogs[i], "--status", NULL};
        BERun(dogs[i], argv);
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
    a_cstr(v_diff,   "diff");   a_cstr(v_patch,  "patch");
    a_cstr(v_merge,  "merge");  a_cstr(v_delete, "delete");
    a_cstr(v_sync,   "sync");   a_cstr(v_status, "status");

    u8cs verb = {};
    $mv(verb, c.verb);

    // Get first URI if available
    uri *u = (c.nuris > 0) ? &c.uris[0] : NULL;
    frag fr = {};
    if (u != NULL && !$empty(u->fragment))
        FRAGu8sDrain(u->fragment, &fr);

    // No verb → view/search mode
    if ($empty(verb)) {
        if (u != NULL && (fr.type == FRAG_SPOT || fr.type == FRAG_PCRE ||
                          fr.type == FRAG_IDENT)) {
            // Search → spot
            char uri_z[FILE_PATH_MAX_LEN] = {};
            snprintf(uri_z, sizeof(uri_z), "%.*s",
                     (int)$len(u->data), (char *)u->data[0]);
            char *argv[] = {"spot", uri_z, NULL};
            call(BERun, "spot", argv);
        } else if (u != NULL && !$empty(u->path)) {
            // View → bro
            char uri_z[FILE_PATH_MAX_LEN] = {};
            snprintf(uri_z, sizeof(uri_z), "%.*s",
                     (int)$len(u->data), (char *)u->data[0]);
            char *argv[] = {"bro", uri_z, NULL};
            call(BERun, "bro", argv);
        } else {
            call(BEDefault);
        }
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
