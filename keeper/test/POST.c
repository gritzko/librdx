//  POST unit test: `keeper post <ssh-uri>` must
//    (1) create a new commit in the local keeper store
//    (2) push it to the remote bare repo over ssh.
//
//  Self-contained: everything lives in a fresh mkdtemp() under TMPDIR,
//  one tiny seed blob, one seed commit. No treadmill data sets.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "abc/PRO.h"
#include "abc/TEST.h"

static int runcmd(char const *cmd) {
    fprintf(stderr, "  $ %s\n", cmd);
    return system(cmd);
}

static int popen_line(char *out, size_t outlen, char const *cmd) {
    FILE *p = popen(cmd, "r");
    if (!p) return -1;
    out[0] = 0;
    if (fgets(out, (int)outlen, p)) {
        size_t n = strlen(out);
        while (n > 0 && (out[n - 1] == '\n' || out[n - 1] == '\r'))
            out[--n] = 0;
    }
    return pclose(p);
}

ok64 maintest() {
    sane(1);

    char const *bin = getenv("BIN");
    if (!bin || !*bin) bin = "build-debug/bin";

    char tmp[] = "/tmp/keeper-post-XXXXXX";
    want(mkdtemp(tmp) != NULL);

    char origin[256], work[256];
    snprintf(origin, sizeof(origin), "%s/origin", tmp);
    snprintf(work,   sizeof(work),   "%s/work",   tmp);

    // --- 1. Fresh bare origin with one tiny seed commit ---
    fprintf(stderr, "--- seed origin ---\n");
    {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
            "git init --bare %s >/dev/null && "
            "S=%s/seed && git init $S >/dev/null && "
            "git -C $S config user.email t@t && "
            "git -C $S config user.name T && "
            "echo seed > $S/f && "
            "git -C $S add . && "
            "git -C $S commit -m seed >/dev/null && "
            "git -C $S push %s master:master 2>/dev/null",
            origin, tmp, origin);
        want(runcmd(cmd) == 0);
    }

    char seed_sha[64] = {};
    {
        char cmd[512];
        snprintf(cmd, sizeof(cmd),
                 "git --git-dir=%s rev-parse master", origin);
        want(popen_line(seed_sha, sizeof(seed_sha), cmd) == 0);
        want(strlen(seed_sha) == 40);
        fprintf(stderr, "  seed=%.12s\n", seed_sha);
    }

    // --- 2. Empty .dogs/keeper in a fresh work dir ---
    {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "mkdir -p %s/.dogs/keeper", work);
        want(runcmd(cmd) == 0);
    }

    // --- 3. Bootstrap the store: fetch seed objects via git-upload-pack ---
    fprintf(stderr, "--- keeper get (bootstrap) ---\n");
    {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
            "cd %s && %s/keeper get //localhost%s 2>&1",
            work, bin, origin);
        want(runcmd(cmd) == 0);
    }

    // --- 4. Post: create a commit and push it back over ssh ---
    fprintf(stderr, "--- keeper post ---\n");
    {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
            "cd %s && %s/keeper post ssh://localhost%s 2>&1",
            work, bin, origin);
        want(runcmd(cmd) == 0);
    }

    // --- 5. Origin must have a new commit whose parent is the seed ---
    fprintf(stderr, "--- verify origin ---\n");
    char new_sha[64] = {};
    {
        char cmd[512];
        snprintf(cmd, sizeof(cmd),
                 "git --git-dir=%s rev-parse master", origin);
        want(popen_line(new_sha, sizeof(new_sha), cmd) == 0);
        want(strlen(new_sha) == 40);
        want(strcmp(new_sha, seed_sha) != 0);
        fprintf(stderr, "  new HEAD=%.12s\n", new_sha);
    }
    {
        char cmd[512], parent[64] = {};
        snprintf(cmd, sizeof(cmd),
                 "git --git-dir=%s rev-parse %s^", origin, new_sha);
        want(popen_line(parent, sizeof(parent), cmd) == 0);
        want(strcmp(parent, seed_sha) == 0);
        fprintf(stderr, "  parent OK\n");
    }
    {
        char cmd[512], kind[32] = {};
        snprintf(cmd, sizeof(cmd),
                 "git --git-dir=%s cat-file -t %s", origin, new_sha);
        want(popen_line(kind, sizeof(kind), cmd) == 0);
        want(strcmp(kind, "commit") == 0);
    }

    fprintf(stderr, "--- post OK ---\n");

    // --- 6. Local-only post: no //host → commit created, no push ---
    fprintf(stderr, "--- keeper post (local only) ---\n");
    char pre_sha[64] = {};
    {
        char cmd[512];
        snprintf(cmd, sizeof(cmd),
                 "git --git-dir=%s rev-parse master", origin);
        want(popen_line(pre_sha, sizeof(pre_sha), cmd) == 0);
    }
    {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
            "cd %s && %s/keeper post 2>&1", work, bin);
        want(runcmd(cmd) == 0);
    }
    {
        char cmd[512], post_sha[64] = {};
        snprintf(cmd, sizeof(cmd),
                 "git --git-dir=%s rev-parse master", origin);
        want(popen_line(post_sha, sizeof(post_sha), cmd) == 0);
        want(strcmp(post_sha, pre_sha) == 0);
        fprintf(stderr, "  origin unchanged after local post\n");
    }

    { char cmd[512]; snprintf(cmd, sizeof(cmd), "rm -rf %s", tmp);
      runcmd(cmd); }
    done;
}

TEST(maintest)
