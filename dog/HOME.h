#ifndef DOG_HOME_H
#define DOG_HOME_H

#include "abc/PATH.h"

con ok64 NOHOME = 0x4836c65a56;

// Walk up from cwd to the workspace dir (first ancestor with .git).
// This is the worktree checkout dir — use it for `git ls-files` and
// for resolving file paths.
// Returns FAILSANITY if no .git is found above cwd.
ok64 HOMEFind(path8b out);

// Find the directory where .dogs/ should live:
//   1. Walk up to .git.
//   2. If .git is a directory → use that dir.
//   3. If .git is a file (worktree) → follow to the parent repo root.
//   4. If step 3 fails → fall back to the worktree dir from step 1.
ok64 HOMEFindDogs(path8b out);

// Follow a .git worktree file to the parent repo root.  On success,
// writes the parent repo root (the dir containing the parent .git
// directory) into `out`.  Any failure (unreadable file, malformed
// content, dangling gitdir, unexpected layout) returns an error and
// leaves `out` unmodified.
ok64 HOMEFollowWorktree(path8b out, path8cg gitfile);

// Resolve a sibling of the current executable: looks for `name` in
// dirname(/proc/self/exe).  If found and executable, writes the
// absolute path into `out` (NUL-terminated).  Otherwise writes the
// bare `name` so the caller can fall back to PATH lookup via execvp.
// Always returns OK (the bare-name fallback is a valid result).
ok64 HOMEResolveSibling(char *out, size_t outsz, char const *name);

#endif
