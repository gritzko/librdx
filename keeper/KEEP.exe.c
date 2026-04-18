//  KEEPExec — run a parsed CLI against an open keeper state.
//  Same effect as invoking `keeper ...` as a separate process.
//
#include "KEEP.h"
#include "REFS.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PRO.h"
#include "dog/CLI.h"
#include "dog/DOG.h"
#include "dog/WHIFF.h"

// --- Verb / flag tables ---

char const *const KEEP_CLI_VERBS[] = {
    "get", "put", "post", "status", "import", "verify",
    "refs", "alias", "help", NULL
};

char const KEEP_CLI_VAL_FLAGS[] = "--want\0--have\0";

// --- Usage ---

static void keep_usage(void) {
    fprintf(stderr,
        "Usage: keeper <verb> [flags] [URI...]\n"
        "\n"
        "  Verbs:\n"
        "    get //remote[?ref]         fetch objects from remote\n"
        "    get .#hashprefix           cat object to stdout\n"
        "    get .?refname              resolve ref to SHA\n"
        "    put .?ref .#sha            move local ref pointer\n"
        "    put //remote?ref           push to remote (stub)\n"
        "    post //remote              create+push a commit on HEAD\n"
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
        total_idx += (u64)wh128csLen(k->runs[i]) * sizeof(wh128);
    fprintf(stdout, "  packs: %llu bytes\n", (unsigned long long)total_pack);
    fprintf(stdout, "  index: %llu entries\n",
            (unsigned long long)(total_idx / sizeof(wh128)));
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
    a_path(keepdir, u8bDataC(k->h->root), KEEP_DIR_S);
    int rcount = 0;
    ok64 o = REFSEach($path(keepdir), refs_print_cb, &rcount);
    if (o != OK && o != REFSNONE)
        fprintf(stderr, "keeper: refs: %s\n", ok64str(o));
    fprintf(stdout, "keeper: %d ref(s)\n", rcount);
    done;
}

// --- Verb: alias ---

static ok64 keeper_alias(keeper *k, uri *name_uri, uri *target_uri) {
    sane(k && name_uri && target_uri);
    a_path(keepdir, u8bDataC(k->h->root), KEEP_DIR_S);

    a_pad(u8, fbuf, 256);
    a_cstr(slashes, "//");
    u8bFeed(fbuf, slashes);
    u8bFeed(fbuf, name_uri->authority);
    a_dup(u8c, from, u8bData(fbuf));

    a_dup(u8c, target, target_uri->data);

    ok64 o = REFSAppend($path(keepdir), from, target);
    if (o != OK) return o;
    fprintf(stdout, "keeper: alias %.*s → %.*s\n",
            (int)u8csLen(from), (char *)from[0],
            (int)u8csLen(target), (char *)target[0]);
    done;
}

// --- Verb: get ---

static ok64 keeper_get_remote(keeper *k, cli *c, uri *g) {
    sane(k && g);
    a_path(keepdir, u8bDataC(k->h->root), KEEP_DIR_S);

    u8bp rmap = NULL;
    ref rarr[REFS_MAX_REFS];
    u32 rn = 0;
    REFSLoad(rarr, &rn, REFS_MAX_REFS, &rmap, $path(keepdir));

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
            u8csc val = {rarr[i].val[0], rarr[i].val[1]};
            if (DOGParseURI(&resolved, val) == OK &&
                !u8csEmpty(resolved.host)) {
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

    #define MAX_WANTHAVE 1024
    static char want_shas[MAX_WANTHAVE][44];
    static char have_shas[MAX_WANTHAVE][44];
    char const *want_list[MAX_WANTHAVE + 1] = {};
    char const *have_list[MAX_WANTHAVE + 1] = {};
    int nwants = 0, nhaves = 0;

    if (!u8csEmpty(g->query)) {
        snprintf(want_shas[nwants], 44, "%.*s",
                 (int)u8csLen(g->query), (char *)g->query[0]);
        want_list[nwants] = want_shas[nwants];
        nwants++;
    }

    for (u32 fi = 0; fi + 1 < c->nflags; fi += 2) {
        a_cstr(wf, "--want");
        a_cstr(hf, "--have");
        if ($eq(c->flags[fi], wf) && nwants < MAX_WANTHAVE) {
            snprintf(want_shas[nwants], 44, "%.*s",
                     (int)u8csLen(c->flags[fi + 1]),
                     (char *)c->flags[fi + 1][0]);
            want_list[nwants] = want_shas[nwants];
            nwants++;
        } else if ($eq(c->flags[fi], hf) && nhaves < MAX_WANTHAVE) {
            snprintf(have_shas[nhaves], 44, "%.*s",
                     (int)u8csLen(c->flags[fi + 1]),
                     (char *)c->flags[fi + 1][0]);
            have_list[nhaves] = have_shas[nhaves];
            nhaves++;
        }
    }

    #define MAX_AUTO_HAVES 256
    for (u32 i = 0; i < rn && nhaves < MAX_AUTO_HAVES; i++) {
        if (u8csEmpty(rarr[i].val) || *rarr[i].val[0] != '?') continue;
        u8cs val = {rarr[i].val[0] + 1, rarr[i].val[1]};
        if (u8csLen(val) < 40) continue;
        u64 hashlet = WHIFFHexHashlet60(val);
        u64 dummy = 0;
        if (KEEPLookup(k, hashlet, 15, &dummy) != OK) continue;
        snprintf(have_shas[nhaves], 44, "%.*s",
                 (int)u8csLen(val), (char *)val[0]);
        have_list[nhaves] = have_shas[nhaves];
        nhaves++;
    }

    if (rmap) u8bUnMap(rmap);

    a_pad(u8, oubuf, FILE_PATH_MAX_LEN);
    if (u8csEmpty(g->host)) {
        a_cstr(file_pfx, "file://");
        u8bFeed(oubuf, file_pfx);
        if (!u8csEmpty(g->path)) u8bFeed(oubuf, g->path);
    } else {
        // Strip any ?query or #fragment — origin_uri must be just
        // scheme+authority+path so recorded refs get a clean key.
        u8cs trim = {g->data[0], g->data[1]};
        if (!u8csEmpty(g->query))    trim[1] = g->query[0] - 1;
        if (!u8csEmpty(g->fragment)) trim[1] = g->fragment[0] - 1;
        u8bFeed(oubuf, trim);
    }
    a_dup(u8c, origin_uri, u8bData(oubuf));
    return KEEPSync(k, remote, origin_uri,
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
    a_path(keepdir, u8bDataC(k->h->root), KEEP_DIR_S);

    a_pad(u8, qbuf, 256);
    u8bFeed1(qbuf, '?');
    u8bFeed(qbuf, query);
    a_dup(u8c, qkey, u8bData(qbuf));

    a_pad(u8, arena, 1024);
    uri resolved = {};
    ok64 ro = REFSResolve(&resolved, arena, $path(keepdir), qkey);
    if (ro == OK && !u8csEmpty(resolved.query)) {
        fprintf(stdout, "%.*s\n",
                (int)u8csLen(resolved.query),
                (char *)resolved.query[0]);
        done;
    }
    fprintf(stderr, "keeper: ref not found\n");
    return REFSNONE;
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

    a_path(keepdir, u8bDataC(k->h->root), KEEP_DIR_S);

    a_pad(u8, fbuf, 256);
    u8bFeed1(fbuf, '?');
    u8bFeed(fbuf, ref_name);
    a_dup(u8c, from, u8bData(fbuf));

    a_pad(u8, tbuf, 256);
    u8bFeed1(tbuf, '?');
    u8bFeed(tbuf, sha_frag);
    a_dup(u8c, to, u8bData(tbuf));

    ok64 o = REFSAppend($path(keepdir), from, to);
    if (o != OK) return o;

    fprintf(stdout, "keeper: %.*s → %.*s\n",
            (int)u8csLen(from), (char *)from[0],
            (int)u8csLen(to), (char *)to[0]);
    done;
}

// --- Verb: post ---

//  Resolve the URI's authority through `keeper alias` and return the
//  effective host/path slices.  The returned slices either point into
//  `g` (no alias) or into the mmap'd REFS file, which must outlive
//  their use — hence *rmap_out is returned to the caller for unmap.
static void post_resolve_remote(keeper *k, uri *g,
                                u8cs host_out, u8cs path_out,
                                u8bp *rmap_out) {
    a_path(keepdir, u8bDataC(k->h->root), KEEP_DIR_S);

    ref rarr[REFS_MAX_REFS];
    u32 rn = 0;
    REFSLoad(rarr, &rn, REFS_MAX_REFS, rmap_out, $path(keepdir));

    a_pad(u8, apad, 256);
    a_cstr(slashes, "//");
    u8bFeed(apad, slashes);
    u8bFeed(apad, g->authority);
    a_dup(u8c, akey, u8bData(apad));

    u8csMv(host_out, g->host);
    u8csMv(path_out, g->path);
    for (u32 i = 0; i < rn; i++) {
        if (REFMatch(&rarr[i], akey)) {
            uri resolved = {};
            u8csc val = {rarr[i].val[0], rarr[i].val[1]};
            if (DOGParseURI(&resolved, val) == OK &&
                !u8csEmpty(resolved.host)) {
                u8csMv(host_out, resolved.host);
                u8csMv(path_out, resolved.path);
            }
            break;
        }
    }
}

//  Extract the tree SHA-1 (as 40 hex chars) from a commit object body.
//  A git commit always starts with "tree <40hex>\n" per object format.
static ok64 post_extract_tree_hex(u8 *out40, u8csc body) {
    if ($len(body) < 46) return KEEPFAIL;
    if (memcmp(body[0], "tree ", 5) != 0) return KEEPFAIL;
    memcpy(out40, body[0] + 5, 40);
    return OK;
}

static ok64 keeper_post(keeper *k, cli *c) {
    sane(k && c);
    uri *g = (c->nuris > 0) ? &c->uris[0] : NULL;
    b8 has_remote = g && !u8csEmpty(g->host);
    a_path(keepdir, u8bDataC(k->h->root), KEEP_DIR_S);

    // 1. Resolve local HEAD → parent commit SHA (40 hex).
    a_cstr(q_head, "?HEAD");
    a_pad(u8, arena, 256);
    uri resolved = {};
    ok64 ro = REFSResolve(&resolved, arena, $path(keepdir), q_head);
    if (ro != OK || $len(resolved.query) != 40) {
        fprintf(stderr, "keeper: post: HEAD not set (do `keeper get` first)\n");
        return KEEPFAIL;
    }
    u8 old_hex[40];
    memcpy(old_hex, resolved.query[0], 40);

    // 2. Read parent commit from local store; pull tree SHA out of it.
    u8cs old_cs = {old_hex, old_hex + 40};
    u64 hashlet = WHIFFHexHashlet60(old_cs);
    Bu8 pbuf = {};
    call(u8bMap, pbuf, 1UL << 20);
    u8 ptype = 0;
    ok64 go = KEEPGet(k, hashlet, 15, pbuf, &ptype);
    if (go != OK || ptype != KEEP_OBJ_COMMIT) {
        u8bUnMap(pbuf);
        fprintf(stderr, "keeper: post: parent commit not in store\n");
        return KEEPFAIL;
    }
    u8 tree_hex[40];
    {
        a_dup(u8c, body, u8bData(pbuf));
        ok64 eo = post_extract_tree_hex(tree_hex, body);
        u8bUnMap(pbuf);
        if (eo != OK) {
            fprintf(stderr, "keeper: post: can't find tree in parent\n");
            return eo;
        }
    }

    // 3. Build the new commit body.  Same tree as parent → an empty-diff
    //    commit; enough to demonstrate the push handshake end-to-end.
    Bu8 com = {};
    call(u8bAllocate, com, 4096);
    a_cstr(tree_label, "tree ");
    u8bFeed(com, tree_label);
    u8csc tree_s = {tree_hex, tree_hex + 40};
    u8bFeed(com, tree_s);
    u8bFeed1(com, '\n');
    a_cstr(par_label, "parent ");
    u8bFeed(com, par_label);
    u8csc old_s = {old_hex, old_hex + 40};
    u8bFeed(com, old_s);
    u8bFeed1(com, '\n');

    char who[256];
    int wlen = snprintf(who, sizeof(who),
        "keeper <keeper@localhost> %lld +0000\n", (long long)time(NULL));
    u8csc who_cs = {(u8cp)who, (u8cp)who + wlen};
    a_cstr(auth_label, "author ");
    u8bFeed(com, auth_label);
    u8bFeed(com, who_cs);
    a_cstr(comm_label, "committer ");
    u8bFeed(com, comm_label);
    u8bFeed(com, who_cs);
    u8bFeed1(com, '\n');
    a_cstr(msg, "keeper post\n");
    u8bFeed(com, msg);

    u8csc com_data = {u8bDataHead(com), u8bIdleHead(com)};

    // 4. Store the new commit locally.
    keep_pack pk = {};
    ok64 po = KEEPPackOpen(k, &pk);
    if (po != OK) { u8bFree(com); return po; }
    sha1 new_sha = {};
    ok64 fo = KEEPPackFeed(k, &pk, KEEP_OBJ_COMMIT, com_data, &new_sha);
    if (fo != OK) { KEEPPackClose(k, &pk); u8bFree(com); return fo; }
    po = KEEPPackClose(k, &pk);
    if (po != OK) { u8bFree(com); return po; }

    sha1hex new_hex = {};
    sha1hexFromSha1(&new_hex, &new_sha);

    // 5. Push via git-receive-pack to refs/heads/master — only when
    //    the URI has a host.  Bare `keeper post` (no URI, or a URI
    //    without an authority) just commits locally.
    if (has_remote) {
        u8cs rhost = {}, rpath = {};
        u8bp rmap = NULL;
        post_resolve_remote(k, g, rhost, rpath, &rmap);
        u8csc old_hex_cs = {old_hex, old_hex + 40};
        u8csc new_hex_cs = {new_hex.data, new_hex.data + 40};
        ok64 pu = KEEPPush(k, rhost, rpath, "refs/heads/master",
                           old_hex_cs, new_hex_cs, com_data);
        if (rmap) u8bUnMap(rmap);
        u8bFree(com);
        if (pu != OK) return pu;
    } else {
        u8bFree(com);
    }

    // 6. Record the new HEAD locally: ?master → ?<new>.
    a_cstr(k_master, "?master");
    a_pad(u8, vbuf, 64);
    u8bFeed1(vbuf, '?');
    u8cs new_s = {new_hex.data, new_hex.data + 40};
    u8bFeed(vbuf, new_s);
    a_dup(u8c, v, u8bData(vbuf));
    REFSAppend($path(keepdir), k_master, v);

    fprintf(stdout, "keeper: post %.12s → %.12s\n",
            (char *)old_hex, new_hex.data);
    done;
}

// --- Entry ---

ok64 KEEPExec(keeper *k, cli *c) {
    sane(k && c);

    a_cstr(v_help,   "help");
    a_cstr(v_get,    "get");
    a_cstr(v_put,    "put");
    a_cstr(v_post,   "post");
    a_cstr(v_status, "status");
    a_cstr(v_import, "import");
    a_cstr(v_verify, "verify");
    a_cstr(v_refs,   "refs");
    a_cstr(v_alias,  "alias");

    if ($eq(c->verb, v_help) || CLIHas(c, "-h") || CLIHas(c, "--help")) {
        keep_usage(); done;
    }

    if ($empty(c->verb)) {
        keep_usage();
        fail(KEEPFAIL);
    }

    if ($eq(c->verb, v_status))  return keeper_status(k);
    if ($eq(c->verb, v_refs))    return keeper_refs(k);
    if ($eq(c->verb, v_get))     return keeper_get(k, c);
    if ($eq(c->verb, v_put))     return keeper_put(k, c);
    if ($eq(c->verb, v_post))    return keeper_post(k, c);

    if ($eq(c->verb, v_import)) {
        if (c->nuris < 1) {
            fprintf(stderr, "keeper: import requires a packfile path\n");
            return KEEPFAIL;
        }
        return keeper_import(k, c->uris[0].path);
    }

    if ($eq(c->verb, v_verify)) {
        if (c->nuris < 1 || u8csEmpty(c->uris[0].fragment)) {
            fprintf(stderr, "keeper: verify requires #sha\n");
            return KEEPFAIL;
        }
        return keeper_verify(k, c->uris[0].fragment);
    }

    if ($eq(c->verb, v_alias)) {
        if (c->nuris < 2 || u8csEmpty(c->uris[0].authority)) {
            fprintf(stderr, "keeper: alias requires //name <uri>\n");
            return KEEPFAIL;
        }
        return keeper_alias(k, &c->uris[0], &c->uris[1]);
    }

    fprintf(stderr, "keeper: unknown verb '%.*s'\n",
            (int)$len(c->verb), (char *)c->verb[0]);
    return KEEPFAIL;
}
