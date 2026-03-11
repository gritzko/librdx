#include "BE.h"
#include "BESRV.h"

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/NET.h"
#include "abc/PRO.h"
#include "abc/TCP.h"
#include "abc/TEST.h"
#include "abc/URI.h"
#include "json/TLKV.h"

// Server thread: runs POL loop until stopped
typedef struct {
    BESRVctx srv;
    BEp be;
    int port;
    ok64 result;
} SrvThread;

static void *srv_thread_fn(void *arg) {
    SrvThread *t = (SrvThread *)arg;
    t->result = BESRVRun(&t->srv);
    return NULL;
}

// Helper: create temp worktree, init repo, post files
static ok64 TestSetupRepo(BE *be, u8p work_pp[4], u8cs be_uri,
                           int filec, u8cs *filenames, u8cs *contents) {
    sane(be != NULL && work_pp != NULL);
    call(path8bAlloc, work_pp);
    a_cstr(tmp_base, "/tmp");
    call(u8sFeed, u8bIdle(work_pp), tmp_base);
    call(path8gTerm, path8gIn(work_pp));
    a_cstr(tmpl, "BESRVtest_XXXXXX");
    call(path8gAddTmp, path8gIn(work_pp), tmpl);
    call(FILEMakeDir, path8cgIn(work_pp));

    for (int i = 0; i < filec; i++) {
        a_path(fpath, "");
        call(path8gDup, path8gIn(fpath), path8cgIn(work_pp));
        call(path8gPush, path8gIn(fpath), filenames[i]);
        int fd = -1;
        call(FILECreate, &fd, path8cgIn(fpath));
        call(FILEFeedall, fd, contents[i]);
        call(FILEClose, &fd);
    }

    call(BEInit, be, be_uri, path8cgIn(work_pp));
    u8cs msg = $u8str("test commit");
    call(BEPost, be, filec, filenames, msg);
    done;
}

static int test_port_offset = 0;
static int TestPort() { return NETRandomPort() + test_port_offset++; }

// Helper: start server on a thread, return port
static ok64 TestStartSrv(SrvThread *st, BEp be, pthread_t *tid) {
    sane(st != NULL && be != NULL);
    st->port = TestPort();
    call(BESRVInit, &st->srv, be, st->port);
    pthread_create(tid, NULL, srv_thread_fn, st);
    usleep(100000);
    done;
}

// Helper: stop server, cleanup repo
static ok64 TestCleanup(SrvThread *st, pthread_t tid,
                        BEp be, u8p work_pp[4]) {
    sane(st != NULL);
    BESRVStop(&st->srv);
    pthread_join(tid, NULL);
    BESRVFree(&st->srv);
    a_path(repo_path, "");
    call(path8gDup, path8gIn(repo_path), path8cgIn(be->repo_pp));
    call(BEClose, be);
    call(FILErmrf, path8cgIn(work_pp));
    call(FILErmrf, path8cgIn(repo_path));
    path8bFree(work_pp);
    done;
}

// Helper: HTTP GET, return response body (headers stripped)
static ok64 TestHTTPGet(u8bp buf, int port, const char *path) {
    sane(buf != NULL && path != NULL);
    char addr[64];
    snprintf(addr, sizeof(addr), "tcp://127.0.0.1:%d", port);
    a_cstr(addrcs, addr);
    int fd = -1;
    call(TCPConnect, &fd, addrcs, NO);

    char req[512];
    int rlen = snprintf(req, sizeof(req),
                        "GET %s HTTP/1.1\r\n"
                        "Host: localhost\r\n"
                        "Connection: close\r\n"
                        "\r\n",
                        path);
    u8cs reqcs = {(u8cp)req, (u8cp)req + rlen};
    call(FILEFeedall, fd, reqcs);

    u8 tmp[4096];
    for (;;) {
        ssize_t n = read(fd, tmp, sizeof(tmp));
        if (n <= 0) break;
        u8cs chunk = {tmp, tmp + n};
        call(u8bFeed, buf, chunk);
    }
    close(fd);

    u8cp d0 = buf[1], d1 = buf[2];
    for (u8cp p = d0; p + 3 < d1; p++) {
        if (p[0] == '\r' && p[1] == '\n' && p[2] == '\r' && p[3] == '\n') {
            size_t body_off = (p + 4) - d0;
            u8cs empty = {};
            call(u8bSplice, buf, 0, body_off, empty);
            done;
        }
    }
    fail(BESRVFAIL);
}

// Helper: HTTP GET, return full response (headers included)
static ok64 TestHTTPGetRaw(u8bp buf, int port, const char *path) {
    sane(buf != NULL && path != NULL);
    char addr[64];
    snprintf(addr, sizeof(addr), "tcp://127.0.0.1:%d", port);
    a_cstr(addrcs, addr);
    int fd = -1;
    call(TCPConnect, &fd, addrcs, NO);

    char req[512];
    int rlen = snprintf(req, sizeof(req),
                        "GET %s HTTP/1.1\r\n"
                        "Host: localhost\r\n"
                        "Connection: close\r\n"
                        "\r\n",
                        path);
    u8cs reqcs = {(u8cp)req, (u8cp)req + rlen};
    call(FILEFeedall, fd, reqcs);

    u8 tmp[4096];
    for (;;) {
        ssize_t n = read(fd, tmp, sizeof(tmp));
        if (n <= 0) break;
        u8cs chunk = {tmp, tmp + n};
        call(u8bFeed, buf, chunk);
    }
    close(fd);
    done;
}

// Helper: check body contains substring
static b8 TestContains(u8cs body, u8cs needle) {
    for (u8cp p = body[0]; p + $len(needle) <= body[1]; p++) {
        if (memcmp(p, needle[0], $len(needle)) == 0) return YES;
    }
    return NO;
}

// ---- Test 1: Basic BASON streaming ----
ok64 BESRVtest1() {
    sane(1);
    u8cs fname = $u8str("hello.c");
    u8cs content = $u8str("int x = 42;\n");
    u8cs be_uri = $u8str("be://BESRVtest1/@test/p1?main");

    BE be = {};
    u8p work_pp[4] = {};
    call(TestSetupRepo, &be, work_pp, be_uri, 1, &fname, &content);

    SrvThread st = {};
    pthread_t tid;
    call(TestStartSrv, &st, &be, &tid);

    u8p resp[4] = {};
    call(u8bAllocate, resp, 1 << 20);
    call(TestHTTPGet, resp, st.port, "/.bason");

    u8cp rd0 = resp[1], rd1 = resp[2];
    want(rd1 > rd0);

    u8cs body = {rd0, rd1};
    u8 type = 0;
    u8cs key = {};
    u8cs val = {};
    ok64 o = TLKVDrain(body, &type, key, val);
    want(o == OK && !$empty(key));

    a_cstr(be_prefix, "be:");
    want(memcmp(key[0], be_prefix[0], $len(be_prefix)) == 0);
    a_cstr(hello, "hello.c");
    want(TestContains(key, hello));

    u8bFree(resp);
    call(TestCleanup, &st, tid, &be, work_pp);
    done;
}

// ---- Test 2: Prefix filtering ----
ok64 BESRVtest2() {
    sane(1);
    u8cs fnames[2] = {$u8str("src/a.c"), $u8str("doc/b.txt")};
    u8cs contents[2] = {$u8str("int a;\n"), $u8str("hello\n")};
    u8cs be_uri = $u8str("be://BESRVtest2/@test/p2?main");

    BE be = {};
    u8p work_pp[4] = {};

    call(path8bAlloc, work_pp);
    a_cstr(tmp_base, "/tmp");
    call(u8sFeed, u8bIdle(work_pp), tmp_base);
    call(path8gTerm, path8gIn(work_pp));
    a_cstr(tmpl, "BESRVt2_XXXXXX");
    call(path8gAddTmp, path8gIn(work_pp), tmpl);
    call(FILEMakeDir, path8cgIn(work_pp));

    a_path(srcdir, "");
    call(path8gDup, path8gIn(srcdir), path8cgIn(work_pp));
    a_cstr(src_name, "src");
    call(path8gPush, path8gIn(srcdir), src_name);
    call(FILEMakeDir, path8cgIn(srcdir));

    a_path(docdir, "");
    call(path8gDup, path8gIn(docdir), path8cgIn(work_pp));
    a_cstr(doc_name, "doc");
    call(path8gPush, path8gIn(docdir), doc_name);
    call(FILEMakeDir, path8cgIn(docdir));

    {
        a_path(fp, "");
        call(path8gDup, path8gIn(fp), path8cgIn(srcdir));
        a_cstr(n, "a.c");
        call(path8gPush, path8gIn(fp), n);
        int fd = -1;
        call(FILECreate, &fd, path8cgIn(fp));
        call(FILEFeedall, fd, contents[0]);
        call(FILEClose, &fd);
    }
    {
        a_path(fp, "");
        call(path8gDup, path8gIn(fp), path8cgIn(docdir));
        a_cstr(n, "b.txt");
        call(path8gPush, path8gIn(fp), n);
        int fd = -1;
        call(FILECreate, &fd, path8cgIn(fp));
        call(FILEFeedall, fd, contents[1]);
        call(FILEClose, &fd);
    }

    call(BEInit, &be, be_uri, path8cgIn(work_pp));
    u8cs msg = $u8str("test");
    call(BEPost, &be, 2, fnames, msg);

    SrvThread st = {};
    pthread_t tid;
    call(TestStartSrv, &st, &be, &tid);

    // BASON prefix filtering
    u8p resp[4] = {};
    call(u8bAllocate, resp, 1 << 20);
    call(TestHTTPGet, resp, st.port, "/src/.bason");

    u8cp rd0 = resp[1], rd1 = resp[2];
    want(rd1 > rd0);

    a_cstr(a_file, "src/a.c");
    a_cstr(b_file, "doc/b.txt");
    u8cs scan = {rd0, rd1};
    b8 found_a = NO, found_b = NO;
    while (!$empty(scan)) {
        u8 t = 0;
        u8cs k = {}, v = {};
        if (TLKVDrain(scan, &t, k, v) != OK) break;
        if (TestContains(k, a_file)) found_a = YES;
        if (TestContains(k, b_file)) found_b = YES;
    }
    want(found_a);
    want(!found_b);

    // DIR listing (reuse same repo with subdirs)
    u8bReset(resp);
    call(TestHTTPGet, resp, st.port, "/");
    rd0 = resp[1];
    rd1 = resp[2];
    want(rd1 > rd0);
    u8cs dirbody = {rd0, rd1};
    a_cstr(src_entry, "src/\n");
    a_cstr(doc_entry, "doc/\n");
    want(TestContains(dirbody, src_entry));
    want(TestContains(dirbody, doc_entry));

    u8bFree(resp);
    call(TestCleanup, &st, tid, &be, work_pp);
    done;
}

// ---- Test 3: Concurrent clients ----
typedef struct {
    u8p resp[4];
    int port;
    const char *path;
    ok64 result;
} ClientThread;

static void *client_thread_fn(void *arg) {
    ClientThread *ct = (ClientThread *)arg;
    ct->result = TestHTTPGet(ct->resp, ct->port, ct->path);
    return NULL;
}

ok64 BESRVtest3() {
    sane(1);
    u8cs fname = $u8str("main.c");
    u8cs content = $u8str("int main() { return 0; }\n");
    u8cs be_uri = $u8str("be://BESRVtest3/@test/p3?main");

    BE be = {};
    u8p work_pp[4] = {};
    call(TestSetupRepo, &be, work_pp, be_uri, 1, &fname, &content);

    SrvThread st = {};
    pthread_t srv_tid;
    call(TestStartSrv, &st, &be, &srv_tid);

    ClientThread c1 = {.port = st.port, .path = "/.bason"};
    ClientThread c2 = {.port = st.port, .path = "/.bason"};
    call(u8bAllocate, c1.resp, 1 << 20);
    call(u8bAllocate, c2.resp, 1 << 20);

    pthread_t t1, t2;
    pthread_create(&t1, NULL, client_thread_fn, &c1);
    usleep(50000);
    pthread_create(&t2, NULL, client_thread_fn, &c2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    want(c1.result == OK && c2.result == OK);
    want(u8bDataLen(c1.resp) > 0 && u8bDataLen(c2.resp) > 0);
    u8cp a0 = c1.resp[1], a1 = c1.resp[2];
    u8cp b0 = c2.resp[1], b1 = c2.resp[2];
    want(a1 - a0 == b1 - b0);
    want(memcmp(a0, b0, a1 - a0) == 0);

    u8bFree(c1.resp);
    u8bFree(c2.resp);
    call(TestCleanup, &st, srv_tid, &be, work_pp);
    done;
}

// ---- Test 4: Empty prefix returns empty body ----
ok64 BESRVtest4() {
    sane(1);
    u8cs fname = $u8str("file.c");
    u8cs content = $u8str("void f() {}\n");
    u8cs be_uri = $u8str("be://BESRVtest4/@test/p4?main");

    BE be = {};
    u8p work_pp[4] = {};
    call(TestSetupRepo, &be, work_pp, be_uri, 1, &fname, &content);

    SrvThread st = {};
    pthread_t tid;
    call(TestStartSrv, &st, &be, &tid);

    u8p resp[4] = {};
    call(u8bAllocate, resp, 1 << 20);
    call(TestHTTPGet, resp, st.port, "/nonexistent/.bason");
    want(u8bDataLen(resp) == 0);

    u8bFree(resp);
    call(TestCleanup, &st, tid, &be, work_pp);
    done;
}

// ---- Test 5: Four modes on single file ----
// .bason, .stat, raw, 404 — all from one server instance
ok64 BESRVtest5() {
    sane(1);
    u8cs fname = $u8str("hello.c");
    u8cs content = $u8str("int x = 42;\n");
    u8cs be_uri = $u8str("be://BESRVtest5/@test/p5?main");

    BE be = {};
    u8p work_pp[4] = {};
    call(TestSetupRepo, &be, work_pp, be_uri, 1, &fname, &content);

    SrvThread st = {};
    pthread_t tid;
    call(TestStartSrv, &st, &be, &tid);

    u8p resp[4] = {};
    call(u8bAllocate, resp, 1 << 20);

    // .bason mode: key starts with "be:", contains "hello.c"
    call(TestHTTPGet, resp, st.port, "/hello.c.bason");
    want(u8bDataLen(resp) > 0);
    u8cs body = {resp[1], resp[2]};
    u8 type = 0;
    u8cs key = {}, val = {};
    want(TLKVDrain(body, &type, key, val) == OK);
    a_cstr(be_pfx, "be:");
    want(memcmp(key[0], be_pfx[0], $len(be_pfx)) == 0);
    a_cstr(hello, "hello.c");
    want(TestContains(key, hello));

    // .stat mode: key starts with "stat:"
    u8bReset(resp);
    call(TestHTTPGet, resp, st.port, "/hello.c.stat");
    want(u8bDataLen(resp) > 0);
    u8cs sbody = {resp[1], resp[2]};
    type = 0;
    u8cs sk = {}, sv = {};
    want(TLKVDrain(sbody, &type, sk, sv) == OK);
    a_cstr(stat_pfx, "stat:");
    want(memcmp(sk[0], stat_pfx[0], $len(stat_pfx)) == 0);

    // RAW mode: returns source text
    u8bReset(resp);
    call(TestHTTPGet, resp, st.port, "/hello.c");
    want(u8bDataLen(resp) > 0);
    u8cs raw = {resp[1], resp[2]};
    a_cstr(expected, "int x = 42;\n");
    want($len(raw) == $len(expected));
    want(memcmp(raw[0], expected[0], $len(expected)) == 0);

    // 404 for nonexistent file
    u8bReset(resp);
    call(TestHTTPGetRaw, resp, st.port, "/nonexistent.c");
    want(u8bDataLen(resp) > 0);
    u8cs rawresp = {resp[1], resp[2]};
    a_cstr(notfound, "404");
    want(TestContains(rawresp, notfound));

    u8bFree(resp);
    call(TestCleanup, &st, tid, &be, work_pp);
    done;
}

// Helper: HTTP POST, return status code
static ok64 TestHTTPPost(int *status, int port, const char *path,
                          u8cs body) {
    sane(status != NULL && path != NULL);
    char addr[64];
    snprintf(addr, sizeof(addr), "tcp://127.0.0.1:%d", port);
    a_cstr(addrcs, addr);
    int fd = -1;
    call(TCPConnect, &fd, addrcs, NO);

    char req[512];
    int rlen = snprintf(req, sizeof(req),
                        "POST %s HTTP/1.1\r\n"
                        "Host: localhost\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n"
                        "\r\n",
                        path, $len(body));
    u8cs reqcs = {(u8cp)req, (u8cp)req + rlen};
    call(FILEFeedall, fd, reqcs);
    if (!$empty(body)) {
        call(FILEFeedall, fd, body);
    }

    // Read response
    u8 tmp[4096];
    u8p resp[4] = {};
    call(u8bAllocate, resp, 1 << 16);
    for (;;) {
        ssize_t n = read(fd, tmp, sizeof(tmp));
        if (n <= 0) break;
        u8cs chunk = {tmp, tmp + n};
        call(u8bFeed, resp, chunk);
    }
    close(fd);

    // Parse status code from "HTTP/1.1 NNN ..."
    u8cp d0 = resp[1];
    u8cp d1 = resp[2];
    *status = 0;
    if (d1 - d0 >= 12) {
        // Skip "HTTP/1.1 " (9 chars), parse 3 digits
        for (int i = 9; i < 12 && d0[i] >= '0' && d0[i] <= '9'; i++) {
            *status = *status * 10 + (d0[i] - '0');
        }
    }
    u8bFree(resp);
    done;
}

// ---- Test 6: HTTP POST roundtrip ----
ok64 BESRVtest6() {
    sane(1);
    u8cs fname = $u8str("hello.c");
    u8cs content = $u8str("int x = 42;\n");
    u8cs be_uri = $u8str("be://BESRVtest6/@test/p6?main");

    BE be = {};
    u8p work_pp[4] = {};
    call(TestSetupRepo, &be, work_pp, be_uri, 1, &fname, &content);

    SrvThread st = {};
    pthread_t tid;
    call(TestStartSrv, &st, &be, &tid);

    // POST new content for hello.c
    u8cs new_content = $u8str("int x = 99;\n");
    int status = 0;
    call(TestHTTPPost, &status, st.port, "/hello.c", new_content);
    want(status == 200);

    // GET it back in RAW mode
    u8p resp[4] = {};
    call(u8bAllocate, resp, 1 << 20);
    call(TestHTTPGet, resp, st.port, "/hello.c");
    want(u8bDataLen(resp) > 0);

    u8cs raw = {resp[1], resp[2]};
    a_cstr(expected, "int x = 99;\n");
    want($len(raw) == $len(expected));
    want(memcmp(raw[0], expected[0], $len(expected)) == 0);

    u8bFree(resp);
    call(TestCleanup, &st, tid, &be, work_pp);
    done;
}

ok64 maintest() {
    sane(1);
    signal(SIGPIPE, SIG_IGN);
    call(FILEInit);
    call(BESRVtest1);
    call(BESRVtest2);
    call(BESRVtest3);
    call(BESRVtest4);
    call(BESRVtest5);
    call(BESRVtest6);
    done;
}

TEST(maintest)
