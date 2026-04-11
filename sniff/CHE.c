//  CHE: checkout a commit tree from keeper.
//
#include "CHE.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "keeper/GIT.h"
#include "keeper/WALK.h"

// Build absolute path: reporoot/rel
static ok64 che_fullpath(path8b out, u8cs reporoot, u8cs rel) {
    sane($ok(reporoot) && $ok(rel));
    a_cstr(sep, "/");
    call(u8bFeed, out, reporoot);
    call(u8bFeed, out, sep);
    call(u8bFeed, out, rel);
    call(PATHu8gTerm, PATHu8gIn(out));
    done;
}

// Recursive tree checkout
static ok64 che_tree(sniff *s, keeper *k, u8cs reporoot,
                     u8cp tree_sha, u8cs prefix) {
    sane(s && k && tree_sha);

    u64 hashlet = wh64Hashlet(tree_sha);
    Bu8 buf = {};
    call(u8bAllocate, buf, 1UL << 24);
    u8 otype = 0;
    ok64 o = KEEPGet(k, hashlet, 10, buf, &otype);
    if (o != OK) { u8bFree(buf); fail(o); }
    if (otype != KEEP_OBJ_TREE) { u8bFree(buf); fail(SNIFFFAIL); }

    // Copy tree content (KEEPGet may reuse buffer)
    size_t tsz = u8bDataLen(buf);
    u8 *tcopy = malloc(tsz);
    if (!tcopy) { u8bFree(buf); fail(SNIFFFAIL); }
    memcpy(tcopy, u8bDataHead(buf), tsz);
    u8bFree(buf);

    u8cs tree = {tcopy, tcopy + tsz};
    u8cs file = {}, esha = {};
    ok64 result = OK;

    while (GITu8sDrainTree(tree, file, esha) == OK) {
        u8cs scan = {file[0], file[1]};
        if (u8csFind(scan, ' ') != OK) continue;
        u8cs mode_s = {file[0], scan[0]};
        u8cp name_start = scan[0];
        ++name_start;
        u8cs name_s = {name_start, file[1]};

        // mode 40000=tree, 100644/100755=file, 160000=submodule, 120000=symlink
        b8 is_dir = ($at(mode_s, 0) == '4');
        b8 is_submodule = ($len(mode_s) >= 2 &&
                           $at(mode_s, 0) == '1' && $at(mode_s, 1) == '6');
        b8 is_symlink = ($len(mode_s) >= 2 &&
                         $at(mode_s, 0) == '1' && $at(mode_s, 1) == '2');
        if (is_submodule) continue;

        // Build relative path: prefix/name
        a_pad(u8, rel, 2048);
        if (!$empty(prefix)) {
            u8bFeed(rel, prefix);
            u8bFeed1(rel, '/');
        }
        u8bFeed(rel, name_s);
        PATHu8gTerm(PATHu8gIn(rel));
        u8cs relpath = {u8bDataHead(rel), rel[2]};

        u64 entry_hashlet = wh64Hashlet(esha[0]);

        if (is_dir) {
            a_path(dp);
            che_fullpath(dp, reporoot, relpath);
            FILEMakeDir(PATHu8cgIn(dp));

            u32 idx = SNIFFIntern(s, relpath);
            SNIFFRecord(s, SNIFF_HASHLET, idx, entry_hashlet);

            result = che_tree(s, k, reporoot, esha[0], relpath);
            if (result != OK) break;
        } else {
            u32 idx = SNIFFIntern(s, relpath);

            // Skip unchanged
            u64 old_hashlet = SNIFFGet(s, SNIFF_HASHLET, idx);
            if (old_hashlet == entry_hashlet && old_hashlet != 0) continue;

            // Protect dirty files
            u64 co = SNIFFGet(s, SNIFF_CHECKOUT, idx);
            u64 ch = SNIFFGet(s, SNIFF_CHANGED, idx);
            if (ch != 0 && ch != co) {
                u8cs p = {};
                SNIFFPath(p, s, idx);
                fprintf(stderr, "sniff: skip dirty %.*s\n",
                        (int)$len(p), (char *)p[0]);
                continue;
            }

            // Get blob
            Bu8 blob = {};
            result = u8bAllocate(blob, 1UL << 24);
            if (result != OK) break;
            u8 bt = 0;
            result = KEEPGet(k, entry_hashlet, 10, blob, &bt);
            if (result != OK) { u8bFree(blob); break; }

            a_path(fp);
            result = che_fullpath(fp, reporoot, relpath);
            if (result != OK) { u8bFree(blob); break; }

            if (is_symlink) {
                u8bFeed1(blob, 0);
                symlink((char *)u8bDataHead(blob),
                        (char *)u8bDataHead(fp));
            } else {
                int fd = -1;
                result = FILECreate(&fd, PATHu8cgIn(fp));
                if (result != OK) { u8bFree(blob); break; }
                u8cs data = {u8bDataHead(blob), u8bIdleHead(blob)};
                result = FILEFeedall(fd, data);
                close(fd);
                if (result != OK) { u8bFree(blob); break; }

                if ($len(mode_s) >= 6 && $at(mode_s, 3) == '7')
                    chmod((char *)u8bDataHead(fp), 0755);
            }
            u8bFree(blob);

            SNIFFRecord(s, SNIFF_HASHLET, idx, entry_hashlet);

            struct stat sb = {};
            if (FILEStat(&sb, PATHu8cgIn(fp)) == OK)
                SNIFFRecord(s, SNIFF_CHECKOUT, idx,
                            (u64)sb.st_mtim.tv_sec);
        }
    }

    free(tcopy);
    return result;
}

// --- Public API ---

ok64 CHECheckout(sniff *s, keeper *k, u8cs reporoot, u8cs hex) {
    sane(s && k && $ok(hex));

    size_t hexlen = $len(hex);
    if (hexlen > 10) hexlen = 10;
    u64 hashlet = wh64HashletFromHex((char const *)hex[0], hexlen);

    // Get commit
    Bu8 buf = {};
    call(u8bAllocate, buf, 1UL << 24);
    u8 otype = 0;
    ok64 o = KEEPGet(k, hashlet, hexlen, buf, &otype);
    if (o != OK || otype != KEEP_OBJ_COMMIT) {
        u8bFree(buf);
        fprintf(stderr, "sniff: object not found or not a commit\n");
        fail(SNIFFFAIL);
    }

    // Extract tree SHA
    u8 tree_sha[GIT_SHA1_LEN] = {};
    u8cs commit = {u8bDataHead(buf), u8bIdleHead(buf)};
    o = WALKCommitTree(commit, tree_sha);
    u8bFree(buf);
    if (o != OK) {
        fprintf(stderr, "sniff: bad commit (no tree)\n");
        fail(SNIFFFAIL);
    }

    // Checkout tree
    u8cs no_prefix = {};
    call(che_tree, s, k, reporoot, tree_sha, no_prefix);

    fprintf(stderr, "sniff: checkout done\n");
    done;
}
