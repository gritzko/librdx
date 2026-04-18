#ifndef DOG_HOME_H
#define DOG_HOME_H

#include "abc/B.h"
#include "abc/BUF.h"
#include "abc/PATH.h"

con ok64 NOHOME = 0x5d845858e;
con ok64 NOCONF = 0x8e603bce7bc;

#define HOME_ARENA_SIZE (1ULL << 32)   // 4 GB VA, pages on demand
#define HOME_CONFIG_MAX (1UL << 16)    // 64 KB is plenty for .dogs/config

// Per-invocation ambient state for every dog.  Owned by the top of
// the call chain; each dog embeds a `home *base` in its state struct
// and propagates it through DOGOpen.  All HOME functions take `home
// *` as their first argument.
typedef struct {
    path8b root;     // worktree root, NUL-termed
    u8b    config;   // mmap of <root>/.dogs/config (empty if none)
    u8b    arena;    // scratch: 4 GB VA, stack-like consumption.  Each
                     // dog function must rewind the arena to its entry
                     // state before returning.  Use Bu8mark + Bu8rewind
                     // around any cross-dog call as a safety net.
    b8     rw;
} home;

// Initialize a `home` in place.  `at` is either an explicit repo root
// (absolute path) or empty to auto-detect via HOMEFindDogs from cwd.
// rw=YES allows downstream dogs to create their `.dogs/<name>/` dirs.
// Reserves the arena, mmaps `.dogs/config` if present.
ok64 HOMEOpen(home *h, u8cs at, b8 rw);

// Release arena, config mmap, and path buffer.
ok64 HOMEClose(home *h);

// Walk up from cwd to the workspace dir (first ancestor with .git or
// .dogs).  Feeds the found path into h->root.  Returns NOHOME if none.
ok64 HOMEFind(home *h);

// Find the directory where .dogs/ should live:
//   1. Walk up to .git or .dogs.
//   2. If .git is a directory → use that dir.
//   3. If .git is a file (worktree) → follow to the parent repo root.
//   4. If step 3 fails → fall back to the worktree dir from step 1.
ok64 HOMEFindDogs(home *h);

// Follow a .git worktree file to the parent repo root.  On success
// feeds the parent repo root (the dir containing the parent .git
// directory) into h->root.  Any failure leaves h->root unchanged.
ok64 HOMEFollowWorktree(home *h, path8s gitfile);

// Resolve a peer binary into `out`: same directory as `argv0`
// (preserving symlinks), or, if `argv0` has no '/', the PATH entry
// that holds it.  Falls back to feeding just `name`.
ok64 HOMEResolveSibling(home *h, path8b out, u8csc name, u8csc argv0);

// Read one value from <root>/.dogs/config (TOML) addressed by a dotted
// path-style `needle` — e.g. for `[a.b] c = "v"` caller builds
// `a_path(n, "a", "b", "c")` and passes `$path(n)`.  Feeds the value
// bytes into `value`, advancing value[0] past them.  Returns NOCONF if
// the file is absent or the needle doesn't resolve.  Lexer errors
// propagate.
ok64 HOMEGetConfig(home *h, u8s value, path8s needle);

#endif
