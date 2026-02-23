#include "LSM.h"

#include "abc/OK.h"
#include "abc/S.h"

ok64 LSMNext(u8s into, u8css lsm, u8xs x, u8csz z, u8ys y) {
    sane(u8sOK(into) && u8cssOK(lsm) && x && z && y);
    u8cs next = {};
    a_pad(u8cs, in, LSM_MAX_INPUTS);

    do {
        call(x, next, **lsm);
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

ok64 LSMDrainRuns(u8csb heap, u8cs input, u8xs x, u8csz z) {
    sane(Bok(heap) && $ok(input) && u8csbHasRoom(heap) && x != NULL && z != NULL);
    u8cs last = {};
    call(x, last, input);
    a_dup(u8c, run, last);
    while (!$empty(input) && $len(u8csbIdle(heap)) > 1) {
        u8cs rec;
        call(x, rec, input);
        b8 less = z(&last, &rec);
        if (!less) {  // last >= rec, start new run
            call(HEAPu8csPushZ, heap, &run, z);
            $mv(run, rec);
        } else {
            run[1] = rec[1];
        }
        $mv(last, rec);
    }
    call(HEAPu8csPushZ, heap, &run, z);
    done;
}

ok64 LSMSort1(size_t* runs, u8s into, u8cs data, u8xs x, u8csz z, u8ys y) {
    sane($ok(into) && $ok(data) && $len(into) >= $len(data) && x != NULL &&
         z != NULL && y != NULL);
    *runs = 0;
    while (!$empty(data)) {
        aBpad2(u8cs, heap, LSM_MAX_INPUTS);
        call(LSMDrainRuns, heapbuf, data, x, z);
        if ($len(heapdata) == 1 && *runs == 0) return OK;
        while (!$empty(heapdata)) {
            call(LSMNext, into, heapdata, x, z, y);
        }
        ++*runs;
    }
    done;
}

ok64 LSMSort(u8s data, u8xs x, u8csz z, u8ys y, u8s tmp) {
    sane($ok(tmp) && $ok(data) && $len(tmp) >= $len(data) && x != NULL &&
         z != NULL && y != NULL);
    size_t runs = 0;
    do {
        a_dup(u8, out, tmp);
        a_dup(u8c, in, data);
        call(LSMSort1, &runs, out, in, x, z, y);
        if (runs == 0) return OK;
        u8cs in2 = {tmp[0], out[0]};
        $u8 out2 = {data[0], data[0] + $len(in2)};
        if (runs == 1) {
            $mv(data, out2);
            u8sCopy(data, in2);
            return OK;
        }
        call(LSMSort1, &runs, out2, in2, x, z, y);
        data[1] = out2[0];
    } while (runs > 1);
    done;
}
