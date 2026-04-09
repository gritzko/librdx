//  sniff CLI — index, watch daemon, query
//
#include "SNIFF.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/FSW.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "dog/HOME.h"

// --- Helpers ---

static b8 argeq(u8cs a, const char *b) {
    size_t blen = strlen(b);
    return $len(a) == blen && memcmp(a[0], b, blen) == 0;
}

// --- Mode 1: Index (stat all git-tracked paths, record mtimes) ---

// Build absolute path from reporoot + relative path (with / separator)
static ok64 sniff_fullpath(path8b out, u8cs reporoot, u8cs rel) {
    sane($ok(reporoot) && $ok(rel));
    a_cstr(sep, "/");
    call(u8bFeed, out, reporoot);
    call(u8bFeed, out, sep);
    call(u8bFeed, out, rel);
    call(PATHu8gTerm, PATHu8gIn(out));
    done;
}

static ok64 sniff_index(sniff *s, u8cs reporoot) {
    sane(s);
    u32 n = SNIFFCount(s);
    u32 count = 0;
    for (u32 i = 0; i < n; i++) {
        u8cs rel = {};
        if (SNIFFPath(rel, s, i) != OK) continue;

        a_path(fp);
        if (sniff_fullpath(fp, reporoot, rel) != OK) continue;

        struct stat sb = {};
        if (FILEStat(&sb, PATHu8cgIn(fp)) != OK) continue;

        SNIFFRecord(s, i, (u64)sb.st_mtim.tv_sec,
                    (u32)sb.st_mtim.tv_nsec);
        count++;
    }
    fprintf(stderr, "sniff: indexed %u file(s)\n", count);
    done;
}

// --- Mode 2: Watch daemon ---

static volatile sig_atomic_t sniff_quit = 0;

static void sniff_sighandler(int sig) {
    (void)sig;
    sniff_quit = 1;
}

static ok64 sniff_write_pid(u8cs reporoot) {
    sane($ok(reporoot));
    a_path(pp, reporoot);
    a_cstr(rel, "/" SNIFF_DIR "/pid");
    call(u8bFeed, pp, rel);
    call(PATHu8gTerm, PATHu8gIn(pp));

    FILE *fp = fopen((char *)u8bDataHead(pp), "w");
    if (!fp) fail(SNIFFFAIL);
    fprintf(fp, "%d\n", (int)getpid());
    fclose(fp);
    done;
}

static ok64 sniff_rm_pid(u8cs reporoot) {
    sane($ok(reporoot));
    a_path(pp, reporoot);
    a_cstr(rel, "/" SNIFF_DIR "/pid");
    call(u8bFeed, pp, rel);
    call(PATHu8gTerm, PATHu8gIn(pp));
    unlink((char *)u8bDataHead(pp));
    done;
}

// Callback for FILEScan: add each directory to FSW
typedef struct {
    int wfd;
    u32 count;
} watchdir_ctx;

static ok64 sniff_watchdir_cb(voidp arg, path8p path) {
    watchdir_ctx *ctx = (watchdir_ctx *)arg;
    u8csc p = {*path, path[1]};
    ok64 o = FSWDir(ctx->wfd, p);
    if (o == OK) ctx->count++;
    return OK;  // keep going even if one dir fails
}

// After FSWPoll wakeup, stat all known paths and record changes.
static ok64 sniff_rescan(sniff *s, u8cs reporoot) {
    sane(s);
    u32 n = SNIFFCount(s);
    for (u32 i = 0; i < n; i++) {
        u8cs rel = {};
        if (SNIFFPath(rel, s, i) != OK) continue;

        a_path(fp);
        if (sniff_fullpath(fp, reporoot, rel) != OK) continue;

        struct stat sb = {};
        if (FILEStat(&sb, PATHu8cgIn(fp)) != OK) continue;

        SNIFFRecord(s, i, (u64)sb.st_mtim.tv_sec,
                    (u32)sb.st_mtim.tv_nsec);
    }
    done;
}

// Drain callback: just consume events
static ok64 sniff_drain_cb(u8cs path, void *ctx) {
    (void)path;
    (void)ctx;
    return OK;
}

static ok64 sniff_daemon(sniff *s, u8cs dogsroot, u8cs worktree) {
    sane(s);

    // Daemonize: fork, setsid, redirect stdio
    pid_t pid = fork();
    if (pid < 0) fail(SNIFFFAIL);
    if (pid > 0) {
        // Parent: print child PID and exit
        fprintf(stderr, "sniff: daemon pid %d\n", (int)pid);
        _exit(0);
    }
    // Child continues
    setsid();
    int devnull = open("/dev/null", O_RDWR);
    if (devnull >= 0) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        if (devnull > STDERR_FILENO) close(devnull);
    }

    call(sniff_write_pid, dogsroot);

    // Signal handling
    struct sigaction sa = {.sa_handler = sniff_sighandler};
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    // Init FSW and watch all directories under the worktree
    int wfd = -1;
    call(FSWInit, &wfd);

    {
        u8csc rp = {worktree[0], worktree[1]};
        FSWDir(wfd, rp);
    }

    watchdir_ctx wctx = {.wfd = wfd};
    {
        a_path(wp, worktree);
        FILEScan(wp,
                 (FILE_SCAN)(FILE_SCAN_DIRS | FILE_SCAN_DEEP),
                 sniff_watchdir_cb, &wctx);
    }

    // Main loop
    while (!sniff_quit) {
        ok64 o = FSWPoll(wfd, 1000);  // 1s timeout for signal check
        if (o != OK) continue;

        FSWDrain(wfd, sniff_drain_cb, NULL);
        sniff_rescan(s, worktree);
    }

    FSWClose(wfd);
    sniff_rm_pid(dogsroot);
    done;
}

// --- Mode 3: Stop daemon ---

static ok64 sniff_stop(u8cs reporoot) {
    sane($ok(reporoot));
    a_path(pp, reporoot);
    a_cstr(rel, "/" SNIFF_DIR "/pid");
    call(u8bFeed, pp, rel);
    call(PATHu8gTerm, PATHu8gIn(pp));

    FILE *fp = fopen((char *)u8bDataHead(pp), "r");
    if (!fp) {
        fprintf(stderr, "sniff: no daemon running (no pidfile)\n");
        done;
    }
    int dpid = 0;
    if (fscanf(fp, "%d", &dpid) != 1 || dpid <= 0) {
        fclose(fp);
        fprintf(stderr, "sniff: bad pidfile\n");
        fail(SNIFFFAIL);
    }
    fclose(fp);

    if (kill(dpid, SIGTERM) != 0) {
        fprintf(stderr, "sniff: kill(%d) failed\n", dpid);
        unlink((char *)u8bDataHead(pp));
        fail(SNIFFFAIL);
    }
    fprintf(stderr, "sniff: stopped daemon pid %d\n", dpid);
    unlink((char *)u8bDataHead(pp));
    done;
}

// --- Mode 4: Query (latest change per path) ---

static ok64 sniff_changed(sniff *s) {
    sane(s);
    u32 npath = SNIFFCount(s);
    if (npath == 0) {
        fprintf(stderr, "sniff: no paths known\n");
        done;
    }

    // Allocate a "seen" bitmap + mtime array
    // Walk changes backward, first occurrence per index wins
    u64 *latest_sec = calloc(npath, sizeof(u64));
    u32 *latest_nsec = calloc(npath, sizeof(u32));
    u8 *seen = calloc(npath, sizeof(u8));
    if (!latest_sec || !latest_nsec || !seen) {
        free(latest_sec);
        free(latest_nsec);
        free(seen);
        fail(SNIFFFAIL);
    }

    // Walk changes log backward
    u64cs elog = {(u64cp)u8bDataHead(s->changes),
                  (u64cp)u8bIdleHead(s->changes)};
    size_t nentries = $len(elog);

    for (size_t i = nentries; i > 0; i--) {
        u64 e = $at(elog, i - 1);
        u32 idx = SNIFFChangeIndex(e);
        if (idx >= npath) continue;
        if (seen[idx]) continue;
        seen[idx] = 1;
        latest_sec[idx] = SNIFFChangeSec(e);
        latest_nsec[idx] = SNIFFChangeNsec(e);
    }

    // Print paths that have recorded changes
    for (u32 i = 0; i < npath; i++) {
        if (!seen[i]) continue;
        u8cs path = {};
        if (SNIFFPath(path, s, i) != OK) continue;
        printf("%llu.%u\t%.*s\n",
               (unsigned long long)latest_sec[i], latest_nsec[i],
               (int)$len(path), (char *)path[0]);
    }

    free(latest_sec);
    free(latest_nsec);
    free(seen);
    done;
}

// --- Mode 5: List all paths ---

static ok64 sniff_list(sniff *s) {
    sane(s);
    u32 n = SNIFFCount(s);
    for (u32 i = 0; i < n; i++) {
        u8cs path = {};
        if (SNIFFPath(path, s, i) != OK) continue;
        printf("%.*s\n", (int)$len(path), (char *)path[0]);
    }
    fprintf(stderr, "sniff: %u path(s)\n", n);
    done;
}

// --- Usage ---

static void sniff_usage(void) {
    fprintf(stderr,
            "Usage: sniff [options]\n"
            "\n"
            "  sniff                 index (stat all known paths)\n"
            "  sniff -w | --watch    start watch daemon\n"
            "  sniff --stop          stop watch daemon\n"
            "  sniff -q | --changed  show latest change per path\n"
            "  sniff -l | --list     list all known paths\n"
            "  sniff -h | --help     this message\n");
}

// --- Entry ---

ok64 sniffcli() {
    sane(1);
    call(FILEInit);

    // Find worktree root (for git commands and file paths)
    a_path(wtroot);
    call(HOMEFind, wtroot);
    a_dup(u8c, worktree, u8bDataC(wtroot));

    // Find .dogs/ location (may differ for worktrees)
    a_path(dgroot);
    ok64 ho = HOMEFindDogs(dgroot);
    if (ho != OK) {
        // Fallback: use worktree root
        u8bReset(dgroot);
        call(u8bFeed, dgroot, worktree);
        call(PATHu8gTerm, PATHu8gIn(dgroot));
    }
    a_dup(u8c, dogsroot, u8bDataC(dgroot));

    // Parse args
    b8 do_watch = NO;
    b8 do_stop = NO;
    b8 do_changed = NO;
    b8 do_list = NO;

    for (u32 i = 1; i < $arglen; i++) {
        u8cs a = {};
        $mv(a, $arg(i));
        if (argeq(a, "-h") || argeq(a, "--help")) {
            sniff_usage();
            done;
        } else if (argeq(a, "-w") || argeq(a, "--watch")) {
            do_watch = YES;
        } else if (argeq(a, "--stop")) {
            do_stop = YES;
        } else if (argeq(a, "-q") || argeq(a, "--changed")) {
            do_changed = YES;
        } else if (argeq(a, "-l") || argeq(a, "--list")) {
            do_list = YES;
        } else {
            fprintf(stderr, "sniff: unknown option: %.*s\n",
                    (int)$len(a), (char *)a[0]);
            sniff_usage();
            fail(SNIFFFAIL);
        }
    }

    // --stop doesn't need SNIFFInit
    if (do_stop) {
        call(sniff_stop, dogsroot);
        done;
    }

    sniff s = {};
    call(SNIFFInit, &s, dogsroot, worktree);

    ok64 ret = OK;
    if (do_watch) {
        ret = sniff_daemon(&s, dogsroot, worktree);
    } else if (do_changed) {
        ret = sniff_changed(&s);
    } else if (do_list) {
        ret = sniff_list(&s);
    } else {
        ret = sniff_index(&s, worktree);
    }

    SNIFFFree(&s);
    return ret;
}

MAIN(sniffcli);
