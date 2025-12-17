
#include "LSM.h"

#include "RDX.h"
#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/S.h"
#include "abc/TEST.h"

con ok64 SRC_ABC = 0x259a7;

ok64 LSMTestBasics() {
    sane(1);
    a_path(path, "/tmp/LSMTestBasics");
    u8b buf = {};
    call(FILEMapCreate, buf, path, MB);

    // rdx lsm = {.format=RDX_FORMAT_LSM, .host=buf};
    a_cstr(jdrstr, "{@abc-120 one:1 two:2.0 three:\"3\"}");
    rdx jdr = {.format = RDX_FORMAT_JDR};
    u8csFork(jdrstr, jdr.data);
    rdx lsm = {.format = RDX_FORMAT_LSM};
    u8sFork(u8bIdle(buf), lsm.into);

    /*
    rdxp top = rdxbAtP(lsm, 0);
    top->type = RDX_TYPE_EULER;
    top->r.src = SRC_ABC;
    top->r.seq = 1;
    call(rdxNext, top);

    call(rdxbInto, lsm);
    call(rdxbCopy, lsm, jdr);
    call(rdxbOuto, lsm);
    */

    call(FILETrimMap, buf);
    call(FILEUnMap, buf);

    // todo open, read

    done;
}

pro(RDXtest) {
    sane(1);
    call(LSMTestBasics);
    done;
}

TEST(RDXtest);
