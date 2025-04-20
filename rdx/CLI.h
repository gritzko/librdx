#ifndef RDX_CLI_H
#define RDX_CLI_H
#include "RDX.h"
#include "UNIT.h"

static const ok64 CLIbadarg = 0xc5529a5a25dab;
static const ok64 CLInoverb = 0xc552cb3ea9da6;
static const ok64 CLIunknown = 0x3154b9cafcb3ef2;

extern u8c* RDXlits[];

typedef ok64 (*cmdfn)(void* ctx, $u8c args);

typedef struct {
    $u8c name;
    cmdfn fn;
    $u8c legend;
} CLIcmd;

ok64 JDRdrainargs($u8 into);

ok64 CLI(CLIcmd const commands[], void* context);

ok64 RDXingest(Bu8 buf, $u8c fn);
ok64 RDXstdingest(Bu8 buf);
ok64 RDXingestall(BBu8 buf, $u8c args);

#endif
