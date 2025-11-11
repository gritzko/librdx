//
// Created by gritzko on 11/6/25.
//
#include "JABC.hpp"
#include "rdx/RDX2.h"
#include "rdx/JDR2.h"
#include "rdx/BRIX2.h"
#include "abc/PRO.h"

u8b JABC_RDX_STAGE = {};

JABC_FN_DEFINE(JABCrdxParse) {
    JABC_FN_ARG_ALLOC_STRING(0, jdr, "no tip tag specified");
    a_path(defhome, ".rdx/");
    u8cs rec;

    ok64 o = BRIXu8bAdd(JABC_RDX_STAGE, rec);
    if (o==noroom) {
        // bump  todo maybe inf canvas
        o = BRIXu8bResize2(JABC_RDX_STAGE);
    }
    if (o!=OK) JABC_FN_THROW(ok64str(o));


    // if ($len(homepath) == 0) u8csDup(homepath, defhome);
    JABC_FN_RETURN_UNDEFINED;
}

ok64 JABCrdxInstall() {
    sane(1);
    int home;
    // todo len
    // todo no file
    call(BRIXu8bCreate, JABC_RDX_STAGE, home, nullptr);

    JABC_API_OBJECT(brix);
    JABC_API_FN(brix, "parse", JABCrdxParse);
    return notimplyet;
}

ok64 JABCrdxUninstall() {
    return notimplyet;
}
