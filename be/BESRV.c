#include "BESRV.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "abc/FILE.h"
#include "abc/HTTP.h"
#include "abc/MIME.h"
#include "abc/POL.h"
#include "abc/PRO.h"
#include "abc/TCP.h"
#include "abc/URI.h"
#include "json/TLKV.h"

// Forward declarations
static short BESRVClientCB(int fd, poller *p);

// Stop pipe callback: when signaled, call POLStop from the POL thread.
// Pipe is non-blocking; timeout delivery may fire this without data.
static short BESRVStopCB(int fd, poller *p) {
    u8 buf[1];
    if (read(fd, buf, 1) == 1) POLStop();
    return 0;
}

static ok64 BESRVClientFree(BEClientp cl) {
    if (cl == NULL) return OK;
    ROCKIterClose(&cl->it);
    if (cl->fd >= 0) {
        POLIgnoreEvents(cl->fd);
        close(cl->fd);
    }
    u8bFree(cl->wbuf);
    free(cl);
    return OK;
}

// Rewrite BASON root key: DB record has empty root key,
// we replace it with the full DB key.
// Input: val = raw BASON blob from DB
// Output: rewritten record fed into wbuf
static ok64 BESRVRewriteRoot(u8bp wbuf, u8cs dbkey, u8cs val) {
    sane(wbuf != NULL && $ok(val) && !$empty(val));
    // Drain the root header to get type + children span
    u8cs from = {val[0], val[1]};
    u8 type = 0;
    u8cs key = {};
    u8cs children = {};
    call(TLKVDrain, from, &type, key, children);
    // Feed new root with dbkey as key and children as val.
    call(TLKVFeed, u8bIdle(wbuf), type, dbkey, children);
    done;
}

// Fill write buffer with next record(s) from iterator
static ok64 BESRVFillBuf(BEClientp cl) {
    sane(cl != NULL);
    u8bReset(cl->wbuf);

    while (ROCKIterValid(&cl->it)) {
        u8cs k = {};
        ROCKIterKey(&cl->it, k);

        // Check prefix boundary
        if ($len(k) < $len(cl->prefix) ||
            memcmp(k[0], cl->prefix[0], $len(cl->prefix)) != 0) {
            cl->iter_done = YES;
            break;
        }

        // Parse the key as URI to classify it
        uri ku = {};
        ok64 o = URIutf8Drain(k, &ku);
        if (o != OK) {
            call(ROCKIterNext, &cl->it);
            continue;
        }

        // Skip fragment keys (commit messages etc)
        if (!$empty(ku.fragment)) {
            call(ROCKIterNext, &cl->it);
            continue;
        }

        // Match base (stamp=0, origin=0) and waypoints uniformly
        ron60 stamp = 0;
        ron60 br_val = 0;
        if (!$empty(ku.query)) {
            ron120 ver = {};
            o = VERParse(&ver, ku.query);
            if (o == OK) {
                stamp = VERTime(&ver);
                br_val = VEROrigin(&ver);
            }
        }
        if (VERFormMatch(cl->formcs, stamp, br_val)) {
            u8cs v = {};
            ROCKIterVal(&cl->it, v);
            if (!$empty(v)) {
                BESRVRewriteRoot(cl->wbuf, k, v);
            }
        }

        call(ROCKIterNext, &cl->it);

        // If buffer has enough data, stop filling to flush
        if (u8bDataLen(cl->wbuf) > BESRV_WBUF_SIZE / 2) break;
    }

    if (!ROCKIterValid(&cl->it)) cl->iter_done = YES;

    u8cp d0 = cl->wbuf[1], d1 = cl->wbuf[2];
    cl->pending[0] = d0;
    cl->pending[1] = d1;
    done;
}

// POLLOUT callback: write pending data, fill more, close when done
static short BESRVClientCB(int fd, poller *p) {
    BEClientp cl = (BEClientp)p->payload;

    // Try to send pending data
    if (!$empty(cl->pending)) {
        ssize_t n = write(fd, cl->pending[0], $len(cl->pending));
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return POLLOUT;
            BESRVClientFree(cl);
            p->callback = NULL;
            return 0;
        }
        cl->pending[0] += n;
        if (!$empty(cl->pending)) return POLLOUT;
    }

    // Pending empty; if iterator done, close
    if (cl->iter_done) {
        BESRVClientFree(cl);
        p->callback = NULL;
        return 0;
    }

    // Fill buffer with more records
    ok64 o = BESRVFillBuf(cl);
    if (o != OK || $empty(cl->pending)) {
        cl->iter_done = YES;
        BESRVClientFree(cl);
        p->callback = NULL;
        return 0;
    }

    return POLLOUT;
}

// Setup streaming for BASON or STATE mode (shared logic)
static short BESRVAcceptStream(BESRVctxp ctx, int cfd, u8cs http_path,
                               ron120cs formcs_in, u8cs scheme) {
    // Allocate per-client state
    BEClientp cl = (BEClientp)calloc(1, sizeof(BEClient));
    if (cl == NULL) {
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }
    cl->fd = cfd;
    cl->iter_done = NO;

    // Copy formula into client-local storage
    size_t fcount = $len(formcs_in);
    if (fcount > VER_MAX) fcount = VER_MAX;
    memcpy(cl->form, formcs_in[0], fcount * sizeof(ron120));
    cl->formcs[0] = cl->form;
    cl->formcs[1] = cl->form + fcount;

    // Build file path, then scan prefix via BEKeyBuild
    u8 fpbuf[256];
    u8s fps = {fpbuf, fpbuf + sizeof(fpbuf)};
    u8sFeed(fps, ctx->be->loc.path);
    u8sFeed1(fps, '/');
    if (!$empty(http_path)) u8sFeed(fps, http_path);
    u8cs fpath = {fpbuf, fps[0]};
    u8s pfx = {cl->pfxbuf, cl->pfxbuf + sizeof(cl->pfxbuf)};
    BEKeyBuild(pfx, scheme, fpath, 0, 0);
    cl->prefix[0] = cl->pfxbuf;
    cl->prefix[1] = pfx[0];

    // Allocate write buffer
    ok64 o = u8bAllocate(cl->wbuf, BESRV_WBUF_SIZE);
    if (o != OK) {
        free(cl);
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    // Open iterator and seek to prefix
    o = ROCKIterOpen(&cl->it, &ctx->be->db);
    if (o != OK) {
        u8bFree(cl->wbuf);
        free(cl);
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }
    o = ROCKIterSeek(&cl->it, cl->prefix);
    if (o != OK) {
        ROCKIterClose(&cl->it);
        u8bFree(cl->wbuf);
        free(cl);
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    // Send HTTP response headers (streaming, no Content-Length)
    a_cstr(resp_hdr,
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: application/x-bason\r\n"
           "Connection: close\r\n"
           "\r\n");
    o = FILEFeedall(cfd, resp_hdr);
    if (o != OK) {
        BESRVClientFree(cl);
        return POLLIN;
    }

    // Set non-blocking for async writes
    int flags = fcntl(cfd, F_GETFL, 0);
    fcntl(cfd, F_SETFL, flags | O_NONBLOCK);

    // Fill initial buffer
    BESRVFillBuf(cl);

    // Register with POL for POLLOUT
    poller client_pol = {
        .callback = BESRVClientCB,
        .payload = cl,
        .tofd = cfd,
        .events = POLLOUT,
    };
    o = POLTrackEvents(cfd, client_pol);
    if (o != OK) {
        BESRVClientFree(cl);
        return POLLIN;
    }

    return POLLIN;
}

// RAW mode: merge file + export to source text, send synchronously
static short BESRVAcceptRaw(BESRVctxp ctx, int cfd, u8cs http_path,
                            ron120cs formcs) {
    u8p merged[4] = {};
    ok64 o = u8bAllocate(merged, 1 << 18);
    if (o != OK) {
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    o = BEMergeFile(&ctx->be->db, ctx->be->loc.path, http_path, formcs,
                    merged);
    if (o != OK) {
        u8bFree(merged);
        a_cstr(err, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    // Export BASON to source text
    u8p outbuf[4] = {};
    o = u8bAllocate(outbuf, 1 << 18);
    if (o != OK) {
        u8bFree(merged);
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    u8cp m0 = merged[1], m1 = merged[2];
    u8cs bason = {m0, m1};
    aBpad(u64, stk, 256);
    o = BASTExport(u8bIdle(outbuf), stk, bason);
    u8bFree(merged);
    if (o != OK) {
        u8bFree(outbuf);
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    // Send HTTP 200 with Content-Length
    u8cp s0 = outbuf[1], s1 = outbuf[2];
    size_t bodylen = (size_t)(s1 - s0);
    const char *mime = MIMEByPath(http_path);
    char hdrbuf[256];
    int hlen = snprintf(hdrbuf, sizeof(hdrbuf),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n"
                        "\r\n",
                        mime, bodylen);
    u8cs hdr = {(u8cp)hdrbuf, (u8cp)hdrbuf + hlen};
    FILEFeedall(cfd, hdr);
    u8cs body = {s0, s1};
    FILEFeedall(cfd, body);
    u8bFree(outbuf);
    close(cfd);
    return POLLIN;
}

// DIR mode: list files under prefix, send synchronously
static short BESRVAcceptDir(BESRVctxp ctx, int cfd, u8cs http_path) {
    // Build file path with trailing /, then prefix via BEKeyBuild
    a_cstr(sch_be, BE_SCHEME_BE);
    u8 fpbuf[256];
    u8s fps = {fpbuf, fpbuf + sizeof(fpbuf)};
    u8sFeed(fps, ctx->be->loc.path);
    u8sFeed1(fps, '/');
    if (!$empty(http_path)) {
        u8sFeed(fps, http_path);
        u8sFeed1(fps, '/');
    }
    u8cs fpath = {fpbuf, fps[0]};
    u8 pfxbuf[512];
    u8s pfx = {pfxbuf, pfxbuf + sizeof(pfxbuf)};
    BEKeyBuild(pfx, sch_be, fpath, 0, 0);
    u8cs prefix = {pfxbuf, pfx[0]};
    size_t pfxlen = $len(prefix);

    u8p outbuf[4] = {};
    ok64 o = u8bAllocate(outbuf, 1 << 16);
    if (o != OK) {
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    // Previous component for dedup
    u8 prevbuf[256];
    u8cs prev = {};

    ROCKiter it = {};
    o = ROCKIterOpen(&it, &ctx->be->db);
    if (o != OK) {
        u8bFree(outbuf);
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }
    o = ROCKIterSeek(&it, prefix);
    if (o != OK) {
        ROCKIterClose(&it);
        u8bFree(outbuf);
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);

        // Check prefix boundary
        if ($len(k) < pfxlen ||
            memcmp(k[0], prefix[0], pfxlen) != 0)
            break;

        // Parse key, skip fragment/waypoint keys (base keys only)
        uri ku = {};
        o = URIutf8Drain(k, &ku);
        if (o != OK || !$empty(ku.fragment) || !$empty(ku.query)) {
            ROCKIterNext(&it);
            continue;
        }

        // Extract the next path component after prefix
        u8cs rest = {k[0] + pfxlen, k[1]};
        if ($empty(rest)) {
            ROCKIterNext(&it);
            continue;
        }

        // Find the next '/' to determine component boundary
        u8cs component = {rest[0], rest[1]};
        b8 is_dir = NO;
        for (u8cp p = rest[0]; p < rest[1]; p++) {
            if (*p == '/' || *p == '?') {
                component[1] = p;
                if (*p == '/') is_dir = YES;
                break;
            }
        }

        if ($empty(component)) {
            ROCKIterNext(&it);
            continue;
        }

        // Dedup: skip if same as previous component
        if (!$empty(prev) && $len(prev) == $len(component) &&
            memcmp(prev[0], component[0], $len(prev)) == 0) {
            ROCKIterNext(&it);
            continue;
        }

        // Save component for dedup (copy to local buffer)
        size_t clen = $len(component);
        if (clen > sizeof(prevbuf)) clen = sizeof(prevbuf);
        memcpy(prevbuf, component[0], clen);
        prev[0] = prevbuf;
        prev[1] = prevbuf + clen;

        // Emit: component + optional "/" + "\n"
        u8sFeed(u8bIdle(outbuf), component);
        if (is_dir) u8sFeed1(u8bIdle(outbuf), '/');
        u8sFeed1(u8bIdle(outbuf), '\n');

        ROCKIterNext(&it);
    }
    ROCKIterClose(&it);

    // Send HTTP 200 with Content-Length
    u8cp d0 = outbuf[1], d1 = outbuf[2];
    size_t bodylen = (size_t)(d1 - d0);
    char hdrbuf[256];
    int hlen = snprintf(hdrbuf, sizeof(hdrbuf),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n"
                        "\r\n",
                        bodylen);
    u8cs hdr = {(u8cp)hdrbuf, (u8cp)hdrbuf + hlen};
    FILEFeedall(cfd, hdr);
    u8cs body = {d0, d1};
    FILEFeedall(cfd, body);
    u8bFree(outbuf);
    close(cfd);
    return POLLIN;
}

// Detect mode from path suffix, strip suffix, return mode
static int BESRVDetectMode(u8cs *path) {
    size_t len = $len(*path);
    a_cstr(bason_sfx, ".bason");
    a_cstr(state_sfx, ".stat");
    if (len >= $len(bason_sfx) &&
        memcmp(path[0][1] - $len(bason_sfx), bason_sfx[0],
               $len(bason_sfx)) == 0) {
        path[0][1] -= $len(bason_sfx);
        return BESRV_MODE_BASON;
    }
    if (len >= $len(state_sfx) &&
        memcmp(path[0][1] - $len(state_sfx), state_sfx[0],
               $len(state_sfx)) == 0) {
        path[0][1] -= $len(state_sfx);
        return BESRV_MODE_STATE;
    }
    if (len == 0 || *(path[0][1] - 1) == '/') {
        // Strip trailing /
        if (len > 0 && *(path[0][1] - 1) == '/') path[0][1]--;
        return BESRV_MODE_DIR;
    }
    return BESRV_MODE_RAW;
}

// POST handler: read body, parse source, post to repo
static short BESRVAcceptPost(BESRVctxp ctx, int cfd, u8cs http_path,
                              u8cs req_query, u8cs request) {
    // Find Content-Length header
    u8cs cl_val = {};
    a_cstr(cl_key, "Content-Length");
    u8cs hdr_data = {request[0], request[1]};
    HTTPstate hparse = {};
    u8cs hdr_pairs[32] = {};
    u8csb hdr_store = {hdr_pairs, hdr_pairs, hdr_pairs, hdr_pairs + 32};
    hparse.headers = u8csbIdle(hdr_store);
    HTTPutf8Drain(hdr_data, &hparse);
    u8css hdrs = {hdr_pairs, hdr_store[2]};
    ok64 o = HTTPfind(&cl_val, cl_key, hdrs);
    if (o != OK || $empty(cl_val)) {
        a_cstr(err,
               "HTTP/1.1 411 Length Required\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    // Parse content length
    size_t content_len = 0;
    for (u8cp p = cl_val[0]; p < cl_val[1]; p++) {
        if (*p < '0' || *p > '9') break;
        content_len = content_len * 10 + (*p - '0');
    }
    if (content_len == 0 || content_len > (1 << 24)) {
        a_cstr(err,
               "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    // Find end of headers (\r\n\r\n) in request data
    u8cp body_start = NULL;
    for (u8cp p = request[0]; p + 3 < request[1]; p++) {
        if (p[0] == '\r' && p[1] == '\n' && p[2] == '\r' && p[3] == '\n') {
            body_start = p + 4;
            break;
        }
    }
    if (body_start == NULL) {
        a_cstr(err,
               "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    // Allocate body buffer
    u8p bodybuf[4] = {};
    o = u8bAllocate(bodybuf, content_len + 1);
    if (o != OK) {
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    // Copy bytes already past headers
    size_t have = (size_t)(request[1] - body_start);
    if (have > content_len) have = content_len;
    if (have > 0) {
        u8cs chunk = {body_start, body_start + have};
        u8bFeed(bodybuf, chunk);
    }

    // Read remaining body
    while (u8bDataLen(bodybuf) < content_len) {
        size_t need = content_len - u8bDataLen(bodybuf);
        u8 tmp[4096];
        size_t rdsz = need < sizeof(tmp) ? need : sizeof(tmp);
        ssize_t n = read(cfd, tmp, rdsz);
        if (n <= 0) break;
        u8cs chunk = {tmp, tmp + n};
        u8bFeed(bodybuf, chunk);
    }

    if (u8bDataLen(bodybuf) < content_len) {
        u8bFree(bodybuf);
        a_cstr(err,
               "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    u8cp bd0 = bodybuf[1], bd1 = bodybuf[2];
    u8cs source = {bd0, bd1};

    // Parse optional ?branch from URL query
    u8cs branch = {};
    if ($ok(req_query) && !$empty(req_query)) {
        $mv(branch, req_query);
    }

    u8cs empty_msg = {};
    o = BEPostData(ctx->be, http_path, source, branch, empty_msg);
    u8bFree(bodybuf);

    if (o != OK) {
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    a_cstr(resp,
           "HTTP/1.1 200 OK\r\n"
           "Content-Length: 0\r\n"
           "Connection: close\r\n"
           "\r\n");
    FILEFeedall(cfd, resp);
    close(cfd);
    return POLLIN;
}

// Accept callback: handle new connection
static short BESRVAcceptCB(int fd, poller *p) {
    BESRVctxp ctx = (BESRVctxp)p->payload;

    aNETraw(addrbuf);
    int cfd = -1;
    ok64 o = TCPAccept(&cfd, addrbuf, fd);
    if (o != OK) return POLLIN;

    // Read HTTP request (small, blocking read is OK for headers)
    u8 reqbuf[4096];
    u8s req = {reqbuf, reqbuf};
    for (int i = 0; i < 10; i++) {
        ssize_t n = read(cfd, req[0], reqbuf + sizeof(reqbuf) - req[0]);
        if (n <= 0) break;
        req[0] += n;
        u8cs data = {reqbuf, req[0]};
        for (u8cp pp = data[0]; pp + 3 < data[1]; pp++) {
            if (pp[0] == '\r' && pp[1] == '\n' && pp[2] == '\r' &&
                pp[3] == '\n') {
                goto headers_done;
            }
        }
    }
headers_done:;

    u8cs request = {reqbuf, req[0]};
    if ($empty(request)) {
        close(cfd);
        return POLLIN;
    }

    // Parse HTTP request
    u8cs hdr_pairs[32] = {};
    u8csb hdr_store = {hdr_pairs, hdr_pairs, hdr_pairs, hdr_pairs + 32};
    HTTPstate http = {};
    http.headers = u8csbIdle(hdr_store);
    o = HTTPutf8Drain(request, &http);
    if (o != OK) {
        a_cstr(err, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    a_cstr(get_method, "GET");
    a_cstr(post_method, "POST");
    if (!$eq(http.method, get_method) && !$eq(http.method, post_method)) {
        a_cstr(err,
               "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    // Parse URI: path = prefix, query = formula
    uri req_uri = {};
    o = URIutf8Drain(http.uri, &req_uri);
    if (o != OK) {
        a_cstr(err, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
        FILEFeedall(cfd, err);
        close(cfd);
        return POLLIN;
    }

    // Strip leading / from path
    u8cs http_path = {req_uri.path[0], req_uri.path[1]};
    if (!$empty(http_path) && *http_path[0] == '/') {
        http_path[0]++;
    }

    // POST: accept body, post to repo
    if ($eq(http.method, post_method)) {
        return BESRVAcceptPost(ctx, cfd, http_path, req_uri.query, request);
    }

    // Detect mode from suffix (modifies http_path to strip suffix)
    int mode = BESRVDetectMode(&http_path);

    // Parse formula from query string
    ron120 form[VER_MAX];
    ron120s form_s = {form, form + VER_MAX};
    if ($ok(req_uri.query) && !$empty(req_uri.query)) {
        o = VERFormParse(form_s, req_uri.query);
        if (o != OK) {
            o = VERFormFromBranches(form_s, ctx->be->branchc,
                                   ctx->be->branches);
        }
    } else {
        o = VERFormFromBranches(form_s, ctx->be->branchc,
                               ctx->be->branches);
    }
    ron120cs formcs = {form, form_s[0]};

    // Dispatch by mode
    switch (mode) {
        case BESRV_MODE_BASON: {
            a_cstr(sch, BE_SCHEME_BE);
            return BESRVAcceptStream(ctx, cfd, http_path, formcs, sch);
        }
        case BESRV_MODE_STATE: {
            a_cstr(sch, BE_SCHEME_STAT);
            return BESRVAcceptStream(ctx, cfd, http_path, formcs, sch);
        }
        case BESRV_MODE_RAW:
            return BESRVAcceptRaw(ctx, cfd, http_path, formcs);
        case BESRV_MODE_DIR:
            return BESRVAcceptDir(ctx, cfd, http_path);
        default:
            close(cfd);
            return POLLIN;
    }
}

ok64 BESRVInit(BESRVctxp ctx, BEp be, int port) {
    sane(ctx != NULL && be != NULL && port > 0);
    memset(ctx, 0, sizeof(BESRVctx));
    ctx->be = be;
    ctx->listen_fd = -1;
    ctx->stop_pipe[0] = -1;
    ctx->stop_pipe[1] = -1;

    // Create stop pipe for cross-thread signaling (non-blocking so
    // timeout-driven callback reads don't block the event loop)
    test(pipe(ctx->stop_pipe) == 0, BESRVFAIL);
    fcntl(ctx->stop_pipe[0], F_SETFL,
          fcntl(ctx->stop_pipe[0], F_GETFL, 0) | O_NONBLOCK);

    // Listen on port
    char addr[64];
    snprintf(addr, sizeof(addr), "tcp://0.0.0.0:%d", port);
    a_cstr(addrcs, addr);
    call(TCPListen, &ctx->listen_fd, addrcs);

    done;
}

ok64 BESRVRun(BESRVctxp ctx) {
    sane(ctx != NULL && ctx->listen_fd >= 0);

    // POL is thread_local, so init + register on the running thread
    call(POLInit, 1024);

    poller stop_pol = {
        .callback = BESRVStopCB,
        .payload = ctx,
        .tofd = ctx->stop_pipe[0],
        .events = POLLIN,
    };
    call(POLTrackEvents, ctx->stop_pipe[0], stop_pol);

    poller listen_pol = {
        .callback = BESRVAcceptCB,
        .payload = ctx,
        .tofd = ctx->listen_fd,
        .events = POLLIN,
    };
    call(POLTrackEvents, ctx->listen_fd, listen_pol);

    ok64 o = POLLoop(POLNever);
    POLFree();
    return o;
}

ok64 BESRVStop(BESRVctxp ctx) {
    if (ctx != NULL && ctx->stop_pipe[1] >= 0) {
        u8 b = 1;
        write(ctx->stop_pipe[1], &b, 1);
    }
    return OK;
}

ok64 BESRVFree(BESRVctxp ctx) {
    if (ctx == NULL) return OK;
    if (ctx->listen_fd >= 0) {
        close(ctx->listen_fd);
        ctx->listen_fd = -1;
    }
    if (ctx->stop_pipe[0] >= 0) {
        close(ctx->stop_pipe[0]);
        ctx->stop_pipe[0] = -1;
    }
    if (ctx->stop_pipe[1] >= 0) {
        close(ctx->stop_pipe[1]);
        ctx->stop_pipe[1] = -1;
    }
    memset(ctx, 0, sizeof(BESRVctx));
    return OK;
}
