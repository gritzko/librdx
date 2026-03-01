#include "BE.h"
#include "BESRV.h"
#include "BESYNC.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/NET.h"
#include "abc/PRO.h"
#include "abc/TEST.h"

// Server thread arg
typedef struct {
    BESRVctx srv;
    BEp be;
    int port;
    ok64 result;
} SrvThread;

static void *besync_srv_thread(void *arg) {
    SrvThread *t = (SrvThread *)arg;
    t->result = BESRVRun(&t->srv);
    return NULL;
}

// ---- Test 1: Clone roundtrip ----
ok64 BESYNCtest1() {
    sane(1);

    // Create temp worktree for source repo
    a_path(src_work, "/tmp");
    a_cstr(tmpl1, "BESYNCsrc_XXXXXX");
    call(path8gAddTmp, path8gIn(src_work), tmpl1);
    call(FILEMakeDir, path8cgIn(src_work));

    // Create a test source file
    a_path(fpath, "");
    call(path8gDup, path8gIn(fpath), path8cgIn(src_work));
    a_cstr(fname, "hello.c");
    call(path8gPush, path8gIn(fpath), fname);
    u8cs source = $u8str("int hello = 42;\n");
    int fd = 0;
    call(FILECreate, &fd, path8cgIn(fpath));
    call(FILEFeedall, fd, source);
    call(FILEClose, &fd);

    // Init source repo and POST
    BE src_be = {};
    u8cs uri = $u8str("be://BESYNCtest1/@test/proj");
    call(BEInit, &src_be, uri, path8cgIn(src_work));
    u8cs relpath = $u8str("hello.c");
    u8cs *paths = &relpath;
    u8cs msg = $u8str("initial");
    call(BEPost, &src_be, 1, paths, msg);

    // Start server on random port
    int port = NETRandomPort();
    SrvThread srv_t = {};
    srv_t.be = &src_be;
    srv_t.port = port;
    call(BESRVInit, &srv_t.srv, &src_be, port);

    pthread_t tid;
    pthread_create(&tid, NULL, besync_srv_thread, &srv_t);

    // Give server time to start
    usleep(100000);

    // Create temp worktree for clone destination
    a_path(dst_work, "/tmp");
    a_cstr(tmpl2, "BESYNCdst_XXXXXX");
    call(path8gAddTmp, path8gIn(dst_work), tmpl2);
    call(FILEMakeDir, path8cgIn(dst_work));

    // Build remote URL
    char url[128];
    snprintf(url, sizeof(url), "http://127.0.0.1:%d", port);
    u8cs remote = $u8str(url);

    // Clone
    ok64 co = BESyncClone(remote, path8cgIn(dst_work));

    // Stop server
    BESRVStop(&srv_t.srv);
    pthread_join(tid, NULL);
    BESRVFree(&srv_t.srv);

    want(co == OK);

    // Verify: open cloned DB and check for the key
    a_path(clone_repo, "");
    const char *home = getenv("HOME");
    want(home != NULL);
    a_cstr(homecs, home);
    call(u8sFeed, u8bIdle(clone_repo), homecs);
    call(path8gTerm, path8gIn(clone_repo));
    a_cstr(dotbe, ".be");
    call(path8gPush, path8gIn(clone_repo), dotbe);
    u8 rname[64];
    int rnlen = snprintf((char *)rname, sizeof(rname), "127.0.0.1.%d", port);
    u8cs repo_name = {rname, rname + rnlen};
    call(path8gPush, path8gIn(clone_repo), repo_name);

    ROCKdb clonedb = {};
    ok64 ro = ROCKOpenRO(&clonedb, path8cgIn(clone_repo));
    want(ro == OK);

    // Check for the head key
    u8 kbuf[512];
    u8s key = {kbuf, kbuf + sizeof(kbuf)};
    u8cs proj = $u8str("/@test/proj");
    call(BEKeyHead, key, proj, relpath);
    u8cs head_key = {kbuf, key[0]};
    aBpad(u8, vbuf, 65536);
    ok64 go = ROCKGet(&clonedb, vbuf, head_key);
    want(go == OK);

    call(ROCKClose, &clonedb);

    // Cleanup
    a_path(src_repo, "");
    call(path8gDup, path8gIn(src_repo), path8cgIn(src_be.repo_pp));
    call(BEClose, &src_be);
    call(FILErmrf, path8cgIn(src_work));
    call(FILErmrf, path8cgIn(src_repo));
    call(FILErmrf, path8cgIn(dst_work));
    call(FILErmrf, path8cgIn(clone_repo));

    done;
}

// ---- Test 2: BESyncIsRemote ----
ok64 BESYNCtest2() {
    sane(1);

    u8cs http_url = $u8str("http://example.com:8080");
    want(BESyncIsRemote(http_url) == YES);

    u8cs https_url = $u8str("https://example.com");
    want(BESyncIsRemote(https_url) == YES);

    u8cs be_uri = $u8str("be://repo/@proj");
    want(BESyncIsRemote(be_uri) == NO);

    u8cs local = $u8str("./some/path");
    want(BESyncIsRemote(local) == NO);

    u8cs empty = {};
    want(BESyncIsRemote(empty) == NO);

    done;
}

ok64 maintest() {
    sane(1);
    call(FILEInit);
    call(BESYNCtest2);
    call(BESYNCtest1);
    done;
}

TEST(maintest)
