#include "HOME.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"

ok64 HOMEFollowWorktree(path8b out, path8s gitfile) {
    sane(out != NULL && ($ok(gitfile) && !$empty(gitfile)));
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
        a_dup(u8c, gd, u8bDataC(gitdir));
        o = PATHu8bFeed(out, gd);
    }
out:
    FILEUnMap(mapped);
    return o;
}

// Walk up from cwd to the first dir containing .git OR .dogs.
// Either marks a repo root: .git is the classic git layout, .dogs/ is
// what keeper-cloned dirs have (no .git at all).
// Returns NOHOME if neither found before reaching /.
// Sets *is_worktree to YES iff a .git *file* (worktree marker) is what
// stopped the walk; .git-as-dir or .dogs-only both leave it NO.
static ok64 HOMEWalkUp(path8b out, b8 *is_worktree) {
    sane(out != NULL);
    char cwdbuf[FILE_PATH_MAX_LEN];
    test(getcwd(cwdbuf, sizeof(cwdbuf)) != NULL, NOHOME);
    u8cs cwds = {(u8cp)cwdbuf, (u8cp)cwdbuf + strlen(cwdbuf)};
    call(PATHu8bFeed, out, cwds);
    if (is_worktree) *is_worktree = NO;

    a_cstr(dotgit,  ".git");
    a_cstr(dotdogs, ".dogs");
    for (;;) {
        struct stat sb = {};
        // Prefer .git when present (preserves worktree-marker semantics
        // for callers that follow the gitdir pointer).
        {
            a_path(probe);
            a_dup(u8c, cur, u8bDataC(out));
            call(PATHu8bFeed, probe, cur);
            call(PATHu8bPush, probe, dotgit);
            if (stat((char const *)*$path(probe), &sb) == 0) {
                if (is_worktree)
                    *is_worktree = (sb.st_mode & S_IFDIR) ? NO : YES;
                done;
            }
        }
        // No .git here — accept .dogs/ as a fresh-clone repo root.
        {
            a_path(probe);
            a_dup(u8c, cur, u8bDataC(out));
            call(PATHu8bFeed, probe, cur);
            call(PATHu8bPush, probe, dotdogs);
            if (stat((char const *)*$path(probe), &sb) == 0 &&
                (sb.st_mode & S_IFDIR)) {
                done;
            }
        }

        size_t before = $len(u8bDataC(out));
        call(PATHu8bPop, out);
        size_t after = $len(u8bDataC(out));
        if (after >= before) return NOHOME;
    }
}

ok64 HOMEFind(path8b out) {
    sane(out != NULL);
    return HOMEWalkUp(out, NULL);
}

ok64 HOMEFindDogs(path8b out) {
    sane(out != NULL);
    b8 is_wt = NO;
    call(HOMEWalkUp, out, &is_wt);

    if (!is_wt) {
        // .git is a directory — use this dir for .dogs/
        done;
    }

    // .git is a file — worktree. Try following to the parent repo.
    a_cstr(dotgit, ".git");
    a_path(probe);
    a_dup(u8c, cur, u8bDataC(out));
    call(PATHu8bFeed, probe, cur);
    call(PATHu8bPush, probe, dotgit);

    a_path(parent);
    ok64 fo = HOMEFollowWorktree(parent, $path(probe));
    if (fo == OK) {
        // Parent repo found — use it for .dogs/
        u8bReset(out);
        a_dup(u8c, ps, u8bDataC(parent));
        call(PATHu8bFeed, out, ps);
    }
    // If follow failed, keep out as the worktree dir (fallback).
    done;
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
