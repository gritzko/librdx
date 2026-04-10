//  keeper CLI: local git object store operations.
//
//  keeper sync [remote]         fetch missing objects
//  keeper get <hash-prefix>     cat object to stdout
//  keeper has <hash-prefix>     exit 0 if present, 1 if not
//  keeper info                  show stats
//
#include "KEEP.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "dog/HOME.h"

static void usage(void) {
    fprintf(stderr,
        "Usage: keeper <command> [args...]\n"
        "\n"
        "  keeper import <packfile>   import a git packfile\n"
        "  keeper sync [remote]       fetch missing objects from remote\n"
        "  keeper get <hash-prefix>   cat object to stdout\n"
        "  keeper has <hash-prefix>   exit 0 if present, 1 if not\n"
        "  keeper info                show store stats\n"
    );
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

    if (argeq(cmd, "info")) {
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
        ok64 o = KEEPGet(&k, hashlet, hexlen, out);
        if (o == OK) {
            u8cs data = {u8bDataHead(out), u8bDataHead(out) + u8bDataLen(out)};
            write(STDOUT_FILENO, data[0], $len(data));
        } else {
            fprintf(stderr, "keeper: object not found\n");
        }
        u8bUnMap(out);
        KEEPClose(&k);
        if (o != OK) return o;

    } else if (argeq(cmd, "import")) {
        if (argn < 3) { fprintf(stderr, "keeper: import requires a packfile path\n"); fail(KEEPFAIL); }
        u8cs path = {};
        $mv(path, $arg(2));
        call(KEEPOpen, &k, reporoot);
        call(KEEPImport, &k, path);
        KEEPClose(&k);

    } else if (argeq(cmd, "sync")) {
        u8cs remote = {};
        if (argn >= 3) $mv(remote, $arg(2));
        call(KEEPOpen, &k, reporoot);
        ok64 o = KEEPSync(&k, remote);
        KEEPClose(&k);
        if (o != OK) return o;

    } else {
        usage();
        fail(KEEPFAIL);
    }

    done;
}

MAIN(keepercli);
