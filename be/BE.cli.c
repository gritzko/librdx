#include "BE.h"
#include "BESYNC.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/PRO.h"

// Detect URI vs file path: starts with "be://", "//", or "?"
static b8 BEIsURI(u8cs arg) {
    if (!$ok(arg) || $empty(arg)) return NO;
    u8cp p = arg[0];
    if (*p == '?') return YES;
    if (*p == '/' && $len(arg) > 1 && p[1] == '/') return YES;
    a_cstr(scheme, "be://");
    if ($len(arg) >= $len(scheme) && memcmp(p, scheme[0], $len(scheme)) == 0)
        return YES;
    return NO;
}

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

// Print status info
static ok64 BEStatus(BEp be) {
    sane(be != NULL);
    a_cstr(hdr, "beagle: ");
    call(FILEout, hdr);
    call(FILEout, be->loc.data);
    call(FILEout, NL);

    if ($ok(be->loc.host) && !$empty(be->loc.host)) {
        a_cstr(lbl, "  repo: ");
        call(FILEout, lbl);
        call(FILEout, be->loc.host);
        call(FILEout, NL);
    }
    if ($ok(be->loc.path) && !$empty(be->loc.path)) {
        a_cstr(lbl, "  project: ");
        call(FILEout, lbl);
        call(FILEout, be->loc.path);
        call(FILEout, NL);
    }

    // Show branches
    if (be->branchc > 0) {
        a_cstr(lbl, "  branches: ");
        call(FILEout, lbl);
        for (int i = 0; i < be->branchc; i++) {
            if (i > 0) {
                a_cstr(sep, ", ");
                call(FILEout, sep);
            }
            if (i == 0) {
                a_cstr(star, "*");
                call(FILEout, star);
            }
            call(FILEout, be->branches[i]);
        }
        call(FILEout, NL);
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

    u8 cbuf[128];
    int clen = snprintf((char *)cbuf, sizeof(cbuf),
                        "  base files: %d, waypoints: %d\n",
                        cctx.base_count, cctx.wp_count);
    u8cs cline = {cbuf, cbuf + clen};
    call(FILEout, cline);
    call(BEStatusFiles, be);
    done;
}

// ---- verb: post ----
static ok64 BECLIPost(int argc) {
    sane(1);
    u8cs uri_arg = {};
    u8cs file_args[64] = {};
    int filec = 0;

    for (int i = 2; i < argc; i++) {
        a$rg(arg, i);
        if (BEIsURI(arg)) {
            $mv(uri_arg, arg);
        } else {
            test(filec < 64, BEBAD);
            $mv(file_args[filec], arg);
            filec++;
        }
    }

    BE be = {};
    if ($ok(uri_arg) && !$empty(uri_arg)) {
        // Parse URI to check if host-only (checkpoint) or has path (init)
        u8 ubuf[512];
        size_t ulen = $len(uri_arg);
        test(ulen < sizeof(ubuf), BEBAD);
        memcpy(ubuf, uri_arg[0], ulen);
        uri u = {};
        u.data[0] = ubuf;
        u.data[1] = ubuf + ulen;
        call(URILexer, &u);

        if ($empty(u.path)) {
            // //newrepo — checkpoint (fork) into new depot
            call(BEOpenCwd, &be);
            call(BECheckpoint, &be, u.host);
        } else {
            // //repo/project — init new project
            a_pad(u8, cpath, FILE_PATH_MAX_LEN);
            call(BECwd, path8gIn(cpath));
            call(BEInit, &be, uri_arg, path8cgIn(cpath));
            u8cs empty = {};
            call(BEPost, &be, filec, filec > 0 ? file_args : NULL, empty);
        }
    } else {
        call(BEOpenCwd, &be);
        u8cs empty = {};
        call(BEPost, &be, filec, filec > 0 ? file_args : NULL, empty);
    }
    call(BEClose, &be);
    done;
}

// ---- verb: get ----
static ok64 BECLIGet(int argc) {
    sane(1);

    if (argc > 2) {
        a$rg(first_arg, 2);
        if (BESyncIsRemote(first_arg)) {
            a_pad(u8, cpath, FILE_PATH_MAX_LEN);
            call(BECwd, path8gIn(cpath));
            call(BESyncClone, first_arg, path8cgIn(cpath));
            done;
        }
        // //repo/project — local depot checkout
        if ($len(first_arg) > 2 &&
            first_arg[0][0] == '/' && first_arg[0][1] == '/') {
            // Extract project name from URI path for directory
            u8 ubuf[512];
            size_t ulen = $len(first_arg);
            test(ulen < sizeof(ubuf), BEBAD);
            memcpy(ubuf, first_arg[0], ulen);
            uri u = {};
            u.data[0] = ubuf;
            u.data[1] = ubuf + ulen;
            call(URILexer, &u);
            u8cs projname = {};
            path8gBase(projname, (path8cg){u.path[0], u.path[1], u.path[1]});
            test(!$empty(projname), BEBAD);
            // Create project directory and cd into it
            a_pad(u8, cpath, FILE_PATH_MAX_LEN);
            call(BECwd, path8gIn(cpath));
            call(path8gPush, path8gIn(cpath), projname);
            call(FILEMakeDir, path8cgIn(cpath));
            BE be = {};
            call(BEInit, &be, first_arg, path8cgIn(cpath));
            u8cs empty = {};
            call(BEGet, &be, 0, NULL, empty);
            call(BEClose, &be);
            done;
        }
    }

    BE be = {};
    call(BEOpenCwd, &be);

    u8cs branch = {};
    u8cs file_args[64] = {};
    int filec = 0;

    for (int i = 2; i < argc; i++) {
        a$rg(arg, i);
        if ($len(arg) > 0 && arg[0][0] == '?') {
            branch[0] = arg[0] + 1;
            branch[1] = arg[1];
        } else {
            test(filec < 64, BEBAD);
            $mv(file_args[filec], arg);
            filec++;
        }
    }

    call(BEGet, &be, filec, filec > 0 ? file_args : NULL, branch);
    if (filec == 0) {
        call(BEGetDeps, &be, NO);
    }
    call(BEClose, &be);
    done;
}

// ---- verb: deps ----
static ok64 BECLIDeps(int argc) {
    sane(1);
    BE be = {};
    call(BEOpenCwd, &be);
    b8 include_opt = NO;
    if (argc > 2) {
        a$rg(arg, 2);
        a_cstr(all, "all");
        if ($eq(arg, all)) include_opt = YES;
    }
    call(BEGetDeps, &be, include_opt);
    call(BEClose, &be);
    done;
}

// ---- verb: put ----
static ok64 BECLIPut(int argc) {
    sane(1);
    BE be = {};
    call(BEOpenCwd, &be);

    u8cs branch = {};
    if (argc > 2) {
        a$rg(arg, 2);
        if ($len(arg) > 0 && arg[0][0] == '?') {
            branch[0] = arg[0] + 1;
            branch[1] = arg[1];
        } else {
            $mv(branch, arg);
        }
    }
    test($ok(branch) && !$empty(branch), BEBAD);
    u8cs empty = {};
    call(BEPut, &be, branch, empty);
    call(BEClose, &be);
    done;
}

// ---- verb: delete ----
static ok64 BECLIDelete(int argc) {
    sane(1);
    test(argc > 2, BEBAD);
    BE be = {};
    call(BEOpenCwd, &be);
    a$rg(target, 2);
    call(BEDelete, &be, target);
    call(BEClose, &be);
    done;
}

// ---- verb: come (add/switch branch) ----
static ok64 BECLICome(int argc) {
    sane(1);
    test(argc > 2, BEBAD);
    BE be = {};
    call(BEOpenCwd, &be);

    a$rg(arg, 2);
    u8cs branch = {};
    if ($len(arg) > 0 && arg[0][0] == '?') {
        branch[0] = arg[0] + 1;
        branch[1] = arg[1];
    } else {
        $mv(branch, arg);
    }
    test($ok(branch) && !$empty(branch), BEBAD);

    call(BESetActive, &be, branch);
    call(BEClose, &be);
    done;
}

// ---- verb: lay ----
static ok64 BECLILay() {
    sane(1);
    BE be = {};
    call(BEOpenCwd, &be);
    u8cs empty = {};
    call(BEPost, &be, 0, NULL, empty);
    call(BEClose, &be);
    done;
}

// ---- verb: mark (milestone) ----
static ok64 BECLIMark(int argc) {
    sane(1);
    BE be = {};
    call(BEOpenCwd, &be);
    u8cs name = {};
    if (argc > 2) {
        a$rg(arg, 2);
        $mv(name, arg);
    }
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

static ok64 BECLIGrep(int argc) {
    sane(1);
    test(argc > 2, BEBAD);
    BE be = {};
    call(BEOpenCwd, &be);
    a$rg(arg, 2);

    // Parse as URI: bare text → treat as fragment (#substr)
    // Also handles #substr, ?branch#substr, //repo/proj?branch#substr
    u8 ubuf[512];
    u8s us = {ubuf, ubuf + sizeof(ubuf)};
    u8cp a = arg[0];
    if (*a != '#' && *a != '?' && !(*a == '/' && $len(arg) > 1 && a[1] == '/')) {
        // bare text: prepend #
        u8sFeed1(us, '#');
    }
    call(u8sFeed, us, arg);
    uri gu = {};
    gu.data[0] = ubuf;
    gu.data[1] = us[0];
    call(URILexer, &gu);

    call(BEGrep, &be, &gu, BEGrepCB, NULL);
    call(BEClose, &be);
    done;
}

// ---- verb: diff ----
static ok64 BECLIDiff(int argc) {
    sane(1);
    BE be = {};
    call(BEOpenCwd, &be);

    u8cs file_args[64] = {};
    int filec = 0;
    for (int i = 2; i < argc; i++) {
        a$rg(arg, i);
        test(filec < 64, BEBAD);
        $mv(file_args[filec], arg);
        filec++;
    }

    call(BEDiffFiles, &be, filec, filec > 0 ? file_args : NULL);
    call(BEClose, &be);
    done;
}

// ---- verb: fit (merge branch into main) ----
static ok64 BECLIFit(int argc) {
    sane(1);
    BE be = {};
    call(BEOpenCwd, &be);

    u8cs source = {};
    if (argc > 2) {
        a$rg(arg, 2);
        if ($len(arg) > 0 && arg[0][0] == '?') {
            source[0] = arg[0] + 1;
            source[1] = arg[1];
        } else {
            $mv(source, arg);
        }
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

    if ($eq(verb, v_post)) {
        call(BECLIPost, argc);
    } else if ($eq(verb, v_get)) {
        call(BECLIGet, argc);
    } else if ($eq(verb, v_put)) {
        call(BECLIPut, argc);
    } else if ($eq(verb, v_delete)) {
        call(BECLIDelete, argc);
    } else if ($eq(verb, v_come)) {
        call(BECLICome, argc);
    } else if ($eq(verb, v_lay)) {
        call(BECLILay);
    } else if ($eq(verb, v_mark)) {
        call(BECLIMark, argc);
    } else if ($eq(verb, v_fit)) {
        call(BECLIFit, argc);
    } else if ($eq(verb, v_deps)) {
        call(BECLIDeps, argc);
    } else if ($eq(verb, v_grep)) {
        call(BECLIGrep, argc);
    } else if ($eq(verb, v_diff)) {
        call(BECLIDiff, argc);
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
