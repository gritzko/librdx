#ifndef ABC_ANSI_H
#define ABC_ANSI_H

#include "BUF.h"

typedef enum {
    BOLD = 1,
    WEAK = 2,
    HIGHLIGHT = 3,
    UNDERLINE = 4,
    BLACK = 30,
    DARK_RED = 31,
    DARK_GREEN = 32,
    DARK_YELLOW = 33,
    DARK_BLUE = 34,
    DARK_PINK = 35,
    DARK_CYAN = 36,
    BLACK_BG = 40,
    DARK_RED_BG = 41,
    DARK_GREEN_BG = 42,
    DARK_YELLOW_BG = 43,
    DARK_BLUE_BG = 44,
    DARK_PINK_BG = 45,
    DARK_CYAN_BG = 46,
    GRAY = 90,
    LIGHT_RED = 91,
    LIGHT_GREEN = 92,
    LIGHT_YELLOW = 93,
    LIGHT_BLUE = 94,
    LIGHT_PINK = 95,
    LIGHT_CYAN = 96,
    LIGHT_GRAY = 97,
    GRAY_BG = 100,
    LIGHT_RED_BG = 101,
    LIGHT_GREEN_BG = 102,
    LIGHT_YELLOW_BG = 103,
    LIGHT_BLUE_BG = 104,
    LIGHT_PINK_BG = 105,
    LIGHT_CYAN_BG = 106,
    LIGHT_GRAY_BG = 107,
} ANSI_COLOR;

fun ok64 escfeed($u8 data, u8 esc) {
    if (!$ok(data) || $size(data) < 7) return BADarg;
    u8sFeed1(data, 033);  //"\033[91m"
    u8sFeed1(data, '[');
    u64decfeed(data, esc);
    u8sFeed1(data, 'm');
    return OK;
}

#endif
