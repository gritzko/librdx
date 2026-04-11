//  sniff CLI — index, watch daemon, status
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

// --- Mode: Index (stat all git-tracked paths, record mtimes) ---

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

        SNIFFRecord(s, 0, i, (u64)sb.st_mtim.tv_sec);
        count++;
    }
    fprintf(stderr, "sniff: indexed %u file(s)\n", count);
    done;
}

// --- Mode: Watch daemon ---

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

typedef struct {
    int wfd;
    u32 count;
} watchdir_ctx;

static ok64 sniff_watchdir_cb(voidp arg, path8p path) {
    watchdir_ctx *ctx = (watchdir_ctx *)arg;
    u8csc p = {*path, path[1]};
    ok64 o = FSWDir(ctx->wfd, p);
    if (o == OK) ctx->count++;
    return OK;
}

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

        SNIFFRecord(s, 0, i, (u64)sb.st_mtim.tv_sec);
    }
    done;
}

static ok64 sniff_drain_cb(u8cs path, void *ctx) {
    (void)path;
    (void)ctx;
    return OK;
}

static ok64 sniff_daemon(sniff *s, u8cs reporoot) {
    sane(s);

    pid_t pid = fork();
    if (pid < 0) fail(SNIFFFAIL);
    if (pid > 0) {
        fprintf(stderr, "sniff: daemon pid %d\n", (int)pid);
        _exit(0);
    }
    setsid();
    int devnull = open("/dev/null", O_RDWR);
    if (devnull >= 0) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        if (devnull > STDERR_FILENO) close(devnull);
    }

    call(sniff_write_pid, reporoot);

    struct sigaction sa = {.sa_handler = sniff_sighandler};
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    int wfd = -1;
    call(FSWInit, &wfd);

    {
        u8csc rp = {reporoot[0], reporoot[1]};
        FSWDir(wfd, rp);
    }

    watchdir_ctx wctx = {.wfd = wfd};
    {
        a_path(wp, reporoot);
        FILEScan(wp,
                 (FILE_SCAN)(FILE_SCAN_DIRS | FILE_SCAN_DEEP),
                 sniff_watchdir_cb, &wctx);
    }

    while (!sniff_quit) {
        ok64 o = FSWPoll(wfd, 1000);
        if (o != OK) continue;
        FSWDrain(wfd, sniff_drain_cb, NULL);
        sniff_rescan(s, reporoot);
    }

    FSWClose(wfd);
    sniff_rm_pid(reporoot);
    done;
}

// --- Mode: Stop daemon ---

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

// --- Mode: Status (paths changed since token) ---

static ok64 sniff_status(sniff *s, u64 since) {
    sane(s);
    u64cs elog = {(u64cp)u8bDataHead(s->changes),
                  (u64cp)u8bIdleHead(s->changes)};
    u64 total = (u64)$len(elog);

    if (since > total) since = total;

    u32 npath = SNIFFCount(s);
    u8 *seen = calloc(npath, sizeof(u8));
    if (!seen) fail(SNIFFFAIL);

    for (u64 i = since; i < total; i++) {
        u32 idx = wh64Id($at(elog, i));
        if (idx < npath) seen[idx] = 1;
    }

    for (u32 i = 0; i < npath; i++) {
        if (!seen[i]) continue;
        u8cs path = {};
        if (SNIFFPath(path, s, i) != OK) continue;
        printf("%.*s\n", (int)$len(path), (char *)path[0]);
    }

    free(seen);
    fprintf(stderr, "%llu\n", (unsigned long long)total);
    done;
}

// --- Mode: List all paths ---

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
            "  sniff -i | --index    rebuild index (stat all tracked paths)\n"
            "  sniff -u | --update   update index (incremental)\n"
            "  sniff -s [T]          status (paths changed since token T)\n"
            "  sniff -w | --watch    start watch daemon\n"
            "  sniff --stop          stop watch daemon\n"
            "  sniff -l | --list     list all known paths\n"
            "  sniff -h | --help     this message\n");
}

// --- Entry ---

ok64 sniffcli() {
    sane(1);
    call(FILEInit);

    // Find worktree root
    a_path(root);
    call(HOMEFind, root);
    a_dup(u8c, reporoot, u8bDataC(root));

    // Parse args
    b8 do_index = NO;
    b8 do_update = NO;
    b8 do_status = NO;
    b8 do_watch = NO;
    b8 do_stop = NO;
    b8 do_list = NO;
    u64 since_token = 0;

    for (u32 i = 1; i < $arglen; i++) {
        u8cs a = {};
        $mv(a, $arg(i));
        if (argeq(a, "-h") || argeq(a, "--help")) {
            sniff_usage();
            done;
        } else if (argeq(a, "-i") || argeq(a, "--index")) {
            do_index = YES;
        } else if (argeq(a, "-u") || argeq(a, "--update")) {
            do_update = YES;
        } else if (argeq(a, "-s") || argeq(a, "--status")) {
            do_status = YES;
            if (i + 1 < $arglen) {
                u8cs v = {};
                $mv(v, $arg(i + 1));
                if (v[0][0] >= '0' && v[0][0] <= '9') {
                    since_token = (u64)atoll((char *)v[0]);
                    i++;
                }
            }
        } else if (argeq(a, "-w") || argeq(a, "--watch")) {
            do_watch = YES;
        } else if (argeq(a, "--stop")) {
            do_stop = YES;
        } else if (argeq(a, "-l") || argeq(a, "--list")) {
            do_list = YES;
        } else {
            fprintf(stderr, "sniff: unknown option: %.*s\n",
                    (int)$len(a), (char *)a[0]);
            sniff_usage();
            fail(SNIFFFAIL);
        }
    }

    if (do_stop) {
        call(sniff_stop, reporoot);
        done;
    }

    b8 rw = do_index || do_update || do_watch;
    sniff s = {};
    call(SNIFFOpen, &s, reporoot, rw);

    ok64 ret = OK;
    if (do_watch) {
        ret = sniff_daemon(&s, reporoot);
    } else if (do_status) {
        ret = sniff_status(&s, since_token);
    } else if (do_list) {
        ret = sniff_list(&s);
    } else if (do_update) {
        ret = sniff_index(&s, reporoot);
    } else {
        ret = sniff_index(&s, reporoot);
    }

    SNIFFClose(&s);
    return ret;
}

MAIN(sniffcli);
