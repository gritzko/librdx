#ifndef LIBRDX_BE_H
#define LIBRDX_BE_H

#include "abc/FILE.h"
#include "abc/PATH.h"
#include "abc/RAP.h"
#include "abc/ROCK.h"
#include "abc/RON.h"
#include "abc/URI.h"
#include "ast/BAST.h"
#include "json/BASON.h"
#include "json/BIFF.h"
#include "VER.h"

// Error codes
con ok64 BEFAIL = 0x2ce3ca495;
con ok64 BEBAD = 0xb38b28d;
con ok64 BENONE = 0x2ce5d85ce;

// --- Key scheme prefixes ---
// stat:  → local-only cache (BASON: mtime, hash) — never replicated
// be:    → BASON tree / patch content (key has '*' suffix for exec)
// tri:   → trigram posting lists (BASON object, merge = set-union)
#define BE_SCHEME_STAT "stat"
#define BE_SCHEME_BE   "be"
#define BE_SCHEME_TRI  "tri"
#define BE_SCHEME_SYM  "sym"

// Local-only stat cache: mtime + content hash
typedef struct {
    u64 mtime;       // nanosecond-precision mtime (local only)
    u64 hash;        // RAPHash of file content
} BEstat;

// Build BASON stat value from BEstat
ok64 BEStatFeedBason(u8bp buf, u64bp idx, BEstat s);
// Drain BASON stat value into BEstat
ok64 BEStatDrainBason(BEstat *s, u8cs bason);
// Fill stat from file content + struct stat
ok64 BEStatFromFile(BEstat *s, struct stat *st, u8cs content);

// Build a scheme-prefixed key: scheme:path?query#fragment
ok64 BEKeyBuild(u8s into, u8cs scheme, u8cs path, u8cs query, u8cs fragment);

// Scratch buffer slots (1GB anonymous mmap each, lazy-paged)
enum {
    BE_READ = 0,   // ROCKGet results
    BE_RENDER = 1, // BASTExport output
    BE_PARSE = 2,  // BASTParse output
    BE_PATCH = 3,  // diff/merge intermediates
    BE_WRAP = 4,   // metadata BASON staging
    BE_PATHS = 5,  // collected relpaths in BEGetProject
    BE_SCRATCH = 6
};
#define BE_SCRATCH_LEN (1UL << 30)  // 1GB

// Max branches in .be URI query
#define BE_MAX_BRANCHES 16

// Beagle SCM handle
typedef struct {
    ROCKdb db;
    uri loc;                          // parsed .be URI
    u8b loc_buf;                      // URI string backing store
    path8b repo_pp;                   // $HOME/.be/<repo>/
    path8b work_pp;                   // worktree root
    u8p scratch[BE_SCRATCH][4];       // scratch buffers
    // Multi-branch: parsed from query "branch1&branch2&main"
    ron120 branches[BE_MAX_BRANCHES];
    int branchc;
    b8 initial;  // YES during first post after BEInit
    b8 to_stdout;  // YES → BEExportFile writes to stdout
} BE;
typedef BE *BEp;

// --- BE key format: path?query#fragment (URI) ---
// Keys are URIs. Build via uri struct + URIutf8Feed.
// Parse via URIutf8Drain. Query sub-structure: "STAMP-branch".

// Build query sub-structure "pad10_stamp-branch" into buffer
ok64 BEQueryBuild(u8s into, ron60 stamp, ron60 branch);

// Extract branch origin from key's query part (via VERParse)
ok64 BEKeyBranch(ron60 *branch, u8cs key);

// Extract timestamp from key's query part (via VERParse)
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

// POST data: parse source text, diff against DB, create waypoint.
// If branch is empty, uses be->branches[0].
ok64 BEPostData(BEp be, u8cs relpath, u8cs source, u8cs branch, u8cs message);

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

// --- Branch formula (see VER.h) ---

// --- Scan ---

// Scan callback: relpath, merged BASON, exec bit from key
typedef ok64 (*BEScanCBf)(voidp arg, u8cs relpath, u8cs bason, b8 is_exec);

// Scan all files under loc->path, merge base+waypoints per loc->query formula
ok64 BEScan(BEp be, uricp loc, BEScanCBf cb, voidp arg);

// Same but only files that have waypoints matching the formula
ok64 BEScanChanged(BEp be, uricp loc, BEScanCBf cb, voidp arg);

// Stat cache callback: relpath + cached stat (no content)
typedef ok64 (*BEStatCBf)(voidp arg, u8cs relpath, BEstat cached);

// Iterate stat: keys (local cache, no waypoints), call cb per file
ok64 BEScanStat(BEp be, u8cs prefix_filter,
                BEStatCBf cb, voidp arg);

// Update local stat: cache for a file
ok64 BEStatUpdate(BEp be, u8cs relpath, BEstat s);

// --- Status ---
ok64 BEStatusFiles(BEp be);

// --- Diff ---
// Colored diff of worktree changes vs repo state (all tracked files)
ok64 BEDiffFiles(BEp be, int pathc, u8cs *paths);

// --- Export ---

// Flatten BASON tree back to source text
ok64 BASTExport(u8s out, u64bp stack, u8csc data);

// Flatten BASON tree with ANSI syntax highlighting
ok64 BASTCat(u8s out, u64bp stack, u8csc data);

// File record callback: called for base and each matching waypoint
typedef ok64 (*BEFileCBf)(voidp arg, u8cs key, u8cs val);

// Scan base + formula-matching waypoints for a single file
ok64 BEScanFile(ROCKdbp db, u8cs project, u8cs relpath,
                ron120cs formcs, BEFileCBf cb, voidp arg);

// Merge file content: formula-filtered, thread-safe (allocates own buffers)
ok64 BEMergeFile(ROCKdbp db, u8cs project, u8cs relpath,
                 ron120cs formcs, u8bp result);

// GET single file: content from be:, merge. is_exec from key trailing '*'.
ok64 BEGetFileMerged(BEp be, u8cs project, u8cs relpath,
                     u8bp result, b8 *is_exec_out);

// --- Trigram index ---

// Compute 2-char RON64 hashlet from file path (12 bits = 4096 buckets)
ok64 BEHashlet(u8s into, u8cs path);

// Extract ASCII trigrams from BASON string leaves, call cb for each unique
typedef ok64 (*BETriCBf)(voidp arg, u8cs trigram);
ok64 BETriExtract(u8csc bason, BETriCBf cb, voidp arg);

// Extract symbol names from BASON 'B'-tagged leaves, call cb for each
typedef ok64 (*BESymCBf)(voidp arg, u8cs symbol);
ok64 BESymExtract(u8csc bason, BESymCBf cb, voidp arg);

// AST node filter callback for BASTGrepNodes
// Return YES to include this node's source lines in output
typedef b8 (*BASTNodeCBf)(const bason *node, void *ctx);

// Walk BASON tree, select nodes via callback, output matching source lines
// with k context lines. Non-contiguous groups separated by "--\n".
ok64 BASTGrepNodes(u8s out, u8cs bason_data, int k,
                   BASTNodeCBf cb, void *ctx);

// Build unified diff BASON from old BASON + patch (from BASONDiff).
// Keys get 1-byte status prefix: '=' equal, '-' removed, '+' added,
// '~' container with changed children.
ok64 BASTDiffBuild(u8bp out, u64bp idx,
                   u64bp ostk, u8csc odata,
                   u64bp pstk, u8csc pdata);

// Render unified diff BASON (from BASTDiffBuild) with k context lines.
// Only changed lines (+ and -) shown, with ANSI colors.
// Non-contiguous groups separated by "--\n".
ok64 BASTDiffRender(u8s out, u8cs bason_data, int k);

// Line-level diff of two texts with k context lines.
// Deleted lines in red, added lines in green.
ok64 BASTTextDiff(u8s out, u8cs old_text, u8cs new_text, int k);

// Grep result callback: filepath, line number (1-based), matching line
typedef ok64 (*BEGrepCBf)(voidp arg, u8cs filepath, int lineno, u8cs line);

// Search: URI fragment = substring, query = branch filter (optional)
// be grep substr == be grep #substr == be grep //repo/proj?branch#substr
ok64 BEGrep(BEp be, uricp grep_uri, BEGrepCBf result_cb, voidp arg);

#endif  // LIBRDX_BE_H
