#ifndef LIBRDX_BESRV_H
#define LIBRDX_BESRV_H

#include "BE.h"

con ok64 BESRVFAIL = 0x2ce1b6d53ca495;
con ok64 BESRVBAD = 0x2ce1b6d54b28d;

// Cached file entry for serving
typedef struct {
    u8cs name;     // filename (points into names_buf)
    u64 size;      // file size
} BESRVfile;

// Server context
typedef struct {
    BEp be;                    // open repo
    int listen_fd;             // TCP listen socket
    BESRVfile files[256];      // cached file list
    int filec;                 // file count
    u8p names_buf[4];         // buffer for file names
    u8p repo_path_pp[4];     // DB directory path
} BESRVctx;
typedef BESRVctx *BESRVctxp;

// Initialize server: take snapshot, cache file list, bind port
ok64 BESRVInit(BESRVctxp ctx, BEp be, int port);

// Run blocking event loop
ok64 BESRVRun(BESRVctxp ctx);

// Stop the event loop
ok64 BESRVStop(BESRVctxp ctx);

// Cleanup: release snapshot, close socket
ok64 BESRVFree(BESRVctxp ctx);

#endif  // LIBRDX_BESRV_H
