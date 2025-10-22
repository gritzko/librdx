#include "POLL.h"

#include "FILE.h"
#include "NET.h"
#include "PRO.h"
#include "TCP.h"
#include "TEST.h"

ok64 funIecho(POLLctl* ctl) { return POLLfeed(ctl, u8cbData(ctl->readbuf)); }

u8 count = 0;

ok64 funIcount(POLLctl* ctl) {
    u8 n = 0;
    ok64 o = u8sDrain1(u8cbData(ctl->readbuf), &n);
    if (o == OK && n != count) return faileq;
    ++count;
    a$rawc($n, count);
    return POLLfeed(ctl, $n);
}

pro(POLLtest1) {
    sane(1);
    a$strc(addr, "tcp://127.0.0.1:23456");
    int sfd;
    call(TCPListen, &sfd, addr);
    int cfd;
    call(TCPConnect, &cfd, addr, 0);
    int scfd;
    aNETraw(myself);
    call(TCPAccept, &scfd, myself, sfd);

    POLLstate state = {};
    a$str(cname, "client");
    a$str(sname, "server");

    call(POLLadd, state, cfd, cname, funIcount);
    call(POLLadd, state, scfd, sname, funIcount);

    u8 v0 = 0;
    a$rawc(s0, v0);
    call(FILEFeed, cfd, s0);

    while (count < 100) {
        call(POLLonce, state, 10);
    }

    call(POLLdel, state, cfd, OK);
    call(POLLdel, state, scfd, OK);

    call(TCPClose, cfd);
    call(TCPClose, scfd);
    call(TCPClose, sfd);
    done;
}

pro(POLLtest) {
    sane(1);
    call(POLLtest1);
    done;
}

TEST(POLLtest);
