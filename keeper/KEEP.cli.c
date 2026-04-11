//  keeper CLI: local git object store operations.
//
//  keeper sync [remote]         fetch missing objects
//  keeper get <hash-prefix>     cat object to stdout
//  keeper has <hash-prefix>     exit 0 if present, 1 if not
//  keeper info                  show stats
//
#include "KEEP.h"
#include "REFS.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "dog/HOME.h"

static void usage(void) {
    fprintf(stderr,
        "Usage: keeper [options] [args...]\n"
        "\n"
        "  keeper -i | --index <packfile>  import a git packfile\n"
        "  keeper -u | --update [remote]   fetch missing objects\n"
        "  keeper -s | --status            show store stats\n"
        "  keeper get <hash-prefix>        cat object to stdout\n"
        "  keeper has <hash-prefix>        check if object exists\n"
        "  keeper --verify <full-sha>      verify object + recurse\n"
        "  keeper --refs                   list known refs\n"
        "  keeper --alias <name> <uri>     add remote alias\n"
    );
}

static ok64 refs_print_cb(u8cs from, u8cs to, ron60 stamp, void *ctx) {
    (void)stamp;
    int *count = (int *)ctx;
    fprintf(stdout, "  %.*s\t→ %.*s\n",
            (int)$len(from), (char *)from[0],
            (int)$len(to), (char *)to[0]);
    (*count)++;
    return OK;
}

static b8 argeq(u8cs a, char const *b) {
    size_t blen = strlen(b);
    return $len(a) == blen && memcmp(a[0], b, blen) == 0;
}

ok64 keepercli() {
    sane(1);

    int argn = (int)$arglen;
    if (argn < 2) { usage(); fail(KEEPFAIL); }

    u8cs cmd = {};
    $mv(cmd, $arg(1));

    // Find repo root
    a_path(root);
    ok64 rho = HOMEFind(root);
    u8cs reporoot = {};
    if (rho == OK) {
        reporoot[0] = u8bDataHead(root);
        reporoot[1] = u8bDataHead(root) + u8bDataLen(root);
    }

    keeper k = {};

    if (argeq(cmd, "-s") || argeq(cmd, "--status") || argeq(cmd, "info")) {
        call(KEEPOpen, &k, reporoot);
        fprintf(stdout, "keeper: %u pack file(s), %u index run(s)\n",
                k.npacks, k.nruns);
        u64 total_pack = 0;
        for (u32 i = 0; i < k.npacks; i++)
            total_pack += (u64)(u8bIdleHead(k.packs[i]) - u8bDataHead(k.packs[i]));
        u64 total_idx = 0;
        for (u32 i = 0; i < k.nruns; i++)
            total_idx += (u64)(k.runs[i][1] - k.runs[i][0]) * sizeof(kv64);
        fprintf(stdout, "  packs: %llu bytes\n", (unsigned long long)total_pack);
        fprintf(stdout, "  index: %llu entries\n",
                (unsigned long long)(total_idx / sizeof(kv64)));
        KEEPClose(&k);

    } else if (argeq(cmd, "has")) {
        if (argn < 3) { fprintf(stderr, "keeper: has requires a hash prefix\n"); fail(KEEPFAIL); }
        u8cs prefix = {};
        $mv(prefix, $arg(2));
        if ($len(prefix) < HASH_MIN_HEX) {
            fprintf(stderr, "keeper: prefix too short (min %d hex chars)\n", HASH_MIN_HEX);
            fail(KEEPFAIL);
        }
        call(KEEPOpen, &k, reporoot);
        size_t hexlen = $len(prefix);
        u64 hashlet = wh64HashletFromHex((char const *)prefix[0], hexlen);
        ok64 o = KEEPHas(&k, hashlet, hexlen);
        KEEPClose(&k);
        if (o == OK) {
            fprintf(stdout, "found\n");
        } else {
            fprintf(stdout, "not found\n");
            return KEEPNONE;
        }

    } else if (argeq(cmd, "get")) {
        if (argn < 3) { fprintf(stderr, "keeper: get requires a hash prefix\n"); fail(KEEPFAIL); }
        u8cs prefix = {};
        $mv(prefix, $arg(2));
        if ($len(prefix) < HASH_MIN_HEX) {
            fprintf(stderr, "keeper: prefix too short (min %d hex chars)\n", HASH_MIN_HEX);
            fail(KEEPFAIL);
        }
        call(KEEPOpen, &k, reporoot);
        size_t hexlen = $len(prefix);
        u64 hashlet = wh64HashletFromHex((char const *)prefix[0], hexlen);
        Bu8 out = {};
        call(u8bMap, out, 64UL << 20);
        u8 obj_type = 0;
        ok64 o = KEEPGet(&k, hashlet, hexlen, out, &obj_type);
        if (o == OK) {
            u8cs data = {u8bDataHead(out), u8bDataHead(out) + u8bDataLen(out)};
            write(STDOUT_FILENO, data[0], $len(data));
        } else {
            fprintf(stderr, "keeper: object not found\n");
        }
        u8bUnMap(out);
        KEEPClose(&k);
        if (o != OK) return o;

    } else if (argeq(cmd, "-i") || argeq(cmd, "--index") || argeq(cmd, "import")) {
        if (argn < 3) { fprintf(stderr, "keeper: import requires a packfile path\n"); fail(KEEPFAIL); }
        u8cs path = {};
        $mv(path, $arg(2));
        call(KEEPOpen, &k, reporoot);
        call(KEEPImport, &k, path);
        KEEPClose(&k);

    } else if (argeq(cmd, "--verify")) {
        if (argn < 3) { fprintf(stderr, "keeper: --verify requires a full SHA\n"); fail(KEEPFAIL); }
        u8cs sha = {};
        $mv(sha, $arg(2));
        call(KEEPOpen, &k, reporoot);
        ok64 o = KEEPVerify(&k, sha);
        KEEPClose(&k);
        if (o != OK) return o;

    } else if (argeq(cmd, "-u") || argeq(cmd, "--update") || argeq(cmd, "sync")) {
        u8cs remote = {};
        if (argn >= 3) $mv(remote, $arg(2));

        // Collect --want and --have args
        char const *want_list[64] = {};
        char const *have_list[64] = {};
        int nwants = 0, nhaves = 0;
        for (int i = 3; i < argn; i++) {
            u8cs a = {};
            $mv(a, $arg(i));
            if (argeq(a, "--want") && i + 1 < argn) {
                i++;
                u8cs v = {};
                $mv(v, $arg(i));
                // Copy to static buffer
                static char wbuf[64][44];
                if (nwants < 63) {
                    snprintf(wbuf[nwants], 44, "%.*s", (int)$len(v), (char*)v[0]);
                    want_list[nwants] = wbuf[nwants];
                    nwants++;
                }
            } else if (argeq(a, "--have") && i + 1 < argn) {
                i++;
                u8cs v = {};
                $mv(v, $arg(i));
                static char hbuf[64][44];
                if (nhaves < 63) {
                    snprintf(hbuf[nhaves], 44, "%.*s", (int)$len(v), (char*)v[0]);
                    have_list[nhaves] = hbuf[nhaves];
                    nhaves++;
                }
            }
        }

        call(KEEPOpen, &k, reporoot);
        ok64 o = KEEPSync(&k, remote,
                          nwants > 0 ? want_list : NULL,
                          nhaves > 0 ? have_list : NULL);
        KEEPClose(&k);
        if (o != OK) return o;

    } else if (argeq(cmd, "--refs")) {
        call(KEEPOpen, &k, reporoot);
        u8cs keepdir = {(u8cp)k.dir, (u8cp)k.dir + strlen(k.dir)};
        int rcount = 0;
        ok64 o = REFSEach(keepdir, refs_print_cb, &rcount);
        KEEPClose(&k);
        if (o != OK && o != REFSNONE)
            fprintf(stderr, "keeper: refs: %s\n", ok64str(o));
        fprintf(stdout, "keeper: %d ref(s)\n", rcount);

    } else if (argeq(cmd, "--alias")) {
        if (argn < 4) {
            fprintf(stderr, "keeper: --alias requires <name> <uri>\n");
            fail(KEEPFAIL);
        }
        u8cs aname = {};
        $mv(aname, $arg(2));
        u8cs target = {};
        $mv(target, $arg(3));

        call(KEEPOpen, &k, reporoot);
        u8cs keepdir = {(u8cp)k.dir, (u8cp)k.dir + strlen(k.dir)};

        // build "//name" as from-URI
        a_pad(u8, fbuf, 256);
        u8p fp = u8bIdleHead(fbuf);
        *fp++ = '/'; *fp++ = '/';
        memcpy(fp, aname[0], $len(aname));
        fp += $len(aname);
        u8bFed(fbuf, (size_t)(fp - u8bIdleHead(fbuf)));
        a_dup(u8c, from, u8bData(fbuf));

        ok64 o = REFSAppend(keepdir, from, target);
        KEEPClose(&k);
        if (o != OK) return o;
        fprintf(stdout, "keeper: alias %.*s → %.*s\n",
                (int)$len(from), (char *)from[0],
                (int)$len(target), (char *)target[0]);

    } else {
        usage();
        fail(KEEPFAIL);
    }

    done;
}

MAIN(keepercli);
