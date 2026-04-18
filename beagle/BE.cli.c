#include "abc/URI.h"
#include "dog/CLI.h"
#include "dog/FRAG.h"

#include <stdio.h>
#include <string.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/HOME.h"
#include "keeper/REFS.h"

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
        "  get [uri]            checkout / fetch / view / search\n"
        "  put [files]          stage files into a new base tree\n"
        "  delete [files]       stage removals into a new base tree\n"
        "  post -m <msg>        commit base tree; push if remote\n"
        "  patch [uri]          search & replace, reindex\n"
        "  diff [uri]           token-level diff\n"
        "  merge [uri]          3-way merge\n"
        "  sync [uri]           fetch + merge (or push)\n"
        "  status               show repo status\n"
        "\n"
        "URI format: [//remote] [path] [?ref] [#search]\n"
        "\n"
        "Bare `be` = ensure indexes are current, show status.\n"
    );
}

// --- Run a sibling tool ---

// Run a sibling tool.  `tool` is the dog name (also argv[0] in argv);
// resolved against this process's own argv[0] via HOMEResolveSibling.
static ok64 BERun(u8csc tool, u8css argv, b8 bg) {
    sane($ok(tool) && !$empty(tool));
    a_path(path);
    a$rg(a0, 0);
    HOMEResolveSibling(NULL, path, tool, a0);
    pid_t pid = 0;
    call(FILESpawn, $path(path), argv, NULL, NULL, &pid);
    if (bg) done;
    int rc = 0;
    ok64 r = FILEReap(pid, &rc);
    if (r == FILESIGNAL) {
        char const *sname = strsignal(rc);
        fprintf(stderr, "be: " U8SFMT " killed by signal %d (%s)\n",
                u8sFmt(tool), rc, sname ? sname : "?");
        return BEDOGSIG;
    }
    if (r != OK) return r;
    if (rc != 0) {
        fprintf(stderr, "be: " U8SFMT " exited %d\n", u8sFmt(tool), rc);
        return BEDOGEXIT;
    }
    done;
}

// --- Read .dogs/DOGS list ---

static u32 BEReadDogs(char out[][64], u32 maxn) {
    home h = {};
    u8cs at = {};
    if (HOMEOpen(&h, at, NO) != OK) return 0;
    a_path(p);
    a_dup(u8c, root_s, u8bDataC(h.root));
    if (PATHu8bFeed(p, root_s) != OK) { HOMEClose(&h); return 0; }
    a_cstr(rel, ".dogs/DOGS");
    if (PATHu8bAdd(p, rel) != OK) { HOMEClose(&h); return 0; }

    u8bp mapped = NULL;
    if (FILEMapRO(&mapped, $path(p)) != OK) { HOMEClose(&h); return 0; }
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
    HOMEClose(&h);
    return count;
}

// --- Verb dispatch: forward URI to dogs in order ---
//
// Each dog parses the URI and handles its part:
//   get:    keeper (fetch) → sniff (checkout) → spot (index) → graf (index)
//   put:    sniff (stage tree) → spot (index) → graf (index)
//             [local only — no HEAD move, no ref push]
//   delete: sniff (stage removal) → spot (index) → graf (index)
//             [local only — same as put]
//   post:   sniff (commit, HEAD move) → keeper (push ref) → spot → graf

typedef struct {
    u8cs dog;
    u8cs verb;
    b8 bg;             // run in background (don't wait)
} dog_step;

static ok64 BEDispatch(cli *c, dog_step const *steps, u32 nsteps,
                        b8 seq) {
    sane(c && steps);
    for (u32 i = 0; i < nsteps; i++) {
        // argv: dog verb [flags...] [URIs...]
        // cli.flags and cli.uris[].data already are u8cs slices.
        a_pad(u8cs, args, 2 + CLI_MAX_FLAGS * 2 + CLI_MAX_URIS);
        u8csbFeed1(args, steps[i].dog);
        u8csbFeed1(args, steps[i].verb);
        // Flags come as {flag, val} pairs; val is the empty-string
        // sentinel for booleans.  Forward the flag name always; only
        // forward its value if it's genuinely non-empty, otherwise the
        // callee's CLIParse would pick it up as a spurious URI.
        for (u32 j = 0; j + 1 < c->nflags; j += 2) {
            u8csbFeed1(args, c->flags[j]);
            if (!u8csEmpty(c->flags[j + 1]))
                u8csbFeed1(args, c->flags[j + 1]);
        }
        for (u32 j = 0; j < c->nuris; j++)
            u8csbFeed1(args, c->uris[j].data);
        a_dup(u8cs, argv, u8csbData(args));
        call(BERun, steps[i].dog, argv, seq ? NO : steps[i].bg);
    }
    done;
}

//  `be get <local-dir>` creates a worktree in cwd that shares
//  keeper/graf/spot with the primary repo via symlinks; sniff is
//  real (per-worktree).  Returns OK after setup whether or not any
//  action was taken; only dies on a real error (mkdir/symlink fail).
// Static storage for the rewritten URI after a worktree is wired up:
// "?<40-hex-sha>" points every downstream dog at the primary's HEAD.
static u8 wt_uri_text[42];  // '?' + 40 hex + NUL

static ok64 BEGetWorktree(uri *u) {
    sane(1);
    if (u == NULL || !u8csEmpty(u->authority)) done;
    if (u8csEmpty(u->path)) done;

    // Primary candidate has to be an existing dir containing .dogs/.
    a_cstr(dotdogs, ".dogs");
    a_dup(u8c, prim_s, u->path);
    a_path(prim_dogs, prim_s, dotdogs);
    if (FILEisdir($path(prim_dogs)) != OK) done;

    // Skip if cwd is already a repo (its own .dogs/ exists).
    a_path(cwd);
    call(FILEGetCwd, cwd);
    a_path(cwd_dogs);
    a_dup(u8c, cwd_s, u8bDataC(cwd));
    call(PATHu8bFeed, cwd_dogs, cwd_s);
    call(PATHu8bPush, cwd_dogs, dotdogs);
    if (FILEisdir($path(cwd_dogs)) == OK) done;

    call(FILEMakeDirP, $path(cwd_dogs));

    // Real per-worktree dir.
    a_cstr(sniff_name, "sniff");
    a_path(sniff_path, $path(cwd_dogs), sniff_name);
    call(FILEMakeDirP, $path(sniff_path));

    // Shared subdirs → symlinks into primary.
    u8cs shared[] = {u8slit("keeper"), u8slit("graf"), u8slit("spot")};
    for (u32 i = 0; i < 3; i++) {
        a_path(tgt, $path(prim_dogs), shared[i]);
        a_path(lnk, $path(cwd_dogs),  shared[i]);
        ok64 so = FILESymLink($path(tgt), $path(lnk));
        if (so != OK && so != FILEEXIST) return so;
    }
    fprintf(stderr, "be: worktree from %.*s\n",
            (int)$len(u->path), (char *)u->path[0]);

    // Resolve the primary's current commit via keeper refs keyed by
    // `file://<primary-abs-path>` (set by `sniff post` / `keeper get`).
    // Rewrite this URI to "?<sha>" so downstream sniff checks out
    // that commit in the worktree.
    a_cstr(keeper_name, "keeper");
    a_path(keepdir, $path(prim_dogs), keeper_name);
    a_pad(u8, key_buf, 1280);
    a_cstr(scheme, "file://");
    u8bFeed(key_buf, scheme);
    u8bFeed(key_buf, prim_s);
    a_dup(u8c, key_slice, u8bData(key_buf));
    a_pad(u8, arena_buf, 256);
    uri resolved = {};
    if (REFSResolve(&resolved, arena_buf, $path(keepdir), key_slice) != OK)
        done;
    if ($len(resolved.query) < 40) done;
    wt_uri_text[0] = '?';
    for (int i = 0; i < 40; i++) wt_uri_text[1 + i] = resolved.query[0][i];
    wt_uri_text[41] = 0;

    u->data[0]      = wt_uri_text;
    u->data[1]      = wt_uri_text + 41;
    u->scheme[0]    = u->scheme[1]    = NULL;
    u->authority[0] = u->authority[1] = NULL;
    u->host[0]      = u->host[1]      = NULL;
    u->port[0]      = u->port[1]      = NULL;
    u->user[0]      = u->user[1]      = NULL;
    u->path[0]      = u->path[1]      = NULL;
    u->query[0]     = wt_uri_text + 1;
    u->query[1]     = wt_uri_text + 41;
    u->fragment[0]  = u->fragment[1]  = NULL;
    done;
}

static ok64 BEGet(cli *c, b8 seq) {
    sane(c);
    static dog_step const steps[] = {
        {u8slit("keeper"), u8slit("get"), NO},
        {u8slit("sniff"),  u8slit("get"), NO},
        {u8slit("spot"),   u8slit("get"), NO},
        {u8slit("graf"),   u8slit("get"), NO},  // foreground: surface
                                 // graf's stderr before the next prompt
    };
    // Skip keeper fetch if no remote (no authority)
    uri *u = (c->nuris > 0) ? &c->uris[0] : NULL;
    u32 start = (u != NULL && !$empty(u->authority)) ? 0 : 1;

    // Local-path URI → worktree from a sibling repo.
    call(BEGetWorktree, u);

    // Bootstrap: when cloning into a fresh dir (remote URI, no existing
    // .dogs/ anywhere up to /), create .dogs/ in cwd so each dog can
    // place its own subdir. Without this, every dog fails with
    // KEEPFAIL/SNIFFFAIL/etc. before printing anything useful.
    if (start == 0) {
        home probe_h = {};
        u8cs at = {};
        ok64 ho = HOMEOpen(&probe_h, at, NO);
        HOMEClose(&probe_h);
        if (ho != OK) {
            a_path(here);
            if (FILEGetCwd(here) == OK) {
                a_cstr(dotdogs, ".dogs");
                call(PATHu8bPush, here, dotdogs);
                call(FILEMakeDirP, $path(here));
            }
        }
    }
    return BEDispatch(c, steps + start, 4 - start, seq);
}

//  `be put` stages a new base tree locally — no commit object and no
//  ref push.  Spot/graf re-index the worktree so search stays current.
static ok64 BEPut(cli *c, b8 seq) {
    sane(c);
    static dog_step const steps[] = {
        {u8slit("sniff"),  u8slit("put"), NO},
        {u8slit("spot"),   u8slit("get"), NO},
        {u8slit("graf"),   u8slit("get"), YES},
    };
    return BEDispatch(c, steps, 3, seq);
}

//  `be delete` is the mirror of `be put`: stage tree without a file.
static ok64 BEDelete(cli *c, b8 seq) {
    sane(c);
    static dog_step const steps[] = {
        {u8slit("sniff"),  u8slit("delete"), NO},
        {u8slit("spot"),   u8slit("get"),    NO},
        {u8slit("graf"),   u8slit("get"),    YES},
    };
    return BEDispatch(c, steps, 3, seq);
}

//  `be post`:
//    -m <msg>  → sniff makes a local commit (always).
//    <uri>     → keeper pushes the current commit to that remote.
//  Bare `be post` with neither is a no-op shell; flags decide what runs.
static ok64 BEPost(cli *c, b8 seq) {
    sane(c);
    uri *u = (c->nuris > 0) ? &c->uris[0] : NULL;
    b8 has_remote = (u != NULL && !u8csEmpty(u->authority));
    a_cstr(mf, "-m");
    b8 has_msg = NO;
    for (u32 fi = 0; fi + 1 < c->nflags; fi += 2) {
        if ($eq(c->flags[fi], mf)) { has_msg = YES; break; }
    }
    dog_step steps[4];
    u32 nsteps = 0;
    if (has_msg) {
        steps[nsteps++] = (dog_step){u8slit("sniff"),  u8slit("post"), NO};
    }
    if (has_remote) {
        steps[nsteps++] = (dog_step){u8slit("keeper"), u8slit("post"), NO};
    }
    // Re-index: spot and graf always.
    steps[nsteps++] = (dog_step){u8slit("spot"),  u8slit("get"),  NO};
    steps[nsteps++] = (dog_step){u8slit("graf"),  u8slit("get"),  YES};
    return BEDispatch(c, steps, nsteps, seq);
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
    // Run --update on every dog; remember the worst error so a crashing
    // or non-zero-exiting dog is surfaced after status output completes.
    ok64 worst = OK;
    u8cs upd = u8slit("--update");
    u8cs sta = u8slit("--status");
    for (u32 i = 0; i < ndogs; i++) {
        a_cstr(dog_s, dogs[i]);
        a_pad(u8cs, args, 2);
        u8csbFeed1(args, dog_s);
        u8csbFeed1(args, upd);
        a_dup(u8cs, argv, u8csbData(args));
        ok64 r = BERun(dog_s, argv, NO);
        if (r != OK && worst == OK) worst = r;
    }
    for (u32 i = 0; i < ndogs; i++) {
        a_cstr(dog_s, dogs[i]);
        a_pad(u8cs, args, 2);
        u8csbFeed1(args, dog_s);
        u8csbFeed1(args, sta);
        a_dup(u8cs, argv, u8csbData(args));
        ok64 r = BERun(dog_s, argv, NO);
        if (r != OK && worst == OK) worst = r;
    }
    if (worst != OK) fail(worst);
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
        u8cs spot = u8slit("spot");
        u8cs bro  = u8slit("bro");
        if (u != NULL && (fr.type == FRAG_SPOT || fr.type == FRAG_PCRE ||
                          fr.type == FRAG_IDENT)) {
            // Search → spot.  u->data borrows from argv (NUL-terminated).
            a_pad(u8cs, args, 2);
            u8csbFeed1(args, spot);
            u8csbFeed1(args, u->data);
            a_dup(u8cs, argv, u8csbData(args));
            call(BERun, spot, argv, NO);
        } else if (u != NULL && !$empty(u->path)) {
            // View → bro
            a_pad(u8cs, args, 2);
            u8csbFeed1(args, bro);
            u8csbFeed1(args, u->data);
            a_dup(u8cs, argv, u8csbData(args));
            call(BERun, bro, argv, NO);
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
        fprintf(stderr, "be: verb '" U8SFMT "' not yet implemented\n",
                u8sFmt(verb));
    }

    done;
}

MAIN(becli);
