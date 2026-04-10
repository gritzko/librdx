//  belt: ersatz git repo CLI
//
//  Usage:
//    belt clone <git-repo> <belt-dir>    clone repo into .belt format
//    belt get   <belt-dir> <sha1-hex>    retrieve and print object
//
#include "keeper/BELT.h"

#include <stdio.h>
#include <string.h>

#include "abc/PRO.h"

static char const *type_names[] = {
    [BELT_COMMIT] = "commit",
    [BELT_TREE] = "tree",
    [BELT_BLOB] = "blob",
    [BELT_TAG] = "tag",
};

ok64 belt_main() {
    sane(1);

    if ($arglen < 2) {
        fprintf(stderr,
            "usage: belt clone  <git-repo>  <belt-dir>\n"
            "       belt import <pack-file> <belt-dir>\n"
            "       belt get    <belt-dir>  <sha1-hex>\n");
        fail(BELTFAIL);
    }

    a$rg(verb, 1);

    if ($len(verb) == 5 && memcmp(verb[0], "clone", 5) == 0) {
        if ($arglen < 4) fail(BELTFAIL);
        a$rg(repo, 2);
        a$rg(dir, 3);
        call(BELTClone, repo, dir);
    } else if ($len(verb) == 6 && memcmp(verb[0], "import", 6) == 0) {
        if ($arglen < 4) fail(BELTFAIL);
        a$rg(packfile, 2);
        a$rg(dir, 3);
        call(BELTImport, packfile, dir);
    } else if ($len(verb) == 3 && memcmp(verb[0], "get", 3) == 0) {
        if ($arglen < 4) fail(BELTFAIL);
        a$rg(dir, 2);
        a$rg(hash, 3);

        static u8 outbuf[1 << 26];
        u8g out = {outbuf, outbuf, outbuf + sizeof(outbuf)};
        u8 obj_type = 0;

        call(BELTGet, dir, hash, out, &obj_type);

        u64 sz = u8gLeftLen(out);
        if (obj_type >= 1 && obj_type <= 4)
            fprintf(stderr, "%s %lu\n", type_names[obj_type],
                    (unsigned long)sz);

        fwrite(outbuf, 1, sz, stdout);
    } else {
        fprintf(stderr, "unknown command: %.*s\n",
                (int)$len(verb), (char *)verb[0]);
        fail(BELTFAIL);
    }

    done;
}

MAIN(belt_main)
