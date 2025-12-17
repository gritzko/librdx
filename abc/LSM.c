#include "LSM.h"

#include "SKIP.h"
#include "abc/OK.h"
#include "abc/S.h"
#include "abc/TLV.h"

ok64 LSMnext(u8s into, u8css lsm, u8csz z, u8ys y) {
    sane(u8sOK(into) && u8cssOK(lsm) && z && y);
    u8cs next = {}, _;
    a_pad(u8cs, in, LSM_MAX_INPUTS);

    do {
        call(TLVDrain$, next, **lsm);
        while (!$empty(**lsm) && (~TLVaA & ****lsm) == SKIP_TLV_TYPE)
            call(TLVDrain$, _, **lsm);
        call(u8cssFeedP, in_idle, &next);
        if ($empty(**lsm)) {
            u8csSwap($head(lsm), $last(lsm));
            --$term(lsm);
            if ($empty(lsm)) break;
        }
        u8cssDownZ(lsm, z);
    } while (!z($head(lsm), &next) && !z(&next, $head(lsm)));

    if ($len(in_data) == 1) {
        call(u8sFeed, into, next);
    } else {
        call(y, into, in_data);
    }
    done;
}

ok64 LSMdrainruns(u8csb heap, u8cs input, u8csz cmp) {
    sane(Bok(heap) && $ok(input) && u8csbHasRoom(heap) && cmp != NULL);
    u8cs last = {};
    call(TLVDrain$, last, input);
    a_dup(u8c, run, last);
    while (!$empty(input) && $len(u8csbIdle(heap)) > 1) {
        u8cs rec;
        call(TLVDrain$, rec, input);
        int z = cmp(&last, &rec);
        if (z >= 0) {
            call(HEAPu8csPushZ, heap, &run, cmp);
            $mv(run, rec);
        } else {
            run[1] = rec[1];
        }
        $mv(last, rec);
    }
    call(HEAPu8csPushZ, heap, &run, cmp);
    done;
}

ok64 LSMsort1(size_t* runs, u8s into, u8cs data, u8csz cmp, u8ys mrg) {
    sane($ok(into) && $ok(data) && $len(into) >= $len(data) && cmp != NULL &&
         mrg != NULL);
    *runs = 0;
    while (!$empty(data)) {
        aBpad2(u8cs, runs, LSM_MAX_INPUTS);
        call(LSMdrainruns, runsbuf, data, cmp);
        if ($len(runsdata) == 1 && *runs == 0) return OK;
        while (!$empty(runsdata)) {
            call(LSMnext, into, runsdata, cmp, mrg);
        }
        ++*runs;
    }
    done;
}

ok64 LSMsort(u8s data, u8csz cmp, u8ys mrg, u8s tmp) {
    sane($ok(tmp) && $ok(data) && $len(tmp) >= $len(data) && cmp != NULL &&
         mrg != NULL);
    size_t runs = 0;
    do {
        a_dup(u8, out, tmp);
        a_dup(u8c, in, data);
        call(LSMsort1, &runs, out, in, cmp, mrg);
        if (runs == 0) return OK;
        u8cs in2 = {tmp[0], out[0]};
        $u8 out2 = {data[0], data[0] + $len(in2)};
        if (runs == 1) {
            $mv(data, out2);
            u8sCopy(data, in2);
            return OK;
        }
        call(LSMsort1, &runs, out2, in2, cmp, mrg);
        data[1] = out2[0];
    } while (runs > 1);
    done;
}
