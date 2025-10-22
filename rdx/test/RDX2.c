
#include "abc/01.h"
#include "abc/FILE.h"
#include "abc/PRO.h"
#include "abc/TLV.h"
#include "rdx/JDR2.h"
// and the new version
#include "rdx/RDX2.h"

ok64 RDX2MergeTest() {
    sane(1);
    a_cstr(jdrExt, ".jdr");
    eats(u8cs, arg, u8csbData(STD_ARGS)) {
        if (!u8csHasSuffix(*arg, jdrExt)) continue;
        Bu8 jdr = {};
        call(FILEMapRO, jdr, *arg);

        $println(*arg);
        a_pad(u8, rdx, PAGESIZE);
        a_pad(u8, err, 256);
        try(RDXutf8sParse, u8cbData(jdr), rdx, u8bIdle(err));
        nedo { $println(u8cbData(err)); }
        then {
            u8cs rdxdata, correct, rec;
            u8csDup(rdxdata, u8cbData(rdx));
            a_pad(u8cs, inputs, 128);
            scan(TLVDrain$, rec, rdxdata) { call(u8csbFeed1, inputs, rec); }
            u8csDup(correct, *$last(inputs_data));
            inputs_data[1]--;  // todo pop
            a_pad(u8, fact, PAGESIZE);
            call(RDXu8sMerge, fact, inputs_data);
            if ($eq(fact_data, correct)) continue;
            // OK, some error
            $println(u8cbData(jdr));
            a_pad(u8, jdr2, PAGESIZE);
            call(RDXutf8sFeed, jdr2_idle, fact_datac);
            $println(jdr2_datac);
        }

        call(FILEUnMap, jdr);
    }
    done;
}

MAIN(RDX2MergeTest);
