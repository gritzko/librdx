#ifndef DOG_LS_H
#define DOG_LS_H

//  LS: append-only buffer of NUL-terminated path strings.
//
//  Paths are stored back-to-back with trailing '\0'.  Identity of a
//  stored path = its byte offset within the buffer (useful as the
//  val of a sha -> path map).  In-memory only; nothing is persisted.
//
//  Typical use:
//
//    ls  paths;  LSOpen(&paths, 1 << 20);
//    u64 off;    LSFeed(&paths, u8cs_of("Makefile"), &off);
//    u8csc p  =  LSGet(&paths, off);   // {"Makefile", "Makefile"+8}
//    ...
//    LSClose(&paths);

#include "abc/01.h"
#include "abc/BUF.h"

con ok64 LSFAIL   = 0x1a3ca495;
con ok64 LSNOROOM = 0x1a35d86d8616;

typedef struct {
    Bu8 buf;  // underlying storage; DATA grows, IDLE shrinks
} ls;

//  Initialize with a virtual reservation of `reserve` bytes
//  (mapped anonymous, pages on demand).
ok64 LSOpen(ls *s, u64 reserve);

//  Append `path` plus a NUL terminator.  Writes the starting byte
//  offset into *off_out.  Returns LSNOROOM if the buffer's IDLE
//  region is too small.
ok64 LSFeed(ls *s, u8csc path, u64 *off_out);

//  Write a slice spanning the stored path at `off` into `out`; the
//  slice stops at the trailing NUL (not including it).  Behaviour is
//  undefined if `off` does not point at the start of a previously-Fed
//  record.
void LSGet(ls const *s, u64 off, u8csp out);

//  Current DATA size (bytes of stored strings including terminators).
fun u64 LSLen(ls const *s) { return (u64)u8bDataLen(s->buf); }

void LSClose(ls *s);

#endif
