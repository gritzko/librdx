#include "BESRV.h"

#include <dirent.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/HTTP.h"
#include "abc/PRO.h"
#include "abc/TCP.h"

// Accumulate a file entry into the server's file list
static ok64 BESRVAddFile(BESRVctxp ctx, u8cs filename, u64 size) {
    sane(ctx != NULL);
    test(ctx->filec < 256, BESRVFAIL);
    u8sp idle = u8bIdle(ctx->names_buf);
    u8cp start = *idle;
    call(u8sFeed, idle, filename);
    ctx->files[ctx->filec].name[0] = start;
    ctx->files[ctx->filec].name[1] = *idle;
    ctx->files[ctx->filec].size = size;
    ctx->filec++;
    done;
}

ok64 BESRVInit(BESRVctxp ctx, BEp be, int port) {
    sane(ctx != NULL && be != NULL && port > 0);
    memset(ctx, 0, sizeof(BESRVctx));
    ctx->be = be;
    ctx->listen_fd = -1;

    call(u8bAllocate, ctx->names_buf, 65536);

    // Create a checkpoint for serving (contains all SSTs, MANIFEST, CURRENT)
    call(path8bAlloc, ctx->repo_path_pp);
    call(path8gDup, path8gIn(ctx->repo_path_pp), path8cgIn(be->repo_pp));
    a_cstr(cp_suffix, ".srv");
    call(path8gPush, path8gIn(ctx->repo_path_pp), cp_suffix);

    // Remove existing checkpoint dir if any
    FILErmrf(path8cgIn(ctx->repo_path_pp));
    call(ROCKCheckpoint, &be->db, path8cgIn(ctx->repo_path_pp));

    // Scan checkpoint directory for servable files
    // (using opendir/stat directly because d_type may be DT_UNKNOWN)
    DIR *dir = opendir((const char *)ctx->repo_path_pp[1]);
    test(dir != NULL, BESRVFAIL);
    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (de->d_name[0] == '.') continue;
        a_cstr(lock_n, "LOCK");
        a_cstr(ident_n, "IDENTITY");
        a_cstr(log_pfx, "LOG");
        a_cstr(log_sfx, ".log");
        a_cstr(dname, de->d_name);
        if ($eq(dname, lock_n)) continue;
        if ($eq(dname, ident_n)) continue;
        if ($len(dname) >= $len(log_pfx) &&
            memcmp(dname[0], log_pfx[0], $len(log_pfx)) == 0)
            continue;
        if ($len(dname) > $len(log_sfx) &&
            memcmp(dname[1] - $len(log_sfx), log_sfx[0], $len(log_sfx)) == 0)
            continue;
        a_path(epath, "");
        path8gDup(path8gIn(epath), path8cgIn(ctx->repo_path_pp));
        path8gPush(path8gIn(epath), dname);
        struct stat est;
        ok64 eo = FILEStat(&est, path8cgIn(epath));
        if (eo != OK || !S_ISREG(est.st_mode)) continue;
        BESRVAddFile(ctx, dname, (u64)est.st_size);
    }
    closedir(dir);

    // Listen on port
    char addr[64];
    snprintf(addr, sizeof(addr), "tcp://0.0.0.0:%d", port);
    a_cstr(addrcs, addr);
    call(TCPListen, &ctx->listen_fd, addrcs);

    done;
}

// Send HTTP response with given status, content-type, and body
static ok64 BESRVSendResponse(int fd, int status, const char *content_type,
                               u8cs body) {
    sane(fd >= 0);
    char hdr[512];
    const char *reason = (status == 200) ? "OK" : "Not Found";
    int hlen =
        snprintf(hdr, sizeof(hdr),
                 "HTTP/1.1 %d %s\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %zu\r\n"
                 "Connection: close\r\n"
                 "\r\n",
                 status, reason, content_type, (size_t)$len(body));
    u8cs hdrcs = {(u8cp)hdr, (u8cp)hdr + hlen};
    ok64 o = FILEFeedall(fd, hdrcs);
    if (o != OK) return o;
    if (!$empty(body)) {
        o = FILEFeedall(fd, body);
    }
    return o;
}

// Find file by name in cached list
static int BESRVFindFile(BESRVctxp ctx, u8cs name) {
    for (int i = 0; i < ctx->filec; i++) {
        if ($eq(ctx->files[i].name, name)) return i;
    }
    return -1;
}

// Handle one HTTP request on a connected socket
static ok64 BESRVHandleConn(BESRVctxp ctx, int cfd) {
    sane(ctx != NULL && cfd >= 0);

    // Read request headers
    u8 reqbuf[4096];
    u8s req = {reqbuf, reqbuf};
    for (int i = 0; i < 10; i++) {
        ssize_t n = read(cfd, req[0], reqbuf + sizeof(reqbuf) - req[0]);
        if (n <= 0) break;
        req[0] += n;
        u8cs data = {reqbuf, req[0]};
        for (u8cp p = data[0]; p + 3 < data[1]; p++) {
            if (p[0] == '\r' && p[1] == '\n' && p[2] == '\r' &&
                p[3] == '\n') {
                goto headers_done;
            }
        }
    }
headers_done:;

    u8cs request = {reqbuf, req[0]};
    if ($empty(request)) {
        close(cfd);
        done;
    }

    // Parse HTTP request
    u8cs hdr_pairs[32] = {};
    u8csb hdr_store = {hdr_pairs, hdr_pairs, hdr_pairs, hdr_pairs + 32};
    HTTPstate http = {};
    http.headers = u8csbIdle(hdr_store);
    ok64 o = HTTPutf8Drain(request, &http);
    if (o != OK) {
        a_cstr(err_body, "Bad Request");
        BESRVSendResponse(cfd, 400, "text/plain", err_body);
        close(cfd);
        done;
    }

    a_cstr(get_method, "GET");
    if (!$eq(http.method, get_method)) {
        a_cstr(err_body, "Method Not Allowed");
        BESRVSendResponse(cfd, 405, "text/plain", err_body);
        close(cfd);
        done;
    }

    u8cs uri_path = {http.uri[0], http.uri[1]};

    // Dispatch
    a_cstr(files_prefix, "/_files/");
    a_cstr(files_list, "/_files");
    a_cstr(status_path, "/_status");

    if ($eq(uri_path, files_list)) {
        // Return file listing
        u8 lbuf[16384];
        u8s listing = {lbuf, lbuf + sizeof(lbuf)};
        for (int i = 0; i < ctx->filec; i++) {
            u8sFeed(listing, ctx->files[i].name);
            u8sFeed1(listing, '\n');
        }
        u8cs body = {lbuf, listing[0]};
        BESRVSendResponse(cfd, 200, "text/plain", body);

    } else if ($len(uri_path) > $len(files_prefix) &&
               memcmp(uri_path[0], files_prefix[0], $len(files_prefix)) == 0) {
        // Serve specific file
        u8cs fname = {uri_path[0] + $len(files_prefix), uri_path[1]};

        int idx = BESRVFindFile(ctx, fname);
        if (idx < 0) {
            a_cstr(err_body, "Not Found");
            BESRVSendResponse(cfd, 404, "text/plain", err_body);
        } else {
            a_path(fpath, "");
            call(path8gDup, path8gIn(fpath), path8cgIn(ctx->repo_path_pp));
            call(path8gPush, path8gIn(fpath), fname);

            u64 fsize = ctx->files[idx].size;
            if (fsize == 0) {
                u8cs empty_body = {};
                BESRVSendResponse(cfd, 200, "application/octet-stream",
                                  empty_body);
            } else {
                u8bp mapbuf = NULL;
                o = FILEMapRO(&mapbuf, path8cgIn(fpath));
                if (o != OK) {
                    a_cstr(err_body, "Internal Error");
                    BESRVSendResponse(cfd, 500, "text/plain", err_body);
                } else {
                    u8cp d0 = mapbuf[1], d1 = mapbuf[2];
                    u8cs body = {d0, d1};
                    BESRVSendResponse(cfd, 200, "application/octet-stream",
                                      body);
                    FILEUnMap(mapbuf);
                }
            }
        }

    } else if ($eq(uri_path, status_path)) {
        u8 sbuf[4096];
        int slen = snprintf((char *)sbuf, sizeof(sbuf),
                            "files: %d\n", ctx->filec);
        u8cs body = {sbuf, sbuf + slen};
        BESRVSendResponse(cfd, 200, "text/plain", body);

    } else {
        a_cstr(err_body, "Not Found");
        BESRVSendResponse(cfd, 404, "text/plain", err_body);
    }

    close(cfd);
    done;
}

static volatile b8 besrv_running = YES;

ok64 BESRVRun(BESRVctxp ctx) {
    sane(ctx != NULL && ctx->listen_fd >= 0);
    besrv_running = YES;

    while (besrv_running) {
        struct pollfd pfd = {.fd = ctx->listen_fd, .events = POLLIN};
        int rc = poll(&pfd, 1, 1000);
        if (rc <= 0) continue;
        if (!(pfd.revents & POLLIN)) continue;

        struct sockaddr_storage addr;
        socklen_t alen = sizeof(addr);
        int cfd = accept(ctx->listen_fd, (struct sockaddr *)&addr, &alen);
        if (cfd < 0) continue;

        BESRVHandleConn(ctx, cfd);
    }
    done;
}

ok64 BESRVStop(BESRVctxp ctx) {
    besrv_running = NO;
    return OK;
}

ok64 BESRVFree(BESRVctxp ctx) {
    if (ctx == NULL) return OK;
    if (ctx->listen_fd >= 0) {
        close(ctx->listen_fd);
        ctx->listen_fd = -1;
    }
    if (ctx->repo_path_pp[1] != NULL) {
        FILErmrf(path8cgIn(ctx->repo_path_pp));
    }
    u8bFree(ctx->names_buf);
    path8bFree(ctx->repo_path_pp);
    memset(ctx, 0, sizeof(BESRVctx));
    return OK;
}
