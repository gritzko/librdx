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

// File metadata prefix (8 bytes prepended to every stored value)
#define BE_META_SIZE 8

typedef struct {
    u32 mtime;       // unix seconds (LE)
    u32 modeftype;   // mode(12) | ftype(18) | spare(2)
} BEmeta;

// Pack metadata into 8-byte buffer
ok64 BEMetaPack(u8s into, BEmeta m);
// Unpack metadata from 8-byte buffer
ok64 BEMetaUnpack(BEmeta *m, u8cs from);
// Fill metadata from stat + extension
ok64 BEMetaFromStat(BEmeta *m, struct stat *st, u8cs ext);

// Scratch buffer slots (1GB anonymous mmap each, lazy-paged)
enum {
    BE_READ = 0,   // ROCKGet results
    BE_RENDER = 1, // BASTExport output
    BE_PARSE = 2,  // BASTParse output
    BE_PATCH = 3,  // diff/merge intermediates
    BE_WRAP = 4,   // meta+bason concatenation
    BE_SCRATCH = 5
};
#define BE_SCRATCH_LEN (1UL << 30)  // 1GB

// Max branches in .be URI query
#define BE_MAX_BRANCHES 16

// Beagle SCM handle
typedef struct {
    ROCKdb db;
    uri loc;                          // parsed .be URI
    u8 loc_buf[512];                  // URI string backing store
    u8p repo_pp[4];                   // $HOME/.be/<repo>/
    u8p work_pp[4];                   // worktree root
    u8p scratch[BE_SCRATCH][4];       // scratch buffers
    // Multi-branch: parsed from query "branch1&branch2&main"
    u8cs branches[BE_MAX_BRANCHES];
    int branchc;
} BE;
typedef BE *BEp;

// --- BE key format: path?query#fragment (URI) ---
// Keys are URIs. Build via uri struct + URIutf8Feed.
// Parse via URIutf8Drain. Query sub-structure: "STAMP-branch".

// Build query sub-structure "pad10_stamp-branch" into buffer
ok64 BEQueryBuild(u8s into, ron60 stamp, u8cs branch);

// Parse query sub-structure into stamp + branch
ok64 BEQueryParse(ron60 *stamp, u8csp branch, u8cs query);

// Extract branch from key's query part (via URIutf8Drain + BEQueryParse)
ok64 BEKeyBranchSuffix(u8csp branch, u8cs key);

// Extract timestamp from key's query part (via URIutf8Drain + BEQueryParse)
ok64 BEKeyStamp(ron60 *stamp, u8cs key);

// --- Lifecycle ---

// Create repo: make $HOME/.be/<repo>/, write .be file, open DB
ok64 BEInit(BEp be, u8cs be_uri, path8cg worktree);

// Find .be file, parse URI, open DB
ok64 BEOpen(BEp be, path8cg worktree);

// Close DB, zero struct
ok64 BEClose(BEp be);

// Add/remove branch in .be URI
ok64 BEAddBranch(BEp be, u8cs branch);
ok64 BERemoveBranch(BEp be, u8cs branch);

// Set active branch for POST (must be in branches list)
ok64 BESetActive(BEp be, u8cs branch);

// --- CRUD ---

// POST: worktree files -> repo (independent waypoint)
ok64 BEPost(BEp be, int pathc, u8cs *paths, u8cs message);

// GET: repo -> worktree files (merge base + waypoints)
ok64 BEGet(BEp be, int pathc, u8cs *paths, u8cs branch);

// PUT: merge source branch waypoints into active branch
ok64 BEPut(BEp be, u8cs source_branch, u8cs message);

// DELETE: file (empty waypoint tombstone), or branch waypoints
ok64 BEDelete(BEp be, u8cs target);

// GET deps from .beget file (same repo for now)
ok64 BEGetDeps(BEp be, b8 include_opt);

// Checkpoint (fork repo)
ok64 BECheckpoint(BEp be, u8cs new_repo);

// Milestone: fold main waypoints into base, delete folded
ok64 BEMilestone(BEp be, u8cs name);

// --- Export ---

// Flatten BASON tree back to source text
ok64 BASTExport(u8s out, u64bp stack, u8csc data);

#endif  // LIBRDX_BE_H
