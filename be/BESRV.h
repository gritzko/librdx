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
#define BESRV_MODE_POST  4

#define BESRV_PHASE_HEADERS 0
#define BESRV_PHASE_BODY    1
#define BESRV_PHASE_WRITE   2
#define BESRV_PHASE_STREAM  3

#define BESRV_RBUF_INIT  4096
#define BESRV_BODY_MAX   (1 << 24)

// Forward declaration
typedef struct BESRVctx BESRVctx;
typedef BESRVctx *BESRVctxp;

// Per-client state
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
    // non-blocking state machine fields
    BESRVctxp ctx;         // server context back-pointer
    u8p rbuf[4];           // read buffer (headers + POST body)
    size_t hdr_len;        // byte offset of body start in rbuf
    size_t content_len;    // POST expected body length
    u8 pathbuf[256];       // copy of parsed HTTP path
    u8cs http_path;        // slice into pathbuf
    u8 querybuf[256];      // copy of parsed query string
    u8cs req_query;        // slice into querybuf
    u8 phase;              // BESRV_PHASE_*
    u8 mode;               // BESRV_MODE_*
} BEClient;
typedef BEClient *BEClientp;

// Server context
struct BESRVctx {
    BEp be;
    int listen_fd;
    int stop_pipe[2];  // write end [1] to signal stop, read end [0] in POL
};

// Initialize server: bind port, set up POL
ok64 BESRVInit(BESRVctxp ctx, BEp be, int port);

// Run POL event loop
ok64 BESRVRun(BESRVctxp ctx);

// Stop the event loop
ok64 BESRVStop(BESRVctxp ctx);

// Cleanup: close socket
ok64 BESRVFree(BESRVctxp ctx);

#endif  // LIBRDX_BESRV_H
