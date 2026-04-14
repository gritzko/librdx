//  sniff CLI — index, watch, status, checkout, commit
//
#include "SNIFF.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "DEL.h"
#include "GET.h"
#include "POST.h"
#include "PUT.h"
#include "dog/CLI.h"
#include "dog/IGNO.h"
#include "keeper/REFS.h"

#include "abc/FILE.h"
#include "abc/FSW.h"
#include "abc/PATH.h"
#include "abc/PRO.h"
#include "abc/UTF8.h"

// --- Helpers ---

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
    ok64 o = GETCheckout(s, &k, reporoot, hex);
    KEEPClose(&k);
    return o;
}

// Checkout from a parsed URI: resolve ?ref via keeper REFS, then checkout.
static ok64 SNIFFGetURI(sniff *s, u8cs reporoot, uri *u) {
    sane(s && u);
    if (!$empty(u->query)) {
        keeper k = {};
        call(KEEPOpen, &k, reporoot);
        a_cstr(keepdir, k.dir);
        a_pad(u8, arena, 512);
        uri resolved = {};
        ok64 o = REFSResolve(&resolved, arena, keepdir, u->data);
        if (o == OK && !$empty(resolved.query)) {
            o = GETCheckout(s, &k, reporoot, resolved.query);
            KEEPClose(&k);
            return o;
        }
        // Resolve failed — try the query as raw hex
        o = GETCheckout(s, &k, reporoot, u->query);
        KEEPClose(&k);
        return o;
    }
    // No query — treat the whole URI data as hex (legacy)
    return sniff_checkout(s, reporoot, u->data);
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
            "Usage: sniff <command> [options] [files...]\n"
            "\n"
            "  sniff index                rebuild index\n"
            "  sniff update               update mtimes\n"
            "  sniff status               show dirty/deleted files\n"
            "  sniff checkout <hex>       checkout commit from keeper\n"
            "  sniff commit -m <msg> --parent <hex>   commit changes\n"
            "  sniff commit -a -m <msg> --parent <hex>  commit all\n"
            "  sniff commit -m <msg> --parent <hex> f1 f2  commit files\n"
            "  sniff watch                start watch daemon\n"
            "  sniff stop                 stop watch daemon\n"
            "  sniff list                 list all known paths\n"
            "  sniff help                 this message\n"
            "\n"
            "  Shared verbs (dispatched by be):\n"
            "  sniff get <hex>            checkout (repo -> worktree)\n"
            "  sniff post --parent <hex>  build tree (worktree -> keeper)\n"
            "  sniff put -m <msg> --parent <hex>  commit\n"
            "  sniff delete --parent <hex> f1 f2  remove files from tree\n"
            "\n"
            "  Flags:\n"
            "    -m <msg>       commit message\n"
            "    --parent <hex> parent commit SHA\n"
            "    --author <who> author string\n"
            "    -a             commit all (scan for new files)\n");
}

// --- Verb names ---

static char const *const sniff_verbs[] = {
    "index", "update", "status", "checkout",
    "commit", "watch", "stop", "list", "help",
    "get", "post", "put", "delete", NULL
};

// Value flags: -m, --parent, --author (NUL-separated, triple-NUL end)
static char const sniff_val_flags[] =
    "-m\0--parent\0--author\0";

// --- Entry ---

ok64 sniffcli() {
    sane(1);
    call(FILEInit);

    cli c = {};
    call(CLIParse, &c, sniff_verbs, sniff_val_flags);

    u8cs reporoot = {};
    if ($ok(c.repo)) {
        $mv(reporoot, c.repo);
    } else {
        char cwd[1024];
        if (!getcwd(cwd, sizeof(cwd))) fail(SNIFFFAIL);
        a_cstr(cwds, cwd);
        reporoot[0] = cwds[0];
        reporoot[1] = cwds[1];
    }

    a_cstr(v_help, "help");
    a_cstr(v_index, "index");
    a_cstr(v_update, "update");
    a_cstr(v_status, "status");
    a_cstr(v_checkout, "checkout");
    a_cstr(v_commit, "commit");
    a_cstr(v_watch, "watch");
    a_cstr(v_stop, "stop");
    a_cstr(v_list, "list");
    a_cstr(v_get, "get");
    a_cstr(v_post, "post");
    a_cstr(v_put, "put");
    a_cstr(v_delete, "delete");

    if ($eq(c.verb, v_help) || CLIHas(&c, "-h") || CLIHas(&c, "--help")) {
        sniff_usage(); done;
    }

    if ($eq(c.verb, v_stop)) {
        call(sniff_stop, reporoot); done;
    }

    b8 is_checkout = $eq(c.verb, v_checkout) || $eq(c.verb, v_get);
    b8 is_commit = $eq(c.verb, v_commit) || $eq(c.verb, v_put);
    b8 is_index = $eq(c.verb, v_index);
    b8 is_update = $eq(c.verb, v_update);
    b8 is_watch = $eq(c.verb, v_watch);
    b8 is_status = $eq(c.verb, v_status);
    b8 is_list = $eq(c.verb, v_list);
    b8 is_post = $eq(c.verb, v_post);
    b8 is_delete = $eq(c.verb, v_delete);

    b8 rw = is_index || is_update || is_watch || is_checkout || is_commit
             || is_post || is_delete;
    sniff s = {};
    call(SNIFFOpen, &s, reporoot, rw);

    ok64 ret = OK;

    if (is_commit) {
        u8cs commit_msg = {};
        CLIFlag(commit_msg, &c, "-m");
        u8cs commit_parent = {};
        CLIFlag(commit_parent, &c, "--parent");
        u8cs commit_author = {};
        CLIFlag(commit_author, &c, "--author");

        if (!$ok(commit_msg) || !$ok(commit_parent)) {
            fprintf(stderr, "sniff: commit requires -m and --parent\n");
            ret = SNIFFFAIL;
        } else {
            if (!$ok(commit_author)) {
                a_cstr(def, "sniff <sniff@dogs>");
                commit_author[0] = def[0];
                commit_author[1] = def[1];
            }

            b8 do_all = CLIHas(&c, "-a");

            if (do_all)
                sniff_scan_new(&s, reporoot);

            sniff_stat_all(&s, reporoot, SNIFF_CHANGED);

            u8p cset = NULL;
            u32 npaths = SNIFFCount(&s);

            if (c.nuris > 0) {
                Bu8 csbuf = {};
                u8bAllocate(csbuf, npaths);
                memset(u8bDataHead(csbuf), 0, npaths);
                cset = u8bDataHead(csbuf);
                for (u32 f = 0; f < c.nuris; f++) {
                    u32 idx = SNIFFIntern(&s, c.uris[f].data);
                    if (idx < npaths) cset[idx] = 1;
                }
            }

            keeper k = {};
            ret = KEEPOpen(&k, reporoot);
            if (ret == OK) {
                sha1 sha = {};
                ret = PUTCommit(&s, &k, reporoot, commit_parent,
                                commit_msg, commit_author, cset, &sha);
                KEEPClose(&k);
            }
        }
    } else if (is_post) {
        u8cs commit_parent = {};
        CLIFlag(commit_parent, &c, "--parent");
        if (!$ok(commit_parent)) {
            fprintf(stderr, "sniff: post requires --parent\n");
            ret = SNIFFFAIL;
        } else {
            sniff_stat_all(&s, reporoot, SNIFF_CHANGED);

            u8p cset = NULL;
            u32 npaths = SNIFFCount(&s);
            if (c.nuris > 0) {
                Bu8 csbuf = {};
                u8bAllocate(csbuf, npaths);
                memset(u8bDataHead(csbuf), 0, npaths);
                cset = u8bDataHead(csbuf);
                for (u32 f = 0; f < c.nuris; f++) {
                    u32 idx = SNIFFIntern(&s, c.uris[f].data);
                    if (idx < npaths) cset[idx] = 1;
                }
            }

            keeper k = {};
            ret = KEEPOpen(&k, reporoot);
            if (ret == OK) {
                keep_pack p = {};
                ret = KEEPPackOpen(&k, &p);
                if (ret == OK) {
                    sha1 tree = {};
                    ret = POSTTree(&tree, &s, &k, &p, reporoot,
                                    commit_parent, cset);
                    KEEPPackClose(&k, &p);
                    if (ret == OK) {
                        a_pad(u8, hex, 40);
                        a_rawc(ts, tree);
                        HEXu8sFeedSome(hex_idle, ts);
                        fprintf(stderr, "sniff: tree %.*s\n",
                                (int)u8bDataLen(hex),
                                (char *)u8bDataHead(hex));
                    }
                }
                KEEPClose(&k);
            }
        }
    } else if (is_delete) {
        u8cs commit_parent = {};
        CLIFlag(commit_parent, &c, "--parent");
        if (!$ok(commit_parent) || c.nuris < 1) {
            fprintf(stderr,
                    "sniff: delete requires --parent and file(s)\n");
            ret = SNIFFFAIL;
        } else {
            u32 npaths = SNIFFCount(&s);
            Bu8 dsbuf = {};
            u8bAllocate(dsbuf, npaths);
            memset(u8bDataHead(dsbuf), 0, npaths);
            u8p dset = u8bDataHead(dsbuf);
            for (u32 f = 0; f < c.nuris; f++) {
                u32 idx = SNIFFIntern(&s, c.uris[f].data);
                if (idx < npaths) dset[idx] = 1;
            }

            keeper k = {};
            ret = KEEPOpen(&k, reporoot);
            if (ret == OK) {
                keep_pack p = {};
                ret = KEEPPackOpen(&k, &p);
                if (ret == OK) {
                    sha1 tree = {};
                    ret = DELTree(&tree, &s, &k, &p, reporoot,
                                   commit_parent, dset);
                    KEEPPackClose(&k, &p);
                    if (ret == OK) {
                        a_pad(u8, hex, 40);
                        a_rawc(ts, tree);
                        HEXu8sFeedSome(hex_idle, ts);
                        fprintf(stderr, "sniff: tree %.*s\n",
                                (int)u8bDataLen(hex),
                                (char *)u8bDataHead(hex));
                    }
                }
                KEEPClose(&k);
            }
        }
    } else if (is_checkout) {
        if (c.nuris < 1) {
            fprintf(stderr, "sniff: get/checkout requires a URI or hex\n");
            ret = SNIFFFAIL;
        } else if ($eq(c.verb, v_get)) {
            ret = SNIFFGetURI(&s, reporoot, &c.uris[0]);
        } else {
            ret = sniff_checkout(&s, reporoot, c.uris[0].data);
        }
    } else if (is_watch) {
        ret = sniff_daemon(&s, reporoot);
    } else if (is_status) {
        ret = sniff_status(&s, reporoot);
    } else if (is_list) {
        ret = sniff_list(&s);
    } else if (is_update) {
        ret = sniff_stat_all(&s, reporoot, SNIFF_CHANGED);
    } else {
        // Default: index (record SNIFF_CHECKOUT mtimes)
        ret = sniff_stat_all(&s, reporoot, SNIFF_CHECKOUT);
    }

    SNIFFClose(&s);
    return ret;
}

MAIN(sniffcli);
