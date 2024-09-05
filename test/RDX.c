#include "RDX.h"

#include <unistd.h>

#include "B.h"
#include "RDX.h"
#include "TEST.h"

pro(RDXtest1) {
    sane(1);
    aBpad(u8, pad, 0x1000);
    u8** into = Bu8idle(pad);
    $u8c hello = $u8str("Hello");
    aRDXid(id, 1, 2);
    call(RDXfeed, into, 'A', id, hello);
    same(10, $len(Bu8data(pad)));

    u8 const** from = Bu8cdata(pad);
    u8 t;
    id128 reid = {};
    $u8c rehello = {};
    call(RDXdrain, &t, &reid, rehello, from);
    same('A', t);
    same(0, $cmp(hello, rehello));
    same(0, id128cmp(&id, &reid));
    done;
}

pro(RDXtest) {
    sane(1);
    call(RDXtest1);
    done;
}

TEST(RDXtest);
