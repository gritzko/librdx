//  CHE: checkout a commit tree from keeper.
//
#include "CHE.h"

#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "keeper/GIT.h"
#include "keeper/WALK.h"

// Recursive tree checkout.  seen[idx]=1 for every path visited.
static ok64 che_tree(sniff *s, keeper *k, u8cs reporoot,
                     u8cp tree_sha, u8cs prefix, u8p seen) {
    sane(s && k && tree_sha);

    u64 hashlet = wh64Hashlet(tree_sha);
    Bu8 buf = {};
    call(u8bAllocate, buf, 1UL << 24);
    u8 otype = 0;
    ok64 o = KEEPGet(k, hashlet, 10, buf, &otype);
    if (o != OK) { u8bFree(buf); fail(o); }
    if (otype != KEEP_OBJ_TREE) { u8bFree(buf); fail(SNIFFFAIL); }

    // Snapshot tree content (KEEPGet may reuse buffer)
    size_t tsz = u8bDataLen(buf);
    Bu8 tcopy = {};
    o = u8bAllocate(tcopy, tsz);
    if (o != OK) { u8bFree(buf); fail(o); }
    u8bFeed(tcopy, u8bDataC(buf));
    u8bFree(buf);

    u8cs tree = {u8bDataHead(tcopy), u8bIdleHead(tcopy)};
    u8cs file = {}, esha = {};
    ok64 result = OK;

    while (GITu8sDrainTree(tree, file, esha) == OK) {
        // file = "mode name"; find the space
        u8cs scan = {file[0], file[1]};
        if (u8csFind(scan, ' ') != OK) continue;
        u8cs mode_s = {file[0], scan[0]};
        // scan[0] is at ' '; name starts after it
        u8cs rest = {scan[0], file[1]};
        if ($len(rest) < 2) continue;
        u8cs name_s = {rest[0], file[1]};
        // skip the space
        ++name_s[0];

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
            SNIFFFullpath(dp, reporoot, relpath);
            FILEMakeDir(PATHu8cgIn(dp));

            u32 idx = SNIFFIntern(s, relpath);
            seen[idx] = 1;
            SNIFFRecord(s, SNIFF_HASHLET, idx, entry_hashlet);

            result = che_tree(s, k, reporoot, esha[0], relpath, seen);
            if (result != OK) break;
        } else {
            u32 idx = SNIFFIntern(s, relpath);
            seen[idx] = 1;

            u64 old_hashlet = SNIFFGet(s, SNIFF_HASHLET, idx);
            if (old_hashlet == entry_hashlet && old_hashlet != 0) continue;

            u64 co = SNIFFGet(s, SNIFF_CHECKOUT, idx);
            u64 ch = SNIFFGet(s, SNIFF_CHANGED, idx);
            if (ch != 0 && ch != co) {
                u8cs p = {};
                SNIFFPath(p, s, idx);
                fprintf(stderr, "sniff: skip dirty %.*s\n",
                        (int)$len(p), (char *)p[0]);
                continue;
            }

            Bu8 blob = {};
            result = u8bAllocate(blob, 1UL << 24);
            if (result != OK) break;
            u8 bt = 0;
            result = KEEPGet(k, entry_hashlet, 10, blob, &bt);
            if (result != OK) { u8bFree(blob); break; }

            a_path(fp);
            result = SNIFFFullpath(fp, reporoot, relpath);
            if (result != OK) { u8bFree(blob); break; }

            if (is_symlink) {
                unlink((char *)u8bDataHead(fp));
                u8bFeed1(blob, 0);
                symlink((char *)u8bDataHead(blob),
                        (char *)u8bDataHead(fp));
            } else {
                int fd = -1;
                result = FILECreate(&fd, PATHu8cgIn(fp));
                if (result != OK) { u8bFree(blob); break; }
                u8cs data = {u8bDataHead(blob), u8bIdleHead(blob)};
                result = FILEFeedall(fd, data);
                FILEClose(&fd);
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

    u8bFree(tcopy);
    return result;
}

// Remove tracked files not in the new tree
static ok64 che_prune(sniff *s, u8cs reporoot, u8cp seen) {
    sane(s);
    u32 n = SNIFFCount(s);
    u32 removed = 0;

    for (u32 i = 0; i < n; i++) {
        if (seen[i]) continue;
        u64 h = SNIFFGet(s, SNIFF_HASHLET, i);
        if (h == 0) continue;

        u8cs rel = {};
        if (SNIFFPath(rel, s, i) != OK) continue;

        a_path(fp);
        if (SNIFFFullpath(fp, reporoot, rel) != OK) continue;

        unlink((char *)u8bDataHead(fp));

        SNIFFRecord(s, SNIFF_HASHLET, i, 0);
        SNIFFRecord(s, SNIFF_CHECKOUT, i, 0);
        removed++;
    }

    if (removed > 0)
        fprintf(stderr, "sniff: removed %u stale file(s)\n", removed);
    done;
}

// --- Public API ---

ok64 CHECheckout(sniff *s, keeper *k, u8cs reporoot, u8cs hex) {
    sane(s && k && $ok(hex));

    size_t hexlen = $len(hex);
    if (hexlen > 10) hexlen = 10;
    u64 hashlet = wh64HashletFromHex((char const *)hex[0], hexlen);

    Bu8 buf = {};
    call(u8bAllocate, buf, 1UL << 24);
    u8 otype = 0;
    ok64 o = KEEPGet(k, hashlet, hexlen, buf, &otype);
    if (o != OK) {
        u8bFree(buf);
        fprintf(stderr, "sniff: object not found\n");
        fail(SNIFFFAIL);
    }

    // Dereference annotated tag
    if (otype == KEEP_OBJ_TAG) {
        u8cs body = {u8bDataHead(buf), u8bIdleHead(buf)};
        u8cs field = {}, value = {};
        a_pad(u8, shabin, GIT_SHA1_LEN);
        b8 found = NO;
        while (GITu8sDrainCommit(body, field, value) == OK) {
            if ($empty(field)) break;
            if ($len(field) == 6 && memcmp(field[0], "object", 6) == 0 &&
                $len(value) >= 40) {
                u8cs hex40 = {value[0], value[0]};
                hex40[1] = $atp(value, 40);
                HEXu8sDrainSome(shabin_idle, hex40);
                found = YES;
                break;
            }
        }
        if (!found) {
            u8bFree(buf);
            fprintf(stderr, "sniff: bad tag (no object)\n");
            fail(SNIFFFAIL);
        }
        u64 commit_hashlet = wh64Hashlet(u8bDataHead(shabin));
        u8bReset(buf);
        o = KEEPGet(k, commit_hashlet, 10, buf, &otype);
        if (o != OK || otype != KEEP_OBJ_COMMIT) {
            u8bFree(buf);
            fprintf(stderr, "sniff: tag target not a commit\n");
            fail(SNIFFFAIL);
        }
    }

    if (otype != KEEP_OBJ_COMMIT) {
        u8bFree(buf);
        fprintf(stderr, "sniff: not a commit\n");
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

    // Allocate seen bitmap
    u32 npath = SNIFFCount(s);
    u32 seen_size = npath + SNIFF_HASH_SIZE;
    Bu8 seen_buf = {};
    call(u8bAllocate, seen_buf, seen_size);
    memset(u8bDataHead(seen_buf), 0, seen_size);

    u8cs no_prefix = {};
    o = che_tree(s, k, reporoot, tree_sha, no_prefix, u8bDataHead(seen_buf));

    if (o == OK)
        o = che_prune(s, reporoot, u8bDataHead(seen_buf));

    u8bFree(seen_buf);
    if (o == OK)
        fprintf(stderr, "sniff: checkout done\n");
    return o;
}
