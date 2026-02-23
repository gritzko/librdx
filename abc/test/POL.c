#include "POL.h"

#include "CURL.h"
#include "PRO.h"
#include "TEST.h"

static int timer_calls = 0;

// Concurrent HTTP test state
#define N_URLS 5
static int responses_received = 0;
static long response_status[N_URLS];
static size_t response_len[N_URLS];
static b8 found_http[N_URLS];

static void on_http_response(CURLreq *req, long status, u8cs body) {
    int idx = (int)(intptr_t)req->userdata;
    response_status[idx] = status;
    response_len[idx] = u8csLen(body);

    // Search for "HTTP/" in response headers
    a_cstr(needle, "HTTP/");
    u8$ hdr = u8bData(req->headers);
    a_dup(u8, search, hdr);
    size_t hdrlen = u8sLen(search);
    found_http[idx] = (u8sFindS(search, needle) == OK);

    // Find and extract Server: header value
    a_cstr(server_needle, "server:");
    a_dup(u8, hsearch, hdr);
    char server[64] = "(none)";
    if (u8sFindS(hsearch, server_needle) == OK) {
        u8* val = *hsearch + 7;  // skip "server:"
        while (val < hsearch[1] && *val == ' ') val++;  // skip spaces
        a_dup(u8, line, hsearch);
        *line = val;
        if (u8sFind(line, '\r') == OK || u8sFind(line, '\n') == OK) {
            size_t len = *line - val;
            if (len > 63) len = 63;
            memcpy(server, val, len);
            server[len] = 0;
        }
    }

    responses_received++;
    trace("Response %d: status=%ld server=%s", idx, status, server);
}

u32 test_timer(u64 ns) {
    timer_calls++;
    return 100;  // 100ms
}

// Test: after POLIgnoreTime, queue should be empty and loop should exit
ok64 POLtest1() {
    sane(1);
    call(POLInit, 16);

    // Track timer
    call(POLTrackTime, test_timer);

    // Run loop briefly - timer should fire
    call(POLLoop, 50 * POLNanosPerMSec);

    // Ignore timer
    call(POLIgnoreTime);

    // Loop should exit immediately since no events
    call(POLLoop, 100 * POLNanosPerMSec);

    // Cleanup
    call(POLFree);
    done;
}

// Test: concurrent HTTP requests to 5 uptime pages
ok64 POLtest2() {
    sane(1);

    const char *urls[N_URLS] = {
        "https://www.cloudflare.com/",
        "https://www.facebook.com/",
        "https://www.fastly.com/",
        "https://www.google.com/",
        "https://aws.amazon.com/",
    };

    call(POLInit, 64);
    call(CURLInit);

    // Reset state
    responses_received = 0;
    for (int i = 0; i < N_URLS; i++) {
        response_status[i] = 0;
        response_len[i] = 0;
        found_http[i] = 0;
    }

    // Fire off all requests concurrently
    for (int i = 0; i < N_URLS; i++) {
        call(CURLGet, urls[i], on_http_response, (void *)(intptr_t)i);
    }

    // Wait for all responses (max 15 seconds)
    u64 deadline = POLNow() + 15 * POLNanosPerSec;
    while (responses_received < N_URLS && POLNow() < deadline) {
        POLLoop(100 * POLNanosPerMSec);
    }

    // Verify all responses received
    want(responses_received == N_URLS);
    int http_found = 0;
    for (int i = 0; i < N_URLS; i++) {
        // Accept 200, 301, 302, 303, 307, 308 (redirects are OK)
        want(response_status[i] >= 200 && response_status[i] < 400);
        want(response_len[i] > 0);
        if (found_http[i]) http_found++;
    }
    // Verify all responses have HTTP headers
    want(http_found == N_URLS);

    CURLFree();
    POLFree();
    done;
}

ok64 POLtest() {
    sane(1);
    call(POLtest1);
    call(POLtest2);
    done;
}

TEST(POLtest);
