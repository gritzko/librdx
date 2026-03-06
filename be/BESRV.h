#ifndef LIBRDX_BESRV_H
#define LIBRDX_BESRV_H

#include "BE.h"
#include "VER.h"
#include "abc/ROCK.h"

con ok64 BESRVFAIL = 0x2ce1b6d53ca495;
con ok64 BESRVBAD = 0x2ce1b6d54b28d;

#define BESRV_WBUF_SIZE (1 << 16)

#define BESRV_MODE_BASON 0
#define BESRV_MODE_STATE 1
#define BESRV_MODE_RAW   2
#define BESRV_MODE_DIR   3

// Per-client streaming state
typedef struct {
    ROCKiter it;           // ROCK iterator (independent per client)
    ron120 form[VER_MAX];  // formula entries
    ron120cs formcs;       // formula slice (into form[])
    u8 pfxbuf[512];        // scan prefix storage
    u8cs prefix;           // scan prefix slice
    int fd;                // client socket
    u8p wbuf[4];           // write buffer
    u8cs pending;          // un-sent data in wbuf
    b8 iter_done;          // iterator exhausted
} BEClient;
typedef BEClient *BEClientp;

// Server context
typedef struct {
    BEp be;
    int listen_fd;
    int stop_pipe[2];  // write end [1] to signal stop, read end [0] in POL
} BESRVctx;
typedef BESRVctx *BESRVctxp;

// Initialize server: bind port, set up POL
ok64 BESRVInit(BESRVctxp ctx, BEp be, int port);

// Run POL event loop
ok64 BESRVRun(BESRVctxp ctx);

// Stop the event loop
ok64 BESRVStop(BESRVctxp ctx);

// Cleanup: close socket
ok64 BESRVFree(BESRVctxp ctx);

#endif  // LIBRDX_BESRV_H
