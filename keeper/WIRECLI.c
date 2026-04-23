//  WIRECLI: client side of the git wire protocol (WIRE.md Phase 7).
//
//  WIREFetch — spawn ssh/local upload-pack, drain refs advertisement,
//              send wants/haves, ingest the returned packfile into the
//              local keeper, append a fresh REFS tip entry.
//  WIREPush  — spawn ssh/local receive-pack, drain its advertisement,
//              build a packfile from our reachable closure for the
//              chosen branch, send a single ref update, drain the
//              unpack/per-ref status reply.
//
//  Transport dispatch lives here (URI parsing → ssh argv | local
//  argv).  Everything else is shared with the server-side WIRE.c
//  through library primitives (PKT, REFADV, KEEPIngestFile,
//  KEEPGetExact, ZINFDeflate).

#include "WIRE.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PRO.h"
#include "abc/URI.h"
#include "dog/SHA1.h"
#include "keeper/GIT.h"
#include "keeper/KEEP.h"
#include "keeper/PKT.h"
#include "keeper/REFADV.h"
#include "keeper/REFS.h"
#include "keeper/SHA1.h"
#include "keeper/ZINF.h"

// --- small slice helpers ------------------------------------------------

static b8 wcli_starts_with(u8csc s, u8c const *pfx, size_t plen) {
    if ((size_t)u8csLen(s) < plen) return NO;
    return memcmp(s[0], pfx, plen) == 0;
}

static b8 wcli_eq_lit(u8csc s, u8c const *lit, size_t llen) {
    if ((size_t)u8csLen(s) != llen) return NO;
    return memcmp(s[0], lit, llen) == 0;
}

static b8 wcli_decode_sha(sha1 *out, u8csc hex) {
    if (u8csLen(hex) != 40) return NO;
    a_dup(u8c, hex_dup, hex);
    u8s bin = {out->data, out->data + 20};
    if (HEXu8sDrainSome(bin, hex_dup) != OK) return NO;
    if (bin[0] != out->data + 20) return NO;
    return YES;
}

static void wcli_sha_to_hex(u8 *out40, sha1 const *s) {
    u8s hs = {out40, out40 + 40};
    u8cs bs = {s->data, s->data + 20};
    HEXu8sFeedSome(hs, bs);
}

// --- pkt-line drain with refill ----------------------------------------

#define WCLI_BUF (1u << 16)

//  Drain one pkt-line, refilling from in_fd via FILEDrain on NODATA.
//  Returns OK / PKTFLUSH / PKTDELIM / WIRECLIFAIL.
//
//  When IDLE runs out we compact: bytes already consumed via `adv` head
//  are reclaimed into IDLE so further reads have room.  Without this
//  the fixed-size WCLI_BUF (64 KiB) overruns on large advertisements —
//  vanilla git's `~/src/git` advertises ~1000 refs (≈100 KiB), enough
//  to fail mid-parse; the parent then closes pipes and the upstream
//  ssh git-upload-pack dies with SIGPIPE.
static ok64 wcli_read_pkt(int in_fd, u8b buf, u8cs adv, u8csp line) {
    for (;;) {
        ok64 o = PKTu8sDrain(adv, line);
        if (o != NODATA) return o;
        if (!u8bHasRoom(buf)) {
            size_t consumed = (size_t)(adv[0] - u8bDataC(buf)[0]);
            if (consumed == 0) return WIRECLIFAIL;
            u8bUsed(buf, consumed);
            u8bShift(buf, 0);
            adv[0] = u8bDataC(buf)[0];
            adv[1] = u8csTerm(u8bDataC(buf));
            if (!u8bHasRoom(buf)) return WIRECLIFAIL;
        }
        u8s fill;
        u8sFork(u8bIdle(buf), fill);
        ok64 fr = FILEDrain(in_fd, fill);
        if (fr == FILEEND) return WIRECLIFAIL;
        if (fr != OK) return WIRECLIFAIL;
        u8sJoin(u8bIdle(buf), fill);
        adv[1] = u8csTerm(u8bDataC(buf));
    }
}

// --- transport spawn (ssh / local) -------------------------------------
//
//  Parse `remote_uri` and decide what to exec:
//    file:///P or keeper://local/P    → exec `keeper <verb> P` locally.
//    keeper://host/P or be://host/P   → exec `ssh host keeper <verb> P`
//                                       (keeper-protocol over ssh).
//    //host/P or //host/P.git         → exec `ssh host git-<verb> P` so
//                                       a vanilla git server still works.
//                                       Bare-ssh defaults to git protocol
//                                       since most peers will be plain git
//                                       (mill-tags.sh against ~/src/git);
//                                       use keeper:// to force keeper.
//
//  `verb` is "upload-pack" (fetch) or "receive-pack" (push).  Sets
//  *out_pid + parent's stdin_w / stdout_r ends on success.

static u8c const WCLI_KEEPER_BIN_S[] = "keeper";
static u8c const WCLI_SSH_BIN_S[]    = "/usr/bin/ssh";
static u8c const WCLI_GIT_DOT_S[]    = ".git";

//  Locate the keeper binary to exec for local transport.  Honors the
//  KEEPER_BIN env var so tests can point at the just-built binary
//  without it being on $PATH.  Writes the chosen path slice into
//  `out_path` (alias of either env or the default literal).
static void wcli_keeper_bin(u8cs out_path) {
    char const *env = getenv("KEEPER_BIN");
    if (env && *env) {
        out_path[0] = (u8cp)env;
        out_path[1] = (u8cp)env + strlen(env);
        return;
    }
    out_path[0] = WCLI_KEEPER_BIN_S;
    out_path[1] = WCLI_KEEPER_BIN_S + sizeof(WCLI_KEEPER_BIN_S) - 1;
}

//  Path "P.git" (suffix) → choose vanilla git binary instead of keeper.
static b8 wcli_path_is_git(u8csc path) {
    if ((size_t)u8csLen(path) < sizeof(WCLI_GIT_DOT_S) - 1) return NO;
    u8c const *tail = path[1] - (sizeof(WCLI_GIT_DOT_S) - 1);
    return memcmp(tail, WCLI_GIT_DOT_S, sizeof(WCLI_GIT_DOT_S) - 1) == 0;
}

static ok64 wcli_spawn(u8csc remote_uri, char const *verb,
                       int *wfd, int *rfd, pid_t *pid) {
    sane(verb && wfd && rfd && pid);

    uri u = {};
    a_dup(u8c, ru, remote_uri);
    if (URIutf8Drain(ru, &u) != OK) return WIRECLIFAIL;

    //  Path is what the peer's upload-pack sees as argv[1].  URI parser
    //  delivers it with a leading '/' for absolute forms (file:///foo,
    //  //host/foo) which is exactly what the peer expects.
    u8cs path = {u.path[0], u.path[1]};
    if (u8csEmpty(path)) return WIRECLIFAIL;

    a_cstr(file_s,    "file");
    a_cstr(keeper_s,  "keeper");
    a_cstr(be_s,      "be");
    b8 is_file   = wcli_eq_lit(u.scheme, file_s[0],   (size_t)$len(file_s));
    b8 is_keeper = wcli_eq_lit(u.scheme, keeper_s[0], (size_t)$len(keeper_s));
    b8 is_be     = wcli_eq_lit(u.scheme, be_s[0],     (size_t)$len(be_s));
    b8 has_host  = !u8csEmpty(u.host);

    //  Build a verb slice to pass into argv.
    u8csc verb_s = {(u8cp)verb, (u8cp)verb + strlen(verb)};

    //  Local exec branch: file://, keeper://local, or no host at all.
    if (is_file || (is_keeper && (!has_host ||
                                  wcli_eq_lit(u.host, (u8c *)"local", 5))) ||
        (!has_host && u8csEmpty(u.scheme))) {
        u8cs kbin = {};
        wcli_keeper_bin(kbin);
        u8cs argv_arr[3] = {
            {(u8cp)"keeper", (u8cp)"keeper" + 6},
            {verb_s[0], verb_s[1]},
            {path[0], path[1]},
        };
        u8css argv = {argv_arr, argv_arr + 3};
        u8csc kbin_cs = {kbin[0], kbin[1]};
        return FILESpawn(kbin_cs, argv, wfd, rfd, pid);
    }

    //  ssh remote.  Default to vanilla `git-<verb>` so plain git peers
    //  (the common case — mill-tags.sh against ~/src/git, GitHub, etc.)
    //  work transparently.  Use `keeper://host/path` (or `be://`) to
    //  force the keeper protocol when both ends speak it.  `.git` suffix
    //  is honored as an extra git marker.
    a_cstr(ssh_path_s, "/usr/bin/ssh");
    u8cs host = {u.host[0], u.host[1]};
    if (u8csEmpty(host)) return WIRECLIFAIL;

    //  HOME-relative convention: //host/path delivers `path` with the
    //  URI parser's leading '/' attached.  ssh peers expect a path
    //  relative to the remote login's HOME, so strip it.  Absolute
    //  remote paths need to come through file:/// or be encoded
    //  differently — matching what KEEPSync/keeper_get_remote did pre-Phase8.
    if (!u8csEmpty(path) && *path[0] == '/') path[0]++;
    if (u8csEmpty(path)) return WIRECLIFAIL;

    b8 use_keeper_ssh = is_keeper || is_be;
    b8 force_git      = wcli_path_is_git(path);

    if (use_keeper_ssh && !force_git) {
        //  ssh <host> [PATH=...] keeper <verb> <path>
        //
        //  $DOG_REMOTE_PATH is prepended to the remote shell's PATH so
        //  test harnesses can point at an out-of-tree `keeper` binary
        //  without touching the remote's login rc.  When set, we have
        //  to invoke the remote command via `sh -c` so the assignment
        //  takes effect in the same process that exec()s keeper.
        char const *rpath = getenv("DOG_REMOTE_PATH");
        if (rpath && *rpath) {
            a_pad(u8, rcmd, 1024);
            a_cstr(pre1, "PATH='");
            u8bFeed(rcmd, pre1);
            a_cstr(rp_s, rpath);
            u8bFeed(rcmd, rp_s);
            a_cstr(pre2, "':\"$PATH\" exec keeper ");
            u8bFeed(rcmd, pre2);
            u8bFeed(rcmd, verb_s);
            u8bFeed1(rcmd, ' ');
            u8bFeed(rcmd, path);
            u8cs argv_arr[3] = {
                {(u8cp)"ssh", (u8cp)"ssh" + 3},
                {host[0], host[1]},
                {u8bDataHead(rcmd), u8bIdleHead(rcmd)},
            };
            u8css argv = {argv_arr, argv_arr + 3};
            return FILESpawn(ssh_path_s, argv, wfd, rfd, pid);
        }
        u8cs argv_arr[5] = {
            {(u8cp)"ssh", (u8cp)"ssh" + 3},
            {host[0], host[1]},
            {(u8cp)"keeper", (u8cp)"keeper" + 6},
            {verb_s[0], verb_s[1]},
            {path[0], path[1]},
        };
        u8css argv = {argv_arr, argv_arr + 5};
        return FILESpawn(ssh_path_s, argv, wfd, rfd, pid);
    }
    //  ssh <host> git-<verb> <path>
    a_pad(u8, gitverb, 32);
    a_cstr(git_dash, "git-");
    u8bFeed(gitverb, git_dash);
    u8bFeed(gitverb, verb_s);
    u8cs argv_arr[4] = {
        {(u8cp)"ssh", (u8cp)"ssh" + 3},
        {host[0], host[1]},
        {u8bDataHead(gitverb), u8bIdleHead(gitverb)},
        {path[0], path[1]},
    };
    u8css argv = {argv_arr, argv_arr + 4};
    return FILESpawn(ssh_path_s, argv, wfd, rfd, pid);
}

// --- ref-name normalisation --------------------------------------------

//  Build the local REFS key for a `want_ref` of one of these shapes:
//      "heads/<X>"   -> "?heads/<X>"
//      "tags/<X>"    -> "?tags/<X>"
//      "refs/heads/<X>" or "refs/tags/<X>" -> "?heads/<X>" / "?tags/<X>"
//  Returns OK on success.  Caller passes a pre-reset u8b.
static ok64 wcli_refkey(u8b out, u8csc want_ref) {
    sane(u8bOK(out));
    a_cstr(refs_pfx, "refs/");
    u8cs r = {want_ref[0], want_ref[1]};
    if (wcli_starts_with(r, refs_pfx[0], (size_t)$len(refs_pfx)))
        r[0] += (size_t)$len(refs_pfx);
    if (u8csEmpty(r)) return WIRECLIFAIL;
    u8bFeed1(out, '?');
    u8bFeed(out, r);
    done;
}

//  Build the wire refname (refs/heads/X or refs/tags/X) from a short
//  ref like "heads/X" / "tags/X" / "refs/heads/X".  Pre-reset u8b.
static ok64 wcli_wirerefname(u8b out, u8csc short_ref) {
    sane(u8bOK(out));
    a_cstr(refs_pfx, "refs/");
    u8cs r = {short_ref[0], short_ref[1]};
    if (wcli_starts_with(r, refs_pfx[0], (size_t)$len(refs_pfx))) {
        u8bFeed(out, r);
        done;
    }
    u8bFeed(out, refs_pfx);
    u8bFeed(out, r);
    done;
}

//  Match a candidate advertised refname against `want_ref`.  Treats
//  "heads/main" as matching "refs/heads/main", "main" as matching
//  "refs/heads/main", and an exact "refs/..." string as itself.
static b8 wcli_refname_match(u8csc adv_name, u8csc want_ref) {
    if (u8csEmpty(want_ref)) return NO;
    if (u8csLen(adv_name) == u8csLen(want_ref) &&
        memcmp(adv_name[0], want_ref[0], (size_t)u8csLen(want_ref)) == 0)
        return YES;
    a_cstr(refs_pfx, "refs/");
    if (wcli_starts_with(want_ref, refs_pfx[0], (size_t)$len(refs_pfx)))
        return NO;
    a_pad(u8, full, 256);
    a_cstr(rp, "refs/");
    u8bFeed(full, rp);
    u8bFeed(full, want_ref);
    if (u8csLen(adv_name) == (ssize_t)u8bDataLen(full) &&
        memcmp(adv_name[0], u8bDataHead(full),
               u8bDataLen(full)) == 0)
        return YES;
    //  Bare "main" → "refs/heads/main".
    a_pad(u8, full2, 256);
    a_cstr(hp, "refs/heads/");
    u8bFeed(full2, hp);
    u8bFeed(full2, want_ref);
    if (u8csLen(adv_name) == (ssize_t)u8bDataLen(full2) &&
        memcmp(adv_name[0], u8bDataHead(full2),
               u8bDataLen(full2)) == 0)
        return YES;
    return NO;
}

// --- WIREFetch ---------------------------------------------------------

//  Drain a peer's refs advertisement, looking for the entry that
//  matches `want_ref`.  Sets *out_sha on success and copies the matched
//  refname (without the leading "refs/" prefix, e.g. "heads/master") into
//  `name_out`.  If `want_ref` is empty, the entry that follows the
//  symref `HEAD` (matching by sha) wins — matching git's `clone`
//  default-branch discovery; falls back to the first non-HEAD entry.
static ok64 wcli_match_advert(int rfd, u8b buf, u8csc want_ref,
                              sha1 *out_sha, u8b name_out) {
    sane(rfd >= 0 && out_sha && u8bOK(name_out));
    u8cs adv = {u8bDataHead(buf), u8bDataHead(buf)};
    b8   picked = NO;
    sha1 head_sha = {};
    b8   have_head = NO;
    sha1 first_sha = {};
    u8cs first_name = {NULL, NULL};
    b8   first_seen = NO;
    a_cstr(head_lit, "HEAD");
    a_cstr(refs_pfx, "refs/");

    //  Helper: strip "refs/" and copy into name_out.
    #define WCLI_RECORD_NAME(name) do {                                      \
        u8bReset(name_out);                                                  \
        u8cs _n = {(name)[0], (name)[1]};                                    \
        if (wcli_starts_with(_n, refs_pfx[0], (size_t)$len(refs_pfx)))       \
            _n[0] += (size_t)$len(refs_pfx);                                 \
        u8bFeed(name_out, _n);                                               \
    } while (0)

    for (;;) {
        u8cs line = {};
        ok64 d = wcli_read_pkt(rfd, buf, adv, line);
        if (d == PKTFLUSH) break;
        if (d == PKTDELIM) continue;
        if (d != OK) return WIRECLIFAIL;

        //  Trim trailing '\n'.
        if (u8csLen(line) > 0 && line[1][-1] == '\n') line[1]--;
        if (u8csLen(line) < 41) continue;          // not "<sha> <name>"

        u8csc hex = {line[0], line[0] + 40};
        sha1 sha = {};
        if (!wcli_decode_sha(&sha, hex)) continue;
        if (line[0][40] != ' ') continue;

        u8cs name = {line[0] + 41, line[1]};
        //  Strip everything from the first NUL (capability list).
        u8c *nul = name[0];
        while (nul < name[1] && *nul != 0) nul++;
        name[1] = nul;
        //  Strip "^{}" peeled-tag suffix (we want the tag's own sha here).
        if (u8csLen(name) >= 3 &&
            name[1][-1] == '}' && name[1][-2] == '{' && name[1][-3] == '^')
            continue;

        //  Track HEAD separately — git advertises "HEAD" + capability
        //  list as the very first entry, and the matching branch ref
        //  follows with the same sha.
        if (wcli_eq_lit(name, head_lit[0], (size_t)$len(head_lit))) {
            head_sha = sha;
            have_head = YES;
            continue;
        }

        if (!first_seen) {
            first_sha = sha;
            first_name[0] = name[0];
            first_name[1] = name[1];
            first_seen = YES;
        }
        if (u8csEmpty(want_ref)) {
            //  Match the branch that shares HEAD's sha.
            if (have_head && sha1eq(&sha, &head_sha)) {
                *out_sha = sha;
                WCLI_RECORD_NAME(name);
                picked = YES;
            }
        } else if (wcli_refname_match(name, want_ref)) {
            *out_sha = sha;
            WCLI_RECORD_NAME(name);
            picked = YES;
        }
    }
    if (!picked) {
        if (u8csEmpty(want_ref) && first_seen) {
            *out_sha = first_sha;
            u8cs fn = {first_name[0], first_name[1]};
            WCLI_RECORD_NAME(fn);
            done;
        }
        return WIRECLINOREF;
    }
    #undef WCLI_RECORD_NAME
    done;
}

//  Harvest local-tip have shas from the keeper's REFADV.  Caps at
//  WIRE_MAX_HAVES to bound the request size.
static u32 wcli_collect_haves(refadvcp adv, sha1 *out, u32 cap) {
    if (!adv) return 0;
    u32 n = 0;
    for (u32 i = 0; i < adv->count && n < cap; i++) {
        out[n++] = adv->ents[i].tip;
    }
    return n;
}

//  Send the upload-pack request: want <sha> caps + flush + haves +
//  flush + done.  No multi_ack — server replies with one NAK + pack.
static ok64 wcli_send_request(int wfd, sha1 const *want_sha,
                              sha1 const *haves, u32 nhaves) {
    sane(wfd >= 0 && want_sha);

    Bu8 frame = {};
    call(u8bAllocate, frame, (1u << 16));

    //  want line.
    {
        a_pad(u8, line, 256);
        a_cstr(want_pfx, "want ");
        u8bFeed(line, want_pfx);
        u8 hex[40];
        wcli_sha_to_hex(hex, want_sha);
        u8csc hexs = {hex, hex + 40};
        u8bFeed(line, hexs);
        a_cstr(caps_s, " no-progress ofs-delta\n");
        u8bFeed(line, caps_s);
        a_dup(u8c, payload, u8bData(line));
        ok64 po = PKTu8sFeed(u8bIdle(frame), payload);
        if (po != OK) { u8bFree(frame); return po; }
    }
    ok64 fo = PKTu8sFeedFlush(u8bIdle(frame));
    if (fo != OK) { u8bFree(frame); return fo; }

    //  have lines.
    for (u32 i = 0; i < nhaves; i++) {
        a_pad(u8, line, 64);
        a_cstr(have_pfx, "have ");
        u8bFeed(line, have_pfx);
        u8 hex[40];
        wcli_sha_to_hex(hex, &haves[i]);
        u8csc hexs = {hex, hex + 40};
        u8bFeed(line, hexs);
        u8bFeed1(line, '\n');
        a_dup(u8c, payload, u8bData(line));
        ok64 po = PKTu8sFeed(u8bIdle(frame), payload);
        if (po != OK) { u8bFree(frame); return po; }
    }

    //  done.
    {
        a_cstr(done_s, "done\n");
        ok64 po = PKTu8sFeed(u8bIdle(frame), done_s);
        if (po != OK) { u8bFree(frame); return po; }
    }

    a_dup(u8c, fdata, u8bData(frame));
    ok64 wo = FILEFeedAll(wfd, fdata);
    u8bFree(frame);
    return wo;
}

//  Drain everything from rfd to a 1 GiB-mapped Bu8.  Returns OK on
//  EOF (clean) and *bytes_out is the captured slice.
static ok64 wcli_drain_response(int rfd, Bu8 buf) {
    sane(rfd >= 0);
    for (;;) {
        if (!u8bHasRoom(buf)) return WIRECLIFAIL;
        u8s fill;
        u8sFork(u8bIdle(buf), fill);
        ok64 fr = FILEDrain(rfd, fill);
        if (fr == FILEEND) return OK;
        if (fr != OK) return WIRECLIFAIL;
        u8sJoin(u8bIdle(buf), fill);
    }
}

//  Walk past any leading pkt-lines (NAK / ACK / progress) inside
//  `data`, writing the slice that starts at the raw pack bytes into
//  `out`.  A pkt-line whose payload starts with "PACK" is the pack's
//  first 4 bytes inlined into a side-band frame the way some servers
//  do it; treat that case the same as raw bytes.
static void wcli_strip_status(u8cs out, u8cs data) {
    out[0] = data[0];
    out[1] = data[1];
    for (;;) {
        u8cs line = {};
        u8cs probe = {out[0], out[1]};
        ok64 d = PKTu8sDrain(probe, line);
        if (d == PKTFLUSH) {
            out[0] = probe[0];
            return;
        }
        if (d != OK) return;
        if (u8csLen(line) >= 4 && memcmp(line[0], "PACK", 4) == 0) {
            out[0] = line[0];
            out[1] = line[1];
            return;
        }
        if (u8csLen(line) >= 3 &&
            (memcmp(line[0], "NAK", 3) == 0 ||
             memcmp(line[0], "ACK", 3) == 0)) {
            out[0] = probe[0];
            continue;
        }
        return;
    }
}

//  Append `?heads/X` → `?<hex>` (or tags) to local REFS.
static ok64 wcli_record_ref(keeper *k, u8csc want_ref, sha1 const *new_sha) {
    sane(k);
    a_path(keepdir, u8bDataC(k->h->root), KEEP_DIR_S);

    a_pad(u8, kbuf, 256);
    call(wcli_refkey, kbuf, want_ref);
    a_dup(u8c, key, u8bData(kbuf));

    a_pad(u8, vbuf, 64);
    u8bFeed1(vbuf, '?');
    u8 hex[40];
    wcli_sha_to_hex(hex, new_sha);
    u8csc hexs = {hex, hex + 40};
    u8bFeed(vbuf, hexs);
    a_dup(u8c, val, u8bData(vbuf));

    return REFSAppend($path(keepdir), key, val);
}

ok64 WIREFetch(keeper *k, u8csc remote_uri, u8csc want_ref) {
    sane(k);
    if (u8csEmpty(remote_uri)) return WIRECLIFAIL;

    //  Empty want_ref → let wcli_match_advert pick the peer's HEAD or
    //  first advertised ref (mirrors `git clone`'s default-branch
    //  discovery).  Callers that want an explicit fallback should pass
    //  e.g. "heads/main" themselves.
    u8cs effective_ref = {want_ref[0], want_ref[1]};

    int wfd = -1, rfd = -1;
    pid_t pid = 0;
    ok64 so = wcli_spawn(remote_uri, "upload-pack", &wfd, &rfd, &pid);
    if (so != OK) return WIRECLIFAIL;

    Bu8 advbuf = {};
    ok64 rv = WIRECLIFAIL;
    if (u8bAllocate(advbuf, WCLI_BUF) != OK) goto fetch_close;

    //  1.  Drain refs advertisement; pick the want sha + capture the
    //      matched ref name (used for the local REFS write below).
    sha1 want_sha = {};
    a_pad(u8, matched_ref_buf, 256);
    ok64 mo = wcli_match_advert(rfd, advbuf, effective_ref, &want_sha,
                                matched_ref_buf);
    if (mo != OK) { rv = mo; goto fetch_close; }
    u8cs matched_ref = {u8bDataHead(matched_ref_buf),
                        u8bIdleHead(matched_ref_buf)};
    if (u8csEmpty(matched_ref)) {
        matched_ref[0] = effective_ref[0];
        matched_ref[1] = effective_ref[1];
    }

    //  2.  Harvest haves from local REFADV.
    sha1 haves[WIRE_MAX_HAVES] = {};
    u32  nhaves = 0;
    {
        refadv adv = {};
        ok64 ao = REFADVOpen(&adv, k);
        if (ao == OK) {
            nhaves = wcli_collect_haves(&adv, haves, WIRE_MAX_HAVES);
            REFADVClose(&adv);
        }
    }

    //  3.  Send want + haves + done.
    if (wcli_send_request(wfd, &want_sha, haves, nhaves) != OK)
        goto fetch_close;
    close(wfd); wfd = -1;

    //  4.  Drain the response (NAK + pack).
    Bu8 respbuf = {};
    if (u8bMap(respbuf, 1ULL << 30) != OK) goto fetch_close;
    if (wcli_drain_response(rfd, respbuf) != OK) {
        u8bUnMap(respbuf);
        goto fetch_close;
    }
    close(rfd); rfd = -1;

    //  5.  Strip leading status lines, then ingest.
    u8cs all = {u8bDataHead(respbuf), u8bIdleHead(respbuf)};
    u8cs pack = {};
    wcli_strip_status(pack, all);
    if (u8csLen(pack) >= 12) {
        a_dup(u8c, packdup, pack);
        ok64 io = KEEPIngestFile(k, packdup);
        if (io != OK) {
            u8bUnMap(respbuf);
            goto fetch_close;
        }
    }
    u8bUnMap(respbuf);

    //  6.  Record the ref locally under the actually-matched name.
    if (wcli_record_ref(k, matched_ref, &want_sha) != OK)
        goto fetch_close;

    rv = OK;

fetch_close:
    if (advbuf[0]) u8bFree(advbuf);
    if (wfd >= 0) close(wfd);
    if (rfd >= 0) close(rfd);
    if (pid > 0) {
        int rc = 0;
        FILEReap(pid, &rc);
    }
    return rv;
}

// --- WIREPush ----------------------------------------------------------
//
//  MVP: build a packfile carrying the full reachable closure of our
//  local tip (commit + tree + blobs).  No DAG diff against the peer's
//  advertised tip yet — over-ship is the failure mode (the server
//  ingests the whole pack anyway and refs are FF-checked separately).

#define WPUSH_MAX_OBJS 65536

//  Recursively collect tree + blob SHAs reachable from `tree_sha` into
//  `out` (capacity `cap`).  Mirrors keep_walk_tree (KEEP.c) but lives
//  here so WIRECLI doesn't depend on KEEP.c's static helpers.
static ok64 wpush_walk_tree(keeper *k, sha1 const *tree_sha,
                            sha1 *out, u32 *n, u32 cap) {
    sane(k && tree_sha && out && n);
    if (*n >= cap) return WIRECLIFAIL;
    out[(*n)++] = *tree_sha;

    Bu8 tbuf = {};
    ok64 mo = u8bMap(tbuf, 1UL << 20);
    if (mo != OK) return mo;
    u8 ttype = 0;
    if (KEEPGetExact(k, tree_sha, tbuf, &ttype) != OK ||
        ttype != KEEP_OBJ_TREE) {
        u8bUnMap(tbuf);
        done;
    }
    u8cs walk = {u8bDataHead(tbuf), u8bIdleHead(tbuf)};
    u8cs file = {}, sha = {};
    while (GITu8sDrainTree(walk, file, sha) == OK) {
        if ($len(sha) != 20) continue;
        b8 is_tree = NO;
        b8 is_submodule = NO;
        if ($len(file) >= 5 && file[0][0] == '4' && file[0][1] == '0')
            is_tree = YES;
        if ($len(file) >= 6 && file[0][0] == '1' && file[0][1] == '6' &&
            file[0][2] == '0')
            is_submodule = YES;
        if (is_submodule) continue;
        sha1 entry_sha = {};
        memcpy(entry_sha.data, sha[0], 20);
        if (is_tree) {
            wpush_walk_tree(k, &entry_sha, out, n, cap);
        } else {
            if (*n >= cap) break;
            out[(*n)++] = entry_sha;
        }
    }
    u8bUnMap(tbuf);
    done;
}

//  Collect commit + tree + blob SHAs reachable from `commit_sha`.
//  Does not follow parents (server diffs against its existing tips).
static ok64 wpush_walk_commit(keeper *k, sha1 const *commit_sha,
                              sha1 *out, u32 *n, u32 cap) {
    sane(k && commit_sha && out && n);
    *n = 0;
    if (*n >= cap) return WIRECLIFAIL;
    out[(*n)++] = *commit_sha;

    Bu8 cbuf = {};
    ok64 mo = u8bMap(cbuf, 1UL << 20);
    if (mo != OK) return mo;
    u8 ctype = 0;
    if (KEEPGetExact(k, commit_sha, cbuf, &ctype) != OK ||
        ctype != KEEP_OBJ_COMMIT) {
        u8bUnMap(cbuf);
        return WIRECLIFAIL;
    }
    u8cs commit_body = {u8bDataHead(cbuf), u8bIdleHead(cbuf)};
    sha1 tree_sha = {};
    if (GITu8sCommitTree(commit_body, tree_sha.data) != OK) {
        u8bUnMap(cbuf);
        return WIRECLIFAIL;
    }
    u8bUnMap(cbuf);

    return wpush_walk_tree(k, &tree_sha, out, n, cap);
}

//  Append a pack object header (type + size varint, big-endian-ish) to
//  `buf`.  Mirrors keep_feed_obj_hdr in KEEP.c.
static void wpush_feed_obj_hdr(u8b buf, u8 type, u64 size) {
    u8 first = (u8)((type << 4) | (size & 0x0f));
    size >>= 4;
    if (size > 0) first |= 0x80;
    u8bFeed1(buf, first);
    while (size > 0) {
        u8 c = (u8)(size & 0x7f);
        size >>= 7;
        if (size > 0) c |= 0x80;
        u8bFeed1(buf, c);
    }
}

//  Build a v2 packfile containing the listed objects, in order, into
//  `pack_out` (caller pre-mapped).  Each object is fetched via
//  KEEPGetExact and zlib-deflated inline.  Adds the 12-byte PACK
//  header up front and the 20-byte SHA-1 trailer at the end.
static ok64 wpush_build_pack(keeper *k, sha1 const *shas, u32 nshas,
                             u8b pack_out) {
    sane(k && shas && u8bOK(pack_out));

    //  PACK header.
    u8 hdr[12] = {'P','A','C','K', 0,0,0,2, 0,0,0,0};
    hdr[8]  = (u8)((nshas >> 24) & 0xff);
    hdr[9]  = (u8)((nshas >> 16) & 0xff);
    hdr[10] = (u8)((nshas >>  8) & 0xff);
    hdr[11] = (u8) (nshas        & 0xff);
    u8csc hdr_s = {hdr, hdr + 12};
    u8bFeed(pack_out, hdr_s);

    for (u32 i = 0; i < nshas; i++) {
        Bu8 obuf = {};
        ok64 mo = u8bMap(obuf, 1UL << 24);
        if (mo != OK) return mo;
        u8 otype = 0;
        if (KEEPGetExact(k, &shas[i], obuf, &otype) != OK) {
            u8bUnMap(obuf);
            return WIRECLIFAIL;
        }
        u64 olen = u8bDataLen(obuf);

        a_pad(u8, ohdr, 16);
        wpush_feed_obj_hdr(ohdr, otype, olen);
        a_dup(u8c, oh, u8bData(ohdr));
        u8bFeed(pack_out, oh);

        a_dup(u8c, osrc, u8bData(obuf));
        ok64 zo = ZINFDeflate(u8bIdle(pack_out), osrc);
        u8bUnMap(obuf);
        if (zo != OK) return zo;
    }

    //  20-byte SHA-1 trailer over the whole pack so far.
    sha1 psha = {};
    a_dup(u8c, pack_data, u8bData(pack_out));
    SHA1Sum(&psha, pack_data);
    u8csc psha_s = {psha.data, psha.data + 20};
    u8bFeed(pack_out, psha_s);
    done;
}

//  Look up our local tip for `local_branch` via REFADV.  Sets *have=YES
//  if found, *out filled.  `local_branch` accepts the same shapes as
//  WIREFetch's `want_ref` argument.
static void wpush_local_tip(refadvcp adv, u8csc local_branch,
                            sha1 *out, b8 *have) {
    *have = NO;
    if (!adv) return;
    a_pad(u8, full, 256);
    if (wcli_wirerefname(full, local_branch) != OK) return;
    u8cs target = {u8bDataHead(full), u8bIdleHead(full)};
    for (u32 i = 0; i < adv->count; i++) {
        u8cs r = {adv->ents[i].refname[0], adv->ents[i].refname[1]};
        if (u8csLen(r) != u8csLen(target)) continue;
        if (memcmp(r[0], target[0], (size_t)u8csLen(target)) != 0) continue;
        *out  = adv->ents[i].tip;
        *have = YES;
        return;
    }
}

//  Drain peer's refs advertisement; if `branch_refname` matches an
//  advertised entry, capture its sha and set *have=YES.  Otherwise
//  (creation case) leave *have=NO with sha untouched.
static ok64 wpush_peer_tip(int rfd, u8b advbuf, u8csc branch_refname,
                           sha1 *out_sha, b8 *out_have) {
    sane(rfd >= 0 && out_sha && out_have);
    *out_have = NO;
    u8cs adv = {u8bDataHead(advbuf), u8bDataHead(advbuf)};
    for (;;) {
        u8cs line = {};
        ok64 d = wcli_read_pkt(rfd, advbuf, adv, line);
        if (d == PKTFLUSH) break;
        if (d == PKTDELIM) continue;
        if (d != OK) return WIRECLIFAIL;
        if (u8csLen(line) > 0 && line[1][-1] == '\n') line[1]--;
        if (u8csLen(line) < 41) continue;
        u8csc hex = {line[0], line[0] + 40};
        sha1 sha = {};
        if (!wcli_decode_sha(&sha, hex)) continue;
        if (line[0][40] != ' ') continue;
        u8cs name = {line[0] + 41, line[1]};
        u8c *nul = name[0];
        while (nul < name[1] && *nul != 0) nul++;
        name[1] = nul;
        if (u8csLen(name) == u8csLen(branch_refname) &&
            memcmp(name[0], branch_refname[0],
                   (size_t)u8csLen(branch_refname)) == 0) {
            *out_sha  = sha;
            *out_have = YES;
        }
    }
    done;
}

//  Send "<old> <new> <refname>\0report-status\n" + flush.
static ok64 wpush_send_update(int wfd, sha1 const *old_sha,
                              sha1 const *new_sha, u8csc refname,
                              b8 have_old) {
    sane(wfd >= 0 && new_sha);
    Bu8 frame = {};
    call(u8bAllocate, frame, 1024);

    a_pad(u8, line, 512);
    u8 oh[40], nh[40];
    if (have_old) {
        wcli_sha_to_hex(oh, old_sha);
    } else {
        memset(oh, '0', 40);
    }
    wcli_sha_to_hex(nh, new_sha);
    u8csc oh_s = {oh, oh + 40};
    u8csc nh_s = {nh, nh + 40};
    u8bFeed(line, oh_s);
    u8bFeed1(line, ' ');
    u8bFeed(line, nh_s);
    u8bFeed1(line, ' ');
    u8bFeed(line, refname);
    u8bFeed1(line, 0);
    a_cstr(caps, "report-status");
    u8bFeed(line, caps);
    u8bFeed1(line, '\n');
    a_dup(u8c, payload, u8bData(line));
    ok64 po = PKTu8sFeed(u8bIdle(frame), payload);
    if (po != OK) { u8bFree(frame); return po; }
    ok64 fo = PKTu8sFeedFlush(u8bIdle(frame));
    if (fo != OK) { u8bFree(frame); return fo; }

    a_dup(u8c, fdata, u8bData(frame));
    ok64 wo = FILEFeedAll(wfd, fdata);
    u8bFree(frame);
    return wo;
}

//  Drain push response, scanning for "unpack ok" + "ok <refname>".
static ok64 wpush_drain_status(int rfd, u8csc refname) {
    sane(rfd >= 0);
    Bu8 buf = {};
    call(u8bAllocate, buf, WCLI_BUF);
    u8cs adv = {u8bDataHead(buf), u8bDataHead(buf)};
    b8 unpack_ok = NO;
    b8 ref_ok    = NO;

    a_pad(u8, ok_line, 512);
    a_cstr(ok_pfx, "ok ");
    u8bFeed(ok_line, ok_pfx);
    u8bFeed(ok_line, refname);
    a_dup(u8c, ok_match, u8bData(ok_line));

    a_pad(u8, ng_line, 512);
    a_cstr(ng_pfx, "ng ");
    u8bFeed(ng_line, ng_pfx);
    u8bFeed(ng_line, refname);
    a_dup(u8c, ng_match, u8bData(ng_line));

    for (;;) {
        u8cs line = {};
        ok64 d = wcli_read_pkt(rfd, buf, adv, line);
        if (d == PKTFLUSH) break;
        if (d == PKTDELIM) continue;
        if (d != OK) break;
        if (u8csLen(line) >= 9 && memcmp(line[0], "unpack ok", 9) == 0)
            unpack_ok = YES;
        if (u8csLen(line) >= (ssize_t)u8csLen(ok_match) &&
            memcmp(line[0], ok_match[0],
                   (size_t)u8csLen(ok_match)) == 0)
            ref_ok = YES;
        if (u8csLen(line) >= (ssize_t)u8csLen(ng_match) &&
            memcmp(line[0], ng_match[0],
                   (size_t)u8csLen(ng_match)) == 0) {
            //  ng — keep draining but flag failure.
            ref_ok = NO;
        }
    }
    u8bFree(buf);
    return (unpack_ok && ref_ok) ? OK : WIRECLIFAIL;
}

ok64 WIREPush(keeper *k, u8csc remote_uri, u8csc local_branch) {
    sane(k);
    if (u8csEmpty(remote_uri) || u8csEmpty(local_branch))
        return WIRECLIFAIL;

    //  Resolve our local tip via REFADV (one-shot snapshot).
    sha1 local_tip = {};
    b8   have_local = NO;
    {
        refadv adv = {};
        ok64 ao = REFADVOpen(&adv, k);
        if (ao != OK) return ao;
        wpush_local_tip(&adv, local_branch, &local_tip, &have_local);
        REFADVClose(&adv);
    }
    if (!have_local) return WIRECLINOREF;

    //  Build the wire refname (refs/heads/X) once.
    a_pad(u8, refname_buf, 256);
    call(wcli_wirerefname, refname_buf, local_branch);
    u8cs refname = {u8bDataHead(refname_buf), u8bIdleHead(refname_buf)};

    //  Spawn receive-pack on the peer.
    int wfd = -1, rfd = -1;
    pid_t pid = 0;
    ok64 so = wcli_spawn(remote_uri, "receive-pack", &wfd, &rfd, &pid);
    if (so != OK) return WIRECLIFAIL;

    Bu8 advbuf = {};
    ok64 rv = WIRECLIFAIL;
    if (u8bAllocate(advbuf, WCLI_BUF) != OK) goto push_close;

    //  Drain peer advert; capture old tip if peer already has the ref.
    sha1 peer_tip = {};
    b8   have_peer = NO;
    if (wpush_peer_tip(rfd, advbuf, refname, &peer_tip, &have_peer) != OK)
        goto push_close;

    //  Short-circuit: peer already at our tip — nothing to push.
    if (have_peer && sha1eq(&peer_tip, &local_tip)) {
        rv = OK;
        //  Still need to send a flush so the peer closes cleanly.
        Bu8 flush_b = {};
        if (u8bAllocate(flush_b, 8) == OK) {
            PKTu8sFeedFlush(u8bIdle(flush_b));
            a_dup(u8c, fdata, u8bData(flush_b));
            FILEFeedAll(wfd, fdata);
            u8bFree(flush_b);
        }
        goto push_close;
    }

    //  Walk the local commit's reachable closure.
    sha1 *shas = calloc(WPUSH_MAX_OBJS, sizeof(sha1));
    if (!shas) goto push_close;
    u32 nshas = 0;
    if (wpush_walk_commit(k, &local_tip, shas, &nshas, WPUSH_MAX_OBJS) != OK ||
        nshas == 0) {
        free(shas);
        goto push_close;
    }

    //  Build the pack.
    Bu8 packbuf = {};
    if (u8bAllocate(packbuf, 1ULL << 26) != OK) {
        free(shas);
        goto push_close;
    }
    if (wpush_build_pack(k, shas, nshas, packbuf) != OK) {
        free(shas);
        u8bFree(packbuf);
        goto push_close;
    }
    free(shas);

    //  Send the ref-update line + flush.
    if (wpush_send_update(wfd, &peer_tip, &local_tip, refname,
                          have_peer) != OK) {
        u8bFree(packbuf);
        goto push_close;
    }
    //  Send the pack bytes.
    {
        a_dup(u8c, pdata, u8bData(packbuf));
        ok64 wo = FILEFeedAll(wfd, pdata);
        u8bFree(packbuf);
        if (wo != OK) goto push_close;
    }
    close(wfd); wfd = -1;

    //  Drain status.
    rv = wpush_drain_status(rfd, refname);
    close(rfd); rfd = -1;

push_close:
    if (advbuf[0]) u8bFree(advbuf);
    if (wfd >= 0) close(wfd);
    if (rfd >= 0) close(rfd);
    if (pid > 0) {
        int rc = 0;
        FILEReap(pid, &rc);
    }
    return rv;
}
