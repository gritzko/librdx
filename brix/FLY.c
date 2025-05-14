
#include "FLY.h"

#include "abc/PRO.h"

a$strc(FLYdir, ".rdx/fly/");

ok64 FLYnew(FLY* fly, $cu8c path) {
    sane(fly != nil && $ok(path));
    if (FILEisdir(RDXdir) != OK) call(FILEmakedir, RDXdir);
    if (FILEisdir(BRIXdir) != OK) call(FILEmakedir, BRIXdir);
    if (FILEisdir(FLYdir) != OK) call(FILEmakedir, FLYdir);
    done;
}

ok64 FLYopen(FLY* fly, $cu8c path) {
    sane(fly != nil && $ok(path));
    call(BRIXopenrepo, &fly->brix, path);
    call(WALopen, &fly->wal, path);
    done;
}

ok64 FLYclose(FLY* fly) {
    sane(fly != nil);
    call(BRIXcloserepo, &fly->brix);
    call(WALclose, &fly->wal);
    done;
}
