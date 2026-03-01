#include "BE.h"

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
    if ($ok(be->loc.query) && !$empty(be->loc.query)) {
        a_cstr(lbl, "  branch: ");
        call(FILEout, lbl);
        call(FILEout, be->loc.query);
        call(FILEout, NL);
    }

    // Count files in DB under project prefix
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    call(u8sFeed, pfx, be->loc.path);
    u8sFeed1(pfx, '/');
    u8cs prefix = {pfxbuf, pfx[0]};

    int count = 0;
    ROCKiter it = {};
    call(ROCKIterOpen, &it, &be->db);
    call(ROCKIterSeek, &it, prefix);
    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);
        if ($len(k) < $len(prefix) ||
            memcmp(k[0], prefix[0], $len(prefix)) != 0)
            break;
        // Count only head keys (no '?' in the rest)
        u8cs rest = {k[0] + $len(prefix), k[1]};
        b8 has_q = NO;
        u8cp rp = rest[0];
        while (rp < rest[1]) {
            if (*rp == '?') {
                has_q = YES;
                break;
            }
            rp++;
        }
        if (!has_q) count++;
        call(ROCKIterNext, &it);
    }
    call(ROCKIterClose, &it);

    u8 cbuf[64];
    int clen = snprintf((char *)cbuf, sizeof(cbuf), "  files: %d\n", count);
    u8cs cline = {cbuf, cbuf + clen};
    call(FILEout, cline);
    done;
}

// ---- verb: post ----
static ok64 BECLIPost(int argc) {
    sane(1);
    // Scan args 2..argc-1: last URI arg → init uri; rest → file paths
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
        // Init new repo
        a_pad(u8, cpath, FILE_PATH_MAX_LEN);
        call(BECwd, path8gIn(cpath));
        call(BEInit, &be, uri_arg, path8cgIn(cpath));
        u8cs empty = {};
        call(BEPost, &be, filec, filec > 0 ? file_args : NULL, empty);
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
    BE be = {};
    call(BEOpenCwd, &be);

    u8cs branch = {};
    u8cs file_args[64] = {};
    int filec = 0;

    for (int i = 2; i < argc; i++) {
        a$rg(arg, i);
        if ($len(arg) > 0 && arg[0][0] == '?') {
            // Branch: strip leading '?'
            branch[0] = arg[0] + 1;
            branch[1] = arg[1];
        } else {
            test(filec < 64, BEBAD);
            $mv(file_args[filec], arg);
            filec++;
        }
    }

    call(BEGet, &be, filec, filec > 0 ? file_args : NULL, branch);
    // Also get deps when fetching all files
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

// Rewrite .be file with new branch query
static ok64 BESwitchBranch(BEp be, u8cs branch) {
    sane(be != NULL && $ok(branch) && !$empty(branch));
    // Build new URI in temp buffer to avoid aliasing
    u8 tmp[512];
    u8s uri_s = {tmp, tmp + sizeof(tmp)};
    if ($ok(be->loc.scheme) && !$empty(be->loc.scheme)) {
        call(u8sFeed, uri_s, be->loc.scheme);
        a_cstr(sep, "://");
        call(u8sFeed, uri_s, sep);
    }
    if ($ok(be->loc.host) && !$empty(be->loc.host)) {
        call(u8sFeed, uri_s, be->loc.host);
    }
    if ($ok(be->loc.path) && !$empty(be->loc.path)) {
        if (be->loc.path[0][0] != '/') u8sFeed1(uri_s, '/');
        call(u8sFeed, uri_s, be->loc.path);
    }
    a_cstr(qy, "?y=");
    call(u8sFeed, uri_s, qy);
    call(u8sFeed, uri_s, branch);
    size_t ulen = uri_s[0] - tmp;
    test(ulen < sizeof(be->loc_buf), BEBAD);
    memcpy(be->loc_buf, tmp, ulen);
    be->loc.data[0] = be->loc_buf;
    be->loc.data[1] = be->loc_buf + ulen;
    call(URILexer, &be->loc);

    // Rewrite .be file
    u8 dbuf[FILE_PATH_MAX_LEN];
    path8 dpath = {dbuf, dbuf, dbuf, dbuf + FILE_PATH_MAX_LEN};
    call(path8gDup, path8gIn(dpath), path8cgIn(be->work_pp));
    a_cstr(dotbe, ".be");
    call(path8gPush, path8gIn(dpath), dotbe);
    u8cs new_uri = {be->loc_buf, be->loc_buf + ulen};
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(dpath));
    call(FILEFeedall, fd, new_uri);
    call(FILEClose, &fd);
    done;
}

// ---- verb: come ----
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

    // Post current state to head
    u8cs empty = {};
    call(BEPost, &be, 0, NULL, empty);
    // Switch .be to new branch
    call(BESwitchBranch, &be, branch);
    // Post again to populate branch keys
    call(BEPost, &be, 0, NULL, empty);
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

// ---- verb: mark ----
static ok64 BECLIMark(int argc) {
    sane(1);
    test(argc > 2, BEBAD);
    BE be = {};
    call(BEOpenCwd, &be);
    a$rg(message, 2);
    call(BEPost, &be, 0, NULL, message);
    call(BEClose, &be);
    done;
}

// ---- verb: fit ----
static ok64 BECLIFit() {
    sane(1);
    BE be = {};
    call(BEOpenCwd, &be);
    // Extract current branch from loc.query
    test($ok(be.loc.query) && !$empty(be.loc.query), BEBAD);
    u8cs q = {be.loc.query[0], be.loc.query[1]};
    // query is "y=<branch>", skip "y="
    test($len(q) > 2 && q[0][0] == 'y' && q[0][1] == '=', BEBAD);
    u8cs branch = {q[0] + 2, q[1]};
    u8cs empty = {};
    call(BEPut, &be, branch, empty);
    call(BEClose, &be);
    done;
}

ok64 becli() {
    sane(1);
    call(FILEInit);

    int argc = $arglen;

    if (argc < 2) {
        // No verb: status
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
        call(BECLIFit);
    } else if ($eq(verb, v_deps)) {
        call(BECLIDeps, argc);
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
