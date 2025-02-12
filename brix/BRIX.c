#include "BRIX.h"

#include <fcntl.h>

#include "abc/B.h"
#include "abc/BUF.h"
#include "abc/FILE.h"
#include "abc/LSM.h"

a$strc(BRIKext, ".brik");
a$strc(BRIXdir, ".brix");
a$strc(BRIXindex, "INDEX");

ok64 BRIXinit(BRIX* brix, $u8c path) {
    sane(brix != nil && !Bok(brix->store) && $ok(path));
    call(BBu8alloc, brix->store, LSM_MAX_INPUTS);
    call(Bu8alloc, brix->home, $len(path) + 128);
    u8** homeidle = Bu8idle(brix->home);
    u8c** homedata = Bu8cdata(brix->home);
    call($u8feed, homeidle, path);
    call($u8feed1, homeidle, '/');
    call($u8feed, homeidle, BRIXdir);
    call(FILEmakedir, homedata);
    call($u8feed1, homeidle, '/');

    u64 dl = $len(homedata);
    int fd = FILE_CLOSED;

    call($u8feed, homeidle, BRIXindex);
    call(FILEmapnew, (u8**)brix->index, &fd, homedata, PAGESIZE);
    call(FILEclose, &fd);
    call($u8retract, homedata, dl);

    done;
}

ok64 BRIXopen(BRIX* brix, $u8c path) {
    sane(brix != nil && Bnil(brix->store));
    call(BBu8alloc, brix->store, LSM_MAX_INPUTS);

    done;
}

ok64 BRIXclose(BRIX* brix) {
    sane(BRIXok(brix));
    if (Bok(brix->store)) call(BBu8free, brix->store);
    if (Bok(brix->index)) call(FILEunmap, (u8**)brix->index);
    if (Bok(brix->home)) call(Bu8free, brix->home);
    done;
}

ok64 BRIXpush(BRIX* brix, sha256c* head) {
    sane(BRIXok(brix) && head != nil);
    u8c** fn = Bu8cdata(brix->home);
    u8** fi = Bu8idle(brix->home);
    size_t dl = $len(fn);
    call(HEXsha256put, fi, head);
    call($u8feed, fi, BRIKext);
    int fd = FILE_CLOSED;
    a$dup(u8c, fnn, fn);
    $u8retract(fn, dl);
    Bu8 buf = {};
    call(FILEmapro, buf, &fd, fnn);
    call(FILEclose, &fd);
    call(BBu8feed1, brix->store, buf);
    done;
}

ok64 BRIXget($u8 into, BRIX const* brix, id128 key) {
    sane($ok(into) && brix != nil);
    aBpad2($u8c, entries, LSM_MAX_INPUTS);
    Bu8$ ssts = BBu8cdata(brix->store);
    for (Bu8* p = $head(ssts); p < $term(ssts); ++p) {
        u8 t = 0;
        $u8c val = {};
        ok64 o = SSTu128get(&t, val, *p, &key);
    }

    done;
}
