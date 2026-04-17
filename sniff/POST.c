//  POST: wrap the current base tree into a commit object.
//
#include "POST.h"
#include "PUT.h"

#include <string.h>
#include <time.h>

#include "abc/HEX.h"
#include "abc/PRO.h"
#include "keeper/GIT.h"
#include "keeper/SHA1.h"

// Compute full SHA1 of parent commit for the "parent" line.
static ok64 POSTParentSha(sha1 *out, keeper *k, u8cs parent_hex) {
    sane(out && k && $ok(parent_hex));

    size_t hexlen = $len(parent_hex);
    if (hexlen > 15) hexlen = 15;
    u64 hashlet = WHIFFHexHashlet60(parent_hex);

    Bu8 cbuf = {};
    call(u8bAllocate, cbuf, 1UL << 24);
    u8 ctype = 0;
    call(KEEPGet, k, hashlet, hexlen, cbuf, &ctype);

    // Dereference tag
    if (ctype == DOG_OBJ_TAG) {
        u8cs body = {u8bDataHead(cbuf), u8bIdleHead(cbuf)};
        u8cs field = {}, value = {};
        sha1 tag_sha = {};
        a_raw(tag_bin, tag_sha);
        while (GITu8sDrainCommit(body, field, value) == OK) {
            if ($empty(field)) break;
            if ($len(field) == 6 && memcmp(field[0], "object", 6) == 0 &&
                $len(value) >= 40) {
                u8cs hex40 = {value[0], $atp(value, 40)};
                HEXu8sDrainSome(tag_bin, hex40);
                break;
            }
        }
        u64 ch = WHIFFHashlet60(&tag_sha);
        u8bReset(cbuf);
        call(KEEPGet, k, ch, 15, cbuf, &ctype);
    }
    if (ctype != DOG_OBJ_COMMIT) { u8bFree(cbuf); fail(SNIFFFAIL); }

    // SHA1("commit <len>\0" + content)
    size_t csz = u8bDataLen(cbuf);
    char hdr[64];
    int hlen = snprintf(hdr, sizeof(hdr), "commit %zu", csz);
    Bu8 tmp = {};
    call(u8bAllocate, tmp, (u64)hlen + 1 + csz);
    u8cs hs = {(u8cp)hdr, (u8cp)hdr + hlen};
    u8bFeed(tmp, hs);
    u8bFeed1(tmp, 0);
    u8bFeed(tmp, u8bDataC(cbuf));
    a_dup(u8c, _d, u8bData(tmp));
    SHA1Sum(out, _d);
    u8bFree(tmp);
    u8bFree(cbuf);
    done;
}

// Resolve current HEAD to a commit's tree hashlet.  Returns 0 if
// HEAD is unset or does not resolve to a commit.
static u64 post_head_tree_hashlet(sniff *s, keeper *k) {
    u8cs head = {};
    SNIFFHead(head, s);
    if ($empty(head)) return 0;
    sha1 tree_sha = {};
    if (SNIFFParentTreeSha(&tree_sha, k, head) != OK) return 0;
    return WHIFFHashlet40(&tree_sha);
}

// --- Public API ---

ok64 POSTCommit(sniff *s, keeper *k, u8cs reporoot,
                u8cs message, u8cs author, sha1 *sha_out) {
    sane(s && k && $ok(message) && $ok(author));

    keep_pack p = {};
    call(KEEPPackOpen, k, &p);

    u64 base = SNIFFBaseTree(s);
    u64 head_tree = post_head_tree_hashlet(s, k);

    // The tree SHA for the commit body.  Obtained either from the
    // auto-stage return (fresh tree we just packed, not yet indexed
    // by keeper) or via KEEPGet on a previously-staged base.
    sha1 root_tree = {};
    b8 have_root_sha = NO;

    // Auto-stage when nothing has been explicitly PUT / DELETEd yet:
    // "git commit -a" ergonomics.
    if (base == 0 || base == head_tree) {
        ok64 so = PUTStage(&root_tree, s, k, &p, reporoot, NULL);
        if (so != OK) { KEEPPackClose(k, &p); return so; }
        base = SNIFFBaseTree(s);
        if (base == 0) {
            // Empty worktree: commit an empty tree object.
            u8cs empty = {};
            call(KEEPPackFeed, k, &p, DOG_OBJ_TREE, empty, &root_tree);
            base = WHIFFHashlet40(&root_tree);
            SNIFFRecord(s, SNIFF_TREE, SNIFFRootIdx(s), base);
        }
        have_root_sha = !sha1empty(&root_tree);
    }

    // Fall back to keeper lookup.  Pack must be closed first so the
    // idx file is visible; otherwise KEEPGet can't see freshly-packed
    // objects from this invocation.  That path triggers only when a
    // previous (already-closed) PUT/DELETE staged the base tree.
    if (!have_root_sha) {
        KEEPPackClose(k, &p);

        Bu8 tbuf = {};
        call(u8bAllocate, tbuf, 1UL << 24);
        u8 otype = 0;
        ok64 lo = KEEPGet(k, base << 20, 10, tbuf, &otype);
        if (lo != OK || otype != DOG_OBJ_TREE) {
            u8bFree(tbuf);
            fail(SNIFFFAIL);
        }
        KEEPObjSha(&root_tree, DOG_OBJ_TREE, u8bDataC(tbuf));
        u8bFree(tbuf);

        // Reopen a fresh pack for the commit object.
        call(KEEPPackOpen, k, &p);
    }

    // Resolve parent (current HEAD).  Missing HEAD = root commit.
    sha1 parent_sha = {};
    b8 has_parent = NO;
    {
        u8cs head = {};
        SNIFFHead(head, s);
        if (!$empty(head)) {
            ok64 po = POSTParentSha(&parent_sha, k, head);
            if (po == OK) has_parent = YES;
        }
    }

    // Build commit object
    Bu8 com = {};
    call(u8bAllocate, com, 4096);

    a_cstr(tree_label, "tree ");
    u8bFeed(com, tree_label);
    a_pad(u8, tree_hex, 40);
    a_rawc(tsha, root_tree);
    HEXu8sFeedSome(tree_hex_idle, tsha);
    u8bFeed(com, u8bDataC(tree_hex));
    u8bFeed1(com, '\n');

    if (has_parent) {
        a_cstr(par_label, "parent ");
        u8bFeed(com, par_label);
        a_pad(u8, par_hex, 40);
        a_rawc(psha, parent_sha);
        HEXu8sFeedSome(par_hex_idle, psha);
        u8bFeed(com, u8bDataC(par_hex));
        u8bFeed1(com, '\n');
    }

    time_t now = time(NULL);
    char ts[64];
    int tslen = snprintf(ts, sizeof(ts), " %lld +0000\n", (long long)now);
    u8cs ts_s = {(u8cp)ts, (u8cp)ts + tslen};

    a_cstr(auth_label, "author ");
    u8bFeed(com, auth_label);
    u8bFeed(com, author);
    u8bFeed(com, ts_s);

    a_cstr(comm_label, "committer ");
    u8bFeed(com, comm_label);
    u8bFeed(com, author);
    u8bFeed(com, ts_s);

    u8bFeed1(com, '\n');
    u8bFeed(com, message);
    u8bFeed1(com, '\n');

    u8cs com_data = {u8bDataHead(com), u8bIdleHead(com)};
    ok64 o = KEEPPackFeed(k, &p, DOG_OBJ_COMMIT, com_data, sha_out);
    u8bFree(com);
    if (o != OK) { KEEPPackClose(k, &p); return o; }

    call(KEEPPackClose, k, &p);

    a_pad(u8, out_hex, 40);
    u8cs osha = {sha_out->data, sha_out->data + GIT_SHA1_LEN};
    HEXu8sFeedSome(out_hex_idle, osha);

    // Update HEAD to new commit
    u8cs new_hex = {u8bDataHead(out_hex), out_hex[2]};
    SNIFFSetHead(s, new_hex);

    fprintf(stderr, "sniff: commit %.*s\n",
            (int)u8bDataLen(out_hex), (char *)u8bDataHead(out_hex));
    done;
}
