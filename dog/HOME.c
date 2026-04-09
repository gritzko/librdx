#include "HOME.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"

ok64 HOMEFollowWorktree(path8b out, path8cg gitfile) {
    sane(out != NULL && PATHu8cgOK(gitfile));
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
            // Relative to dir containing the .git file.
            a_dupcg(u8, gf, gitfile);
            o = PATHu8bFeed(gitdir, gf);
            if (o == OK) o = PATHu8bPop(gitdir);
            if (o == OK) o = PATHu8bPush(gitdir, gds);
        }
        if (o != OK) goto out;
    }
    // Verify the gitdir actually exists; this is the broken-worktree guard.
    o = FILEisdir(PATHu8cgIn(gitdir));
    if (o != OK) goto out;
    // Expect layout: <root>/.git/worktrees/<name>
    // Pop <name>, then "worktrees", then verify ".git" is a directory.
    o = PATHu8bPop(gitdir);
    if (o == OK) o = PATHu8bPop(gitdir);
    if (o == OK) o = FILEisdir(PATHu8cgIn(gitdir));
    if (o != OK) goto out;
    // One more pop strips the trailing ".git" to land at the parent root.
    o = PATHu8bPop(gitdir);
    if (o != OK) goto out;
    {
        a_dup(u8c, gd, u8bDataC(gitdir));
        o = PATHu8bFeed(out, gd);
    }
out:
    FILEUnMap(mapped);
    return o;
}

ok64 HOMEFind(path8b out) {
    sane(out != NULL);
    char cwdbuf[FILE_PATH_MAX_LEN];
    test(getcwd(cwdbuf, sizeof(cwdbuf)) != NULL, FAILSANITY);
    u8cs cwds = {(u8cp)cwdbuf, (u8cp)cwdbuf + strlen(cwdbuf)};
    call(PATHu8bFeed, out, cwds);

    a_cstr(dotgit, ".git");
    for (;;) {
        a_path(probe);
        a_dup(u8c, cur, u8bDataC(out));
        call(PATHu8bFeed, probe, cur);
        call(PATHu8bPush, probe, dotgit);
        struct stat sb = {};
        if (stat((char const *)*PATHu8cgIn(probe), &sb) == 0) {
            if (!(sb.st_mode & S_IFDIR)) {
                // .git is a file — worktree marker. Try to follow it.
                a_path(parent);
                ok64 fo = HOMEFollowWorktree(parent, PATHu8cgIn(probe));
                if (fo == OK) {
                    // Replace `out` with the parent repo root (absolute).
                    u8bReset(out);
                    a_dup(u8c, ps, u8bDataC(parent));
                    call(PATHu8bFeed, out, ps);
                }
                // On failure, leave `out` as the worktree dir.
            }
            done;
        }

        size_t before = $len(u8bDataC(out));
        call(PATHu8bPop, out);
        size_t after = $len(u8bDataC(out));
        // Pop stops at "/" — bail when no progress is made.
        test(after < before, FAILSANITY);
    }
}

// Try dir/name — if it exists and is executable, write to out and return YES.
static b8 home_try_sibling(char *out, size_t outsz,
                           char const *dir, size_t dirlen,
                           char const *name) {
    int n = snprintf(out, outsz, "%.*s/%s", (int)dirlen, dir, name);
    if (n <= 0 || (size_t)n >= outsz) return NO;
    struct stat sb;
    if (stat(out, &sb) == 0 && (sb.st_mode & S_IXUSR)) return YES;
    return NO;
}

ok64 HOMEResolveSibling(char *out, size_t outsz,
                        char const *name, char const *argv0) {
    sane(out != NULL && outsz > 0 && name != NULL);

    // 1. If argv0 contains '/', dirname(argv0) preserves symlinks.
    if (argv0 != NULL) {
        char const *sl = strrchr(argv0, '/');
        if (sl != NULL) {
            size_t dirlen = (size_t)(sl - argv0);
            if (dirlen == 0) dirlen = 1;  // root "/"
            if (home_try_sibling(out, outsz, argv0, dirlen, name))
                done;
        } else {
            // 2. Bare name — search PATH for argv0, then try sibling.
            char const *path = getenv("PATH");
            if (path != NULL) {
                char const *p = path;
                while (*p) {
                    char const *colon = p;
                    while (*colon && *colon != ':') colon++;
                    size_t plen = (size_t)(colon - p);
                    if (plen > 0) {
                        char probe[FILE_PATH_MAX_LEN];
                        int n = snprintf(probe, sizeof(probe),
                                         "%.*s/%s", (int)plen, p, argv0);
                        if (n > 0 && (size_t)n < sizeof(probe)) {
                            struct stat sb;
                            if (stat(probe, &sb) == 0 &&
                                (sb.st_mode & S_IXUSR)) {
                                // Found argv0 in this PATH dir — look
                                // for the sibling here (same dir).
                                if (home_try_sibling(out, outsz,
                                                     p, plen, name))
                                    done;
                                break;  // right dir, sibling absent
                            }
                        }
                    }
                    p = (*colon) ? colon + 1 : colon;
                }
            }
        }
    }

    // 3. Fallback: bare name for execvp PATH lookup.
    snprintf(out, outsz, "%s", name);
    done;
}
