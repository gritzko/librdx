
#include "POLL.h"

#include <poll.h>
#include <sys/socket.h>

#include "B.h"
#include "FILE.h"

int POLLlen(POLLstate state) {
    if (state[0].fn == nil) return 0;
    int b = 0, e = POLL_MAX_FILES;
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

pro(POLLadd, POLLstate state, int fd, $u8c name, POLLfunI fi) {
    sane(state != nil && fd >= 0);
    int l = POLLlen(state);
    test(l < POLL_MAX_FILES, POLLnoroom);
    POLLctl* ctl = state + l;
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

pro(POLLlisten, POLLstate state, int fd, $u8c name, POLLfunI fi) {
    sane(state != nil && fd >= 0);
    int l = POLLlen(state);
    test(l < POLL_MAX_FILES, POLLnoroom);
    POLLctl* ctl = state + l;
    ctl->o = POLLaccept;
    ctl->fd = fd;
    $u8dup((u8**)ctl->name, name);
    ctl->fn = fi;
    Bu8free(ctl->readbuf);
    Bu8free(ctl->writebuf);
    B$u8cfree(ctl->writes);
    done;
}

pro(POLLdelctl, POLLstate state, POLLctl* ctl, ok64 o) {
    sane(state != nil && ctl != nil && ctl->fd >= 0);
    Bu8free(ctl->readbuf);
    Bu8free(ctl->writebuf);
    B$u8cfree(ctl->writes);
    $u8free(ctl->name);
    int len = POLLlen(state);
    POLLctl* last = state + len - 1;
    memcpy(ctl, last, sizeof(POLLctl));
    zerop(last);
    done;
}

pro(POLLread, POLLctl* ctl) {
    sane(ctl != nil && ctl->fn != nil);
    call(FILEdrain, Bu8idle(ctl->readbuf), ctl->fd);
    call((*ctl->fn), (struct POLLctl*)ctl);
    if (!Bu8hasdata(ctl->readbuf)) Breset(ctl->readbuf);
    done;
}

pro(POLLwrite, POLLctl* ctl) {
    sane(ctl != nil && ctl->fn != nil);
    call(FILEfeedv, ctl->fd, B$u8cdata(ctl->writes));
    if (Bdatalen(ctl->writes) == 0) {
        Breset(ctl->writebuf);
        Breset(ctl->writes);
    }
    done;
}

pro(POLLaccpt, POLLstate state, POLLctl* ctl) {
    sane(ctl != nil && ctl->fn != nil);
    u8 addr[64];
    socklen_t len = 64;
    int cfd = accept(ctl->fd, (struct sockaddr*)addr, &len);
    testc(cfd != -1, POLLfail);
    $u8c name = {addr, addr + len};
    otry(POLLadd, state, cfd, name, ctl->fn);
    oops { close(cfd); }
    done;
}

static const int POLLOOPS = ~(POLLIN | POLLOUT);

pro(POLLonce, POLLstate state, size_t ms) {
    sane(1);
    struct pollfd fds[POLL_MAX_FILES];
    int l = 0;
    while (l < POLL_MAX_FILES && state[l].fn != nil) {
        int e = 0;
        if (B$u8chasdata(state[l].writes)) e |= POLLOUT;
        if (Bu8hasroom(state[l].readbuf)) e |= POLLIN;
        if (state[l].o == POLLaccept) e |= POLLIN;
        fds[l].fd = state[l].fd;
        fds[l].events = e;
        fds[l].revents = 0;
        ++l;
    }
    int c = poll(fds, l, ms);
    testc(c != -1, POLLfail);
    for (int i = 0; c > 0 && i < l; ++i) {
        if (fds[i].revents == 0) continue;
        --c;
        ok64 o = OK;
        if (fds[i].revents & POLLIN) {
            if (state[i].o == POLLaccept) {
                o = POLLaccpt(state, state + i);
            } else {
                o = POLLread(state + i);
            }
        }
        if (o == OK && (fds[i].revents & POLLOUT)) {
            o = POLLwrite(state + i);
        }
        if (o != OK || (fds[i].revents & POLLOOPS)) {
            call(POLLdelctl, state, state + i, o);
            --i;
        }
    }
    done;
}
