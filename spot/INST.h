#ifndef SPOT_INST_H
#define SPOT_INST_H

#include "abc/INT.h"

// Install spot as git diff/merge driver for the repo at reporoot.
// Sets git config, writes .git/info/attributes, installs post-commit hook.
ok64 INSTInstall(u8csc reporoot);

#endif
