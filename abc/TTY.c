#include "TTY.h"

#include "PRO.h"

// Convert RGB to old-style ANSI color code (30-37 or 90-97)
u8 TTYrgb2ansi(u8 r, u8 g, u8 b) {
    // ANSI 3-bit color: red=1, green=2, blue=4
    u8 color = 0;
    if (r >= 128) color |= 1;
    if (g >= 128) color |= 2;
    if (b >= 128) color |= 4;
    // Bright if any channel > 192
    u8 bright = (r > 192 || g > 192 || b > 192) ? 60 : 0;
    return 30 + color + bright;
}

// Feed old-style ANSI foreground escape sequence
ok64 TTYansifeed(utf8s into, u8 code) {
    sane($ok(into));
    test($size(into) >= 5, NOROOM);
    utf8sFeed1(into, 033);
    utf8sFeed1(into, '[');
    utf8sFeed10(into, code);
    utf8sFeed1(into, 'm');
    done;
}

// Feed RGB foreground escape sequence
ok64 TTYrgbfeed(utf8s into, u8 r, u8 g, u8 b) {
    sane($ok(into));
    test($size(into) >= 19, NOROOM);
    utf8sFeed1(into, 033);
    utf8sFeed1(into, '[');
    utf8sFeed1(into, '3');
    utf8sFeed1(into, '8');
    utf8sFeed1(into, ';');
    utf8sFeed1(into, '2');
    utf8sFeed1(into, ';');
    utf8sFeed10(into, r);
    utf8sFeed1(into, ';');
    utf8sFeed10(into, g);
    utf8sFeed1(into, ';');
    utf8sFeed10(into, b);
    utf8sFeed1(into, 'm');
    done;
}

// Feed reset escape sequence
ok64 TTYresetfeed(utf8s into) {
    sane($ok(into));
    test($size(into) >= 4, NOROOM);
    utf8sFeed1(into, 033);
    utf8sFeed1(into, '[');
    utf8sFeed1(into, '0');
    utf8sFeed1(into, 'm');
    done;
}

// Feed color based on terminal type
fun ok64 TTYcolorfeed(utf8s into, tty64 style) {
    sane($ok(into));
    b8 hascolor = style.r || style.g || style.b;
    if (!hascolor || style.term == TTY_TERM_NONE) done;

    if (style.term == TTY_TERM_OLD) {
        u8 code = TTYrgb2ansi(style.r, style.g, style.b);
        call(TTYansifeed, into, code);
    } else {
        call(TTYrgbfeed, into, style.r, style.g, style.b);
    }
    done;
}

// Skip n UTF-8 codepoints, return pointer to rest
fun utf8c *TTYutf8Skip(utf8cs txt, size_t n) {
    utf8c *p = txt[0];
    while (p < txt[1] && n > 0) {
        if ((*p & 0xC0) != 0x80) n--;
        p++;
    }
    // align to codepoint start
    while (p < txt[1] && (*p & 0xC0) == 0x80) p++;
    return p;
}

ok64 TTYutf8sFeed(utf8s into, utf8cs txt, tty64 style) {
    sane($ok(into) && $ok(txt));

    size_t txtlen = utf8CPLen(txt);
    size_t width = style.width;
    b8 hascolor = style.r || style.g || style.b;

    // Feed color escape based on terminal type
    if (hascolor && style.term != TTY_TERM_NONE) {
        call(TTYcolorfeed, into, style);
    }

    // Calculate effective text and padding
    size_t outlen = txtlen;
    size_t padlen = 0;
    utf8cs outtxt = {txt[0], txt[1]};

    if (width > 0) {
        if (txtlen > width) {
            // Need trimming
            outlen = width;
            if (style.trim == TTY_TRIM_ELLIPSIS && width >= 3) {
                outlen = width - 3;
            } else if (style.trim == TTY_TRIM_NONE) {
                outlen = txtlen;  // no trim
            }
            outtxt[1] = TTYutf8Skip(txt, outlen);
        } else if (txtlen < width) {
            padlen = width - txtlen;
        }
    }

    u8 padch = style.padch ? style.padch : ' ';

    // Left padding
    if (style.pad == TTY_PAD_LEFT && padlen > 0) {
        for (size_t i = 0; i < padlen; i++) {
            call(utf8sFeed1, into, padch);
        }
    }

    // Feed the text
    call(utf8sFeed, into, outtxt);

    // Ellipsis if trimmed
    if (style.trim == TTY_TRIM_ELLIPSIS && txtlen > width && width >= 3) {
        call(utf8sFeed1, into, '.');
        call(utf8sFeed1, into, '.');
        call(utf8sFeed1, into, '.');
    }

    // Right padding
    if (style.pad == TTY_PAD_RIGHT && padlen > 0) {
        for (size_t i = 0; i < padlen; i++) {
            call(utf8sFeed1, into, padch);
        }
    }

    // Reset if we had color
    if (hascolor && style.term != TTY_TERM_NONE) {
        call(TTYresetfeed, into);
    }

    done;
}
