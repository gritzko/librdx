#include "MAUS.h"

#include <string.h>
#include <unistd.h>

void MAUSEnable(int fd) {
    // 1000: basic press/release tracking
    // 1002: also report drag (button-motion) events
    // 1006: SGR extended coordinates (no 223-col limit)
    char const *seq =
        "\033[?1000h"
        "\033[?1002h"
        "\033[?1006h";
    (void)write(fd, seq, strlen(seq));
}

void MAUSDisable(int fd) {
    char const *seq =
        "\033[?1006l"
        "\033[?1002l"
        "\033[?1000l";
    (void)write(fd, seq, strlen(seq));
}

// Parse SGR mouse: \033[<Btn;Col;RowM  (press/drag)
//                  \033[<Btn;Col;Rowm  (release)
// Btn encodes button + modifiers:
//   0=left, 1=mid, 2=right, 32+=drag, 64=wheel-up, 65=wheel-down
// Col and Row are 1-based decimal.
// Returns bytes consumed on success, 0 on failure/incomplete.
int MAUSParse(MAUSevent *ev, u8 const *buf, int len) {
    // Minimum: \033[<0;1;1M = 9 bytes
    if (len < 9) return 0;
    if (buf[0] != 033 || buf[1] != '[' || buf[2] != '<') return 0;

    int pos = 3;
    // Parse button number
    u32 btn = 0;
    while (pos < len && buf[pos] >= '0' && buf[pos] <= '9') {
        btn = btn * 10 + (buf[pos] - '0');
        pos++;
    }
    if (pos >= len || buf[pos] != ';') return 0;
    pos++;

    // Parse column
    u32 col = 0;
    while (pos < len && buf[pos] >= '0' && buf[pos] <= '9') {
        col = col * 10 + (buf[pos] - '0');
        pos++;
    }
    if (pos >= len || buf[pos] != ';') return 0;
    pos++;

    // Parse row
    u32 row = 0;
    while (pos < len && buf[pos] >= '0' && buf[pos] <= '9') {
        row = row * 10 + (buf[pos] - '0');
        pos++;
    }
    if (pos >= len) return 0;

    b8 release = (buf[pos] == 'm');
    if (buf[pos] != 'M' && buf[pos] != 'm') return 0;
    pos++;

    ev->row = (u16)row;
    ev->col = (u16)col;

    if (btn >= 64) {
        ev->type   = MAUS_WHEEL;
        ev->button = (u8)btn;  // 64=up, 65=down
    } else if (release) {
        ev->type   = MAUS_RELEASE;
        ev->button = (u8)(btn & 3);
    } else if (btn & 32) {
        ev->type   = MAUS_DRAG;
        ev->button = (u8)(btn & 3);
    } else {
        ev->type   = MAUS_PRESS;
        ev->button = (u8)(btn & 3);
    }
    return pos;
}
