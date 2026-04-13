#include "abc/URI.h"
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

#define BE_GET    1
#define BE_PUT    2
#define BE_PATCH  3
#define BE_DIFF   4
#define BE_MERGE  5
#define BE_DELETE  6
#define BE_SYNC   7
#define BE_STATUS  8

typedef struct {
    const char *name;
    u8 verb;
} BEverb;

static const BEverb BE_VERBS[] = {
    // HTTP-like core
    {"get",     BE_GET},
    {"put",     BE_PUT},
    {"patch",   BE_PATCH},
    {"diff",    BE_DIFF},
    {"merge",   BE_MERGE},
    {"delete",  BE_DELETE},
    {"sync",    BE_SYNC},

    // git-compatible aliases
    {"show",    BE_GET},
    {"cat",     BE_GET},
    {"log",     BE_GET},
    {"add",     BE_PUT},
    {"commit",  BE_PUT},
    {"push",    BE_SYNC},
    {"pull",    BE_SYNC},
    {"rm",      BE_DELETE},
    {"status",  BE_STATUS},

    {NULL, 0},
};

static b8 argeq(u8cs a, const char *b) {
    size_t blen = strlen(b);
    return $len(a) == blen && memcmp(a[0], b, blen) == 0;
}

static u8 BEParseVerb(u8cs arg) {
    for (const BEverb *v = BE_VERBS; v->name != NULL; v++)
        if (argeq(arg, v->name)) return v->verb;
    return 0;
}

static void BEUsage(void) {
    fprintf(stderr,
        "Usage: be <verb> [uri] [flags]\n"
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
        "Aliases: show=get, cat=get, log=get, add=put,\n"
        "         commit=put, push=sync, pull=sync, rm=delete\n"
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
// Each line is a dog name.  Returns count; names in out[][64].

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
        // Find line end
        u8cp nl = data[0];
        while (nl < data[1] && *nl != '\n') nl++;
        size_t len = (size_t)(nl - data[0]);
        // Skip empty and comment lines
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
        // Default pack
        static const char *pack[] = {"spot", "graf", "sniff"};
        for (u32 i = 0; i < 3; i++) {
            memcpy(dogs[i], pack[i], strlen(pack[i]) + 1);
        }
        ndogs = 3;
    }

    // Phase 1: --update each dog
    for (u32 i = 0; i < ndogs; i++) {
        char *argv[] = {dogs[i], "--update", NULL};
        BERun(dogs[i], argv);
    }

    // Phase 2: --status each dog
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

    int argn = (int)$arglen;

    // No args → default action (index + status)
    if (argn <= 1) {
        call(BEDefault);
        done;
    }

    // First positional arg = verb
    a$rg(verb_arg, 1);
    if (argeq(verb_arg, "-h") || argeq(verb_arg, "--help")) {
        BEUsage();
        done;
    }

    u8 verb = BEParseVerb(verb_arg);
    b8 verbless = NO;
    if (verb == 0) {
        // No known verb — bare URI → view mode (no verb)
        verbless = YES;
    }

    // Collect remaining args: URI + flags
    int uri_start = verbless ? 1 : 2;
    uri u = {};
    frag fr = {};
    b8 have_uri = NO;

    for (int i = uri_start; i < argn; i++) {
        a$rg(a, i);
        if ($len(a) >= 1 && a[0][0] == '-') continue;
        $mv(u.data, a);
        if (URILexer(&u) == OK) { have_uri = YES; break; }
    }

    if (!have_uri && !verbless) {
        if (verb == BE_STATUS) {
            call(BEDefault);
            done;
        }
        BEUsage();
        done;
    }

    // Parse fragment
    if (have_uri && !$empty(u.fragment))
        FRAGu8sDrain(u.fragment, &fr);

    // Extract URI components directly
    u8cs remote = {};  // authority sans //
    if (!$empty(u.authority)) {
        $mv(remote, u.authority);
        if ($len(remote) >= 2 && remote[0][0] == '/' && remote[0][1] == '/')
            u8csFed(remote, 2);
    }
    u8cs path = {};  // file path (strip leading /)
    if (!$empty(u.path)) {
        $mv(path, u.path);
        if (*path[0] == '/') u8csFed(path, 1);
    }

    // --- Dispatch ---

    // Shorthand: pass the original URI arg through to a dog
    int uri_argi = verbless ? 1 : 2;

    if (verbless) {
        // No verb = view/search (read-only, never mutates)
        if (fr.type == FRAG_SPOT || fr.type == FRAG_PCRE ||
            fr.type == FRAG_IDENT) {
            // Search → spot with URI arg
            a$rg(ua, uri_argi);
            char uri_z[FILE_PATH_MAX_LEN];
            snprintf(uri_z, sizeof(uri_z), "%.*s",
                     (int)$len(ua), (char *)ua[0]);
            char *argv[] = {"spot", uri_z, NULL};
            call(BERun, "spot", argv);
        } else if (!$empty(path)) {
            // View file → bro
            char path_z[FILE_PATH_MAX_LEN];
            snprintf(path_z, sizeof(path_z), "%.*s",
                     (int)$len(path), (char *)path[0]);
            char *argv[] = {"bro", path_z, NULL};
            call(BERun, "bro", argv);
        } else {
            call(BEDefault);
        }

    } else if (verb == BE_GET) {
        // GET = repo → worktree.  Pass URI through to keeper.
        a$rg(ua, uri_argi);
        char uri_z[FILE_PATH_MAX_LEN];
        snprintf(uri_z, sizeof(uri_z), "%.*s",
                 (int)$len(ua), (char *)ua[0]);
        if (!$empty(remote) || !$empty(u.query) ||
            (fr.type == FRAG_IDENT && $empty(path))) {
            // //remote, ?ref, #hash → keeper
            char *argv[] = {"keeper", uri_z, NULL};
            call(BERun, "keeper", argv);
        } else {
            // path?ref → checkout (sniff, future)
            fprintf(stderr, "be: get checkout not yet implemented\n");
        }

    } else if (verb == BE_DIFF) {
        // DIFF → graf
        char uri_z[FILE_PATH_MAX_LEN];
        if (have_uri) {
            a$rg(uri_arg, 2);
            snprintf(uri_z, sizeof(uri_z), "%.*s",
                     (int)$len(uri_arg), (char *)uri_arg[0]);
        }
        // TODO: translate URI to graf --diff args
        fprintf(stderr, "be: diff dispatch not yet implemented\n");

    } else if (verb == BE_PATCH) {
        // PATCH → spot replace
        // TODO: parse #'old'->'new'.ext from URI
        fprintf(stderr, "be: patch dispatch not yet implemented\n");

    } else {
        fprintf(stderr, "be: verb %d not yet implemented\n", verb);
    }

    done;
}

MAIN(becli);
