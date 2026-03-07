#include "BESRV.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "abc/FILE.h"
#include "abc/PRO.h"

static BESRVctx g_srv = {};

static void besrv_sigint(int sig) { BESRVStop(&g_srv); }

ok64 besrv() {
    sane(1);
    call(FILEInit);

    int argc = $arglen;
    int port = 8080;

    // Parse optional port argument
    if (argc > 1) {
        a$rg(portarg, 1);
        port = atoi((const char *)portarg[0]);
        test(port > 0 && port < 65536, BESRVBAD);
    }

    // Open repo from cwd
    BE be = {};
    {
        char cwd[FILE_PATH_MAX_LEN];
        test(getcwd(cwd, sizeof(cwd)) != NULL, BESRVFAIL);
        a_cstr(cwdcs, cwd);
        a_pad(u8, cpath, FILE_PATH_MAX_LEN);
        call(u8sFeed, u8bIdle(cpath), cwdcs);
        call(path8gTerm, path8gIn(cpath));
        call(BEOpen, &be, path8cgIn(cpath));
    }

    // Print startup info
    {
        u8 msg[256];
        int mlen = snprintf((char *)msg, sizeof(msg),
                            "be-srv: listening on port %d\n", port);
        u8cs msgcs = {msg, msg + mlen};
        FILEerr(msgcs);
    }

    // Setup signal handler for clean shutdown (no SA_RESTART so poll
    // returns EINTR and the stop-pipe byte gets picked up)
    struct sigaction sa = {.sa_handler = besrv_sigint};
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    // Init and run server
    call(BESRVInit, &g_srv, &be, port);
    call(BESRVRun, &g_srv);
    call(BESRVFree, &g_srv);
    call(BEClose, &be);
    done;
}

MAIN(besrv);
