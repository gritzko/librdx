#include "LSM.h"

#include "abc/TLV.h"

ok64 LSMnext($u8 into, $$u8c lsm, $u8cZfn cmp, $u8cYfn mrg) {
    sane($ok(into) && $ok(lsm) && cmp != nil && mrg != nil);
    $u8c next = {};
    aBpad2($u8c, in, LSM_MAX_INPUTS);

    do {
        call(TLVdrain$, next, **lsm);
        call($$u8cfeedp, inidle, &next);
        if ($empty(**lsm)) {
            $u8cswap($head(lsm), $last(lsm));
            --$term(lsm);
            if ($empty(lsm)) break;
        }
        HEAP$u8cdownf(lsm, cmp);
    } while (0 == cmp($head(lsm), &next));

    if ($len(indata) == 1) {
        call($u8feed, into, next);
    } else {
        call(mrg, into, indata);
    }
    done;
}

ok64 LSMsort($u8 into, $u8c input, $u8cZfn cmp, $u8cYfn mrg) {
    sane($ok(into) && $ok(input) && $len(into) >= $len(input) && cmp != nil &&
         mrg != nil);
    aBpad2($u8c, runs, LSM_MAX_INPUTS);
    $u8c run;
    call(TLVdrain$, run, input);
    a$dup(u8c, last, run);
    while (!$empty(input)) {
        $u8c rec;
        call(TLVdrain$, rec, input);
        int z = cmp(&last, &rec);
        if (z >= 0) {
            call(B$u8cpush, runsbuf, &run);
            $mv(run, rec);
        } else {
            run[1] = rec[1];
        }
        $mv(last, rec);
    }
    call(B$u8cpush, runsbuf, &run);
    // FIXME happy path

    call(LSMmerge, into, runsdata, cmp, mrg);
    done;
}

ok64 LSMmergehard($u8 into, $$u8c inputs, $u8cZfn cmp, $u8cYfn mrg) {
    sane($ok(into) && $ok(inputs) && cmp != nil && mrg != nil);
    aBpad2($u8c, lsm, LSM_MAX_INPUTS);
    aBcpad($u8c, post, LSM_MAX_INPUTS);
    aBpad2($u8c, eqs, LSM_MAX_INPUTS);
    for ($u8c* p = $head(inputs); p < $term(inputs); ++p) {
        call(HEAP$u8cpushf, lsmbuf, p, cmp);
    }
    a$dup(u8, in, into);
    $u8c next = {};
    u8c* last = in[0];
    while (!$empty(lsmdata)) {
        do {
            call(TLVdrain$, next, **lsmdata);
            call($$u8cfeedp, eqsidle, &next);
            if ($empty(**lsmdata)) {
                $u8cswap($head(lsmdata), $last(lsmdata));
                --$term(lsmdata);
                if ($empty(lsmdata)) break;
            }
            HEAP$u8cdownf(lsmdata, cmp);
        } while (0 == cmp($head(lsmdata), &next));

        if ($len(eqsdata) == 1) {
            call($u8feed, in, **eqsdata);
        } else {
            call(mrg, in, eqsdata);
        }
        Breset(eqsbuf);

        while (!$empty(lsmdata) && cmp($head(lsmdata), &next) < 0) {
            $u8c baddie;
            call(HEAP$u8cpopf, &baddie, lsmbuf, cmp);
            call(HEAP$u8cpushf, postbuf, &baddie, cmp);
        }

        if ($empty(lsmdata) && !$empty(postdata)) {
            $$u8cfeedall(lsmidle, postdata);
            Breset(postbuf);
            $u8c fresh = {last, in[0]};
            call(HEAP$u8cpushf, lsmbuf, &fresh, cmp);
            last = in[0];
        }
    }

    $u8c lastrun = {last, in[0]};
    $u8move(into, lastrun);
    *into += $size(lastrun);
    done;
}
