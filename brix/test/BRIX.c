#include "BRIX.h"

#include <limits.h>

#include "WAL.h"
#include "abc/01.h"
#include "abc/PRO.h"
#include "abc/SHA.h"
#include "abc/TEST.h"
#include "rdx/JDR.h"
#include "rdx/RDX.h"

ok64 WALtest() {
    sane(1);
    WAL wal = {};
    a$strc(fn, "/tmp/wal");
    call(WALcreate, &wal, fn, PAGESIZE);

    char* inout[][2] = {
        {"{@bob-123 1:one}", "{@bob-123 1:one}"},                   //
        {"{@bob-123 2:two}", "{@bob-123 1:one 2:two}"},             // merge
        {"{@bob-123 (@1 2)}", "{@bob-123 1:one (@1 2)}"},           // purge
        {"{@bob-123 3:three}", "{@bob-123 1:one (@1 2) 3:three}"},  //
        {"{@bob-123 4:four 5:five}",
         "{@bob-123 1:one (@1 2) 3:three 4:four 5:five}"},  // wal merge
        {"", ""},                                           //
    };

    for (int i = 0; *inout[i][0]; ++i) {
        a$rdx(rec1, inout[i][0], PAGESIZE);
        a$rdx(res1, inout[i][1], PAGESIZE);
        id128 key1 = {};
        call(RDXid, &key1, res1);
        aBcpad(u8, res, PAGESIZE);
        call(WALadd, &wal, rec1);
        call(WALget1, residle, &wal, key1);
        if (!$eq(res1, resdata)) {
            a$jdr(okjdr, res1, 128);
            a$jdr(resjdr, resdata, 128);
            $println(okjdr);
            $println(resjdr);
        }
        $testeq(res1, resdata);
        call(WALclose, &wal);
        call(WALopen, &wal, fn);
    }

    call(WALclose, &wal);
    done;
}

ok64 WALtoBRIXtest() {
    sane(1);
    a$strc(home, "/tmp/");
    a$strc(fn, "/tmp/wal");

    WAL wal = {};
    call(WALcreate, &wal, fn, PAGESIZE);
    a$rdx(rdx, "{@bob-123 1:one 2:two}", PAGESIZE);
    id128 id = {};
    call(RDXid, &id, rdx);
    call(WALadd1, &wal, rdx);
    sha256 top = {}, fresh = {};
    call(WAL2brick, &fresh, &top, &wal, home);
    call(WALclose, &wal);

    BRIX brix = {};
    aBcpad(u8, res, 128);
    call(BRIXopenrepo, &brix, home);
    call(BRIXopen, &brix, &fresh);
    call(BRIXget, residle, &brix, RDX_EULER, id);
    call(BRIXcloserepo, &brix);
    $testeq(rdx, resdata);

    done;
}

ok64 BRIXtest() {
    sane(1);
    call(WALtest);
    call(WALtoBRIXtest);
    done;
}

TEST(BRIXtest);
