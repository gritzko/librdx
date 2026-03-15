#ifndef LIBRDX_HILI_H
#define LIBRDX_HILI_H

#include "abc/ANSI.h"

#define HILI_MAXDEPTH 64

// Diff background colors (256-palette)
#define HILI_DEL_BG 217   // light pink/salmon
#define HILI_ADD_BG 194   // light mint green

// Leaf type → ANSI foreground style.  Returns YES if style was emitted.
fun b8 HILILeaf($u8 out, u8 type) {
    switch (type) {
        case 'F': escfeed(out, BOLD); escfeed(out, DARK_YELLOW); return YES;
        case 'W': escfeed(out, HIGHLIGHT); return YES;
        case 'R': escfeed(out, DARK_RED); return YES;
        case 'D': escfeed(out, GRAY); return YES;
        case 'G': escfeed(out, DARK_GREEN); return YES;
        case 'L': escfeed(out, DARK_CYAN); return YES;
        case 'T': escfeed(out, LIGHT_BLUE); return YES;
        case 'H': escfeed(out, DARK_PINK); return YES;
        case 'P': escfeed(out, GRAY); return YES;
        case 'V': escfeed(out, BOLD); return YES;
        case 'J': escfeed(out, STRIKETHROUGH); return YES;
        default:  return NO;
    }
}

// Container type → ANSI style (layered onto children).
fun b8 HILIContainer($u8 out, u8 type) {
    if (type == 'Y') { escfeed(out, BOLD); return YES; }
    return NO;
}

// Reset, then re-emit active container styles from stack.
fun void HILIRestore($u8 out, u8 *cstk, int depth) {
    escfeed(out, 0);
    for (int i = 0; i < depth; i++)
        HILIContainer(out, cstk[i]);
}

#endif
