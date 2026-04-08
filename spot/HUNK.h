#ifndef SPOT_HUNK_H
#define SPOT_HUNK_H

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

// Serialize a hunk as a nested TLV record.  Advances into[0].
ok64 HUNKu8sFeed(u8s into, HUNKhunk const *hk);

// Deserialize a hunk from TLV.  Advances from[0].
// Slices in hk point into the original data (zero-copy).
ok64 HUNKu8sDrain(u8cs from, HUNKhunk *hk);

#endif
