//  sniff CLI — index, watch daemon, status, checkout
//
#include "SNIFF.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "CHE.h"

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

// --- Mode: Index (stat all git-tracked paths, record mtimes) ---

// Stat all known paths, record mtime with given type.
static ok64 sniff_stat_all(sniff *s, u8cs reporoot, u8 type) {
    sane(s);
    u32 n = SNIFFCount(s);
    u32 count = 0;
    for (u32 i = 0; i < n; i++) {
        u8cs rel = {};
        if (SNIFFPath(rel, s, i) != OK) continue;

        a_path(fp);
        if (SNIFFFullpath(fp, reporoot, rel) != OK) continue;

        struct stat sb = {};
        if (FILEStat(&sb, PATHu8cgIn(fp)) != OK) continue;

        SNIFFRecord(s, type, i, (u64)sb.st_mtim.tv_sec);
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
        sniff_stat_all(s, reporoot, SNIFF_CHANGED);
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

// --- Mode: Status (scan worktree, compare mtime against checkout) ---

static ok64 sniff_status(sniff *s, u8cs reporoot) {
    sane(s);
    u32 n = SNIFFCount(s);
    u32 dirty = 0;

    for (u32 i = 0; i < n; i++) {
        u64 co = SNIFFGet(s, SNIFF_CHECKOUT, i);
        if (co == 0) continue;  // no checkout record

        u8cs rel = {};
        if (SNIFFPath(rel, s, i) != OK) continue;

        a_path(fp);
        if (SNIFFFullpath(fp, reporoot, rel) != OK) continue;

        struct stat sb = {};
        if (FILEStat(&sb, PATHu8cgIn(fp)) != OK) continue;

        u64 now = (u64)sb.st_mtim.tv_sec;
        if (now == co) continue;

        printf("%.*s\n", (int)$len(rel), (char *)rel[0]);
        dirty++;
    }

    fprintf(stderr, "sniff: %u dirty file(s)\n", dirty);
    done;
}

// --- Mode: Checkout ---

static ok64 sniff_checkout(sniff *s, u8cs reporoot, u8cs hex) {
    sane(s && $ok(hex));
    keeper k = {};
    call(KEEPOpen, &k, reporoot);
    ok64 o = CHECheckout(s, &k, reporoot, hex);
    KEEPClose(&k);
    return o;
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
            "  sniff -s | --status   dirty files (changed since checkout)\n"
            "  sniff -w | --watch    start watch daemon\n"
            "  sniff --stop          stop watch daemon\n"
            "  sniff -c <hex>        checkout commit from keeper\n"
            "  sniff -l | --list     list all known paths\n"
            "  sniff -h | --help     this message\n");
}

// --- Entry ---

ok64 sniffcli() {
    sane(1);
    call(FILEInit);

    // Find worktree root (HOMEFind for git repos, cwd fallback for keeper-only)
    a_path(root);
    ok64 ho = HOMEFind(root);
    if (ho != OK) {
        char cwd[1024];
        if (!getcwd(cwd, sizeof(cwd))) fail(SNIFFFAIL);
        a_cstr(cwds, cwd);
        call(u8bFeed, root, cwds);
        call(PATHu8gTerm, PATHu8gIn(root));
    }
    a_dup(u8c, reporoot, u8bDataC(root));

    // Parse args
    b8 do_index = NO;
    b8 do_update = NO;
    b8 do_status = NO;
    b8 do_watch = NO;
    b8 do_stop = NO;
    b8 do_list = NO;
    u8cs checkout_hex = {};

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
        } else if ((argeq(a, "-c") || argeq(a, "--checkout"))
                   && i + 1 < $arglen) {
            i++;
            $mv(checkout_hex, $arg(i));
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

    b8 rw = do_index || do_update || do_watch || $ok(checkout_hex);
    sniff s = {};
    call(SNIFFOpen, &s, reporoot, rw);

    ok64 ret = OK;
    if ($ok(checkout_hex)) {
        ret = sniff_checkout(&s, reporoot, checkout_hex);
    } else if (do_watch) {
        ret = sniff_daemon(&s, reporoot);
    } else if (do_status) {
        ret = sniff_status(&s, reporoot);
    } else if (do_list) {
        ret = sniff_list(&s);
    } else if (do_update) {
        ret = sniff_stat_all(&s, reporoot, SNIFF_CHANGED);
    } else {
        ret = sniff_stat_all(&s, reporoot, SNIFF_CHECKOUT);
    }

    SNIFFClose(&s);
    return ret;
}

MAIN(sniffcli);
