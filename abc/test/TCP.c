#include "TCP.h"

#include <stdlib.h>
#include <time.h>

#include "FILE.h"
#include "INT.h"
#include "NET.h"
#include "PRO.h"
#include "TEST.h"

void garble($u8 data) {
    srandom(time(nil));
    for (int i = 0; i < $len(data); ++i) {
        int b = random() % $len(data);
        u8swap($atp(data, i), $atp(data, b));
    }
}

pro(TCPtest1) {
    sane(1);

    char port[16];
    sprintf(port, "%d", NETrandomport());
    aNETaddr(addr, "127.0.0.1", port);
    $println(Bu8cdata(addr));

    int fd;
    call(TCPbind, &fd, addr);

    int cfd;
    call(TCPconnect, &cfd, addr);

    int sfd;
    aNETraw(caddr);
    call(TCPaccept, &sfd, caddr, fd);

    a$str(bubu, "BuBu");
    call(FILEfeedall, cfd, bubu);

    aBpad2(u8, read, 128);
    aNETraw(sndaddr);
    call(FILEdrain, readidle, sfd);
    $testeq(bubu, readdata);

    call(TCPclose, fd);
    call(TCPclose, cfd);
    call(TCPclose, sfd);
    done;
}

pro(TCPtest) {
    sane(1);
    call(TCPtest1);
    done;
}

MAIN(TCPtest);
