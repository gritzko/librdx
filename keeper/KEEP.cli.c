//  keeper CLI: local git object store.
//
//  URI syntax (shell-safe with : prefix):
//    keeper :#hashprefix             cat object
//    keeper :?refname                resolve ref → SHA
//    keeper //remote/path            sync all refs
//    keeper //remote/path?ref        sync specific ref
//    keeper //alias                  sync via alias
//
//  Flags:
//    keeper -i <packfile>            import git packfile
//    keeper -s                       show store stats
//    keeper --verify <full-sha>      verify object + tree
//    keeper --refs                   list known refs
//    keeper --alias <name> <uri>     add remote alias
//
#include "KEEP.h"
#include "REFS.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "dog/CLI.h"

static ok64 refs_print_cb(refcp r, void *ctx) {
    int *count = (int *)ctx;
    fprintf(stdout, "  %.*s\t→ %.*s\n",
            (int)$len(r->key), (char *)r->key[0],
            (int)$len(r->val), (char *)r->val[0]);
    (*count)++;
    return OK;
}

static void usage(void) {
    fprintf(stderr,
        "Usage: keeper [flags] [URI]\n"
        "\n"
        "  URI:\n"
        "    k:#hashprefix              cat object to stdout\n"
        "    k:?refname                 resolve ref → SHA\n"
        "    //remote/path              sync all refs\n"
        "    //alias                    sync via alias\n"
        "    //remote?ref               sync specific ref\n"
        "\n"
        "  Flags:\n"
        "    -i <packfile>              import a git packfile\n"
        "    -s                         show store stats\n"
        "    --verify <sha>             verify object + recurse\n"
        "    --refs                     list known refs\n"
        "    --alias <name> <uri>       add remote alias\n"
    );
}

// val_flags: flags that take a value argument
con char *keeper_val_flags = "-i\0--verify\0--alias\0--want\0--have\0";

ok64 keepercli() {
    sane(1);
    call(FILEInit);

    cli c = {};
    call(CLIParse, &c, NULL, keeper_val_flags);

    keeper k = {};

    // --- Flag-based commands ---

    if (CLIHas(&c, "-s") || CLIHas(&c, "--status")) {
        call(KEEPOpen, &k, c.repo);
        fprintf(stdout, "keeper: %u pack file(s), %u index run(s)\n",
                k.npacks, k.nruns);
        u64 total_pack = 0;
        for (u32 i = 0; i < k.npacks; i++)
            total_pack += (u64)u8bDataLen(k.packs[i]);
        u64 total_idx = 0;
        for (u32 i = 0; i < k.nruns; i++)
            total_idx += (u64)kv64csLen(k.runs[i]) * sizeof(kv64);
        fprintf(stdout, "  packs: %llu bytes\n", (unsigned long long)total_pack);
        fprintf(stdout, "  index: %llu entries\n",
                (unsigned long long)(total_idx / sizeof(kv64)));
        KEEPClose(&k);
        done;
    }

    u8cs import_path = {};
    CLIFlag(&import_path, &c, "-i");
    if (import_path[0]) {
        call(KEEPOpen, &k, c.repo);
        call(KEEPImport, &k, import_path);
        KEEPClose(&k);
        done;
    }

    u8cs verify_sha = {};
    CLIFlag(&verify_sha, &c, "--verify");
    if (verify_sha[0]) {
        call(KEEPOpen, &k, c.repo);
        ok64 o = KEEPVerify(&k, verify_sha);
        KEEPClose(&k);
        if (o != OK) return o;
        done;
    }

    if (CLIHas(&c, "--refs")) {
        call(KEEPOpen, &k, c.repo);
        a_cstr(keepdir, k.dir);
        int rcount = 0;
        ok64 o = REFSEach(keepdir, refs_print_cb, &rcount);
        KEEPClose(&k);
        if (o != OK && o != REFSNONE)
            fprintf(stderr, "keeper: refs: %s\n", ok64str(o));
        fprintf(stdout, "keeper: %d ref(s)\n", rcount);
        done;
    }

    u8cs alias_name = {};
    CLIFlag(&alias_name, &c, "--alias");
    if (alias_name[0] && c.nuris > 0) {
        call(KEEPOpen, &k, c.repo);
        a_cstr(keepdir, k.dir);
        a_pad(u8, fbuf, 256);
        a_cstr(slashes, "//");
        u8bFeed(fbuf, slashes);
        u8bFeed(fbuf, alias_name);
        a_dup(u8c, from, u8bData(fbuf));
        // The URI arg is the alias target
        a_pad(u8, tbuf, 1024);
        a_dup(u8c, tdata, c.uris[0].data);
        u8bFeed(tbuf, tdata);
        a_dup(u8c, target, u8bData(tbuf));
        ok64 o = REFSAppend(keepdir, from, target);
        KEEPClose(&k);
        if (o != OK) return o;
        fprintf(stdout, "keeper: alias %.*s → %.*s\n",
                (int)u8csLen(from), (char *)from[0],
                (int)u8csLen(target), (char *)target[0]);
        done;
    }

    // --- URI-based commands ---

    if (c.nuris == 0) { usage(); fail(KEEPFAIL); }

    uri *g = &c.uris[0];

    call(KEEPOpen, &k, c.repo);
    a_cstr(keepdir, k.dir);

    if (!u8csEmpty(g->authority)) {
        // //host/path?ref → sync from remote

        // Try alias resolution
        a_pad(u8, apad, 256);
        u8bFeed(apad, g->authority);
        a_dup(u8c, akey, u8bData(apad));

        u8bp rmap = NULL;
        ref rarr[REFS_MAX_REFS];
        u32 rn = 0;
        REFSLoad(rarr, &rn, REFS_MAX_REFS, &rmap, keepdir);

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

        // If ?ref → resolve or use as want
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

        // Also collect explicit --want/--have flags
        for (u32 fi = 0; fi + 1 < c.nflags; fi += 2) {
            a_cstr(wf, "--want");
            a_cstr(hf, "--have");
            if ($eq(c.flags[fi], wf) && nwants < 63) {
                static char wbufs[64][44];
                snprintf(wbufs[nwants], 44, "%.*s",
                         (int)u8csLen(c.flags[fi + 1]),
                         (char *)c.flags[fi + 1][0]);
                want_list[nwants] = wbufs[nwants];
                nwants++;
            } else if ($eq(c.flags[fi], hf) && nhaves < 63) {
                static char hbufs[64][44];
                snprintf(hbufs[nhaves], 44, "%.*s",
                         (int)u8csLen(c.flags[fi + 1]),
                         (char *)c.flags[fi + 1][0]);
                have_list[nhaves] = hbufs[nhaves];
                nhaves++;
            }
        }

        if (rmap) u8bUnMap(rmap);

        ok64 o = KEEPSync(&k, remote,
                          nwants > 0 ? want_list : NULL,
                          nhaves > 0 ? have_list : NULL);
        KEEPClose(&k);
        if (o != OK) return o;

    } else if (!u8csEmpty(g->query)) {
        // :?refname or ?refname → resolve and print SHA
        a_pad(u8, qbuf, 256);
        u8bFeed1(qbuf, '?');
        u8bFeed(qbuf, g->query);
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
        KEEPClose(&k);
        if (!found) {
            fprintf(stderr, "keeper: ref not found\n");
            return REFSNONE;
        }

    } else if (!u8csEmpty(g->fragment)) {
        // :#hashprefix → get object
        u8cs prefix = {};
        u8csMv(prefix, g->fragment);
        if (u8csLen(prefix) < HASH_MIN_HEX) {
            fprintf(stderr, "keeper: hash too short (min %d)\n",
                    HASH_MIN_HEX);
            KEEPClose(&k);
            fail(KEEPFAIL);
        }
        size_t hexlen = u8csLen(prefix);
        u64 hashlet = keepHashlet60FromHex(
            (char const *)prefix[0], hexlen);
        Bu8 out = {};
        call(u8bMap, out, 64UL << 20);
        u8 obj_type = 0;
        ok64 o = KEEPGet(&k, hashlet, hexlen, out, &obj_type);
        if (o == OK) {
            a_dup(u8c, data, u8bData(out));
            write(STDOUT_FILENO, data[0], u8csLen(data));
        } else {
            fprintf(stderr, "keeper: object not found\n");
        }
        u8bUnMap(out);
        KEEPClose(&k);
        if (o != OK) return o;

    } else {
        KEEPClose(&k);
        usage();
        fail(KEEPFAIL);
    }

    done;
}

MAIN(keepercli);
