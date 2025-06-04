
#include "PIOL.h"

#include <poll.h>
#include <sys/socket.h>

#include "B.h"
#include "FILE.h"

int PIOLlen(PIOLstate state) {
    if (state[0].fn == nil) return 0;
    int b = 0, e = PIOL_MAX_FILES;
    while (b + 1 < e) {
        int m = (b + e) >> 1;
        if (state[m].fn == nil) {
            e = m;
        } else {
            b = m;
        }
    }
    return e;
}

ok64 PIOLadd(PIOLstate state, int fd, $u8c name, PIOLfunI fi) {
    sane(state != nil && fd >= 0);
    int l = PIOLlen(state);
    test(l < PIOL_MAX_FILES, PIOLnoroom);
    PIOLctl* ctl = state + l;
    ctl->o = OK;
    ctl->fd = fd;
    ctl->fn = fi;
    try($u8dup, (u8**)ctl->name, name);
    then try(Bu8alloc, ctl->readbuf, PAGESIZE);
    then try(Bu8alloc, ctl->writebuf, PAGESIZE);
    then try(B$u8calloc, ctl->writes, 64);
    oops {
        Bu8free(ctl->readbuf);
        Bu8free(ctl->writebuf);
        $u8free(ctl->name);
        B$u8cfree(ctl->writes);
    }
    done;
}

ok64 PIOLlisten(PIOLstate state, int fd, $u8c name, PIOLfunI fi) {
    sane(state != nil && fd >= 0);
    int l = PIOLlen(state);
    test(l < PIOL_MAX_FILES, PIOLnoroom);
    PIOLctl* ctl = state + l;
    ctl->o = PIOLaccept;
    ctl->fd = fd;
    $u8dup((u8**)ctl->name, name);
    ctl->fn = fi;
    Bu8free(ctl->readbuf);
    Bu8free(ctl->writebuf);
    B$u8cfree(ctl->writes);
    done;
}

ok64 PIOLdelctl(PIOLstate state, PIOLctl* ctl, ok64 o) {
    sane(state != nil && ctl != nil && ctl->fd >= 0);
    Bu8free(ctl->readbuf);
    Bu8free(ctl->writebuf);
    B$u8cfree(ctl->writes);
    $u8free(ctl->name);
    int len = PIOLlen(state);
    PIOLctl* last = state + len - 1;
    memcpy(ctl, last, sizeof(PIOLctl));
    zerop(last);
    done;
}

ok64 PIOLread(PIOLctl* ctl) {
    sane(ctl != nil && ctl->fn != nil);
    call(FILEdrain, Bu8idle(ctl->readbuf), ctl->fd);
    call((*ctl->fn), (struct PIOLctl*)ctl);
    if (!Bu8hasdata(ctl->readbuf)) Breset(ctl->readbuf);
    done;
}

ok64 PIOLwrite(PIOLctl* ctl) {
    sane(ctl != nil && ctl->fn != nil);
    call(FILEfeedv, ctl->fd, B$u8cdata(ctl->writes));
    if (Bdatalen(ctl->writes) == 0) {
        Breset(ctl->writebuf);
        Breset(ctl->writes);
    }
    done;
}

ok64 PIOLaccpt(PIOLstate state, PIOLctl* ctl) {
    sane(ctl != nil && ctl->fn != nil);
    u8 addr[64];
    socklen_t len = 64;
    int cfd = accept(ctl->fd, (struct sockaddr*)addr, &len);
    testc(cfd != -1, PIOLfail);
    $u8c name = {addr, addr + len};
    otry(PIOLadd, state, cfd, name, ctl->fn);
    oops { close(cfd); }
    done;
}

static const int PIOLOOPS = ~(POLLIN | POLLOUT);

ok64 PIOLonce(PIOLstate state, size_t ms) {
    sane(1);
    struct pollfd fds[PIOL_MAX_FILES];
    int l = 0;
    while (l < PIOL_MAX_FILES && state[l].fn != nil) {
        int e = 0;
        if (B$u8chasdata(state[l].writes)) e |= POLLOUT;
        if (Bu8hasroom(state[l].readbuf)) e |= POLLIN;
        if (state[l].o == PIOLaccept) e |= POLLIN;
        fds[l].fd = state[l].fd;
        fds[l].events = e;
        fds[l].revents = 0;
        ++l;
    }
    int c = poll(fds, l, ms);
    testc(c != -1, PIOLfail);
    for (int i = 0; c > 0 && i < l; ++i) {
        if (fds[i].revents == 0) continue;
        --c;
        ok64 o = OK;
        if (fds[i].revents & POLLIN) {
            if (state[i].o == PIOLaccept) {
                o = PIOLaccpt(state, state + i);
            } else {
                o = PIOLread(state + i);
            }
        }
        if (o == OK && (fds[i].revents & POLLOUT)) {
            o = PIOLwrite(state + i);
        }
        if (o != OK || (fds[i].revents & PIOLOOPS)) {
            call(PIOLdelctl, state, state + i, o);
            --i;
        }
    }
    done;
}
