#include "BE.h"
#include "BESYNC.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "abc/ANSI.h"
#include "abc/FILE.h"
#include "abc/PRO.h"

// Get cwd into a path buffer
static ok64 BECwd(path8g path) {
    sane(path != NULL);
    char cwd[FILE_PATH_MAX_LEN];
    test(getcwd(cwd, sizeof(cwd)) != NULL, BEFAIL);
    a_cstr(cwdcs, cwd);
    call(u8sFeed, path + 1, cwdcs);
    call(path8gTerm, path);
    done;
}

// Open BE from cwd
static ok64 BEOpenCwd(BEp be) {
    sane(be != NULL);
    a_pad(u8, cpath, FILE_PATH_MAX_LEN);
    call(BECwd, path8gIn(cpath));
    call(BEOpen, be, path8cgIn(cpath));
    done;
}

// Count base keys and waypoints per branch
typedef struct {
    int base_count;
    int wp_count;
    size_t pfxlen;
} BEStatusCtx;

static ok64 BEStatusCB(voidp arg, u8cs key, u8cs val) {
    BEStatusCtx *ctx = (BEStatusCtx *)arg;
    u8cs rest = {key[0] + ctx->pfxlen, key[1]};
    u8cs qs = {rest[0], rest[1]};
    if (u8csFind(qs, '?') != OK) {
        ctx->base_count++;
    } else {
        ctx->wp_count++;
    }
    return OK;
}

// Print gray label + value + newline
static ok64 BEStatusLine(u8cs label, u8cs value) {
    u8 lbuf[512];
    u8s ls = {lbuf, lbuf + sizeof(lbuf)};
    escfeed(ls, GRAY);
    u8sFeed(ls, label);
    u8sFeed(ls, value);
    escfeed(ls, 0);
    u8sFeed1(ls, '\n');
    u8cs line = {lbuf, ls[0]};
    return FILEout(line);
}

// Print status info
static ok64 BEStatus(BEp be) {
    sane(be != NULL);

    if ($ok(be->loc.host) && !$empty(be->loc.host)) {
        a_cstr(lbl_repo, "repo: ");
        call(BEStatusLine, lbl_repo, be->loc.host);
    }
    if ($ok(be->loc.path) && !$empty(be->loc.path)) {
        a_cstr(lbl_proj, "project: ");
        call(BEStatusLine, lbl_proj, be->loc.path);
    }

    // Show branches
    if (be->branchc > 0) {
        u8 lbuf[512];
        u8s ls = {lbuf, lbuf + sizeof(lbuf)};
        escfeed(ls, GRAY);
        a_cstr(lbl, "branches: ");
        u8sFeed(ls, lbl);
        for (int i = 0; i < be->branchc; i++) {
            if (i > 0) {
                a_cstr(sep, ", ");
                u8sFeed(ls, sep);
            }
            if (i == 0) {
                a_cstr(star, "*");
                u8sFeed(ls, star);
            }
            RONutf8sFeed(ls, VEROrigin(&be->branches[i]));
        }
        escfeed(ls, 0);
        u8sFeed1(ls, '\n');
        u8cs line = {lbuf, ls[0]};
        call(FILEout, line);
    }

    // Count files and waypoints (scan stat: for lightweight listing)
    a_cstr(sch_stat, BE_SCHEME_STAT);
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(u8sFeed, pfx, sch_stat);
    u8sFeed1(pfx, ':');
    call(u8sFeed, pfx, be->loc.path);
    u8sFeed1(pfx, '/');
    u8cs prefix = {pfxbuf, pfx[0]};

    BEStatusCtx cctx = {0, 0, $len(prefix)};
    call(ROCKScan, &be->db, prefix, BEStatusCB, &cctx);

    u8 cbuf[256];
    u8s cs = {cbuf, cbuf + sizeof(cbuf)};
    escfeed(cs, GRAY);
    int clen = snprintf((char *)cs[0], (size_t)(cs[1] - cs[0]),
                        "base files: %d, waypoints: %d",
                        cctx.base_count, cctx.wp_count);
    cs[0] += clen;
    escfeed(cs, 0);
    u8sFeed1(cs, '\n');
    u8cs cline = {cbuf, cs[0]};
    call(FILEout, cline);
    call(BEStatusFiles, be);
    done;
}

// ---- verb: post ----
static ok64 BECLIPost(uricp u) {
    sane(1);

    BE be = {};
    if (!$empty(u->host)) {
        if ($empty(u->path)) {
            // //newrepo — checkpoint (fork) into new depot
            call(BEOpenCwd, &be);
            u8cs repo = {};
            $mv(repo, u->host);
            call(BECheckpoint, &be, repo);
        } else {
            // //repo/project — init new project
            a_pad(u8, cpath, FILE_PATH_MAX_LEN);
            call(BECwd, path8gIn(cpath));
            u8cs udata = {};
            $mv(udata, u->data);
            call(BEInit, &be, udata, path8cgIn(cpath));
            u8cs empty = {};
            call(BEPost, &be, 0, NULL, empty);
        }
    } else {
        call(BEOpenCwd, &be);
        u8cs empty = {};
        if (!$empty(u->path)) {
            u8cs paths[1] = {};
            $mv(paths[0], u->path);
            call(BEPost, &be, 1, paths, empty);
        } else {
            call(BEPost, &be, 0, NULL, empty);
        }
    }
    call(BEClose, &be);
    done;
}

// ---- verb: get ----
static ok64 BECLIGet(uricp u) {
    sane(1);

    a_cstr(std_scheme, "std");
    a_cstr(http_scheme, "http");
    a_cstr(https_scheme, "https");

    // Remote clone
    if ($eq(u->scheme, http_scheme) || $eq(u->scheme, https_scheme)) {
        a_pad(u8, cpath, FILE_PATH_MAX_LEN);
        call(BECwd, path8gIn(cpath));
        u8cs udata = {};
        $mv(udata, u->data);
        call(BESyncClone, udata, path8cgIn(cpath));
        done;
    }

    // //repo/project — local depot checkout
    if (!$empty(u->host) && !$empty(u->path)) {
        u8cs projname = {};
        path8gBase(projname, (path8cg){u->path[0], u->path[1], u->path[1]});
        test(!$empty(projname), BEBAD);
        a_pad(u8, cpath, FILE_PATH_MAX_LEN);
        call(BECwd, path8gIn(cpath));
        call(path8gPush, path8gIn(cpath), projname);
        call(FILEMakeDir, path8cgIn(cpath));
        BE be = {};
        u8cs udata = {};
        $mv(udata, u->data);
        call(BEInit, &be, udata, path8cgIn(cpath));
        u8cs empty = {};
        call(BEGet, &be, 0, NULL, empty);
        call(BEClose, &be);
        done;
    }

    BE be = {};
    call(BEOpenCwd, &be);

    // std: scheme → output to stdout
    if ($eq(u->scheme, std_scheme)) {
        be.to_stdout = YES;
    }

    u8cs branch = {};
    $mv(branch, u->query);

    if (!$empty(u->path)) {
        u8cs paths[1] = {};
        $mv(paths[0], u->path);
        call(BEGet, &be, 1, paths, branch);
    } else {
        call(BEGet, &be, 0, NULL, branch);
        call(BEGetDeps, &be, NO);
    }
    call(BEClose, &be);
    done;
}

// ---- verb: deps ----
static ok64 BECLIDeps(uricp u) {
    sane(1);
    BE be = {};
    call(BEOpenCwd, &be);
    b8 include_opt = NO;
    a_cstr(all, "all");
    if ($eq(u->path, all)) include_opt = YES;
    call(BEGetDeps, &be, include_opt);
    call(BEClose, &be);
    done;
}

// ---- verb: put ----
static ok64 BECLIPut(uricp u) {
    sane(1);
    BE be = {};
    call(BEOpenCwd, &be);
    u8cs branch = {};
    if (!$empty(u->query)) {
        $mv(branch, u->query);
    } else {
        $mv(branch, u->path);
    }
    test($ok(branch) && !$empty(branch), BEBAD);
    u8cs empty = {};
    call(BEPut, &be, branch, empty);
    call(BEClose, &be);
    done;
}

// ---- verb: delete ----
static ok64 BECLIDelete(uricp u) {
    sane(1);
    u8cs target = {};
    if (!$empty(u->query)) {
        $mv(target, u->query);
    } else {
        $mv(target, u->path);
    }
    test(!$empty(target), BEBAD);
    BE be = {};
    call(BEOpenCwd, &be);
    call(BEDelete, &be, target);
    call(BEClose, &be);
    done;
}

// ---- verb: come (add/switch branch) ----
static ok64 BECLICome(uricp u) {
    sane(1);
    BE be = {};
    call(BEOpenCwd, &be);
    u8cs branch = {};
    if (!$empty(u->query)) {
        $mv(branch, u->query);
    } else {
        $mv(branch, u->path);
    }
    test($ok(branch) && !$empty(branch), BEBAD);
    call(BESetActive, &be, branch);
    call(BEClose, &be);
    done;
}

// ---- verb: lay ----
static ok64 BECLILay(uricp u) {
    sane(1);
    BE be = {};
    call(BEOpenCwd, &be);
    u8cs empty = {};
    call(BEPost, &be, 0, NULL, empty);
    call(BEClose, &be);
    done;
}

// ---- verb: mark (milestone) ----
static ok64 BECLIMark(uricp u) {
    sane(1);
    BE be = {};
    call(BEOpenCwd, &be);
    u8cs name = {};
    $mv(name, u->path);
    call(BEMilestone, &be, name);
    call(BEClose, &be);
    done;
}

// ---- verb: grep ----
static ok64 BEGrepCB(voidp arg, u8cs filepath, int lineno, u8cs line) {
    ok64 __ = OK;
    call(FILEout, filepath);
    u8 nbuf[16];
    int nlen = snprintf((char *)nbuf, sizeof(nbuf), ":%d:", lineno);
    u8cs ns = {nbuf, nbuf + nlen};
    call(FILEout, ns);
    call(FILEout, line);
    call(FILEout, NL);
    return OK;
}

static ok64 BECLIGrep(uricp u) {
    sane(1);
    BE be = {};
    call(BEOpenCwd, &be);

    // If no fragment, treat path as the search text (bare word)
    uri gu = *u;
    if ($empty(gu.fragment) && !$empty(gu.path)) {
        $mv(gu.fragment, gu.path);
        gu.path[0] = gu.path[1] = NULL;
    }

    call(BEGrep, &be, &gu, BEGrepCB, NULL);
    call(BEClose, &be);
    done;
}

// ---- verb: diff ----
static ok64 BECLIDiff(uricp u) {
    sane(1);
    BE be = {};
    call(BEOpenCwd, &be);

    u8cs *file_args_p = NULL;
    u8cs file_args[1] = {};
    int filec = 0;
    if (!$empty(u->path)) {
        $mv(file_args[0], u->path);
        file_args_p = file_args;
        filec = 1;
    }

    FILE *pager = NULL;
    int saved_stdout = -1;
    if (isatty(STDOUT_FILENO)) {
        char const *cmd = getenv("PAGER");
        if (cmd == NULL) cmd = "less -R";
        pager = popen(cmd, "w");
        if (pager != NULL) {
            saved_stdout = dup(STDOUT_FILENO);
            dup2(fileno(pager), STDOUT_FILENO);
        }
    }

    ok64 o = BEDiffFiles(&be, filec, file_args_p);

    if (pager != NULL) {
        fflush(stdout);
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
        pclose(pager);
    }

    test(o == OK, o);
    call(BEClose, &be);
    done;
}

// ---- verb: cat (syntax-highlighted file view) ----
static ok64 BECLICat(uricp u) {
    sane(1);
    test(!$empty(u->path), BEBAD);
    BE be = {};
    call(BEOpenCwd, &be);

    u8cs relpath = {};
    $mv(relpath, u->path);

    u8bp result = be.scratch[BE_READ];
    u8bReset(result);
    BEmeta meta = {};
    call(BEGetFileMerged, &be, be.loc.path, relpath, result, &meta);

    u8cs bason = {result[1], result[2]};

    u8bp out = be.scratch[BE_RENDER];
    u8bReset(out);
    aBpad(u64, stk, 256);

    b8 use_color = isatty(STDOUT_FILENO) || getenv("BE_COLOR") != NULL;

    if (use_color) {
        call(BASTCat, u8bIdle(out), stk, bason);
    } else {
        call(BASTExport, u8bIdle(out), stk, bason);
    }

    u8cs source = {out[1], out[2]};

    FILE *pager = NULL;
    int saved_stdout = -1;
    if (use_color) {
        char const *cmd = getenv("PAGER");
        if (cmd == NULL) cmd = "less -R";
        pager = popen(cmd, "w");
        if (pager != NULL) {
            saved_stdout = dup(STDOUT_FILENO);
            dup2(fileno(pager), STDOUT_FILENO);
        }
    }

    if (!$empty(source)) {
        fwrite(source[0], 1, $len(source), stdout);
    }

    if (pager != NULL) {
        fflush(stdout);
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
        pclose(pager);
    }

    call(BEClose, &be);
    done;
}

// ---- verb: fit (merge branch into main) ----
static ok64 BECLIFit(uricp u) {
    sane(1);
    BE be = {};
    call(BEOpenCwd, &be);
    u8cs source = {};
    if (!$empty(u->query)) {
        $mv(source, u->query);
    } else {
        $mv(source, u->path);
    }
    test($ok(source) && !$empty(source), BEBAD);
    u8cs empty = {};
    call(BEPut, &be, source, empty);
    call(BEClose, &be);
    done;
}

ok64 becli() {
    sane(1);
    call(FILEInit);

    int argc = $arglen;

    if (argc < 2) {
        BE be = {};
        call(BEOpenCwd, &be);
        call(BEStatus, &be);
        call(BEClose, &be);
        done;
    }

    a$rg(verb, 1);

    // Parse arg[2] as URI (URIutf8Drain consumes data, so restore it)
    uri u = {};
    if (argc > 2) {
        a$rg(arg, 2);
        ok64 uo = URIutf8Drain(arg, &u);
        if (uo == OK) {
            $mv(u.data, arg);
        } else {
            zerop(&u);
            $mv(u.data, arg);
            $mv(u.path, arg);
        }
    }

    a_cstr(v_post, "post");
    a_cstr(v_get, "get");
    a_cstr(v_put, "put");
    a_cstr(v_delete, "delete");
    a_cstr(v_come, "come");
    a_cstr(v_lay, "lay");
    a_cstr(v_mark, "mark");
    a_cstr(v_fit, "fit");
    a_cstr(v_deps, "deps");
    a_cstr(v_grep, "grep");
    a_cstr(v_diff, "diff");
    a_cstr(v_cat, "cat");

    if ($eq(verb, v_post)) {
        call(BECLIPost, &u);
    } else if ($eq(verb, v_get)) {
        call(BECLIGet, &u);
    } else if ($eq(verb, v_put)) {
        call(BECLIPut, &u);
    } else if ($eq(verb, v_delete)) {
        call(BECLIDelete, &u);
    } else if ($eq(verb, v_come)) {
        call(BECLICome, &u);
    } else if ($eq(verb, v_lay)) {
        call(BECLILay, &u);
    } else if ($eq(verb, v_mark)) {
        call(BECLIMark, &u);
    } else if ($eq(verb, v_fit)) {
        call(BECLIFit, &u);
    } else if ($eq(verb, v_deps)) {
        call(BECLIDeps, &u);
    } else if ($eq(verb, v_grep)) {
        call(BECLIGrep, &u);
    } else if ($eq(verb, v_diff)) {
        call(BECLIDiff, &u);
    } else if ($eq(verb, v_cat)) {
        call(BECLICat, &u);
    } else {
        a_cstr(err, "unknown verb: ");
        call(FILEerr, err);
        call(FILEerr, verb);
        call(FILEerr, NL);
        fail(BEBAD);
    }

    done;
}

MAIN(becli);
