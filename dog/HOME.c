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

ok64 HOMEResolveSibling(char *out, size_t outsz, char const *name) {
    sane(out != NULL && outsz > 0 && name != NULL);
    char self[FILE_PATH_MAX_LEN];
    ssize_t slen = readlink("/proc/self/exe", self, sizeof(self) - 1);
    if (slen > 0) {
        self[slen] = 0;
        char *sl = strrchr(self, '/');
        if (sl != NULL) {
            *sl = 0;
            int n = snprintf(out, outsz, "%s/%s", self, name);
            if (n > 0 && (size_t)n < outsz) {
                struct stat sb;
                if (stat(out, &sb) == 0 && (sb.st_mode & S_IXUSR))
                    done;
            }
        }
    }
    snprintf(out, outsz, "%s", name);
    done;
}
