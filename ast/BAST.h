#ifndef LIBRDX_BAST_H
#define LIBRDX_BAST_H

#include "json/BASON.h"

// Parse source into BASON tree using tree-sitter.
// Named nodes become arrays, source text slices are strings.
// Flattening all string leaves in order reproduces the original file.
// ext is the file extension with dot (e.g., ".c", ".py").
ok64 BASTParse(u8bp buf, u64bp idx, u8csc source, u8csc ext);

// Get tree-sitter language for file extension (including dot).
// Returns NULL if extension is not recognized.
struct TSLanguage;
const struct TSLanguage *BASTLanguage(u8csc ext);

#endif
