//  PUT: create a commit from worktree state.
//
#include "PUT.h"
#include "POST.h"

#include <string.h>
#include <time.h>

#include "abc/HEX.h"
#include "abc/PRO.h"
#include "keeper/GIT.h"
#include "keeper/SHA1.h"
#include "keeper/WALK.h"

// Compute full SHA1 of parent commit for the "parent" line.
static ok64 PUTParentSha(sha1 *out, keeper *k, u8cs parent_hex) {
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

// --- Public API ---

ok64 PUTCommit(sniff *s, keeper *k, u8cs reporoot,
               u8cs parent_hex, u8cs message, u8cs author,
               u8cp commit_set, sha1 *sha_out) {
    sane(s && k && $ok(parent_hex) && $ok(message) && $ok(author));

    keep_pack p = {};
    call(KEEPPackOpen, k, &p);

    // Build tree
    sha1 root_tree = {};
    ok64 o = POSTTree(&root_tree, s, k, &p, reporoot,
                       parent_hex, commit_set);
    if (o != OK) { KEEPPackClose(k, &p); return o; }

    // Compute parent full SHA
    sha1 parent_sha = {};
    o = PUTParentSha(&parent_sha, k, parent_hex);
    if (o != OK) { KEEPPackClose(k, &p); return o; }

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

    a_cstr(par_label, "parent ");
    u8bFeed(com, par_label);
    a_pad(u8, par_hex, 40);
    a_rawc(psha, parent_sha);
    HEXu8sFeedSome(par_hex_idle, psha);
    u8bFeed(com, u8bDataC(par_hex));
    u8bFeed1(com, '\n');

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
    o = KEEPPackFeed(k, &p, DOG_OBJ_COMMIT, com_data, sha_out);
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
