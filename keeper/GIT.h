#ifndef KEEPER_GIT_H
#define KEEPER_GIT_H

//  GIT: parsers for git objects (blob, tree, commit)
//
//  Blob: raw content, no parsing needed.
//
//  Tree format:  (<mode> <filename>\0<20-byte-sha1>)*
//  GITu8sDrainTree() consumes one entry per call.
//
//  Commit format:  (<field> <value>\n)*  \n  <body>
//  GITu8sDrainCommit() consumes one header per call;
//  on the blank line separator it returns empty field
//  and the commit body as value.

#include "abc/INT.h"

#define GIT_SHA1_LEN 20

con ok64 GITFAIL = 0x1049d3ca495;
con ok64 GITBADFMT = 0x1049d2ca34f59d;

//  Drain one tree entry: file mode+name into `file`, raw SHA1 into `sha1`.
//  Advances `obj`; returns NODATA when exhausted.
ok64 GITu8sDrainTree(u8cs obj, u8csp file, u8csp sha1);

//  Drain one commit header: field name into `field`, value into `value`.
//  On the blank-line separator, returns empty `field` and commit body
//  as `value`.  Returns NODATA when exhausted.
ok64 GITu8sDrainCommit(u8cs obj, u8csp field, u8csp value);

//  Parse the "tree <hex>" header from a commit body and write the
//  20-byte binary SHA-1 into tree_sha.  Returns GITBADFMT if the
//  tree line is missing or malformed.
ok64 GITu8sCommitTree(u8cs commit, u8 tree_sha[20]);

#endif
