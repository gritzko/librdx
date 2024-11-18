#ifndef ABC_POLL_H
#define ABC_POLL_H

#include <poll.h>

#include "INT.h"

#define POLL_MAX_FILES 1024

con ok64 POLLfail = 0xc2d96a555619;
con ok64 POLLnone = 0xa72cf2555619;
con ok64 POLLnoroom = 0xc73cf6cf2555619;
con ok64 POLLnodata = 0x978968cf2555619;
con ok64 POLLaccept = 0xe34a679e5555619;

struct POLLstate;
struct POLLctl;

typedef ok64 (*POLLfunT)(struct POLLstate* state);

typedef ok64 (*POLLfunI)(struct POLLctl* ctl);

struct POLLctl {
    B$u8c writes;
    Bu8 readbuf;
    Bu8 writebuf;
    $u8c name;
    ok64 o;
    POLLfunI fn;
    int fd;
};
typedef struct POLLctl POLLctl;

typedef POLLctl POLLstate[POLL_MAX_FILES];

fun POLLctl* POLLfind(POLLstate state, int fd) {
    for (int i = 0; i < POLL_MAX_FILES && state[i].fn != nil; ++i)
        if (state[i].fd == fd) return state + i;
    return nil;
}

ok64 POLLadd(POLLstate state, int fd, $u8c name, POLLfunI fi);

ok64 POLLlisten(POLLstate state, int fd, $u8c name, POLLfunI fi);

ok64 POLLdelctl(POLLstate state, POLLctl* ctl, ok64 o);
fun ok64 POLLdel(POLLstate state, int fd, ok64 o) {
    POLLctl* ctl = POLLfind(state, fd);
    if (ctl == nil) return POLLnone;
    return POLLdelctl(state, ctl, o);
}

fun ok64 POLLfeed$(POLLctl* ctl, u8c$ data) {
    $u8c$ idle = B$u8cidle(ctl->writes);
    if ($empty(idle)) return POLLnoroom;
    $$u8cfeed1(idle, data);
    return OK;
}

fun ok64 POLLfeed(POLLctl* ctl, $u8c data) {
    $u8c n = {ctl->writebuf[2], nil};
    ok64 o = $u8feed(Bu8idle(ctl->writebuf), data);
    if (o != OK) return o;
    n[1] = ctl->writebuf[2];
    return POLLfeed$(ctl, n);
}

fun ok64 POLLdrain($u8 into, POLLctl* ctl) {
    return $u8drain(into, Bu8cdata(ctl->readbuf));
}

ok64 POLLonce(POLLstate state, size_t ms);
ok64 POLLloop(POLLstate state, size_t ms);

#endif
