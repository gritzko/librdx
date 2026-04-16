// MAUS recognizes the SGR-mouse subset of CSI: private '<', final
// 'M' (press/drag) or 'm' (release), 3 params (btn, col, row).
// CSI parsing itself lives in abc/ANSI.
#include "MAUS.h"

#include <unistd.h>

#include "abc/ANSI.h"

#define MAUS_SEQ_ON  "\033[?1000h\033[?1002h\033[?1006h"
#define MAUS_SEQ_OFF "\033[?1006l\033[?1002l\033[?1000l"

void MAUSEnable(int fd) {
    (void)write(fd, MAUS_SEQ_ON, sizeof(MAUS_SEQ_ON) - 1);
}

void MAUSDisable(int fd) {
    (void)write(fd, MAUS_SEQ_OFF, sizeof(MAUS_SEQ_OFF) - 1);
}

int MAUSParse(MAUSevent *ev, u8 const *buf, int len) {
    if (ev == NULL || buf == NULL || len <= 0) return 0;
    u8c *base = buf;
    u8cs input = {base, base + len};
    csi c = {};
    if (ANSIu8sDrainCSI(input, &c) != OK) return 0;
    if (c.private != '<') return 0;
    if (c.final != 'M' && c.final != 'm') return 0;
    if (c.nparams < 3) return 0;

    u32 btn = c.params[0], col = c.params[1], row = c.params[2];
    ev->row = (u16)row;
    ev->col = (u16)col;
    if (btn >= 64) {
        ev->type   = MAUS_WHEEL;
        ev->button = (u8)btn;
    } else if (c.final == 'm') {
        ev->type   = MAUS_RELEASE;
        ev->button = (u8)(btn & 3);
    } else if (btn & 32) {
        ev->type   = MAUS_DRAG;
        ev->button = (u8)(btn & 3);
    } else {
        ev->type   = MAUS_PRESS;
        ev->button = (u8)(btn & 3);
    }
    return (int)(input[0] - base);
}
