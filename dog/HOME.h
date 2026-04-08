#ifndef DOG_HOME_H
#define DOG_HOME_H

#include "abc/PATH.h"

// Walk up from cwd to find the workspace root: the first ancestor dir
// containing a .git entry (file or directory). For worktrees (.git is a
// file), follow it to the parent repo root. On any failure, fall back
// to the dir containing the .git file. Returns FAILSANITY if no .git
// is found above cwd.
ok64 HOMEFind(path8b out);

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
