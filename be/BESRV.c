#include "BESRV.h"

#include <errno.h>
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
static short BESRVReadHeaders(BEClientp cl, poller *p);
static short BESRVReadBody(BEClientp cl, poller *p);
static short BESRVWriteResponse(BEClientp cl, poller *p);
static short BESRVStreamResponse(BEClientp cl, poller *p);
static short BESRVDispatch(BEClientp cl, poller *p);
static short BESRVProcessGet(BEClientp cl, poller *p);
static short BESRVProcessPost(BEClientp cl, poller *p);
static short BESRVSetupStream(BEClientp cl, poller *p,
                               ron120cs formcs_in, u8cs scheme);
static short BESRVProcessRaw(BEClientp cl, poller *p, ron120cs formcs);
static short BESRVProcessDir(BEClientp cl, poller *p);
static short BESRVError(BEClientp cl, u8cs status_line);

// Stop pipe callback
static short BESRVStopCB(int fd, poller *p) {
    u8 buf[1];
    if (read(fd, buf, 1) == 1) POLStop();
    return 0;
}

static ok64 BESRVClientFree(BEClientp cl) {
    if (cl == NULL) return OK;
    ROCKIterClose(&cl->it);
    // Do NOT call POLIgnoreEvents here: when called from a POL callback,
    // the callback returns 0 with callback=NULL, and POL ejects the entry.
    // Calling POLIgnoreEvents inside the callback would double-eject,
    // removing a different entry (e.g. the listen socket).
    if (cl->fd >= 0) {
        close(cl->fd);
    }
    u8bFree(cl->wbuf);
    u8bFree(cl->rbuf);
    free(cl);
    return OK;
}

// Send an error response, transition to WRITE phase
static short BESRVError(BEClientp cl, u8cs status_line) {
    if (cl->wbuf[3] == NULL) {
        ok64 o = u8bAllocate(cl->wbuf, 256);
        if (o != OK) {
            BESRVClientFree(cl);
            return 0;
        }
    } else {
        u8bReset(cl->wbuf);
    }
    u8sFeed(u8bIdle(cl->wbuf), status_line);
    u8cp d0 = u8bDataHead(cl->wbuf), d1 = u8bIdleHead(cl->wbuf);
    cl->pending[0] = d0;
    cl->pending[1] = d1;
    cl->phase = BESRV_PHASE_WRITE;
    cl->iter_done = YES;
    return POLLOUT;
}

// Rewrite BASON root key: DB record has empty root key,
// we replace it with the full DB key.
static ok64 BESRVRewriteRoot(u8bp wbuf, u8cs dbkey, u8cs val) {
    sane(wbuf != NULL && $ok(val) && !$empty(val));
    a_dup(u8c,from,val);
    u8 type = 0;
    u8cs key = {};
    u8cs children = {};
    call(TLKVDrain, from, &type, key, children);
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

        if ($len(k) < $len(cl->prefix) ||
            memcmp(k[0], cl->prefix[0], $len(cl->prefix)) != 0) {
            cl->iter_done = YES;
            break;
        }

        uri ku = {};
        ok64 o = URIutf8Drain(k, &ku);
        if (o != OK) {
            call(ROCKIterNext, &cl->it);
            continue;
        }

        if (!$empty(ku.fragment)) {
            call(ROCKIterNext, &cl->it);
            continue;
        }

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

        if (u8bDataLen(cl->wbuf) > BESRV_WBUF_SIZE / 2) break;
    }

    if (!ROCKIterValid(&cl->it)) cl->iter_done = YES;

    u8cp d0 = u8bDataHead(cl->wbuf), d1 = u8bIdleHead(cl->wbuf);
    cl->pending[0] = d0;
    cl->pending[1] = d1;
    done;
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
        if (len > 0 && *(path[0][1] - 1) == '/') path[0][1]--;
        return BESRV_MODE_DIR;
    }
    return BESRV_MODE_RAW;
}

// ---- Accept: create client, set non-blocking, register POLLIN ----
static short BESRVAcceptCB(int fd, poller *p) {
    BESRVctxp ctx = (BESRVctxp)p->payload;

    aNETraw(addrbuf);
    int cfd = -1;
    ok64 o = TCPAccept(&cfd, addrbuf, fd);
    if (o != OK) return POLLIN;

    int flags = fcntl(cfd, F_GETFL, 0);
    fcntl(cfd, F_SETFL, flags | O_NONBLOCK);

    BEClientp cl = (BEClientp)calloc(1, sizeof(BEClient));
    if (cl == NULL) {
        close(cfd);
        return POLLIN;
    }
    cl->fd = cfd;
    cl->ctx = ctx;
    cl->phase = BESRV_PHASE_HEADERS;

    o = u8bAllocate(cl->rbuf, BESRV_RBUF_INIT);
    if (o != OK) {
        free(cl);
        close(cfd);
        return POLLIN;
    }

    poller client_pol = {
        .callback = BESRVClientCB,
        .payload = cl,
        .tofd = cfd,
        .events = POLLIN,
    };
    o = POLTrackEvents(cfd, client_pol);
    if (o != OK) {
        u8bFree(cl->rbuf);
        free(cl);
        close(cfd);
        return POLLIN;
    }

    return POLLIN;
}

// ---- Unified client callback dispatcher ----
static short BESRVClientCB(int fd, poller *p) {
    BEClientp cl = (BEClientp)p->payload;
    switch (cl->phase) {
        case BESRV_PHASE_HEADERS:
            return BESRVReadHeaders(cl, p);
        case BESRV_PHASE_BODY:
            return BESRVReadBody(cl, p);
        case BESRV_PHASE_WRITE:
            return BESRVWriteResponse(cl, p);
        case BESRV_PHASE_STREAM:
            return BESRVStreamResponse(cl, p);
        default:
            BESRVClientFree(cl);
            p->callback = NULL;
            return 0;
    }
}

// ---- Phase: read HTTP headers (POLLIN) ----
static short BESRVReadHeaders(BEClientp cl, poller *p) {
    // Non-blocking read into rbuf idle space
    size_t avail = u8bIdleLen(cl->rbuf);
    if (avail == 0) {
        a_cstr(err, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }

    ssize_t n = read(cl->fd, *u8bIdle(cl->rbuf), avail);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return POLLIN;
        BESRVClientFree(cl);
        p->callback = NULL;
        return 0;
    }
    if (n == 0) {
        BESRVClientFree(cl);
        p->callback = NULL;
        return 0;
    }
    ((u8**)cl->rbuf)[2] += n;

    // Scan for \r\n\r\n in rbuf data
    u8cp d0 = u8bDataHead(cl->rbuf);
    u8cp d1 = u8bIdleHead(cl->rbuf);
    for (u8cp pp = d0; pp + 3 < d1; pp++) {
        if (pp[0] == '\r' && pp[1] == '\n' && pp[2] == '\r' &&
            pp[3] == '\n') {
            return BESRVDispatch(cl, p);
        }
    }

    return POLLIN;
}

// ---- Dispatch: parse headers, route to handler ----
static short BESRVDispatch(BEClientp cl, poller *p) {
    u8cp d0 = u8bDataHead(cl->rbuf);
    u8cp d1 = u8bIdleHead(cl->rbuf);
    u8cs request = {d0, d1};

    // Parse HTTP request
    u8cs hdr_pairs[32] = {};
    u8csb hdr_store = {hdr_pairs, hdr_pairs, hdr_pairs, hdr_pairs + 32};
    HTTPstate http = {};
    http.headers = u8csbIdle(hdr_store);
    ok64 o = HTTPutf8Drain(request, &http);
    if (o != OK) {
        a_cstr(err, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }

    a_cstr(get_method, "GET");
    a_cstr(post_method, "POST");
    if (!$eq(http.method, get_method) && !$eq(http.method, post_method)) {
        a_cstr(err,
               "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }

    // Parse URI
    uri req_uri = {};
    o = URIutf8Drain(http.uri, &req_uri);
    if (o != OK) {
        a_cstr(err, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }

    // Strip leading / from path
    u8cs http_path = {req_uri.path[0], req_uri.path[1]};
    if (!$empty(http_path) && *http_path[0] == '/') {
        http_path[0]++;
    }

    // Copy path into stable pathbuf
    size_t plen = $len(http_path);
    if (plen > sizeof(cl->pathbuf)) plen = sizeof(cl->pathbuf);
    if (plen > 0) memcpy(cl->pathbuf, http_path[0], plen);
    cl->http_path[0] = cl->pathbuf;
    cl->http_path[1] = cl->pathbuf + plen;

    // Copy query into stable querybuf
    if ($ok(req_uri.query) && !$empty(req_uri.query)) {
        size_t qlen = $len(req_uri.query);
        if (qlen > sizeof(cl->querybuf)) qlen = sizeof(cl->querybuf);
        memcpy(cl->querybuf, req_uri.query[0], qlen);
        cl->req_query[0] = cl->querybuf;
        cl->req_query[1] = cl->querybuf + qlen;
    }

    // Find body start offset in rbuf
    for (u8cp pp = d0; pp + 3 < d1; pp++) {
        if (pp[0] == '\r' && pp[1] == '\n' && pp[2] == '\r' && pp[3] == '\n') {
            cl->hdr_len = (size_t)((pp + 4) - d0);
            break;
        }
    }

    // POST handling
    if ($eq(http.method, post_method)) {
        // Parse Content-Length
        u8cs cl_val = {};
        a_cstr(cl_key, "Content-Length");
        u8css hdrs = {hdr_pairs, u8csbIdleHead(hdr_store)};
        o = HTTPfind(&cl_val, cl_key, hdrs);
        if (o != OK || $empty(cl_val)) {
            a_cstr(err,
                   "HTTP/1.1 411 Length Required\r\nConnection: close\r\n\r\n");
            return BESRVError(cl, err);
        }

        size_t content_len = 0;
        for (u8cp pp = cl_val[0]; pp < cl_val[1]; pp++) {
            if (*pp < '0' || *pp > '9') break;
            content_len = content_len * 10 + (*pp - '0');
        }
        if (content_len == 0 || content_len > BESRV_BODY_MAX) {
            a_cstr(err,
                   "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
            return BESRVError(cl, err);
        }
        cl->content_len = content_len;
        cl->mode = BESRV_MODE_POST;

        // Check if body already complete
        size_t have = (size_t)(d1 - (d0 + cl->hdr_len));
        if (have >= content_len) {
            return BESRVProcessPost(cl, p);
        }

        // Need more data — reserve space
        size_t need = cl->hdr_len + content_len;
        if (u8bIdleLen(cl->rbuf) < content_len - have) {
            o = u8bReserve(cl->rbuf, need);
            if (o != OK) {
                a_cstr(err,
                       "HTTP/1.1 500 Internal Server Error\r\n"
                       "Connection: close\r\n\r\n");
                return BESRVError(cl, err);
            }
        }
        cl->phase = BESRV_PHASE_BODY;
        return POLLIN;
    }

    // GET: detect mode (modifies http_path copy)
    int mode = BESRVDetectMode(&cl->http_path);
    cl->mode = (u8)mode;

    return BESRVProcessGet(cl, p);
}

// ---- Phase: read POST body (POLLIN) ----
static short BESRVReadBody(BEClientp cl, poller *p) {
    size_t avail = u8bIdleLen(cl->rbuf);
    if (avail == 0) {
        a_cstr(err, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }

    ssize_t n = read(cl->fd, *u8bIdle(cl->rbuf), avail);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return POLLIN;
        BESRVClientFree(cl);
        p->callback = NULL;
        return 0;
    }
    if (n == 0) {
        // EOF before body complete
        a_cstr(err, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }
    ((u8**)cl->rbuf)[2] += n;

    // Check if body complete
    size_t have = u8bDataLen(cl->rbuf) - cl->hdr_len;
    if (have >= cl->content_len) {
        return BESRVProcessPost(cl, p);
    }

    return POLLIN;
}

// ---- Process GET: dispatch by mode ----
static short BESRVProcessGet(BEClientp cl, poller *p) {
    BESRVctxp ctx = cl->ctx;

    // Parse formula from query string
    ron120 form[VER_MAX];
    ron120s form_s = {form, form + VER_MAX};
    ok64 o;
    if ($ok(cl->req_query) && !$empty(cl->req_query)) {
        o = VERFormParse(form_s, cl->req_query);
        if (o != OK) {
            o = VERFormFromBranches(form_s, ctx->be->branchc,
                                   ctx->be->branches);
        }
    } else {
        o = VERFormFromBranches(form_s, ctx->be->branchc,
                               ctx->be->branches);
    }
    ron120cs formcs = {form, form_s[0]};

    switch (cl->mode) {
        case BESRV_MODE_BASON: {
            a_cstr(sch, BE_SCHEME_BE);
            return BESRVSetupStream(cl, p, formcs, sch);
        }
        case BESRV_MODE_STATE: {
            a_cstr(sch, BE_SCHEME_STAT);
            return BESRVSetupStream(cl, p, formcs, sch);
        }
        case BESRV_MODE_RAW:
            return BESRVProcessRaw(cl, p, formcs);
        case BESRV_MODE_DIR:
            return BESRVProcessDir(cl, p);
        default: {
            BESRVClientFree(cl);
            p->callback = NULL;
            return 0;
        }
    }
}

// ---- Setup streaming (BASON/STATE) ----
static short BESRVSetupStream(BEClientp cl, poller *p,
                               ron120cs formcs_in, u8cs scheme) {
    BESRVctxp ctx = cl->ctx;
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
    if (!$empty(cl->http_path)) u8sFeed(fps, cl->http_path);
    u8cs fpath = {fpbuf, fps[0]};
    u8s pfx = {cl->pfxbuf, cl->pfxbuf + sizeof(cl->pfxbuf)};
    BEKeyBuild(pfx, scheme, fpath, 0, 0);
    cl->prefix[0] = cl->pfxbuf;
    cl->prefix[1] = pfx[0];

    // Allocate write buffer
    ok64 o = u8bAllocate(cl->wbuf, BESRV_WBUF_SIZE);
    if (o != OK) {
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }

    // Open iterator and seek to prefix
    o = ROCKIterOpen(&cl->it, &ctx->be->db);
    if (o != OK) {
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }
    o = ROCKIterSeek(&cl->it, cl->prefix);
    if (o != OK) {
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }

    // Feed HTTP headers into wbuf first, then stream data after
    a_cstr(resp_hdr,
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: application/x-bason\r\n"
           "Connection: close\r\n"
           "\r\n");
    u8sFeed(u8bIdle(cl->wbuf), resp_hdr);

    // Fill initial data after headers
    // (FillBuf resets wbuf, so we need to do first fill differently)
    // Instead, just set pending to the headers for now, fill on next callback
    u8cp d0 = u8bDataHead(cl->wbuf), d1 = u8bIdleHead(cl->wbuf);
    cl->pending[0] = d0;
    cl->pending[1] = d1;

    // Free rbuf — done with input
    u8bFree(cl->rbuf);

    cl->phase = BESRV_PHASE_STREAM;
    return POLLOUT;
}

// ---- Process RAW: merge + export, send one-shot ----
static short BESRVProcessRaw(BEClientp cl, poller *p, ron120cs formcs) {
    BESRVctxp ctx = cl->ctx;

    u8p merged[4] = {};
    ok64 o = u8bAllocate(merged, 1 << 18);
    if (o != OK) {
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }

    o = BEMergeFile(&ctx->be->db, ctx->be->loc.path, cl->http_path, formcs,
                    merged);
    if (o != OK) {
        u8bFree(merged);
        a_cstr(err, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }

    // Export BASON to source text
    u8p outbuf[4] = {};
    o = u8bAllocate(outbuf, 1 << 18);
    if (o != OK) {
        u8bFree(merged);
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }

    u8cp m0 = u8bDataHead(merged), m1 = u8bIdleHead(merged);
    u8cs bason = {m0, m1};
    aBpad(u64, stk, 256);
    o = BASTExport(u8bIdle(outbuf), stk, bason);
    u8bFree(merged);
    if (o != OK) {
        u8bFree(outbuf);
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }

    // Build HTTP response with Content-Length
    u8cp s0 = u8bDataHead(outbuf), s1 = u8bIdleHead(outbuf);
    size_t bodylen = (size_t)(s1 - s0);
    const char *mime = MIMEByPath(cl->http_path);
    char hdrbuf[256];
    int hlen = snprintf(hdrbuf, sizeof(hdrbuf),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n"
                        "\r\n",
                        mime, bodylen);

    // Allocate wbuf for headers + body
    o = u8bAllocate(cl->wbuf, (size_t)hlen + bodylen);
    if (o != OK) {
        u8bFree(outbuf);
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }
    u8cs hdr = {(u8cp)hdrbuf, (u8cp)hdrbuf + hlen};
    u8sFeed(u8bIdle(cl->wbuf), hdr);
    u8cs body = {s0, s1};
    u8sFeed(u8bIdle(cl->wbuf), body);
    u8bFree(outbuf);

    // Free rbuf — done with input
    u8bFree(cl->rbuf);

    u8cp d0 = u8bDataHead(cl->wbuf), d1 = u8bIdleHead(cl->wbuf);
    cl->pending[0] = d0;
    cl->pending[1] = d1;
    cl->phase = BESRV_PHASE_WRITE;
    cl->iter_done = YES;
    return POLLOUT;
}

// ---- Process DIR: scan + send one-shot ----
static short BESRVProcessDir(BEClientp cl, poller *p) {
    BESRVctxp ctx = cl->ctx;

    a_cstr(sch_be, BE_SCHEME_BE);
    u8 fpbuf[256];
    u8s fps = {fpbuf, fpbuf + sizeof(fpbuf)};
    u8sFeed(fps, ctx->be->loc.path);
    u8sFeed1(fps, '/');
    if (!$empty(cl->http_path)) {
        u8sFeed(fps, cl->http_path);
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
        return BESRVError(cl, err);
    }

    u8 prevbuf[256];
    u8cs prev = {};

    ROCKiter it = {};
    o = ROCKIterOpen(&it, &ctx->be->db);
    if (o != OK) {
        u8bFree(outbuf);
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }
    o = ROCKIterSeek(&it, prefix);
    if (o != OK) {
        ROCKIterClose(&it);
        u8bFree(outbuf);
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }

    while (ROCKIterValid(&it)) {
        u8cs k = {};
        ROCKIterKey(&it, k);

        if ($len(k) < pfxlen ||
            memcmp(k[0], prefix[0], pfxlen) != 0)
            break;

        uri ku = {};
        o = URIutf8Drain(k, &ku);
        if (o != OK || !$empty(ku.fragment)) {
            ROCKIterNext(&it);
            continue;
        }

        u8cs rest = {k[0] + pfxlen, k[1]};
        if ($empty(rest)) {
            ROCKIterNext(&it);
            continue;
        }

        a_dup(u8c,component,rest);
        b8 is_dir = NO;
        for (u8cp pp = rest[0]; pp < rest[1]; pp++) {
            if (*pp == '/' || *pp == '?') {
                component[1] = pp;
                if (*pp == '/') is_dir = YES;
                break;
            }
        }

        if ($empty(component)) {
            ROCKIterNext(&it);
            continue;
        }

        if (!$empty(prev) && $len(prev) == $len(component) &&
            memcmp(prev[0], component[0], $len(prev)) == 0) {
            ROCKIterNext(&it);
            continue;
        }

        size_t clen = $len(component);
        if (clen > sizeof(prevbuf)) clen = sizeof(prevbuf);
        memcpy(prevbuf, component[0], clen);
        prev[0] = prevbuf;
        prev[1] = prevbuf + clen;

        u8sFeed(u8bIdle(outbuf), component);
        if (is_dir) u8sFeed1(u8bIdle(outbuf), '/');
        u8sFeed1(u8bIdle(outbuf), '\n');

        ROCKIterNext(&it);
    }
    ROCKIterClose(&it);

    // Build response into wbuf
    u8cp bd0 = u8bDataHead(outbuf), bd1 = u8bIdleHead(outbuf);
    size_t bodylen = (size_t)(bd1 - bd0);
    char hdrbuf[256];
    int hlen = snprintf(hdrbuf, sizeof(hdrbuf),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n"
                        "\r\n",
                        bodylen);

    o = u8bAllocate(cl->wbuf, (size_t)hlen + bodylen);
    if (o != OK) {
        u8bFree(outbuf);
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }
    u8cs hdr = {(u8cp)hdrbuf, (u8cp)hdrbuf + hlen};
    u8sFeed(u8bIdle(cl->wbuf), hdr);
    u8cs body = {bd0, bd1};
    u8sFeed(u8bIdle(cl->wbuf), body);
    u8bFree(outbuf);

    // Free rbuf — done with input
    u8bFree(cl->rbuf);

    u8cp d0 = u8bDataHead(cl->wbuf), d1 = u8bIdleHead(cl->wbuf);
    cl->pending[0] = d0;
    cl->pending[1] = d1;
    cl->phase = BESRV_PHASE_WRITE;
    cl->iter_done = YES;
    return POLLOUT;
}

// ---- Process POST: parse body, post data, send response ----
static short BESRVProcessPost(BEClientp cl, poller *p) {
    BESRVctxp ctx = cl->ctx;

    u8cp d0 = u8bDataHead(cl->rbuf);
    u8cp body_start = d0 + cl->hdr_len;
    u8cs source = {body_start, body_start + cl->content_len};

    u8cs branch = {};
    if ($ok(cl->req_query) && !$empty(cl->req_query)) {
        $mv(branch, cl->req_query);
    }

    u8cs empty_msg = {};
    ok64 o = BEPostData(ctx->be, cl->http_path, source, branch, empty_msg);

    // Free rbuf — done with input
    u8bFree(cl->rbuf);

    if (o != OK) {
        a_cstr(err,
               "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n");
        return BESRVError(cl, err);
    }

    a_cstr(resp,
           "HTTP/1.1 200 OK\r\n"
           "Content-Length: 0\r\n"
           "Connection: close\r\n"
           "\r\n");
    o = u8bAllocate(cl->wbuf, 128);
    if (o != OK) {
        BESRVClientFree(cl);
        p->callback = NULL;
        return 0;
    }
    u8sFeed(u8bIdle(cl->wbuf), resp);
    u8cp w0 = u8bDataHead(cl->wbuf), w1 = u8bIdleHead(cl->wbuf);
    cl->pending[0] = w0;
    cl->pending[1] = w1;
    cl->phase = BESRV_PHASE_WRITE;
    cl->iter_done = YES;
    return POLLOUT;
}

// ---- Phase: drain one-shot response (POLLOUT) ----
static short BESRVWriteResponse(BEClientp cl, poller *p) {
    if (!$empty(cl->pending)) {
        ssize_t n = write(cl->fd, cl->pending[0], $len(cl->pending));
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return POLLOUT;
            BESRVClientFree(cl);
            p->callback = NULL;
            return 0;
        }
        cl->pending[0] += n;
        if (!$empty(cl->pending)) return POLLOUT;
    }

    // Done
    BESRVClientFree(cl);
    p->callback = NULL;
    return 0;
}

// ---- Phase: streaming iterator data (POLLOUT) ----
static short BESRVStreamResponse(BEClientp cl, poller *p) {
    // Try to send pending data
    if (!$empty(cl->pending)) {
        ssize_t n = write(cl->fd, cl->pending[0], $len(cl->pending));
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

ok64 BESRVInit(BESRVctxp ctx, BEp be, int port) {
    sane(ctx != NULL && be != NULL && port > 0);
    memset(ctx, 0, sizeof(BESRVctx));
    ctx->be = be;
    ctx->listen_fd = -1;
    ctx->stop_pipe[0] = -1;
    ctx->stop_pipe[1] = -1;

    test(pipe(ctx->stop_pipe) == 0, BESRVFAIL);
    fcntl(ctx->stop_pipe[0], F_SETFL,
          fcntl(ctx->stop_pipe[0], F_GETFL, 0) | O_NONBLOCK);

    char addr[64];
    snprintf(addr, sizeof(addr), "tcp://0.0.0.0:%d", port);
    a_cstr(addrcs, addr);
    call(TCPListen, &ctx->listen_fd, addrcs);
    int flags = fcntl(ctx->listen_fd, F_GETFL, 0);
    test(flags >= 0, BESRVFAIL);
    test(fcntl(ctx->listen_fd, F_SETFL, flags | O_NONBLOCK) == 0, BESRVFAIL);

    done;
}

ok64 BESRVRun(BESRVctxp ctx) {
    sane(ctx != NULL && ctx->listen_fd >= 0);

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
