#include "CAPO.h"
#include "SPOT_VERSION.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sys/stat.h>

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "spot/INST.h"
#include "spot/LESS.h"

// Try to follow a .git worktree file to the parent repo root.
// On success, writes the parent repo root (the dir containing the parent
// .git directory) into `out`. Any failure (unreadable file, malformed
// content, dangling gitdir, unexpected layout) returns an error and
// leaves `out` unmodified — caller falls back to the worktree dir.
static ok64 SPOTFollowWorktree(path8b out, path8cg gitfile) {
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

// Walk up from cwd to find the workspace root: the first ancestor dir
// containing a .git entry (file or directory). For worktrees (.git is a
// file), try to follow it to the parent repo root. On any failure, fall
// back to the dir containing the .git file. Returns FAILSANITY if no
// .git is found.
static ok64 SPOTFindWorkspace(path8b out) {
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
                ok64 fo = SPOTFollowWorktree(parent, PATHu8cgIn(probe));
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

static void SPOTUsage(void) {
    fprintf(stderr,
        "Usage: spot [options] [files...]\n"
        "\n"
        "  spot                               incremental index update\n"
        "  spot file.c                        syntax-highlighted cat\n"
        "  spot -i | --index                  full reindex\n"
        "  spot -f N | --fork N               parallel reindex on N cores\n"
        "  spot -u | --uncommitted            index staged + unstaged changes\n"
        "  spot -U | --untracked              also index untracked (new) files\n"
        "  spot --hook                        post-commit incremental update\n"
        "  spot -s \"pattern\" .ext             code snippet search\n"
        "  spot -s \"pat\" -r \"repl\" .ext       code snippet search + replace\n"
        "  spot -g \"text\" [.ext]              grep (substring, incl. comments)\n"
        "  spot -g \"text\" -C N [.ext]         grep with N lines of context\n"
        "  spot -p \"regex\" [.ext]             regex grep (Thompson NFA)\n"
        "  spot -p \"regex\" -C N [.ext]        regex grep with context\n"
        "  spot -d | --diff old new           token-level colored diff\n"
        "  spot --gitdiff                     git external diff driver\n"
        "  spot --merge base ours theirs      token-level 3-way merge\n"
        "  spot --merge base ours theirs -o f merge to file\n"
        "  spot -F ...                        disable streaming pager (no-fork mode)\n"
        "  spot -n | --install                install as git diff/merge driver\n"
        "\n"
        "Patterns: single-letter placeholders (a-z match one token/group,\n"
        "A-Z match multiple tokens). Two spaces = skip gap.\n"
        "\n"
        "Git integration (manual, or use spot -n):\n"
        "  git config diff.spot.command \"spot --gitdiff\"\n"
        "  git config merge.spot.driver \"spot --merge %%O %%A %%B -o %%A\"\n"
    );
}

static b8 argeq(u8cs a, const char *b) {
    size_t blen = strlen(b);
    return $len(a) == blen && memcmp(a[0], b, blen) == 0;
}

// Check if a trailing arg is a bare .ext filter known to tok/
static b8 argIsExt(u8cs a) {
    if ($len(a) < 2 || a[0][0] != '.') return NO;
    return CAPOKnownExt(a);
}

// Match "-fVALUE" short flag with attached value, return value or NULL
static char *argshortval(u8cs a, const char *flag) {
    size_t flen = strlen(flag);
    if ($len(a) > flen && memcmp(a[0], flag, flen) == 0)
        return (char *)a[0] + flen;
    return NULL;
}

// Match "--flag=value", return pointer to value after '=' or NULL
static char *argeqval(u8cs a, const char *flag) {
    size_t flen = strlen(flag);
    if ($len(a) > flen + 1 && memcmp(a[0], flag, flen) == 0 &&
        a[0][flen] == '=')
        return (char *)a[0] + flen + 1;
    return NULL;
}

ok64 capocli() {
    sane(1);
    call(FILEInit);
    CAPO_TERM = isatty(STDERR_FILENO) ? YES : NO;
    CAPO_COLOR = isatty(STDOUT_FILENO) ? YES : NO;
    if (getenv("SPOT_COLOR")) CAPO_COLOR = YES;

    // Find workspace root: walk up from cwd until we hit a .git entry.
    a_path(root);
    call(SPOTFindWorkspace, root);
    a_dup(u8c, reporoot, u8bDataC(root));

    // Parse args
    u32 nfork = 0, proc = UINT32_MAX;
    b8 is_hook = NO;
    b8 do_index = NO;
    b8 do_install = NO;
    b8 do_merge = NO;
    b8 do_diff = NO;
    b8 do_gitdiff = NO;
    b8 do_uncommitted = NO;
    b8 do_untracked = NO;
    b8 pipe_mode = isatty(STDOUT_FILENO) ? YES : NO;
    u8c *merge_out[2] = {};
    u8c *spot_ndl[2] = {};
    u8c *spot_rep[2] = {};
    u8c *grep_ndl[2] = {};
    u8c *pcre_ndl[2] = {};
    u32 grep_ctx = 3;
    u8c *trail[16][2] = {};
    int ntrail = 0;
    int argn = (int)$arglen;

    for (int i = 1; i < argn; i++) {
        u8c *a[2] = {};
        $mv(a, $arg(i));
        char *eqval = NULL;
        if (argeq(a, "-v") || argeq(a, "--version")) {
            fprintf(stderr, "spot %s %s\n", SPOT_GIT_TAG, SPOT_COMMIT_HASH);
            done;
        } else if (argeq(a, "-h") || argeq(a, "--help")) {
            SPOTUsage();
            done;
        } else if ((argeq(a, "-f") || argeq(a, "--fork")) && i + 1 < argn) {
            i++;
            u8c *v[2] = {};
            $mv(v, $arg(i));
            nfork = (u32)atoi((char *)v[0]);
        } else if ((eqval = argeqval(a, "--fork"))) {
            nfork = (u32)atoi(eqval);
        } else if ((eqval = argshortval(a, "-f"))) {
            nfork = (u32)atoi(eqval);
        } else if (argeq(a, "--proc") && i + 1 < argn) {
            i++;
            u8c *v[2] = {};
            $mv(v, $arg(i));
            proc = (u32)atoi((char *)v[0]);
        } else if ((eqval = argeqval(a, "--proc"))) {
            proc = (u32)atoi(eqval);
        } else if (argeq(a, "-d") || argeq(a, "--diff")) {
            do_diff = YES;
        } else if (argeq(a, "--gitdiff")) {
            do_gitdiff = YES;
        } else if (argeq(a, "--merge")) {
            do_merge = YES;
        } else if (argeq(a, "-o") && i + 1 < argn) {
            i++;
            $mv(merge_out, $arg(i));
        } else if (argeq(a, "-n") || argeq(a, "--install")) {
            do_install = YES;
        } else if (argeq(a, "-i") || argeq(a, "--index")) {
            do_index = YES;
        } else if (argeq(a, "--hook")) {
            is_hook = YES;
        } else if (argeq(a, "-u") || argeq(a, "--uncommitted")) {
            do_uncommitted = YES;
        } else if (argeq(a, "-U") || argeq(a, "--untracked")) {
            do_uncommitted = YES;
            do_untracked = YES;
        } else if ((argeq(a, "-s") || argeq(a, "--spot")) && i + 1 < argn) {
            i++;
            $mv(spot_ndl, $arg(i));
        } else if ((eqval = argeqval(a, "--spot"))) {
            spot_ndl[0] = (u8cp)eqval;
            spot_ndl[1] = (u8cp)eqval + strlen(eqval);
        } else if ((argeq(a, "-r") || argeq(a, "--replace")) && i + 1 < argn) {
            i++;
            $mv(spot_rep, $arg(i));
        } else if ((eqval = argeqval(a, "--replace"))) {
            spot_rep[0] = (u8cp)eqval;
            spot_rep[1] = (u8cp)eqval + strlen(eqval);
        } else if ((argeq(a, "-g") || argeq(a, "--grep")) && i + 1 < argn) {
            i++;
            $mv(grep_ndl, $arg(i));
        } else if ((eqval = argeqval(a, "--grep"))) {
            grep_ndl[0] = (u8cp)eqval;
            grep_ndl[1] = (u8cp)eqval + strlen(eqval);
        } else if ((argeq(a, "-p") || argeq(a, "--pcre")) && i + 1 < argn) {
            i++;
            $mv(pcre_ndl, $arg(i));
        } else if ((eqval = argeqval(a, "--pcre"))) {
            pcre_ndl[0] = (u8cp)eqval;
            pcre_ndl[1] = (u8cp)eqval + strlen(eqval);
        } else if (argeq(a, "-C") && i + 1 < argn) {
            i++;
            u8c *v[2] = {};
            $mv(v, $arg(i));
            grep_ctx = (u32)atoi((char *)v[0]);
        } else if ((eqval = argeqval(a, "--context"))) {
            grep_ctx = (u32)atoi(eqval);
        } else if (argeq(a, "--context") && i + 1 < argn) {
            i++;
            u8c *v[2] = {};
            $mv(v, $arg(i));
            grep_ctx = (u32)atoi((char *)v[0]);
        } else if (argeq(a, "-F")) {
            pipe_mode = NO;
        } else {
            if (ntrail < 16) { $mv(trail[ntrail], a); ntrail++; }
        }
    }

    // Validate: -r only valid with -s
    if (spot_rep[0] != NULL && spot_ndl[0] == NULL) {
        fprintf(stderr, "spot: --replace requires --spot\n");
        return FAILSANITY;
    }

    // Pipe mode: fork worker/pager for grep, spot, diff, cat
    // Skip pager in replace mode (-r): output goes to stderr
    if (pipe_mode &&
        (grep_ndl[0] != NULL || pcre_ndl[0] != NULL ||
         spot_ndl[0] != NULL || do_diff || ntrail > 0) &&
        !do_gitdiff && !do_merge && !do_index && !is_hook &&
        spot_rep[0] == NULL) {
        int pfd[2];
        test(pipe(pfd) == 0, FAILSANITY);
        pid_t pid = fork();
        test(pid >= 0, FAILSANITY);
        if (pid == 0) {
            // Child = worker: writes TLV to pipe
            close(pfd[0]);
            less_pipe_fd = pfd[1];
            // fall through to normal dispatch
        } else {
            // Parent = pager: reads TLV from pipe
            close(pfd[1]);
            ok64 o = LESSPipeRun(pfd[0]);
            close(pfd[0]);
            waitpid(pid, NULL, 0);
            return o;
        }
    }

    if (do_install) {
        call(INSTInstall, reporoot);
    } else if (do_gitdiff) {
        // git diff driver: path old-file old-hex old-mode new-file new-hex new-mode
        if (ntrail < 7) {
            fprintf(stderr, "spot: --gitdiff expects 7 args from git\n");
            return FAILSANITY;
        }
        CAPO_COLOR = YES;  // git pager handles ANSI
        u8cs nm = {trail[0][0], trail[0][1]};  // logical path
        u8cs op = {trail[1][0], trail[1][1]};  // old-file
        u8cs om = {trail[3][0], trail[3][1]};  // old-mode
        u8cs np = {trail[4][0], trail[4][1]};  // new-file
        u8cs nwm = {trail[6][0], trail[6][1]}; // new-mode
        call(CAPODiff, op, np, nm, om, nwm);
    } else if (do_diff) {
        // Diff mode: expects 2 trailing paths (old new)
        if (ntrail < 2) {
            fprintf(stderr, "spot: --diff requires 2 files: old new\n");
            return FAILSANITY;
        }
        u8cs op = {trail[0][0], trail[0][1]};
        u8cs np = {trail[1][0], trail[1][1]};
        u8cs nomode = {};
        call(CAPODiff, op, np, np, nomode, nomode);
    } else if (do_merge) {
        // Merge mode: expects 3 trailing paths (base ours theirs)
        if (ntrail < 3) {
            fprintf(stderr, "spot: --merge requires 3 files: base ours theirs\n");
            return FAILSANITY;
        }
        u8cs bp = {trail[0][0], trail[0][1]};
        u8cs op = {trail[1][0], trail[1][1]};
        u8cs tp = {trail[2][0], trail[2][1]};
        a_dup(u8c,mo,merge_out);
        call(CAPOMerge, bp, op, tp, mo);
    } else if (grep_ndl[0] != NULL) {
        // GREP mode: .ext optional, file paths restrict search
        u8cs ext = {};
        u8cs gfiles[16] = {};
        int gnf = 0;
        for (int i = 0; i < ntrail; i++) {
            if (argIsExt(trail[i])) {
                $mv(ext, trail[i]);
            } else if (gnf < 16) {
                $mv(gfiles[gnf], trail[i]);
                gnf++;
            }
        }
        // No bare .ext — extract extension from first file path
        if ($empty(ext) && gnf > 0) {
            u8cs pe = {};
            PATHu8sExt(pe, gfiles[0]);
            if (!$empty(pe)) {
                ext[0] = pe[0] - 1;  // include the dot
                ext[1] = pe[1];
            }
        }
        a_dup(u8c,ndl,grep_ndl);
        u8css gf = {gfiles, gfiles + gnf};
        call(CAPOGrep, ndl, ext, reporoot, grep_ctx, gf);
    } else if (pcre_ndl[0] != NULL) {
        // PCRE (regex) mode: .ext optional, file paths restrict search
        u8cs ext = {};
        u8cs gfiles[16] = {};
        int gnf = 0;
        for (int i = 0; i < ntrail; i++) {
            if (argIsExt(trail[i])) {
                $mv(ext, trail[i]);
            } else if (gnf < 16) {
                $mv(gfiles[gnf], trail[i]);
                gnf++;
            }
        }
        if ($empty(ext) && gnf > 0) {
            u8cs pe = {};
            PATHu8sExt(pe, gfiles[0]);
            if (!$empty(pe)) {
                ext[0] = pe[0] - 1;
                ext[1] = pe[1];
            }
        }
        a_dup(u8c,ndl,pcre_ndl);
        u8css gf = {gfiles, gfiles + gnf};
        call(CAPOPcreGrep, ndl, ext, reporoot, grep_ctx, gf);
    } else if (do_uncommitted) {
        call(CAPOUncommitted, reporoot, do_untracked);
    } else if (is_hook) {
        call(CAPOHook, reporoot);
    } else if (nfork > 0 && proc != UINT32_MAX) {
        // Worker mode: I am proc K of N
        call(CAPOReindexProc, reporoot, nfork, proc);
    } else if (nfork > 0) {
        // Orchestrator: fork N children, wait, compact
        a_path(capodir);
        call(CAPOResolveDir, capodir, reporoot);
        a_dup(u8c, dirslice, u8bDataC(capodir));
        call(FILEMakeDirP, PATHu8cgIn(capodir));

        // Get our own executable path
        char self[FILE_PATH_MAX_LEN];
        ssize_t slen = readlink("/proc/self/exe", self, sizeof(self) - 1);
        test(slen > 0, FAILSANITY);
        self[slen] = 0;

        pid_t pids[256];
        u32 n = nfork;
        if (n > 256) n = 256;

        fprintf(stderr, "spot: forking %u workers\n", n);
        for (u32 k = 0; k < n; k++) {
            pid_t pid = fork();
            if (pid == 0) {
                char nstr[16], kstr[16];
                snprintf(nstr, sizeof(nstr), "%u", n);
                snprintf(kstr, sizeof(kstr), "%u", k);
                execl(self, "spot", "--fork", nstr, "--proc", kstr, NULL);
                _exit(127);
            }
            test(pid > 0, FAILSANITY);
            pids[k] = pid;
        }

        int failures = 0;
        for (u32 k = 0; k < n; k++) {
            int status = 0;
            waitpid(pids[k], &status, 0);
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                fprintf(stderr, "spot: worker %u failed (status %d)\n",
                        k, status);
                failures++;
            }
        }

        if (failures > 0)
            fprintf(stderr, "spot: %d workers failed\n", failures);

        fprintf(stderr, "spot: compacting all runs\n");
        call(CAPOCompactAll, dirslice);
        call(CAPOCommitWrite, reporoot, dirslice);
        fprintf(stderr, "spot: done\n");
    } else if (spot_ndl[0] != NULL) {
        // SPOT mode: .ext and/or file paths
        u8cs ext = {};
        u8cs sfiles[16] = {};
        int snf = 0;
        for (int i = 0; i < ntrail; i++) {
            if (argIsExt(trail[i])) {
                $mv(ext, trail[i]);
            } else if (snf < 16) {
                $mv(sfiles[snf], trail[i]);
                snf++;
            }
        }
        // No bare .ext — extract extension from first file path
        if ($empty(ext) && snf > 0) {
            u8cs pe = {};
            PATHu8sExt(pe, sfiles[0]);
            if (!$empty(pe)) {
                ext[0] = pe[0] - 1;  // include the dot
                ext[1] = pe[1];
            }
        }
        if ($empty(ext)) {
            fprintf(stderr, "spot: --spot requires a .ext argument\n");
            return FAILSANITY;
        }
        a_dup(u8c,ndl,spot_ndl);
        a_dup(u8c,rep,spot_rep);
        u8css sf = {sfiles, sfiles + snf};
        call(CAPOSpot, ndl, rep, ext, reporoot, sf);
    } else if (do_index) {
        call(CAPOReindex, reporoot);
    } else if (ntrail > 0) {
        // Cat mode: colorful file output
        u8cs files[16] = {};
        int nf = 0;
        for (int i = 0; i < ntrail && nf < 16; i++) {
            files[nf][0] = trail[i][0];
            files[nf][1] = trail[i][1];
            nf++;
        }
        u8css cf = {files, files + nf};
        call(CAPOCat, cf, reporoot);
    } else {
        call(CAPOHook, reporoot);
    }
    done;
}

MAIN(capocli);
