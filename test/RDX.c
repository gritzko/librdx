#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "B.h"
#include "RDX.h"
#include "TEST.h"

pro(RDXtest1) {
    aBpad(u8, pad, 0x1000);
    u8** into = Bu8idle(pad);
    $u8c hello = $u8str("Hello");
    id128 id = {.seq = 1, .src = 2};
    call(RDXfeedS, into, id, hello);
    $print(Bu8cdata(pad));
    sane(10 == $len(Bu8data(pad)));

    u8 const** from = Bu8cdata(pad);
    id128 reid = {};
    $u8c rehello = {};
    call(RDXdrainS, &reid, rehello, from);

    sane(0 == $cmp(hello, rehello));
    sane(0 == id128cmp(&id, &reid));
    done;
}

pro(RDXtest) {
    call(RDXtest1);
    done;
}

TEST(RDXtest);
