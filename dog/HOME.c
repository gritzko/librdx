#include "HOME.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"
#include "TOMLT.h"

// --- HOMEOpen / HOMEClose ---

// Capture stdout of `git config --global --get <key>` into out.
// Returns NODATA if git exits non-zero (key unset) or the subprocess
// cannot be spawned.  Trailing '\n' is trimmed.
static ok64 home_git_config_get(char const *key, u8s out) {
    sane($ok(out));
    a_cstr(gitp, "/usr/bin/git");
    u8cs av[] = {
        u8slit("git"),
        u8slit("config"),
        u8slit("--global"),
        u8slit("--get"),
        u8scstr(key),
    };
    u8css argv = {av, av + 5};

    pid_t pid = 0;
    int rfd = -1;
    if (FILESpawn(gitp, argv, NULL, &rfd, &pid) != OK) return NODATA;

    a_pad(u8, buf, 256);
    FILEEnsureSoft(rfd, buf, u8bIdleLen(buf));
    FILEClose(&rfd);

    int rc = -1;
    FILEReap(pid, &rc);
    if (rc != 0) return NODATA;

    u8cs raw = {u8bDataHead(buf), u8bIdleHead(buf)};
    while (!$empty(raw) && (raw[1][-1] == '\n' || raw[1][-1] == '\r'))
        raw[1]--;
    if ($empty(raw)) return NODATA;
    return u8sFeed(out, raw);
}

// Fresh clones: seed .dogs/config from git's global config so commits
// and ref authorities get a sensible default identity without manual
// setup.  Called from HOMEOpen when rw=YES and config is absent.
// Best-effort — silent on any failure.
static void home_bootstrap_config(home *h) {
    if (!h->rw) return;

    a_pad(u8, emailbuf, 256);
    a_pad(u8, namebuf,  256);
    u8s email = {emailbuf[0], emailbuf[3]};
    u8s name  = {namebuf[0],  namebuf[3]};
    u8cp email_start = email[0];
    u8cp name_start  = name[0];
    b8 got_email = (home_git_config_get("user.email", email) == OK);
    b8 got_name  = (home_git_config_get("user.name",  name)  == OK);
    if (!got_email && !got_name) return;

    a_path(dotdogs);
    a_dup(u8c, root_s, u8bDataC(h->root));
    if (PATHu8bFeed(dotdogs, root_s) != OK) return;
    a_cstr(dd, ".dogs");
    if (PATHu8bPush(dotdogs, dd) != OK) return;
    if (FILEMakeDirP($path(dotdogs)) != OK) return;

    a_path(cfgp);
    a_dup(u8c, dd_s, u8bDataC(dotdogs));
    if (PATHu8bFeed(cfgp, dd_s) != OK) return;
    a_cstr(cfg_name, "config");
    if (PATHu8bPush(cfgp, cfg_name) != OK) return;

    a_pad(u8, body, 1024);
    a_cstr(hdr, "[user]\n");
    u8bFeed(body, hdr);
    if (got_name) {
        a_cstr(key, "name = \"");
        a_cstr(eol, "\"\n");
        u8cs v = {name_start, name[0]};
        u8bFeed(body, key);
        u8bFeed(body, v);
        u8bFeed(body, eol);
    }
    if (got_email) {
        a_cstr(key, "email = \"");
        a_cstr(eol, "\"\n");
        u8cs v = {email_start, email[0]};
        u8bFeed(body, key);
        u8bFeed(body, v);
        u8bFeed(body, eol);
    }

    int fd = -1;
    if (FILECreate(&fd, $path(cfgp)) != OK) return;
    u8cs data = {u8bDataHead(body), u8bIdleHead(body)};
    FILEFeedAll(fd, data);
    FILEClose(&fd);
}

ok64 HOMEOpen(home *h, u8cs at, b8 rw) {
    sane(h != NULL);
    zerop(h);
    h->rw = rw;

    // 1. Path buffer for root, 1 KB.
    call(u8bAllocate, h->root, FILE_PATH_MAX_LEN);

    // 2. Resolve root: explicit → feed; implicit → HOMEFindDogs.
    if ($ok(at) && !u8csEmpty(at)) {
        call(PATHu8bFeed, h->root, at);
    } else {
        call(HOMEFindDogs, h);
    }

    // 3. Scratch arena — 4 GB VA, pages on demand.
    call(u8bMap, h->arena, HOME_ARENA_SIZE);

    // 4. Config mmap (best-effort).  Missing file is not an error.
    a_path(cfg);
    a_dup(u8c, root_s, u8bDataC(h->root));
    call(PATHu8bFeed, cfg, root_s);
    a_cstr(rel, ".dogs/config");
    ok64 po = PATHu8bAdd(cfg, rel);
    if (po == OK) {
        u8bp mapped = NULL;
        if (FILEMapRO(&mapped, $path(cfg)) != OK && rw) {
            // Fresh clone: seed from git's global config, then retry.
            home_bootstrap_config(h);
            FILEMapRO(&mapped, $path(cfg));
        }
        if (mapped != NULL) {
            for (int i = 0; i < 4; i++)
                ((u8 **)h->config)[i] = mapped[i];
        }
    }
    done;
}

ok64 HOMEClose(home *h) {
    sane(h != NULL);
    if (h->config[0] != NULL) FILEUnMap(h->config);
    if (h->arena[0]  != NULL) u8bUnMap(h->arena);
    if (h->root[0]   != NULL) u8bFree(h->root);
    zerop(h);
    done;
}

// --- Workspace finders ---

ok64 HOMEFollowWorktree(home *h, path8s gitfile) {
    sane(h != NULL && $ok(gitfile) && !$empty(gitfile));

    u8bp mapped = NULL;
    call(FILEMapRO, &mapped, gitfile);
    a_dup(u8c, content, u8bDataC(mapped));
    a_cstr(prefix, "gitdir: ");
    ok64 o = OK;
    a_path(gitdir);

    if ($len(content) <= $len(prefix)) { o = PATHBAD; goto out; }
    {
        u8cs pfx = {content[0], content[0] + $len(prefix)};
        if (!$eq(pfx, prefix)) { o = PATHBAD; goto out; }
    }
    {
        u8cp start = content[0] + $len(prefix);
        u8cp end = content[1];
        while (end > start && (*(end - 1) == '\n' || *(end - 1) == '\r'))
            end--;
        u8cs gds = {start, end};
        if ($empty(gds)) { o = PATHBAD; goto out; }
        if (gds[0][0] == '/') {
            o = PATHu8bFeed(gitdir, gds);
        } else {
            a_dup(u8, gf, gitfile);
            o = PATHu8bFeed(gitdir, gf);
            if (o == OK) o = PATHu8bPop(gitdir);
            if (o == OK) o = PATHu8bPush(gitdir, gds);
        }
        if (o != OK) goto out;
    }
    o = FILEisdir($path(gitdir));
    if (o != OK) goto out;
    o = PATHu8bPop(gitdir);
    if (o == OK) o = PATHu8bPop(gitdir);
    if (o == OK) o = FILEisdir($path(gitdir));
    if (o != OK) goto out;
    o = PATHu8bPop(gitdir);
    if (o != OK) goto out;
    {
        u8bReset(h->root);
        a_dup(u8c, gd, u8bDataC(gitdir));
        o = PATHu8bFeed(h->root, gd);
    }
out:
    FILEUnMap(mapped);
    return o;
}

// Walk up from cwd to the first dir containing .git OR .dogs.  Fills
// h->root with the found path.  *is_worktree=YES iff a .git *file*
// stopped the walk.
static ok64 home_walk_up(home *h, b8 *is_worktree) {
    sane(h != NULL);

    u8bReset(h->root);
    char cwdbuf[FILE_PATH_MAX_LEN];
    test(getcwd(cwdbuf, sizeof(cwdbuf)) != NULL, NOHOME);
    u8cs cwds = {(u8cp)cwdbuf, (u8cp)cwdbuf + strlen(cwdbuf)};
    call(PATHu8bFeed, h->root, cwds);
    if (is_worktree) *is_worktree = NO;

    a_cstr(dotgit,  ".git");
    a_cstr(dotdogs, ".dogs");
    for (;;) {
        struct stat sb = {};
        {
            a_path(probe);
            a_dup(u8c, cur, u8bDataC(h->root));
            call(PATHu8bFeed, probe, cur);
            call(PATHu8bPush, probe, dotgit);
            if (stat((char const *)*$path(probe), &sb) == 0) {
                if (is_worktree)
                    *is_worktree = (sb.st_mode & S_IFDIR) ? NO : YES;
                done;
            }
        }
        {
            a_path(probe);
            a_dup(u8c, cur, u8bDataC(h->root));
            call(PATHu8bFeed, probe, cur);
            call(PATHu8bPush, probe, dotdogs);
            if (stat((char const *)*$path(probe), &sb) == 0 &&
                (sb.st_mode & S_IFDIR)) {
                done;
            }
        }

        size_t before = $len(u8bDataC(h->root));
        call(PATHu8bPop, h->root);
        size_t after = $len(u8bDataC(h->root));
        if (after >= before) return NOHOME;
    }
}

ok64 HOMEFind(home *h) {
    sane(h != NULL);
    return home_walk_up(h, NULL);
}

ok64 HOMEFindDogs(home *h) {
    sane(h != NULL);
    b8 is_wt = NO;
    call(home_walk_up, h, &is_wt);

    if (!is_wt) done;

    a_cstr(dotgit, ".git");
    a_path(probe);
    a_dup(u8c, cur, u8bDataC(h->root));
    call(PATHu8bFeed, probe, cur);
    call(PATHu8bPush, probe, dotgit);

    ok64 fo = HOMEFollowWorktree(h, $path(probe));
    (void)fo;   // failure leaves h->root as the worktree dir (fallback)
    done;
}

// --- Resolve sibling binary ---

static b8 home_is_exe(path8s p) {
    struct stat sb;
    if (FILEStat(&sb, p) != OK) return NO;
    return (sb.st_mode & S_IXUSR) ? YES : NO;
}

// If <dir>/<name> is executable, feed its full path into `out`.
static ok64 home_try_sibling(path8b out, u8csc dir, u8csc name) {
    sane(out != NULL && $ok(dir) && $ok(name));
    a_path(tmp);
    a_dup(u8c, dir_s, dir);
    a_dup(u8c, name_s, name);
    call(PATHu8bFeed, tmp, dir_s);
    call(PATHu8bPush, tmp, name_s);
    if (!home_is_exe($path(tmp))) return NONE;
    a_dup(u8c, src, u8bDataC(tmp));
    call(PATHu8bFeed, out, src);
    done;
}

ok64 HOMEResolveSibling(home *h, path8b out, u8csc name, u8csc argv0) {
    sane(out != NULL && $ok(name));
    (void)h;   // unused — sibling lookup is ambient (argv0 + PATH)

    if ($ok(argv0) && !u8csEmpty(argv0)) {
        // "Has a directory" means argv0 literally contains '/'.
        // PATHu8sDir synthesizes "." for bare names; that's the PATH
        // case, not the dirname case.
        a_dup(u8c, a0_scan, argv0);
        b8 has_slash = u8csFind(a0_scan, '/') == OK;
        if (has_slash) {
            a_dup(u8c, a0, argv0);
            u8cs dir = {};
            PATHu8sDir(dir, a0);
            u8csc dir_c = {dir[0], dir[1]};
            if (home_try_sibling(out, dir_c, name) == OK) done;
        } else {
            // Bare argv0 → scan PATH for it, then look beside it.
            char const *env = getenv("PATH");
            if (env != NULL) {
                a_cstr(env_s, env);
                a_dup(u8c, scan, env_s);
                while (!u8csEmpty(scan)) {
                    u8cs entry = {scan[0], scan[1]};
                    a_dup(u8c, probe, scan);
                    if (u8csFind(probe, ':') == OK) {
                        //  u8csFind advances probe[0] *to* the ':'.
                        //  entry is half-open [scan_start, colon);
                        //  advance scan past the ':' for next iter.
                        entry[1] = probe[0];
                        scan[0]  = probe[0] + 1;
                    } else {
                        scan[0]  = scan[1];
                    }
                    if (u8csEmpty(entry)) continue;

                    a_path(exe);
                    u8csc entry_c = {entry[0], entry[1]};
                    a_dup(u8c, a0_s, argv0);
                    if (PATHu8bFeed(exe, entry)   != OK) continue;
                    if (PATHu8bPush(exe, a0_s)    != OK) continue;
                    if (!home_is_exe($path(exe))) continue;
                    if (home_try_sibling(out, entry_c, name) == OK) done;
                    break;
                }
            }
        }
    }

    // Fallback: feed just `name` — caller can still hand it to execvp.
    a_dup(u8c, name_s, name);
    call(PATHu8bFeed, out, name_s);
    done;
}

// --- Config: .dogs/config (TOML) ---

// The TOML header `[a.b.c]` and dotted keys `a.b = "v"` both express the
// same dotted hierarchy.  We track the full active path in `current`
// (a path8b, segments joined by '/') and match it against `needle`
// supplied by the caller as a path8s.  On a match the cb feeds `out`
// and returns NODATA so TOMLTLexer `fbreak`s immediately.
typedef struct {
    path8s  needle;    // caller-supplied, e.g. $path(a_path(..,"a","b","c"))
    path8bp current;   // active dotted path, borrowed from caller (a_path)
    size_t  hdr_end;   // current's DATA length at end of last header
    u8s     out;
    u8      in_hdr;    // 1 between '[' and ']'
    u8      await_val; // 1 after '=' — next string is the value
} home_cfg_ctx;

static ok64 home_cfg_cb(u8 tag, u8cs tok, void *ctx) {
    sane(ctx != NULL);
    home_cfg_ctx *c = (home_cfg_ctx *)ctx;
    if (tag == 'D') return OK;   // comment

    // Whitespace with a newline closes a kv line; rewind `current`
    // back to the header root so the next line starts fresh.
    if (tag == 'W') {
        b8 nl = NO;
        for (u8cp p = tok[0]; p < tok[1]; p++)
            if (*p == '\n') { nl = YES; break; }
        if (nl && !c->in_hdr) {
            size_t dl = u8bDataLen(c->current);
            if (dl > c->hdr_end) u8bShed(c->current, dl - c->hdr_end);
            c->await_val = 0;
        }
        return OK;
    }

    // Header framing
    if ($len(tok) == 1 && tok[0][0] == '[') {
        c->in_hdr = 1;
        u8bReset(c->current);
        return OK;
    }
    if ($len(tok) == 1 && tok[0][0] == ']') {
        c->in_hdr = 0;
        c->hdr_end = u8bDataLen(c->current);
        return OK;
    }

    // Dotted separator between segments (inside or outside a header).
    // TOMLT tags it 'P' outside headers and 'R' inside (TOKSplitText).
    if ($len(tok) == 1 && tok[0][0] == '.')
        return OK;

    // '='
    if (tag == 'P' && $len(tok) == 1 && tok[0][0] == '=') {
        c->await_val = 1;
        return OK;
    }

    // Identifier: extends the active path (in header or building a key).
    if ((tag == 'S' || tag == 'R') && !c->await_val) {
        PATHu8bPush(c->current, tok);
        return OK;
    }

    // Quoted string value — match → feed, then NODATA to stop the lexer.
    if (c->await_val && tag == 'G' && $len(tok) >= 2) {
        a_dup(u8c, cu, u8bDataC(c->current));
        if ($eq(c->needle, cu)) {
            u8cs val = {tok[0] + 1, tok[1] - 1};
            call(u8sFeed, c->out, val);
            return NODATA;
        }
        return OK;
    }

    return OK;
}

ok64 HOMEHost(home *h, u8s out) {
    sane(h != NULL && $ok(out));
    a_cstr(user_s, "user");
    a_cstr(host_s, "host");
    a_cstr(mail_s, "email");
    a_path(needle, user_s, host_s);
    ok64 o = HOMEGetConfig(h, out, $path(needle));
    if (o == OK) done;
    if (o != NOCONF) return o;
    a_path(email, user_s, mail_s);
    return HOMEGetConfig(h, out, $path(email));
}

ok64 HOMEGetConfig(home *h, u8s value, path8s needle) {
    sane(h != NULL && $ok(value) && $ok(needle));

    if (h->config[0] == NULL) return NOCONF;

    a_path(current);
    home_cfg_ctx ctx = {
        .needle  = {needle[0], needle[1]},
        .current = current,
        .out     = {value[0], value[1]},
    };
    TOMLTstate st = {
        .data = {u8bDataHead(h->config), u8bIdleHead(h->config)},
        .cb   = home_cfg_cb,
        .ctx  = &ctx,
    };
    ok64 lo = TOMLTLexer(&st);
    if (lo != OK && lo != NODATA) return lo;
    if (ctx.out[0] == value[0]) return NOCONF;   // nothing fed
    value[0] = ctx.out[0];
    done;
}
