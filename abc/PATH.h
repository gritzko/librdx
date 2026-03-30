#ifndef ABC_PATH_H
#define ABC_PATH_H

//  PATH: UTF-8 path manipulation
//
//  Valid paths: UTF-8, no \r\t\n\0, / separated
//  All functions set path[1] to 0 for C string compatibility.
//
//  Naming: PATHu8s* (core, on slices), PATHu8g* (gauge wrappers),
//  PATHu8b* (buffer wrappers). u8g/u8b call u8s where applicable.

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
#define PATHu8gIn(buf) u8bDataIdle(buf)

// Create const path8cg view into u8b buffer (for read-only path operations)
#define PATHu8cgIn(buf) ((path8cgp)u8bDataIdle(buf))

// Create const gauge from C string (read-only: idle=end)
#define u8cgOf(str) \
    {(u8cp)(str), (u8cp)(str) + strlen(str), (u8cp)(str) + strlen(str)}
#define PATHu8cgOf(str) \
    {(u8cp)(str), (u8cp)(str) + strlen(str), (u8cp)(str) + strlen(str) + 1}

// Duplicate a gauge (for iteration)
#define a_dupg(T, n, g) T *n[3] = {(g)[0], (g)[1], (g)[2]}
#define a_dupcg(T, n, g) T const *n[3] = {(g)[0], (g)[1], (g)[2]}

// Error codes
con ok64 PATHFAIL = 0x64a7513ca495;  // general failure
con ok64 PATHBAD =
    0x1929d44b28d;  // invalid path (bad UTF-8 or forbidden chars)
con ok64 PATHNOROOM = 0x64a7515d86d8616;  // no space for 0 terminator

// --- Validators ---

fun b8 PATHu8cgOK(path8cg path) {
    return u8cgOK(path) && path[1] < path[2] && *path[1] == 0;
}
fun b8 PATHu8gOK(path8cg path) { return PATHu8cgOK(path); }

// --- u8s core functions (operate on slices/idle pairs) ---

// Set null terminator for C string compatibility (idle pair)
fun ok64 PATHu8sTerm(u8s idle) {
    if (idle[0] >= idle[1]) return PATHNOROOM;
    *(u8 *)idle[0] = 0;
    return OK;
}

// Check null terminator on const gauge
fun ok64 PATHu8cgTerm(path8cg path) { return PATHu8sTerm((u8 **)path); }

// Check if path is absolute (starts with /)
fun b8 PATHu8sIsAbsolute(u8csc path) {
    if (!$ok(path) || $empty(path)) return NO;
    return *path[0] == '/';
}

// Verify path conformance: UTF-8, no \r\t\n\0
ok64 PATHu8sVerify(u8csc path);

// Validate a segment (not a full path, just raw u8cs input)
ok64 PATHu8sVerifySegment(u8cs segment);

// Feed a slice into path idle, 0-terminate
ok64 PATHu8sFeed(u8s idle, u8csc data);

// Consume one segment from path, advances path[0]
ok64 PATHu8sDrain(u8cs path, u8csp seg);

// Extract basename (view into path)
void PATHu8sBase(u8csp out, u8csc path);

// Extract dirname (view into path)
void PATHu8sDir(u8csp out, u8csc path);

// Extract file extension (after last dot in basename, without the dot)
void PATHu8sExt(u8csp out, u8csc path);

// Add tmp segment: / + tmpl + randomize + \0
ok64 PATHu8sAddTmp(u8s idle, u8cs tmpl, u8csc data);

// Aliases
#define PATHu8sNext PATHu8sDrain

// --- u8g functions (gauge-level) ---

// Term: null-terminate gauge
fun ok64 PATHu8gTerm(path8g path) { return PATHu8sTerm(path + 1); }

// Feed: copy data into gauge, 0-terminate
fun ok64 PATHu8gFeed(path8g into, u8csc data) {
    return PATHu8sFeed(into + 1, data);
}

// IsAbsolute
fun b8 PATHu8gIsAbsolute(path8cg path) {
    u8cs s = {path[0], path[1]};
    return PATHu8sIsAbsolute(s);
}

// Verify
fun ok64 PATHu8gVerify(path8cg path) {
    u8cs s = {path[0], path[1]};
    return PATHu8sVerify(s);
}

// Drain: consume one segment from gauge
fun ok64 PATHu8gDrain(path8cg path, u8csp seg) {
    u8cs t = {path[0], path[1]};
    ok64 o = PATHu8sDrain(t, seg);
    path[0] = t[0];
    return o;
}

// Base
fun void PATHu8gBase(u8csp out, path8cg path) {
    u8cs s = {path[0], path[1]};
    PATHu8sBase(out, s);
}

// Dir
fun void PATHu8gDir(u8csp out, path8cg path) {
    u8cs s = {path[0], path[1]};
    PATHu8sDir(out, s);
}

// Ext
fun void PATHu8gExt(u8csp out, path8cg path) {
    u8cs s = {path[0], path[1]};
    PATHu8sExt(out, s);
}

// Gauge-only functions (complex, declared extern)
ok64 PATHu8gPush(path8g path, u8cs segment);
ok64 PATHu8gPop(path8g path);
ok64 PATHu8gDup(path8g into, path8cg orig);
ok64 PATHu8gAdd(path8g into, path8cg rel);
ok64 PATHu8gAddTmp(path8g path, u8cs tmpl);
ok64 PATHu8gRelative(path8g rel, path8cg absbase, path8cg abs);
ok64 PATHu8gAbsolute(path8g abs, path8cg base, path8cg rel);
ok64 PATHu8gReal(path8g real, path8cg path);
ok64 PATHu8gNorm(path8g norm, path8cg orig);

// Aliases
#define PATHu8gNext PATHu8gDrain

// --- u8b functions (buffer wrappers) ---

fun ok64 PATHu8bTerm(path8b p) { return PATHu8gTerm(PATHu8gIn(p)); }
fun ok64 PATHu8bFeed(path8b p, u8csc s) {
    return PATHu8gFeed(PATHu8gIn(p), s);
}
fun ok64 PATHu8bPush(path8b p, u8cs seg) {
    return PATHu8gPush(PATHu8gIn(p), seg);
}
fun ok64 PATHu8bPop(path8b p) { return PATHu8gPop(PATHu8gIn(p)); }
fun ok64 PATHu8bDup(path8b into, path8b from) {
    return PATHu8gDup(PATHu8gIn(into), PATHu8cgIn(from));
}
fun ok64 PATHu8bPushCStr(path8b p, const char *s) {
    u8cs seg = {(u8cp)s, (u8cp)s + strlen(s)};
    return PATHu8gPush(PATHu8gIn(p), seg);
}
fun ok64 PATHu8bAddTmp(path8b p, u8cs tmpl) {
    return PATHu8gAddTmp(PATHu8gIn(p), tmpl);
}
fun ok64 PATHu8bAdd(path8b into, path8cg rel) {
    return PATHu8gAdd(PATHu8gIn(into), rel);
}
fun void PATHu8bExt(u8csp out, path8b buf) {
    PATHu8sExt(out, u8bDataC(buf));
}

// Feed first slice as base path, push rest as segments
fun ok64 PATHu8bBuildN(path8b p, u8csp *slices) {
    if (!*slices) return OK;
    ok64 o = PATHu8bFeed(p, *slices);
    if (o != OK) return o;
    for (slices++; *slices; slices++) {
        o = PATHu8bPush(p, *slices);
        if (o != OK) return o;
    }
    return OK;
}

// --- Stack path macros ---

// a_path(name) -- empty path buffer
// a_path(name, base) -- feed base slice (may contain /)
// a_path(name, base, seg1, seg2) -- feed base, push segments
// All args are u8cs/u8csc slices. Use a_cstr() for literals.
#define a_path(n, ...)                                       \
    a_pad(u8, n, FILE_PATH_MAX_LEN);                        \
    PATHu8bTerm(n);                                          \
    __VA_OPT__({                                             \
        u8csp _sl_##n[] = {__VA_ARGS__, NULL};               \
        PATHu8bBuildN(n, _sl_##n);                           \
    })

// a_abspath(name, seg1, ...) -- absolute path (starts with /)
// All args are u8cs slices pushed as segments.
#define a_abspath(n, ...)                                    \
    a_pad(u8, n, FILE_PATH_MAX_LEN);                        \
    u8sFeed1(n##_idle, '/');                                 \
    PATHu8bTerm(n);                                          \
    __VA_OPT__({                                             \
        u8csp _sl_##n[] = {__VA_ARGS__, NULL};               \
        PATHu8bBuildN(n, _sl_##n);                           \
    })

#endif  // ABC_PATH_H
