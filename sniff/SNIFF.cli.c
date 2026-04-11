//  sniff CLI — index, watch, status, checkout, commit
//
#include "SNIFF.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "CHE.h"
#include "COM.h"
#include "IGNO.h"

#include "abc/FILE.h"
#include "abc/FSW.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/UTF8.h"
#include "dog/HOME.h"

// --- Helpers ---

static b8 argeq(u8cs a, const char *b) {
    size_t blen = strlen(b);
    return $len(a) == blen && memcmp(a[0], b, blen) == 0;
}

// --- Mode: Index ---

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

typedef struct { int wfd; u32 count; } watchdir_ctx;

static ok64 sniff_watchdir_cb(voidp arg, path8p path) {
    watchdir_ctx *ctx = (watchdir_ctx *)arg;
    u8csc p = {u8bDataHead(path), u8bIdleHead(path)};
    ok64 o = FSWDir(ctx->wfd, p);
    if (o == OK) ctx->count++;
    return OK;
}

static ok64 sniff_drain_cb(u8cs path, void *ctx) {
    (void)path; (void)ctx; return OK;
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
    { u8csc rp = {reporoot[0], reporoot[1]}; FSWDir(wfd, rp); }
    watchdir_ctx wctx = {.wfd = wfd};
    {
        a_path(wp, reporoot);
        FILEScan(wp, (FILE_SCAN)(FILE_SCAN_DIRS | FILE_SCAN_DEEP),
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
    if (!fp) { fprintf(stderr, "sniff: no daemon running\n"); done; }
    int dpid = 0;
    if (fscanf(fp, "%d", &dpid) != 1 || dpid <= 0) {
        fclose(fp); fail(SNIFFFAIL);
    }
    fclose(fp);
    if (kill(dpid, SIGTERM) != 0) {
        unlink((char *)u8bDataHead(pp)); fail(SNIFFFAIL);
    }
    fprintf(stderr, "sniff: stopped pid %d\n", dpid);
    unlink((char *)u8bDataHead(pp));
    done;
}

// --- Mode: Status ---

static ok64 sniff_status(sniff *s, u8cs reporoot) {
    sane(s);
    u32 n = SNIFFCount(s);
    u32 dirty = 0;
    for (u32 i = 0; i < n; i++) {
        u64 co = SNIFFGet(s, SNIFF_CHECKOUT, i);
        if (co == 0) continue;
        u8cs rel = {};
        if (SNIFFPath(rel, s, i) != OK) continue;
        a_path(fp);
        if (SNIFFFullpath(fp, reporoot, rel) != OK) continue;
        struct stat sb = {};
        if (FILEStat(&sb, PATHu8cgIn(fp)) != OK) {
            // File deleted
            printf("D %.*s\n", (int)$len(rel), (char *)rel[0]);
            dirty++;
            continue;
        }
        u64 now = (u64)sb.st_mtim.tv_sec;
        if (now == co) continue;
        printf("M %.*s\n", (int)$len(rel), (char *)rel[0]);
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

// --- Scan worktree for new files (commit-all) ---

typedef struct {
    sniff *s;
    u8cs reporoot;
    igno *ig;
    u32 count;
    u32 total;
} scan_ctx;

static ok64 sniff_scan_cb(voidp arg, path8p path) {
    scan_ctx *ctx = (scan_ctx *)arg;

    // Build relative path from full path in buffer data region
    u8cs full = {u8bDataHead(path), u8bIdleHead(path)};
    size_t rlen = $len(ctx->reporoot);
    if ($len(full) <= rlen) return OK;
    u8cs rel = {$atp(full, rlen), full[1]};
    if ($at(rel, 0) == '/') ++rel[0];
    if ($empty(rel)) return OK;

    // Skip .dogs/
    a_cstr(dogs, ".dogs");
    if ($len(rel) >= 5 && memcmp(rel[0], dogs[0], 5) == 0) return OK;

    // Skip ignored files
    if (ctx->ig && IGNOMatch(ctx->ig, rel, NO)) return OK;

    ctx->total++;
    // Intern path (creates entry if new)
    u32 idx = SNIFFIntern(ctx->s, rel);

    // Check if file exists and is UTF-8
    a_path(fp);
    SNIFFFullpath(fp, ctx->reporoot, rel);

    struct stat lsb = {};
    if (lstat((char *)u8bDataHead(fp), &lsb) != 0) return OK;
    if (S_ISDIR(lsb.st_mode)) return OK;
    if (S_ISLNK(lsb.st_mode)) {
        // Symlinks are always trackable
    } else {
        // Regular files: check UTF-8
        Bu8 content = {};
        if (u8bAllocate(content, 1UL << 20) != OK) return OK;
        int fd = -1;
        if (FILEOpen(&fd, PATHu8cgIn(fp), O_RDONLY) != OK) {
            u8bFree(content); return OK;
        }
        FILEdrainall(u8bIdle(content), fd);
        FILEClose(&fd);

        u8cs data = {u8bDataHead(content), u8bIdleHead(content)};
        utf8cs utf = {(utf8cp)data[0], (utf8cp)data[1]};
        ok64 valid = utf8sValid(utf);
        u8bFree(content);
        if (valid != OK) return OK;
    }

    // Record mtime as SNIFF_CHANGED (marks as dirty for commit)
    SNIFFRecord(ctx->s, SNIFF_CHANGED, idx, (u64)lsb.st_mtim.tv_sec);

    ctx->count++;
    return OK;
}

static ok64 sniff_scan_new(sniff *s, u8cs reporoot) {
    sane(s);
    igno ig = {};
    IGNOLoad(&ig, reporoot);

    scan_ctx ctx = {.s = s, .reporoot = {reporoot[0], reporoot[1]},
                    .ig = ig.count > 0 ? &ig : NULL};
    a_path(wp);
    call(PATHu8bFeed, wp, reporoot);
    call(FILEDeepScanFiles, wp, sniff_scan_cb, &ctx);

    if (ctx.count > 0)
        fprintf(stderr, "sniff: scanned %u, %u new\n",
                ctx.total, ctx.count);
    IGNOFree(&ig);
    done;
}

// --- Mode: List ---

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
            "  sniff -i | --index       rebuild index\n"
            "  sniff -u | --update      update mtimes\n"
            "  sniff -s | --status      show dirty/deleted files\n"
            "  sniff -o <hex>           checkout commit from keeper\n"
            "  sniff -c -m <msg> --parent <hex>   commit tracked changes\n"
            "  sniff -a -m <msg> --parent <hex>   commit all (scan new)\n"
            "  sniff -t f1 f2 -m <msg> --parent <hex>  commit listed files\n"
            "  sniff -w | --watch       start watch daemon\n"
            "  sniff --stop             stop watch daemon\n"
            "  sniff -l | --list        list all known paths\n"
            "  sniff -h | --help        this message\n");
}

// --- Entry ---

ok64 sniffcli() {
    sane(1);
    call(FILEInit);

    a_path(root);
    ok64 ho = HOMEFind(root);
    if (ho != OK) {
        char cwd[1024];
        if (!getcwd(cwd, sizeof(cwd))) fail(SNIFFFAIL);
        u8bReset(root);
        a_cstr(cwds, cwd);
        call(u8bFeed, root, cwds);
        call(PATHu8gTerm, PATHu8gIn(root));
    }
    a_dup(u8c, reporoot, u8bDataC(root));

    // Parse args
    b8 do_index = NO, do_update = NO, do_status = NO;
    b8 do_watch = NO, do_stop = NO, do_list = NO;
    b8 do_commit = NO, do_commit_all = NO, do_commit_it = NO;
    u8cs checkout_hex = {};
    u8cs commit_msg = {};
    u8cs commit_parent = {};
    u8cs commit_author = {};
    u8cs commit_files[64] = {};
    u32 ncommit_files = 0;

    for (u32 i = 1; i < $arglen; i++) {
        u8cs a = {};
        $mv(a, $arg(i));
        if (argeq(a, "-h") || argeq(a, "--help")) {
            sniff_usage(); done;
        } else if (argeq(a, "-i") || argeq(a, "--index")) {
            do_index = YES;
        } else if (argeq(a, "-u") || argeq(a, "--update")) {
            do_update = YES;
        } else if (argeq(a, "-s") || argeq(a, "--status")) {
            do_status = YES;
        } else if ((argeq(a, "-o") || argeq(a, "--checkout"))
                   && i + 1 < $arglen) {
            i++; $mv(checkout_hex, $arg(i));
        } else if (argeq(a, "-c") || argeq(a, "--commit")) {
            do_commit = YES;
        } else if (argeq(a, "-a") || argeq(a, "--commit-all")) {
            do_commit_all = YES;
        } else if (argeq(a, "-t") || argeq(a, "--commit-it")) {
            do_commit_it = YES;
            // Consume trailing file args until next flag
            while (i + 1 < $arglen) {
                u8cs peek = {};
                $mv(peek, $arg(i + 1));
                if ($len(peek) > 0 && peek[0][0] == '-') break;
                if (ncommit_files < 64) {
                    commit_files[ncommit_files][0] = peek[0];
                    commit_files[ncommit_files][1] = peek[1];
                    ncommit_files++;
                }
                i++;
            }
        } else if ((argeq(a, "-m") || argeq(a, "--message"))
                   && i + 1 < $arglen) {
            i++; $mv(commit_msg, $arg(i));
        } else if (argeq(a, "--parent") && i + 1 < $arglen) {
            i++; $mv(commit_parent, $arg(i));
        } else if (argeq(a, "--author") && i + 1 < $arglen) {
            i++; $mv(commit_author, $arg(i));
        } else if (argeq(a, "-w") || argeq(a, "--watch")) {
            do_watch = YES;
        } else if (argeq(a, "--stop")) {
            do_stop = YES;
        } else if (argeq(a, "-l") || argeq(a, "--list")) {
            do_list = YES;
        } else {
            fprintf(stderr, "sniff: unknown option: %.*s\n",
                    (int)$len(a), (char *)a[0]);
            sniff_usage(); fail(SNIFFFAIL);
        }
    }

    if (do_stop) { call(sniff_stop, reporoot); done; }

    b8 any_commit = do_commit || do_commit_all || do_commit_it;
    b8 rw = do_index || do_update || do_watch || $ok(checkout_hex)
             || any_commit;
    sniff s = {};
    call(SNIFFOpen, &s, reporoot, rw);

    ok64 ret = OK;

    if (any_commit) {
        if (!$ok(commit_msg) || !$ok(commit_parent)) {
            fprintf(stderr, "sniff: commit requires -m and --parent\n");
            ret = SNIFFFAIL;
        } else {
            if (!$ok(commit_author)) {
                a_cstr(def, "sniff <sniff@dogs>");
                commit_author[0] = def[0];
                commit_author[1] = def[1];
            }

            // For commit-all: scan worktree for new files
            if (do_commit_all)
                sniff_scan_new(&s, reporoot);

            // Update mtimes for all tracked files
            sniff_stat_all(&s, reporoot, SNIFF_CHANGED);

            // Build commit_set if needed
            u8p cset = NULL;
            u32 npaths = SNIFFCount(&s);

            if (do_commit_it) {
                // Explicit file list
                Bu8 csbuf = {};
                u8bAllocate(csbuf, npaths);
                memset(u8bDataHead(csbuf), 0, npaths);
                cset = u8bDataHead(csbuf);
                for (u32 f = 0; f < ncommit_files; f++) {
                    u32 idx = SNIFFIntern(&s, commit_files[f]);
                    if (idx < npaths) cset[idx] = 1;
                }
            }
            // -c and -a: commit_set=NULL means "all changed"

            keeper k = {};
            ret = KEEPOpen(&k, reporoot);
            if (ret == OK) {
                u8 sha[20] = {};
                ret = COMCommit(&s, &k, reporoot, commit_parent,
                                commit_msg, commit_author, cset, sha);
                KEEPClose(&k);
            }
        }
    } else if ($ok(checkout_hex)) {
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
