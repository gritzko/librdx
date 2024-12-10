#ifndef ABC_CT_H
#define ABC_CT_H
#include "BUF.h"
#include "OK.h"

#define CTreset(ct) Breset1(ct)
#define CTidle(b) ((u8**)b + 1)
#define CTdata(b) ((u8c**)b + 0)
#define CTdatac(b) ((u8c**)b + 0)

con ok64 CTnoroom = 0xc73cf6cf274c;
con ok64 CTnodata = 0x978968cf274c;
con ok64 CTnone = 0xa72cf274c;
con ok64 CTbad = 0x2896674c;

ok64 CTinsert(Bu8 ct, ok64 var);

ok64 CTsplice(Bu8 ct, ok64 var);

ok64 CTfeed(Bu8 ct, $u8c insert);

ok64 CTrender($u8 into, Bu8 ct);

#endif
