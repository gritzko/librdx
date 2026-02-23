#ifndef ABC_PATH_H
#define ABC_PATH_H

//  PATH: UTF-8 path manipulation
//
//  Valid paths: UTF-8, no \r\t\n\0, / separated
//  All functions set path[1] to 0 for C string compatibility.
//
//  Uses u8g (gauge) for all path parameters - no ownership implied,
//  paths may live in mmapped buffers or anywhere else.

#include "01.h"
#include "BUF.h"
#include "OK.h"

// path8g is explicitly a gauge (u8g) - no ownership, may be mmapped
// Types: u8g = u8*[3], u8cg = u8 const*[3]
// always null terminated for C stdlib compatibility
#define path8b u8b
#define path8g u8g
#define path8gp u8gp
#define path8cg u8cg
#define path8cgp u8cgp

// one segment of a path as one fixed-length 0-terminated block
typedef bl08 pathseg8;

// Create path8g view into u8b buffer - returns pointer so modifications sync
// automatically buf[1..3] = {data_start, idle_start, idle_end} which is exactly
// path8g layout Uses u8bDataIdle to cast away const (u8b has const pointers)
#define path8gIn(buf) u8bDataIdle(buf)

// Create const path8cg view into u8b buffer (for read-only path operations)
#define path8cgIn(buf) ((path8cgp)u8bDataIdle(buf))

// Create const gauge from C string (read-only: idle=end)
#define u8cgOf(str) \
    {(u8cp)(str), (u8cp)(str) + strlen(str), (u8cp)(str) + strlen(str)}
#define path8cgOf(str) \
    {(u8cp)(str), (u8cp)(str) + strlen(str), (u8cp)(str) + strlen(str) + 1}

// Duplicate a gauge (for iteration)
#define a_dupg(T, n, g) T *n[3] = {(g)[0], (g)[1], (g)[2]}
#define a_dupcg(T, n, g) T const *n[3] = {(g)[0], (g)[1], (g)[2]}

// Error codes
con ok64 PATHFAIL = 0x64a7513ca495;  // general failure
con ok64 PATHBAD =
    0x1929d44b28d;  // invalid path (bad UTF-8 or forbidden chars)
con ok64 PATHNOROOM = 0x64a7515d86d8616;  // no space for 0 terminator

fun b8 path8cgOK(path8cg path) {
    return u8cgOK(path) && path[1] < path[2] && *path[1] == 0;
}
fun b8 path8gOK(path8cg path) { return path8cgOK(path); }
// Set null terminator for C string compatibility
// Returns PATHNOROOM if no space for terminator
fun ok64 path8gTerm(path8g path) {
    if (path[1] >= path[2]) return PATHNOROOM;
    *(u8 *)path[1] = 0;
    return OK;
}
fun ok64 path8cgTerm(path8cg path) { return path8gTerm((u8 **)path); }

// Check if path is absolute (starts with /)
b8 path8gIsAbsolute(path8cg path);

// Verify path conformance: UTF-8, no \r\t\n\0
ok64 path8gVerify(path8cg path);

// Consume one segment from path, returns END when exhausted
// Modifies path to point past the consumed segment
// Segment output is filled with the next segment's bounds
ok64 path8gNext(path8cg path, u8csp segment);

// Duplicate path from orig into into
ok64 path8gDup(path8g into, path8cg orig);

// Push one segment onto path (adds / separator if needed, 0-term)
ok64 path8gPush(path8g path, u8cs segment);

// Pop last segment from path (removes trailing component after last /,
// 0-terminates)
ok64 path8gPop(path8g path);

// Push segment with X characters randomized (for temp files)
ok64 path8gAddTmp(path8g path, u8cs tmpl);

// Add relative path to existing path, 0-terminate
ok64 path8gAdd(path8g into, path8cg rel);

// Compute relative path: rel = path from absbase to abs
// Both absbase and abs must be absolute paths
ok64 path8gRelative(path8g rel, path8cg absbase, path8cg abs);

// Algebraically resolve relative path against base (no filesystem access)
// Handles ., .., normalizes result
ok64 path8gAbsolute(path8g abs, path8cg base, path8cg rel);

// Resolve path in actual filesystem (like realpath)
// Follows symlinks, returns canonical absolute path
ok64 path8gReal(path8g real, path8cg path);

// Normalize path: resolve ./, ../, collapse // sequences
// Does not access filesystem
ok64 path8gNorm(path8g norm, path8cg orig);

// Extract basename (last component after final /)
// Returns view into path, empty slice for "/" or trailing slash
void path8gBase(u8csp out, path8cg path);

// Extract dirname (everything before final /)
// Returns view into path, "." for no directory component
void path8gDir(u8csp out, path8cg path);

#endif  // ABC_PATH_H
