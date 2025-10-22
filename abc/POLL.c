
#include "POLL.h"

#include <poll.h>
#include <sys/socket.h>

#include "B.h"
#include "FILE.h"
#include "PRO.h"

int POLLlen(POLLstate state) {
    if (state[0].fn == NULL) return 0;
    int b = 0, e = POLL_MAX_FILES;
    while (b + 1 < e) {
        int m = (b + e) >> 1;
        if (state[m].fn == NULL) {
            e = m;
        } else {
            b = m;
        }
    }
    return e;
}

pro(POLLadd, POLLstate state, int fd, u8cs name, POLLfunI fi) {
    sane(state != NULL && fd >= 0);
    int l = POLLlen(state);
    test(l < POLL_MAX_FILES, POLLnoroom);
    POLLctl* ctl = state + l;
    ctl->o = OK;
    ctl->fd = fd;
    ctl->fn = fi;
    try($u8dup, (u8**)ctl->name, name);
    then try(u8bAllocate, ctl->readbuf, PAGESIZE);
    then try(u8bAllocate, ctl->writebuf, PAGESIZE);
    then try(u8csbAllocate, ctl->writes, 64);
    oops {
        u8bFree(ctl->readbuf);
        u8bFree(ctl->writebuf);
        $u8free(ctl->name);
        u8csbFree(ctl->writes);
    }
    done;
}

pro(POLLlisten, POLLstate state, int fd, u8cs name, POLLfunI fi) {
    sane(state != NULL && fd >= 0);
    int l = POLLlen(state);
    test(l < POLL_MAX_FILES, POLLnoroom);
    POLLctl* ctl = state + l;
    ctl->o = POLLaccept;
    ctl->fd = fd;
    $u8dup((u8**)ctl->name, name);
    ctl->fn = fi;
    u8bFree(ctl->readbuf);
    u8bFree(ctl->writebuf);
    u8csbFree(ctl->writes);
    done;
}

pro(POLLdelctl, POLLstate state, POLLctl* ctl, ok64 o) {
    sane(state != NULL && ctl != NULL && ctl->fd >= 0);
    u8bFree(ctl->readbuf);
    u8bFree(ctl->writebuf);
    u8csbFree(ctl->writes);
    $u8free(ctl->name);
    int len = POLLlen(state);
    POLLctl* last = state + len - 1;
    memcpy(ctl, last, sizeof(POLLctl));
    zerop(last);
    done;
}

pro(POLLread, POLLctl* ctl) {
    sane(ctl != NULL && ctl->fn != NULL);
    call(FILEdrain, u8bIdle(ctl->readbuf), ctl->fd);
    call((*ctl->fn), (struct POLLctl*)ctl);
    if (!u8bHasData(ctl->readbuf)) Breset(ctl->readbuf);
    done;
}

pro(POLLwrite, POLLctl* ctl) {
    sane(ctl != NULL && ctl->fn != NULL);
    call(FILEFeedv, ctl->fd, u8csbData(ctl->writes));
    if (Bdatalen(ctl->writes) == 0) {
        Breset(ctl->writebuf);
        Breset(ctl->writes);
    }
    done;
}

pro(POLLaccpt, POLLstate state, POLLctl* ctl) {
    sane(ctl != NULL && ctl->fn != NULL);
    u8 addr[64];
    socklen_t len = 64;
    int cfd = accept(ctl->fd, (struct sockaddr*)addr, &len);
    testc(cfd != -1, POLLfail);
    u8cs name = {addr, addr + len};
    otry(POLLadd, state, cfd, name, ctl->fn);
    oops { close(cfd); }
    done;
}

con int POLLOOPS = ~(POLLIN | POLLOUT);

pro(POLLonce, POLLstate state, size_t ms) {
    sane(1);
    struct pollfd fds[POLL_MAX_FILES];
    int l = 0;
    while (l < POLL_MAX_FILES && state[l].fn != NULL) {
        int e = 0;
        if (u8csbHasData(state[l].writes)) e |= POLLOUT;
        if (u8bHasRoom(state[l].readbuf)) e |= POLLIN;
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
