#include "TCP.h"

#include <stdlib.h>
#include <time.h>

#include "FILE.h"
#include "INT.h"
#include "NET.h"
#include "PRO.h"
#include "TEST.h"

void garble($u8 data) {
    srandom(time(NULL));
    for (int i = 0; i < $len(data); ++i) {
        int b = random() % $len(data);
        u8Swap($atp(data, i), $atp(data, b));
    }
}

pro(TCPtest1) {
    sane(1);

    a_cstr(addr, "tcp://127.0.0.1:12345");

    int fd;
    call(TCPListen, &fd, addr);

    int cfd;
    call(TCPConnect, &cfd, addr, 0);

    int sfd;
    aNETraw(caddr);
    call(TCPAccept, &sfd, caddr, fd);

    a$str(bubu, "BuBu");
    call(FILEFeedall, cfd, bubu);

    aBpad2(u8, read, 128);
    aNETraw(sndaddr);
    call(FILEdrain, readidle, sfd);
    $testeq(bubu, readdata);

    call(TCPClose, fd);
    call(TCPClose, cfd);
    call(TCPClose, sfd);
    done;
}

pro(TCPtest) {
    sane(1);
    call(TCPtest1);
    done;
}

MAIN(TCPtest);
