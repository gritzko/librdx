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

// Get codec name for file extension ("c", "py", "text", etc).
void BASTCodec(u8csp codec, u8csc ext);

// Extension to 18-bit ftype (3 RON64 chars packed).
// ext includes the dot, e.g. ".c". Unknown → 0.
u32 BASTFtype(u8csc ext);
// 18-bit ftype → extension (with dot) into buffer. ftype 0 → empty.
ok64 BASTFtypeExt(u8s ext, u32 ftype);

// BAST node tag for symbol name leaf (the identifier inside E/I nodes)
#define BAST_TAG_NAME 'F'

#endif
