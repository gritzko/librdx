#include "BESYNC.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#include "abc/FILE.h"
#include "abc/PRO.h"

b8 BESyncIsRemote(u8cs url) {
    if (!$ok(url) || $empty(url)) return NO;
    a_cstr(http, "http://");
    a_cstr(https, "https://");
    if ($len(url) >= $len(http) &&
        memcmp(url[0], http[0], $len(http)) == 0)
        return YES;
    if ($len(url) >= $len(https) &&
        memcmp(url[0], https[0], $len(https)) == 0)
        return YES;
    return NO;
}

// Write callback for curl_easy_perform
static size_t besync_write_cb(char *ptr, size_t size, size_t nmemb,
                              void *userdata) {
    u8bp buf = (u8bp)userdata;
    size_t bytes = size * nmemb;
    if (Bidlelen(buf) < bytes) {
        u8bReserve(buf, bytes);
    }
    u8cs src = {(u8cp)ptr, (u8cp)ptr + bytes};
    u8bFeed(buf, src);
    return bytes;
}

// Synchronous HTTP GET into a buffer
static ok64 BESyncGet(const char *url, u8bp buf) {
    sane(url != NULL && buf != NULL);
    CURL *easy = curl_easy_init();
    test(easy != NULL, BESYNCFAIL);
    curl_easy_setopt(easy, CURLOPT_URL, url);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, besync_write_cb);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, buf);
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy, CURLOPT_TIMEOUT, 30L);
    CURLcode res = curl_easy_perform(easy);
    if (res != CURLE_OK) {
        curl_easy_cleanup(easy);
        fail(BESYNCFAIL);
    }
    long status = 0;
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(easy);
    test(status == 200, BESYNCFAIL);
    done;
}

// Build repo name from URL host:port
static ok64 BESyncRepoName(u8s into, u8cs url) {
    sane($ok(into) && $ok(url));
    uri u = {};
    u8 ubuf[512];
    size_t ulen = $len(url);
    if (ulen >= sizeof(ubuf)) return BESYNCBAD;
    memcpy(ubuf, url[0], ulen);
    u.data[0] = ubuf;
    u.data[1] = ubuf + ulen;
    call(URILexer, &u);
    if ($ok(u.host) && !$empty(u.host)) {
        call(u8sFeed, into, u.host);
    }
    if ($ok(u.port) && !$empty(u.port)) {
        u8sFeed1(into, '.');
        call(u8sFeed, into, u.port);
    }
    done;
}

ok64 BESyncClone(u8cs remote_url, path8cg worktree) {
    sane($ok(remote_url) && !$empty(remote_url) && worktree != NULL);

    call(FILEInit);
    curl_global_init(CURL_GLOBAL_DEFAULT);

    ok64 result = OK;

    // Build repo name from URL
    u8 rname[256];
    u8s rn = {rname, rname + sizeof(rname)};
    call(BESyncRepoName, rn, remote_url);
    u8cs repo_name = {rname, rn[0]};

    // Build repo path: $HOME/.be/<repo>/
    u8p repo_path_pp[4] = {};
    call(path8bAlloc, repo_path_pp);
    const char *home = getenv("HOME");
    test(home != NULL, BESYNCFAIL);
    a_cstr(homecs, home);
    call(u8sFeed, u8bIdle(repo_path_pp), homecs);
    call(path8gTerm, path8gIn(repo_path_pp));
    a_cstr(dotbe, ".be");
    call(path8gPush, path8gIn(repo_path_pp), dotbe);
    call(path8gPush, path8gIn(repo_path_pp), repo_name);
    call(FILEMakeDirP, path8cgIn(repo_path_pp));

    // Build base URL
    char url_buf[1024];
    size_t url_len = $len(remote_url);
    if (url_len >= sizeof(url_buf) - 64) {
        path8bFree(repo_path_pp);
        curl_global_cleanup();
        fail(BESYNCBAD);
    }

    // Fetch file listing
    u8p list_buf[4] = {};
    call(u8bAllocate, list_buf, 65536);
    memcpy(url_buf, remote_url[0], url_len);
    memcpy(url_buf + url_len, "/_files", 8);

    result = BESyncGet(url_buf, list_buf);
    if (result != OK) goto cleanup;

    // Parse newline-delimited filenames
    u8cs files[256] = {};
    int file_count = 0;
    {
        u8cp d0 = list_buf[1], d1 = list_buf[2];
        u8cp pos = d0;
        while (pos < d1 && file_count < 256) {
            u8cp eol = pos;
            while (eol < d1 && *eol != '\n') eol++;
            if (eol > pos) {
                files[file_count][0] = pos;
                files[file_count][1] = eol;
                file_count++;
            }
            pos = eol < d1 ? eol + 1 : eol;
        }
    }

    // Download each file
    for (int i = 0; i < file_count && result == OK; i++) {
        u8cs fname = {files[i][0], files[i][1]};
        size_t flen = $len(fname);

        memcpy(url_buf, remote_url[0], url_len);
        memcpy(url_buf + url_len, "/_files/", 8);
        if (url_len + 8 + flen >= sizeof(url_buf)) {
            result = BESYNCBAD;
            break;
        }
        memcpy(url_buf + url_len + 8, fname[0], flen);
        url_buf[url_len + 8 + flen] = 0;

        u8p file_buf[4] = {};
        u8bAllocate(file_buf, 1 << 20);
        result = BESyncGet(url_buf, file_buf);
        if (result != OK) {
            u8bFree(file_buf);
            break;
        }

        a_path(fpath, "");
        path8gDup(path8gIn(fpath), path8cgIn(repo_path_pp));
        path8gPush(path8gIn(fpath), fname);

        int fd = -1;
        ok64 wo = FILECreate(&fd, path8cgIn(fpath));
        if (wo != OK) {
            u8bFree(file_buf);
            result = wo;
            break;
        }
        u8cp fd0 = file_buf[1], fd1 = file_buf[2];
        u8cs body = {fd0, fd1};
        if (!$empty(body)) {
            wo = FILEFeedall(fd, body);
        }
        FILEClose(&fd);
        u8bFree(file_buf);
        if (wo != OK) {
            result = wo;
            break;
        }
    }

    if (result == OK) {
        // Verify: open the cloned DB
        ROCKdb verify = {};
        ok64 vo = ROCKOpenRO(&verify, path8cgIn(repo_path_pp));
        if (vo == OK) {
            ROCKClose(&verify);
        } else {
            result = vo;
        }
    }

    if (result == OK) {
        // Write .be file in worktree
        u8 ubuf2[512];
        u8s be_uri = {ubuf2, ubuf2 + sizeof(ubuf2)};
        a_cstr(scheme, "be://");
        u8sFeed(be_uri, scheme);
        u8sFeed(be_uri, repo_name);
        u8cs uri_data = {ubuf2, be_uri[0]};

        a_path(dotbe_path, "");
        path8gDup(path8gIn(dotbe_path), worktree);
        a_cstr(dotbe_name, ".be");
        path8gPush(path8gIn(dotbe_path), dotbe_name);

        int fd = -1;
        ok64 wo = FILECreate(&fd, path8cgIn(dotbe_path));
        if (wo == OK) {
            FILEFeedall(fd, uri_data);
            FILEClose(&fd);
        }
    }

cleanup:;
    u8bFree(list_buf);
    path8bFree(repo_path_pp);
    curl_global_cleanup();
    return result;
}
