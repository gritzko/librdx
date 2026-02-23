#include "CURL.h"
#include "POL.h"
#include "PRO.h"
#include "TEST.h"

static int test_complete = 0;
static long test_status = 0;
static size_t test_body_len = 0;

static void on_response(CURLreq *req, long status, u8cs body) {
    test_status = status;
    test_body_len = u8csLen(body);
    test_complete = 1;
    trace("CURL response: status=%ld len=%zu", status, test_body_len);
}

ok64 CURLtest() {
    sane(1);

    call(POLInit, 64);
    call(CURLInit);

    // Fetch google.com
    call(CURLGet, "https://www.google.com/", on_response, NULL);

    // Run event loop until complete (max 10 seconds)
    u64 deadline = POLNow() + 10 * POLNanosPerSec;
    while (!test_complete && POLNow() < deadline) {
        POLLoop(100 * POLNanosPerMSec);
    }

    want(test_complete);
    want(test_status == 200);
    want(test_body_len > 0);

    CURLFree();
    POLFree();

    done;
}

TEST(CURLtest);
