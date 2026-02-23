#ifndef ABC_TTY_H
#define ABC_TTY_H

#include "UTF8.h"

// Terminal type (term field)
#define TTY_TERM_NONE 0
#define TTY_TERM_OLD 1
#define TTY_TERM_RGB 2

// Padding style
#define TTY_PAD_NONE 0
#define TTY_PAD_LEFT 1
#define TTY_PAD_RIGHT 2

// Trimming style
#define TTY_TRIM_NONE 0
#define TTY_TRIM_CUT 1
#define TTY_TRIM_ELLIPSIS 2

// Styled output descriptor (8 bytes)
typedef union {
    u64 u;
    struct {
        u8 r;        // foreground red
        u8 g;        // foreground green
        u8 b;        // foreground blue
        u8 pad;      // TTY_PAD_*
        u8 padch;    // padding character
        u8 width;    // padded length
        u8 trim;     // TTY_TRIM_*
        u8 term;     // TTY_NONE/TTY_OLD/TTY_RGB
    };
} tty64;

// Reset
#define TTY_RESET "\033[0m"

// Text attributes
#define TTY_BOLD "\033[1m"
#define TTY_DIM "\033[2m"
#define TTY_ITALIC "\033[3m"
#define TTY_UNDERLINE "\033[4m"
#define TTY_BLINK "\033[5m"
#define TTY_INVERSE "\033[7m"
#define TTY_HIDDEN "\033[8m"
#define TTY_STRIKE "\033[9m"

// Attribute off
#define TTY_BOLD_OFF "\033[22m"
#define TTY_DIM_OFF "\033[22m"
#define TTY_ITALIC_OFF "\033[23m"
#define TTY_UNDERLINE_OFF "\033[24m"
#define TTY_BLINK_OFF "\033[25m"
#define TTY_INVERSE_OFF "\033[27m"
#define TTY_HIDDEN_OFF "\033[28m"
#define TTY_STRIKE_OFF "\033[29m"

// Foreground colors (dark)
#define TTY_BLACK "\033[30m"
#define TTY_RED "\033[31m"
#define TTY_GREEN "\033[32m"
#define TTY_YELLOW "\033[33m"
#define TTY_BLUE "\033[34m"
#define TTY_MAGENTA "\033[35m"
#define TTY_CYAN "\033[36m"
#define TTY_WHITE "\033[37m"
#define TTY_DEFAULT "\033[39m"

// Foreground colors (bright)
#define TTY_BRIGHT_BLACK "\033[90m"
#define TTY_BRIGHT_RED "\033[91m"
#define TTY_BRIGHT_GREEN "\033[92m"
#define TTY_BRIGHT_YELLOW "\033[93m"
#define TTY_BRIGHT_BLUE "\033[94m"
#define TTY_BRIGHT_MAGENTA "\033[95m"
#define TTY_BRIGHT_CYAN "\033[96m"
#define TTY_BRIGHT_WHITE "\033[97m"

// Background colors (dark)
#define TTY_BG_BLACK "\033[40m"
#define TTY_BG_RED "\033[41m"
#define TTY_BG_GREEN "\033[42m"
#define TTY_BG_YELLOW "\033[43m"
#define TTY_BG_BLUE "\033[44m"
#define TTY_BG_MAGENTA "\033[45m"
#define TTY_BG_CYAN "\033[46m"
#define TTY_BG_WHITE "\033[47m"
#define TTY_BG_DEFAULT "\033[49m"

// Background colors (bright)
#define TTY_BG_BRIGHT_BLACK "\033[100m"
#define TTY_BG_BRIGHT_RED "\033[101m"
#define TTY_BG_BRIGHT_GREEN "\033[102m"
#define TTY_BG_BRIGHT_YELLOW "\033[103m"
#define TTY_BG_BRIGHT_BLUE "\033[104m"
#define TTY_BG_BRIGHT_MAGENTA "\033[105m"
#define TTY_BG_BRIGHT_CYAN "\033[106m"
#define TTY_BG_BRIGHT_WHITE "\033[107m"

// Cursor movement
#define TTY_CUR_UP "\033[A"
#define TTY_CUR_DOWN "\033[B"
#define TTY_CUR_RIGHT "\033[C"
#define TTY_CUR_LEFT "\033[D"
#define TTY_CUR_HOME "\033[H"
#define TTY_CUR_SAVE "\033[s"
#define TTY_CUR_RESTORE "\033[u"

// Erase
#define TTY_ERASE_LINE "\033[2K"
#define TTY_ERASE_LINE_END "\033[K"
#define TTY_ERASE_LINE_START "\033[1K"
#define TTY_ERASE_SCREEN "\033[2J"
#define TTY_ERASE_SCREEN_END "\033[J"
#define TTY_ERASE_SCREEN_START "\033[1J"

// 256-color mode (compile-time literals only)
#define TTY_FG256(n) "\033[38;5;" #n "m"
#define TTY_BG256(n) "\033[48;5;" #n "m"

// 24-bit RGB true color (compile-time literals only)
#define TTY_RGB(r, g, b) "\033[38;2;" #r ";" #g ";" #b "m"
#define TTY_BG_RGB(r, g, b) "\033[48;2;" #r ";" #g ";" #b "m"

// Underline styles (modern terminals)
#define TTY_UNDERLINE_SINGLE "\033[4:1m"
#define TTY_UNDERLINE_DOUBLE "\033[4:2m"
#define TTY_UNDERLINE_CURLY "\033[4:3m"
#define TTY_UNDERLINE_DOTTED "\033[4:4m"
#define TTY_UNDERLINE_DASHED "\033[4:5m"

// Underline color (modern terminals, compile-time literals only)
#define TTY_UNDERLINE_RGB(r, g, b) "\033[58;2;" #r ";" #g ";" #b "m"

// Box drawing - single line
#define TTY_BOX_H "─"
#define TTY_BOX_V "│"
#define TTY_BOX_TL "┌"
#define TTY_BOX_TR "┐"
#define TTY_BOX_BL "└"
#define TTY_BOX_BR "┘"
#define TTY_BOX_VR "├"
#define TTY_BOX_VL "┤"
#define TTY_BOX_HD "┬"
#define TTY_BOX_HU "┴"
#define TTY_BOX_X "┼"

// Box drawing - double line
#define TTY_BOX2_H "═"
#define TTY_BOX2_V "║"
#define TTY_BOX2_TL "╔"
#define TTY_BOX2_TR "╗"
#define TTY_BOX2_BL "╚"
#define TTY_BOX2_BR "╝"
#define TTY_BOX2_VR "╠"
#define TTY_BOX2_VL "╣"
#define TTY_BOX2_HD "╦"
#define TTY_BOX2_HU "╩"
#define TTY_BOX2_X "╬"

// Box drawing - rounded corners
#define TTY_BOX_RTL "╭"
#define TTY_BOX_RTR "╮"
#define TTY_BOX_RBL "╰"
#define TTY_BOX_RBR "╯"

// Arrows
#define TTY_ARROW_L "←"
#define TTY_ARROW_R "→"
#define TTY_ARROW_U "↑"
#define TTY_ARROW_D "↓"
#define TTY_ARROW2_L "⇐"
#define TTY_ARROW2_R "⇒"
#define TTY_ARROW2_U "⇑"
#define TTY_ARROW2_D "⇓"
#define TTY_TRI_L "◀"
#define TTY_TRI_R "▶"
#define TTY_TRI_U "▲"
#define TTY_TRI_D "▼"

// Convert RGB to old-style ANSI color code (30-37 or 90-97)
u8 TTYrgb2ansi(u8 r, u8 g, u8 b);

// Feed old-style ANSI foreground escape sequence
ok64 TTYansifeed(utf8s into, u8 code);

// Feed RGB foreground escape sequence
ok64 TTYrgbfeed(utf8s into, u8 r, u8 g, u8 b);

// Feed reset escape sequence
ok64 TTYresetfeed(utf8s into);

// Feed styled text: color, padding, trimming
ok64 TTYutf8sFeed(utf8s into, utf8cs txt, tty64 style);

#endif
