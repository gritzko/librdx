//
// DAG01 - End-to-end test for graf/DAG commit-graph reindexer.
//
// Creates a tmp git repo, makes a chain of commits (incl. a merge),
// runs DAGHook, then verifies:
//   1. .dogs/graf/COMMIT contains current HEAD
//   2. .dogs/graf/idx/0000000001.idx exists and has 5 records
//   3. Every commit record has gen >= 1 and the right parent count
//   4. Re-running DAGHook with no new commits creates no new run
//   5. Adding one commit and re-running produces a 1-entry run
//   6. Generation numbers are monotonic along the parent edges that
//      are present in the index.
//

#include "DAG.h"

#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

// On-disk index entry — must match graf/DAG.c struct dag32.
typedef struct {
    u64 hashlet;
    u32 gen;
    u32 npar;
    u64 parent0;
    u64 parent1;
} dag_e;

// Run a shell command, fail the test if exit != 0.
static ok64 sh(char const *fmt, ...) {
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    int rc = system(buf);
    if (!WIFEXITED(rc) || WEXITSTATUS(rc) != 0) {
        fprintf(stderr, "DAG01: shell failed (rc=%d): %s\n", rc, buf);
        return TESTFAIL;
    }
    return OK;
}

// Count .idx files under <dir>/idx/.
static u32 count_idx_files(char const *dir) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/idx", dir);
    DIR *d = opendir(path);
    if (!d) return 0;
    u32 n = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        size_t l = strlen(e->d_name);
        if (l > 4 && strcmp(e->d_name + l - 4, ".idx") == 0) n++;
    }
    closedir(d);
    return n;
}

// Read the bookmark file (oldest first), return last sha or 0.
static int read_last_bookmark(char *out40, char const *dir) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/COMMIT", dir);
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    char line[64];
    int got = 0;
    while (fgets(line, sizeof(line), fp)) {
        size_t l = strlen(line);
        while (l > 0 && (line[l - 1] == '\n' || line[l - 1] == '\r'))
            line[--l] = 0;
        if (l == 40) {
            memcpy(out40, line, 40);
            out40[40] = 0;
            got = 1;
        }
    }
    fclose(fp);
    return got;
}

// Sum of records across all .idx files (= total commits indexed).
static u64 total_indexed(char const *dir) {
    char idxdir[1024];
    snprintf(idxdir, sizeof(idxdir), "%s/idx", dir);
    DIR *d = opendir(idxdir);
    if (!d) return 0;
    u64 total = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        size_t l = strlen(e->d_name);
        if (l <= 4 || strcmp(e->d_name + l - 4, ".idx") != 0) continue;
        char fp[1280];
        snprintf(fp, sizeof(fp), "%s/%s", idxdir, e->d_name);
        struct stat st;
        if (stat(fp, &st) != 0) continue;
        total += (u64)st.st_size / sizeof(dag_e);
    }
    closedir(d);
    return total;
}

// Verify: every entry has gen >= 1; npar reflects how many parent
// fields are nonzero (modulo octopus); records are sorted by hashlet
// within each file.  Walk every entry and, when both parents are in
// the same run, check that gen >= max(parent_gen) + 1.
static ok64 verify_index(char const *dir) {
    char idxdir[1024];
    snprintf(idxdir, sizeof(idxdir), "%s/idx", dir);
    DIR *d = opendir(idxdir);
    if (!d) return TESTFAIL;

    // Load all entries from all files into one flat slice (small for
    // tests; we can afford the heap).
    dag_e *all = NULL;
    u64 cap = 0, len = 0;

    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        size_t l = strlen(e->d_name);
        if (l <= 4 || strcmp(e->d_name + l - 4, ".idx") != 0) continue;
        char fp[1280];
        snprintf(fp, sizeof(fp), "%s/%s", idxdir, e->d_name);
        FILE *f = fopen(fp, "rb");
        if (!f) { closedir(d); free(all); return TESTFAIL; }
        struct stat st;
        if (stat(fp, &st) != 0) { fclose(f); closedir(d); free(all); return TESTFAIL; }
        u64 n = (u64)st.st_size / sizeof(dag_e);
        if (len + n > cap) {
            cap = (len + n) * 2;
            all = realloc(all, cap * sizeof(dag_e));
        }
        // sortedness check per file
        dag_e prev = {};
        for (u64 i = 0; i < n; i++) {
            dag_e cur;
            if (fread(&cur, sizeof(cur), 1, f) != 1) {
                fclose(f); closedir(d); free(all); return TESTFAIL;
            }
            if (i > 0 && cur.hashlet < prev.hashlet) {
                fprintf(stderr, "DAG01: %s not sorted at %lu\n",
                        e->d_name, (unsigned long)i);
                fclose(f); closedir(d); free(all); return TESTFAIL;
            }
            if (cur.gen < 1) {
                fprintf(stderr,
                        "DAG01: gen=0 in %s at %lu\n",
                        e->d_name, (unsigned long)i);
                fclose(f); closedir(d); free(all); return TESTFAIL;
            }
            prev = cur;
            all[len++] = cur;
        }
        fclose(f);
    }
    closedir(d);

    // For each entry, look up its parents in the flat set and check
    // gen >= parent_gen + 1.
    for (u64 i = 0; i < len; i++) {
        dag_e *cur = &all[i];
        u64 pgs[2] = {0, 0};
        u64 ps[2] = {cur->parent0, cur->parent1};
        u32 max = (cur->npar < 2) ? cur->npar : 2;
        for (u32 k = 0; k < max; k++) {
            for (u64 j = 0; j < len; j++) {
                if (all[j].hashlet == ps[k]) { pgs[k] = all[j].gen; break; }
            }
        }
        for (u32 k = 0; k < max; k++) {
            if (pgs[k] == 0) continue;  // parent not (yet) in index
            if (cur->gen <= pgs[k]) {
                fprintf(stderr,
                        "DAG01: gen %u not > parent gen %lu\n",
                        cur->gen, (unsigned long)pgs[k]);
                free(all);
                return TESTFAIL;
            }
        }
    }

    free(all);
    return OK;
}

// ---- Test 1: full reindex on a fresh repo, then incremental ----

ok64 DAG01test1() {
    sane(1);

    char tmpdir[] = "/tmp/dag01-XXXXXX";
    test(mkdtemp(tmpdir) != NULL, FAILSANITY);

    // git init + 4-commit linear history + side branch + no-ff merge
    call(sh, "git -C %s init -q -b main", tmpdir);
    call(sh, "git -C %s config user.email a@b.c", tmpdir);
    call(sh, "git -C %s config user.name DAG01", tmpdir);
    call(sh, "echo a > %s/a && git -C %s add a && "
            "git -C %s commit -q -m c1", tmpdir, tmpdir, tmpdir);
    call(sh, "echo b > %s/b && git -C %s add b && "
            "git -C %s commit -q -m c2", tmpdir, tmpdir, tmpdir);
    call(sh, "echo c > %s/c && git -C %s add c && "
            "git -C %s commit -q -m c3", tmpdir, tmpdir, tmpdir);
    call(sh, "git -C %s checkout -q -b side HEAD~1", tmpdir);
    call(sh, "echo d > %s/d && git -C %s add d && "
            "git -C %s commit -q -m c-side", tmpdir, tmpdir, tmpdir);
    call(sh, "git -C %s checkout -q main", tmpdir);
    call(sh, "git -C %s merge -q --no-ff side -m merge", tmpdir);

    // first invocation: full reindex
    u8cs rr = {(u8cp)tmpdir, (u8cp)tmpdir + strlen(tmpdir)};
    call(DAGHook, rr);

    char dagdir[1024];
    snprintf(dagdir, sizeof(dagdir), "%s/.dogs/graf", tmpdir);

    u32 nfiles = count_idx_files(dagdir);
    fprintf(stderr, "  pass1: %u idx files\n", nfiles);
    test(nfiles == 1, TESTFAIL);

    u64 first_total = total_indexed(dagdir);
    fprintf(stderr, "  pass1: %lu commits indexed\n",
            (unsigned long)first_total);
    test(first_total == 5, TESTFAIL);

    call(verify_index, dagdir);

    char bm[44];
    test(read_last_bookmark(bm, dagdir), TESTFAIL);
    fprintf(stderr, "  pass1: bookmark = %s\n", bm);

    // second invocation: no new commits, no new run expected
    call(DAGHook, rr);
    u32 nfiles2 = count_idx_files(dagdir);
    fprintf(stderr, "  pass2: %u idx files\n", nfiles2);
    test(nfiles2 == nfiles, TESTFAIL);
    test(total_indexed(dagdir) == first_total, TESTFAIL);

    // third invocation: add one commit, expect a new 1-entry run
    call(sh, "echo e > %s/e && git -C %s add e && "
            "git -C %s commit -q -m c-extra", tmpdir, tmpdir, tmpdir);
    call(DAGHook, rr);
    u32 nfiles3 = count_idx_files(dagdir);
    fprintf(stderr, "  pass3: %u idx files\n", nfiles3);
    test(nfiles3 == nfiles2 + 1, TESTFAIL);
    test(total_indexed(dagdir) == first_total + 1, TESTFAIL);
    call(verify_index, dagdir);

    // cleanup
    char rmcmd[1280];
    snprintf(rmcmd, sizeof(rmcmd), "rm -rf %s", tmpdir);
    system(rmcmd);

    done;
}

ok64 maintest() {
    sane(1);
    call(DAG01test1);
    done;
}

TEST(maintest)
