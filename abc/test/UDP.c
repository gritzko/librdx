#include "UDP.h"

#include "FILE.h"
#include "NET.h"
#include "PRO.h"

pro(UDPtest1) {
    sane(1);

    a$str(addr, "udp://127.0.0.1:3456");

    int fd;
    call(UDPBind, &fd, addr);

    int cfd;
    call(UDPConnect, &cfd, addr);

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
