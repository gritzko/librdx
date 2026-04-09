#ifndef BRO_MAUS_H
#define BRO_MAUS_H

#include "abc/INT.h"

// Mouse event types
#define MAUS_PRESS   0
#define MAUS_RELEASE 1
#define MAUS_DRAG    2
#define MAUS_WHEEL   3

// Mouse buttons
#define MAUS_LEFT    0
#define MAUS_MID     1
#define MAUS_RIGHT   2
#define MAUS_UP      64   // wheel up
#define MAUS_DOWN    65   // wheel down

typedef struct {
    u16 row;     // 1-based terminal row
    u16 col;     // 1-based terminal column
    u8  type;    // MAUS_PRESS / MAUS_RELEASE / MAUS_DRAG / MAUS_WHEEL
    u8  button;  // MAUS_LEFT..MAUS_RIGHT, or MAUS_UP/MAUS_DOWN for wheel
} MAUSevent;

// Enable SGR 1006 mouse tracking.  Writes escape sequences to fd.
void MAUSEnable(int fd);

// Disable mouse tracking.  Writes escape sequences to fd.
void MAUSDisable(int fd);

// Try to parse an SGR mouse sequence from buf[0..len).
// SGR format: \033[<Btn;Col;Row[Mm]
// On success: fills *ev and returns the number of bytes consumed.
// On failure (incomplete or not a mouse seq): returns 0.
int MAUSParse(MAUSevent *ev, u8 const *buf, int len);

#endif
