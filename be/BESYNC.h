#ifndef LIBRDX_BESYNC_H
#define LIBRDX_BESYNC_H

#include "BE.h"

con ok64 BESYNCFAIL = 0x2ce1c9cb3ca495;
con ok64 BESYNCBAD = 0x2ce1c9cb4b28d;

// Check if a URI string looks like a remote URL (http:// or https://)
b8 BESyncIsRemote(u8cs url);

// Clone a remote repo into $HOME/.be/<repo>/
// remote_url: http://host:port (repo name derived from host:port)
// worktree: local directory to write .be file into
ok64 BESyncClone(u8cs remote_url, path8cg worktree);

#endif  // LIBRDX_BESYNC_H
