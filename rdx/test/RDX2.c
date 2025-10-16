
#include "abc/01.h"
#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/TLV.h"
#include "rdx/JDR.h"
// and the new version
#include "rdx/RDX2.h"

ok64 RDX2MergeTest() {
    sane(1);
    a$strc(jdrExt, ".jdr");
    eats(u8cs, arg, Bu8csdata(STD_ARGS)) {
        if (!u8csHasSuffix(*arg, jdrExt)) continue;
        Bu8 jdr = {};
        call(FILEmapro, jdr, *arg);

        $println(*arg);
        a_pad(u8, rdx, PAGESIZE);
        a_pad(u8, err, 256);
        try(JDRparse, u8bIdle(rdx), u8bIdle(err), u8cbData(jdr));
        nedo { $println(u8cbData(err)); }
        then {
            u8cs rdxdata, correct, rec;
            u8csDup(rdxdata, u8cbData(rdx));
            a_pad(u8cs, inputs, 128);
            scan(TLVDrain$, rec, rdxdata) { call(u8csbFeed1, inputs, rec); }
            u8csDup(correct, *$last(inputs_data));
            inputs_data[1]--;
            a_pad(u8, fact, PAGESIZE);
            call(RDXu8sMerge, fact_idle, inputs_data);
            if ($eq(fact_data, correct)) done;
            // OK, some error
            $println(u8cbData(jdr));
            a_pad(u8, jdr, PAGESIZE);
            call(JDRfeed, jdr_idle, fact_datac);
            $println(jdr_datac);
        }

        call(FILEunmap, jdr);
    }
    done;
}

MAIN(RDX2MergeTest);
