#include "PIOL.h"

#include "FILE.h"
#include "INT.h"
#include "NET.h"
#include "PRO.h"
#include "TCP.h"
#include "TEST.h"

ok64 funIecho(PIOLctl* ctl) { return PIOLfeed(ctl, Bu8cdata(ctl->readbuf)); }

u8 count = 0;

ok64 funIcount(PIOLctl* ctl) {
    u8 n = 0;
    ok64 o = $u8drain1(&n, Bu8data(ctl->readbuf));
    if (o == OK && n != count) return faileq;
    ++count;
    a$rawc($n, count);
    return PIOLfeed(ctl, $n);
}

ok64 PIOLtest1() {
    sane(1);
    char port[16];
    sprintf(port, "%d", NETrandomport());
    aNETaddr(addr, "127.0.0.1", port);
    int sfd;
    call(TCPbind, &sfd, addr);
    int cfd;
    call(TCPconnect, &cfd, addr);
    int scfd;
    aNETraw(myself);
    call(TCPaccept, &scfd, myself, sfd);

    PIOLstate state = {};
    a$str(cname, "client");
    a$str(sname, "server");

    call(PIOLadd, state, cfd, cname, funIcount);
    call(PIOLadd, state, scfd, sname, funIcount);

    u8 v0 = 0;
    a$rawc(s0, v0);
    call(FILEfeed, cfd, s0);

    while (count < 100) {
        call(PIOLonce, state, 10);
    }

    call(PIOLdel, state, cfd, OK);
    call(PIOLdel, state, scfd, OK);

    call(TCPclose, cfd);
    call(TCPclose, scfd);
    call(TCPclose, sfd);
    done;
}

ok64 PIOLtest() {
    sane(1);
    call(PIOLtest1);
    done;
}

TEST(PIOLtest);
