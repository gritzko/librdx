#ifndef LIBRDX_BE_H
#define LIBRDX_BE_H

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/ROCK.h"
#include "abc/RON.h"
#include "abc/URI.h"
#include "ast/BAST.h"
#include "json/BASON.h"
#include "json/BIFF.h"

// Error codes
con ok64 BEFAIL = 0x2ce3ca495;
con ok64 BEBAD = 0xb38b28d;
con ok64 BEnone = 0x2cecb3ca9;

// Beagle SCM handle
typedef struct {
    ROCKdb db;
    uri loc;                          // parsed .be URI
    u8 loc_buf[512];                  // URI string backing store
    u8 repo_pbuf[FILE_PATH_MAX_LEN];
    u8p repo_pp[4];                   // $HOME/.be/<branch>/
    u8 work_pbuf[FILE_PATH_MAX_LEN];
    u8p work_pp[4];                   // worktree root
} BE;
typedef BE *BEp;

// --- Key builders (project/path?qualifier) ---

// "<project>/<path>"
ok64 BEKeyHead(u8s into, u8cs project, u8cs path);

// "<project>/<path>?v=<pad10_stamp>"
ok64 BEKeyVer(u8s into, u8cs project, u8cs path, ron60 stamp);

// "<project>/<path>?y=<twig>"
ok64 BEKeyTwig(u8s into, u8cs project, u8cs path, u8cs twig);

// "<project>/?v=<pad10_stamp>"
ok64 BEKeyCommit(u8s into, u8cs project, ron60 stamp);

// "<project>/?y=<twig>"
ok64 BEKeyTwigPtr(u8s into, u8cs project, u8cs twig);

// "<project>/?conf.<key>"  or  "?conf.<key>"
ok64 BEKeyConf(u8s into, u8cs project, u8cs confkey);

// Build key for current branch state (head or twig based on loc.query)
ok64 BEKeyCur(u8s into, BEp be, u8cs path);

// --- Lifecycle ---

// Create repo: make $HOME/.be/<branch>/, write .be file, open DB
ok64 BEInit(BEp be, u8cs be_uri, path8cg worktree);

// Find .be file, parse URI, open DB
ok64 BEOpen(BEp be, path8cg worktree);

// Close DB, zero struct
ok64 BEClose(BEp be);

// --- CRUD ---

// POST: worktree files → repo (commit)
ok64 BEPost(BEp be, int pathc, u8cs *paths, u8cs message);

// GET: repo → worktree files (checkout)
ok64 BEGet(BEp be, int pathc, u8cs *paths, u8cs twig);

// PUT: merge twig/branch into head
ok64 BEPut(BEp be, u8cs source_twig, u8cs message);

// DELETE: file, twig, or branch
ok64 BEDelete(BEp be, u8cs target);

// Checkpoint (branch)
ok64 BECheckpoint(BEp be, u8cs new_branch);

// --- Export ---

// Flatten BASON tree back to source text
ok64 BASTExport(u8s out, u64bp stack, u8csc data);

#endif  // LIBRDX_BE_H
