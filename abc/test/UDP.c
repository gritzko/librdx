#include "UDP.h"

#include <stdlib.h>

#include "FILE.h"
#include "INT.h"
#include "NET.h"
#include "PRO.h"
#include "TEST.h"

pro(UDPtest1) {
    sane(1);

    aNETAddress(addr, "127.0.0.1", "1234");

    int fd;
    call(UDPbind, &fd, addr);

    int cfd;
    call(UDPconnect, &cfd, addr);

    a$str(bubu, "BuBu");
    call(FILEFeedall, cfd, bubu);

    aBpad2(u8, read, 128);
    aNETraw(sndaddr);
    call(UDPdrain, readidle, sndaddr, fd);
    $testeq(bubu, readdata);

    call(UDPclose, fd);
    done;
}

pro(UDPtest) {
    sane(1);
    call(UDPtest1);
    done;
}

MAIN(UDPtest);
