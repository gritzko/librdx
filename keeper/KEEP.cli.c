//  keeper CLI: local git object store.
//
//  Verbs:
//    keeper get //remote?ref          fetch from remote
//    keeper get #hashprefix           cat object to stdout
//    keeper get ?refname              resolve ref → SHA
//    keeper put ?ref #sha             move local ref
//    keeper put //remote?ref          push (stub)
//    keeper status                    show store stats
//    keeper import <packfile>         import git packfile
//    keeper verify #sha               verify object + tree
//    keeper refs                      list known refs
//    keeper alias //name <uri>        add remote alias
//    keeper help                      this message
//
#include "KEEP.h"
#include "REFS.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "dog/CLI.h"

// --- Verb table ---

static char const *const keeper_verbs[] = {
    "get", "put", "status", "import", "verify",
    "refs", "alias", "help", NULL
};

static char const keeper_val_flags[] = "--want\0--have\0";

// --- Usage ---

static void usage(void) {
    fprintf(stderr,
        "Usage: keeper <verb> [flags] [URI...]\n"
        "\n"
        "  Verbs:\n"
        "    get //remote[?ref]         fetch objects from remote\n"
        "    get .#hashprefix           cat object to stdout\n"
        "    get .?refname              resolve ref to SHA\n"
        "    put .?ref .#sha            move local ref pointer\n"
        "    put //remote?ref           push to remote (stub)\n"
        "    status                     show store stats\n"
        "    import <packfile>          import a git packfile\n"
        "    verify .#sha               verify object + recurse\n"
        "    refs                       list known refs\n"
        "    alias //name <uri>         add remote alias\n"
        "    help                       this message\n"
    );
}

// --- Helpers ---

static ok64 refs_print_cb(refcp r, void *ctx) {
    int *count = (int *)ctx;
    fprintf(stdout, "  %.*s\t→ %.*s\n",
            (int)$len(r->key), (char *)r->key[0],
            (int)$len(r->val), (char *)r->val[0]);
    (*count)++;
    return OK;
}

// --- Verb: status ---

static ok64 keeper_status(keeper *k) {
    sane(k);
    fprintf(stdout, "keeper: %u pack file(s), %u index run(s)\n",
            k->npacks, k->nruns);
    u64 total_pack = 0;
    for (u32 i = 0; i < k->npacks; i++)
        total_pack += (u64)u8bDataLen(k->packs[i]);
    u64 total_idx = 0;
    for (u32 i = 0; i < k->nruns; i++)
        total_idx += (u64)kv64csLen(k->runs[i]) * sizeof(kv64);
    fprintf(stdout, "  packs: %llu bytes\n", (unsigned long long)total_pack);
    fprintf(stdout, "  index: %llu entries\n",
            (unsigned long long)(total_idx / sizeof(kv64)));
    done;
}

// --- Verb: import ---

static ok64 keeper_import(keeper *k, u8cs path) {
    sane(k && $ok(path));
    call(KEEPImport, k, path);
    done;
}

// --- Verb: verify ---

static ok64 keeper_verify(keeper *k, u8cs hex) {
    sane(k && $ok(hex));
    return KEEPVerify(k, hex);
}

// --- Verb: refs ---

static ok64 keeper_refs(keeper *k) {
    sane(k);
    a_cstr(keepdir, k->dir);
    int rcount = 0;
    ok64 o = REFSEach(keepdir, refs_print_cb, &rcount);
    if (o != OK && o != REFSNONE)
        fprintf(stderr, "keeper: refs: %s\n", ok64str(o));
    fprintf(stdout, "keeper: %d ref(s)\n", rcount);
    done;
}

// --- Verb: alias ---

static ok64 keeper_alias(keeper *k, uri *name_uri, uri *target_uri) {
    sane(k && name_uri && target_uri);
    a_cstr(keepdir, k->dir);

    // Build from-key: //name
    a_pad(u8, fbuf, 256);
    a_cstr(slashes, "//");
    u8bFeed(fbuf, slashes);
    u8bFeed(fbuf, name_uri->authority);
    a_dup(u8c, from, u8bData(fbuf));

    // Target is the full URI data
    a_dup(u8c, target, target_uri->data);

    ok64 o = REFSAppend(keepdir, from, target);
    if (o != OK) return o;
    fprintf(stdout, "keeper: alias %.*s → %.*s\n",
            (int)u8csLen(from), (char *)from[0],
            (int)u8csLen(target), (char *)target[0]);
    done;
}

// --- Verb: get ---

static ok64 keeper_get_remote(keeper *k, cli *c, uri *g) {
    sane(k && g);
    a_cstr(keepdir, k->dir);

    // Resolve alias
    u8bp rmap = NULL;
    ref rarr[REFS_MAX_REFS];
    u32 rn = 0;
    REFSLoad(rarr, &rn, REFS_MAX_REFS, &rmap, keepdir);

    a_pad(u8, apad, 256);
    u8bFeed(apad, g->authority);
    a_dup(u8c, akey, u8bData(apad));

    u8cs rhost = {};
    u8cs rpath = {};
    u8csMv(rhost, g->host);
    u8csMv(rpath, g->path);
    for (u32 i = 0; i < rn; i++) {
        if (REFMatch(&rarr[i], akey)) {
            uri resolved = {};
            resolved.data[0] = rarr[i].val[0];
            resolved.data[1] = rarr[i].val[1];
            if (URILexer(&resolved) == OK && !u8csEmpty(resolved.host)) {
                u8csMv(rhost, resolved.host);
                u8csMv(rpath, resolved.path);
            }
            break;
        }
    }

    a_pad(u8, rbuf, 1024);
    u8bFeed(rbuf, rhost);
    if (!u8csEmpty(rpath)) {
        u8bFeed1(rbuf, ' ');
        u8bFeed(rbuf, rpath);
    }
    a_dup(u8c, remote, u8bData(rbuf));

    // Collect --want/--have from flags
    char const *want_list[64] = {};
    char const *have_list[64] = {};
    int nwants = 0, nhaves = 0;

    if (!u8csEmpty(g->query)) {
        a_pad(u8, qbuf, 256);
        u8bFeed1(qbuf, '?');
        u8bFeed(qbuf, g->query);
        a_dup(u8c, qkey, u8bData(qbuf));

        static char wantsha[44];
        for (u32 i = 0; i < rn; i++) {
            if (REFMatch(&rarr[i], qkey)) {
                a_dup(u8c, val, rarr[i].val);
                if (!u8csEmpty(val) && *val[0] == '?')
                    u8csUsed(val, 1);
                snprintf(wantsha, 44, "%.*s",
                         (int)u8csLen(val), (char *)val[0]);
                want_list[nwants++] = wantsha;
                break;
            }
        }
        if (nwants == 0 && u8csLen(g->query) >= 6) {
            snprintf(wantsha, 44, "%.*s",
                     (int)u8csLen(g->query), (char *)g->query[0]);
            want_list[nwants++] = wantsha;
        }
    }

    for (u32 fi = 0; fi + 1 < c->nflags; fi += 2) {
        a_cstr(wf, "--want");
        a_cstr(hf, "--have");
        if ($eq(c->flags[fi], wf) && nwants < 63) {
            static char wbufs[64][44];
            snprintf(wbufs[nwants], 44, "%.*s",
                     (int)u8csLen(c->flags[fi + 1]),
                     (char *)c->flags[fi + 1][0]);
            want_list[nwants] = wbufs[nwants];
            nwants++;
        } else if ($eq(c->flags[fi], hf) && nhaves < 63) {
            static char hbufs[64][44];
            snprintf(hbufs[nhaves], 44, "%.*s",
                     (int)u8csLen(c->flags[fi + 1]),
                     (char *)c->flags[fi + 1][0]);
            have_list[nhaves] = hbufs[nhaves];
            nhaves++;
        }
    }

    if (rmap) u8bUnMap(rmap);

    return KEEPSync(k, remote,
                    nwants > 0 ? want_list : NULL,
                    nhaves > 0 ? have_list : NULL);
}

static ok64 keeper_get_object(keeper *k, u8cs prefix) {
    sane(k && $ok(prefix));
    if (u8csLen(prefix) < HASH_MIN_HEX) {
        fprintf(stderr, "keeper: hash too short (min %d)\n",
                HASH_MIN_HEX);
        return KEEPFAIL;
    }
    size_t hexlen = u8csLen(prefix);
    u64 hashlet = WHIFFHexHashlet60(prefix);
    Bu8 out = {};
    call(u8bMap, out, 64UL << 20);
    u8 obj_type = 0;
    ok64 o = KEEPGet(k, hashlet, hexlen, out, &obj_type);
    if (o == OK) {
        a_dup(u8c, data, u8bData(out));
        write(STDOUT_FILENO, data[0], u8csLen(data));
    } else {
        fprintf(stderr, "keeper: object not found\n");
    }
    u8bUnMap(out);
    return o;
}

static ok64 keeper_get_ref(keeper *k, u8cs query) {
    sane(k && $ok(query));
    a_cstr(keepdir, k->dir);

    a_pad(u8, qbuf, 256);
    u8bFeed1(qbuf, '?');
    u8bFeed(qbuf, query);
    a_dup(u8c, qkey, u8bData(qbuf));

    u8bp rmap = NULL;
    ref rarr[REFS_MAX_REFS];
    u32 rn = 0;
    REFSLoad(rarr, &rn, REFS_MAX_REFS, &rmap, keepdir);

    b8 found = NO;
    for (u32 i = 0; i < rn; i++) {
        if (REFMatch(&rarr[i], qkey)) {
            a_dup(u8c, val, rarr[i].val);
            if (!u8csEmpty(val) && *val[0] == '?')
                u8csUsed(val, 1);
            fprintf(stdout, "%.*s\n",
                    (int)u8csLen(val), (char *)val[0]);
            found = YES;
            break;
        }
    }
    if (rmap) u8bUnMap(rmap);
    if (!found) {
        fprintf(stderr, "keeper: ref not found\n");
        return REFSNONE;
    }
    done;
}

static ok64 keeper_get(keeper *k, cli *c) {
    sane(k && c);
    if (c->nuris == 0) {
        fprintf(stderr, "keeper: get requires a URI\n");
        return KEEPFAIL;
    }
    uri *g = &c->uris[0];

    if (!u8csEmpty(g->authority))
        return keeper_get_remote(k, c, g);
    if (!u8csEmpty(g->fragment))
        return keeper_get_object(k, g->fragment);
    if (!u8csEmpty(g->query))
        return keeper_get_ref(k, g->query);

    fprintf(stderr, "keeper: get: need //remote, #hash, or ?ref\n");
    return KEEPFAIL;
}

// --- Verb: put ---

static ok64 keeper_put(keeper *k, cli *c) {
    sane(k && c);
    if (c->nuris == 0) {
        fprintf(stderr, "keeper: put requires a URI\n");
        return KEEPFAIL;
    }
    uri *g = &c->uris[0];

    if (!u8csEmpty(g->authority)) {
        fprintf(stderr, "keeper: remote push not yet implemented\n");
        return KEEPFAIL;
    }

    // Local ref update: put ?ref #sha
    // Accept ?ref and #sha from the same or separate URIs
    u8cs ref_name = {};
    u8cs sha_frag = {};

    for (u32 i = 0; i < c->nuris; i++) {
        if (!u8csEmpty(c->uris[i].query) && !$ok(ref_name))
            u8csMv(ref_name, c->uris[i].query);
        if (!u8csEmpty(c->uris[i].fragment) && !$ok(sha_frag))
            u8csMv(sha_frag, c->uris[i].fragment);
    }

    if (!$ok(ref_name) || !$ok(sha_frag)) {
        fprintf(stderr, "keeper: put requires ?ref and #sha\n");
        return KEEPFAIL;
    }

    a_cstr(keepdir, k->dir);

    // Build from: ?refname
    a_pad(u8, fbuf, 256);
    u8bFeed1(fbuf, '?');
    u8bFeed(fbuf, ref_name);
    a_dup(u8c, from, u8bData(fbuf));

    // Build to: ?sha
    a_pad(u8, tbuf, 256);
    u8bFeed1(tbuf, '?');
    u8bFeed(tbuf, sha_frag);
    a_dup(u8c, to, u8bData(tbuf));

    ok64 o = REFSAppend(keepdir, from, to);
    if (o != OK) return o;

    fprintf(stdout, "keeper: %.*s → %.*s\n",
            (int)u8csLen(from), (char *)from[0],
            (int)u8csLen(to), (char *)to[0]);
    done;
}

// --- Entry ---

ok64 keepercli() {
    sane(1);
    call(FILEInit);

    cli c = {};
    call(CLIParse, &c, keeper_verbs, keeper_val_flags);

    a_cstr(v_help,   "help");
    a_cstr(v_get,    "get");
    a_cstr(v_put,    "put");
    a_cstr(v_status, "status");
    a_cstr(v_import, "import");
    a_cstr(v_verify, "verify");
    a_cstr(v_refs,   "refs");
    a_cstr(v_alias,  "alias");

    if ($eq(c.verb, v_help) || CLIHas(&c, "-h") || CLIHas(&c, "--help")) {
        usage(); done;
    }

    if ($empty(c.verb)) {
        usage();
        fail(KEEPFAIL);
    }

    keeper k = {};
    call(KEEPOpen, &k, c.repo);

    ok64 ret = OK;

    if ($eq(c.verb, v_status)) {
        ret = keeper_status(&k);
    } else if ($eq(c.verb, v_refs)) {
        ret = keeper_refs(&k);
    } else if ($eq(c.verb, v_get)) {
        ret = keeper_get(&k, &c);
    } else if ($eq(c.verb, v_put)) {
        ret = keeper_put(&k, &c);
    } else if ($eq(c.verb, v_import)) {
        if (c.nuris < 1) {
            fprintf(stderr, "keeper: import requires a packfile path\n");
            ret = KEEPFAIL;
        } else {
            ret = keeper_import(&k, c.uris[0].path);
        }
    } else if ($eq(c.verb, v_verify)) {
        if (c.nuris < 1 || u8csEmpty(c.uris[0].fragment)) {
            fprintf(stderr, "keeper: verify requires #sha\n");
            ret = KEEPFAIL;
        } else {
            ret = keeper_verify(&k, c.uris[0].fragment);
        }
    } else if ($eq(c.verb, v_alias)) {
        if (c.nuris < 2 || u8csEmpty(c.uris[0].authority)) {
            fprintf(stderr, "keeper: alias requires //name <uri>\n");
            ret = KEEPFAIL;
        } else {
            ret = keeper_alias(&k, &c.uris[0], &c.uris[1]);
        }
    } else {
        fprintf(stderr, "keeper: unknown verb '%.*s'\n",
                (int)$len(c.verb), (char *)c.verb[0]);
        ret = KEEPFAIL;
    }

    KEEPClose(&k);
    return ret;
}

MAIN(keepercli);
