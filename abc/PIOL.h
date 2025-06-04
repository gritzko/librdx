#ifndef ABC_PIOL_H
#define ABC_PIOL_H

#include <limits.h>
#include <poll.h>

#include "INT.h"

#define PIOL_MAX_FILES 1024

static const ok64 PIOLfail = 0x658555aa5b70;
static const ok64 PIOLnone = 0x658555cb3ca9;
static const ok64 PIOLnoroom = 0x658555cb3db3cf1;
static const ok64 PIOLnodata = 0x658555cb3a25e25;
static const ok64 PIOLaccept = 0x6585559679e9d38;

struct PIOLstate;
struct PIOLctl;

typedef ok64 (*PIOLfunT)(struct PIOLstate* state);

typedef ok64 (*PIOLfunI)(struct PIOLctl* ctl);

struct PIOLctl {
    B$u8c writes;
    Bu8 readbuf;
    Bu8 writebuf;
    $u8c name;
    ok64 o;
    PIOLfunI fn;
    int fd;
};
typedef struct PIOLctl PIOLctl;

typedef PIOLctl PIOLstate[PIOL_MAX_FILES];

fun PIOLctl* PIOLfind(PIOLstate state, int fd) {
    for (int i = 0; i < PIOL_MAX_FILES && state[i].fn != nil; ++i)
        if (state[i].fd == fd) return state + i;
    return nil;
}

ok64 PIOLadd(PIOLstate state, int fd, $u8c name, PIOLfunI fi);

ok64 PIOLlisten(PIOLstate state, int fd, $u8c name, PIOLfunI fi);

ok64 PIOLdelctl(PIOLstate state, PIOLctl* ctl, ok64 o);
fun ok64 PIOLdel(PIOLstate state, int fd, ok64 o) {
    PIOLctl* ctl = PIOLfind(state, fd);
    if (ctl == nil) return PIOLnone;
    return PIOLdelctl(state, ctl, o);
}

fun ok64 PIOLfeed$(PIOLctl* ctl, u8c$ data) {
    $u8c$ idle = B$u8cidle(ctl->writes);
    if ($empty(idle)) return PIOLnoroom;
    $$u8cfeed1(idle, data);
    return OK;
}

fun ok64 PIOLfeed(PIOLctl* ctl, $u8c data) {
    $u8c n = {ctl->writebuf[2], nil};
    ok64 o = $u8feed(Bu8idle(ctl->writebuf), data);
    if (o != OK) return o;
    n[1] = ctl->writebuf[2];
    return PIOLfeed$(ctl, n);
}

fun ok64 PIOLdrain($u8 into, PIOLctl* ctl) {
    return $u8drain(into, Bu8cdata(ctl->readbuf));
}

ok64 PIOLonce(PIOLstate state, size_t ms);
ok64 PIOLloop(PIOLstate state, size_t ms);

#endif
