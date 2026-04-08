#ifndef DOG_HUNK_H
#define DOG_HUNK_H

#include "abc/TLV.h"
#include "dog/TOK.h"

// TLV type letters for hunk records
#define HUNK_TLV      'H'  // outer container
#define HUNK_TLV_TTL  'T'  // title text
#define HUNK_TLV_TXT  'X'  // source text bytes
#define HUNK_TLV_TOK  'K'  // tok32 array (packed u32 LE)
#define HUNK_TLV_HILI 'I'  // sparse tok32 bg highlights

// A serializable code hunk
typedef struct {
    u8cs title;    // hunk header ("--- path :: func ---")
    u8cs text;     // source text bytes
    tok32cs toks;  // packed tok32: syntax fg
    tok32cs hili;  // sparse tok32: bg highlights ('I'=INS, 'D'=DEL)
} HUNKhunk;

// Producer callback: yields one hunk at a time.  Slices in `hk` are
// borrowed for the duration of the call (zero-copy into source buffers).
typedef ok64 (*HUNKcb)(HUNKhunk const *hk, void *ctx);

// Serialize a hunk as a nested TLV record.  Advances into[0].
ok64 HUNKu8sFeed(u8s into, HUNKhunk const *hk);

// Deserialize a hunk from TLV.  Advances from[0].
// Slices in hk point into the original data (zero-copy).
ok64 HUNKu8sDrain(u8cs from, HUNKhunk *hk);

// Render a hunk as plain ASCII, no ANSI, git-diff-ish style:
//   title line verbatim, then each text line prefixed with '+'/'-'/' '
//   based on hili spans ('I'=insert, 'D'=delete).
// If hili is empty, every line gets a leading space (grep/cat output).
// A blank line is appended after the hunk.  Advances into[0].
ok64 HUNKu8sFeedText(u8s into, HUNKhunk const *hk);

// Clip file-level toks to [lo,hi), arena-write rebased entries.
// Output slice points into `arena` after this returns.
void HUNKu32sClip(Bu8 arena, u32cs out, u32cs toks, u32 lo, u32 hi);

// Tokenize source bytes via dog/TOK lexer dispatch.
// Strips a leading dot from `ext` if present.  Output is packed tok32
// (tag + end offset) appended to `toks`.
ok64 HUNKu32bTokenize(u32bp toks, u8csc source, u8csc ext);

// Find file extension (incl. leading dot) within a path slice.
// Returns empty if no extension or path ends in '/'.
void HUNKu8sExt(u8cs out, u8cp path, size_t len);

// Format "--- path :: func ---" hunk title with smart truncation.
// filepath may be NULL, funcname may be empty ("").
ok64 HUNKu8sFormatTitle(u8s into, char const *filepath, char const *funcname);

// Maximum visible width of a formatted hunk title.
#define HUNK_TITLE_MAX 64

#endif
