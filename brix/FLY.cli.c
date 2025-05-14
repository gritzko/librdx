#include "FLY.h"

#include <unistd.h>

#include "BRIX.h"
#include "abc/01.h"
#include "abc/B.h"
#include "abc/FILE.h"
#include "abc/HEX.h"
#include "abc/LSM.h"
#include "abc/OK.h"
#include "abc/PRO.h"
#include "abc/SHA.h"
#include "abc/TLV.h"
#include "rdx/CLI.h"
#include "rdx/JDR.h"
#include "rdx/RDX.h"
#include "rdx/RDXC.h"
#include "rdx/RDXZ.h"

a$strc(FLYhome, ".rdx/fly");

ok64 FLY_new(void* ctx, $u8c args) {
    FLY* fly = (FLY*)ctx;
    sane(ctx != nil);
    a$str(path, ".");
    id128 _;
    if (!$empty(args)) {
        RDXCdrainS(path, &_, args);
    }
    call(FLYnew, fly, path);
    done;
}

ok64 FLY_open(void* ctx, $u8c args) {
    sane($ok(args) && ctx != nil);
    FLY* fly = (FLY*)ctx;
    a$str(path, ".");
    call(FLYopen, fly, path);
    done;
}

ok64 FLY_close(void* ctx, $u8c args) {
    sane(!$empty(args) && ctx != nil);
    FLY* fly = (FLY*)ctx;
    done;
}

CLIcmd COMMANDS[] = {
    {$u8str("new"), FLY_new, $u8str("make a new FLY store")},  //
    {$u8str("open"), FLY_open, $u8str("open a FLY store")},    //
    {$u8str("close"), FLY_close, $u8str("close")},             //
    {$u8str(""), nil, nil},
};

ok64 FLYcli() {
    sane(1);
    FLY fly = {};
    a$dup($u8c, stdargs, B$u8cdata(STD_ARGS));
    aBcpad(u8, cmds, PAGESIZE);
    call(JDRdrainargs, cmdsidle);
    u8c$ cmds = cmdsdata;
    a$dup(u8c, path, FLYhome);

    call(CLI, COMMANDS, &fly);

    done;
}

MAIN(FLYcli);
