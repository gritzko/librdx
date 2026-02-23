#include "TTY.h"

#include <stdio.h>

#include "B.h"
#include "TEST.h"

ok64 TTYtestColors() {
    sane(1);
    a_pad(u8, buf, 256);

    // Print a rainbow in RGB mode
    printf("Colors (RGB mode):\n");
    tty64 style = {.term = TTY_TERM_RGB};

    style.r = 255; style.g = 0; style.b = 0;
    a$str(red, "RED ");
    call(TTYutf8sFeed, buf_idle, red, style);

    style.r = 255; style.g = 128; style.b = 0;
    a$str(orange, "ORANGE ");
    call(TTYutf8sFeed, buf_idle, orange, style);

    style.r = 255; style.g = 255; style.b = 0;
    a$str(yellow, "YELLOW ");
    call(TTYutf8sFeed, buf_idle, yellow, style);

    style.r = 0; style.g = 255; style.b = 0;
    a$str(green, "GREEN ");
    call(TTYutf8sFeed, buf_idle, green, style);

    style.r = 0; style.g = 128; style.b = 255;
    a$str(blue, "BLUE ");
    call(TTYutf8sFeed, buf_idle, blue, style);

    style.r = 128; style.g = 0; style.b = 255;
    a$str(purple, "PURPLE");
    call(TTYutf8sFeed, buf_idle, purple, style);

    u8sFeed1(buf_idle, '\n');
    fwrite(buf[1], 1, *buf_idle - buf[1], stdout);

    // Old terminal mode
    u8bReset(buf);
    printf("Colors (OLD mode):\n");
    style.term = TTY_TERM_OLD;

    style.r = 255; style.g = 0; style.b = 0;
    call(TTYutf8sFeed, buf_idle, red, style);

    style.r = 255; style.g = 128; style.b = 0;
    call(TTYutf8sFeed, buf_idle, orange, style);

    style.r = 255; style.g = 255; style.b = 0;
    call(TTYutf8sFeed, buf_idle, yellow, style);

    style.r = 0; style.g = 255; style.b = 0;
    call(TTYutf8sFeed, buf_idle, green, style);

    style.r = 0; style.g = 128; style.b = 255;
    call(TTYutf8sFeed, buf_idle, blue, style);

    style.r = 128; style.g = 0; style.b = 255;
    call(TTYutf8sFeed, buf_idle, purple, style);

    u8sFeed1(buf_idle, '\n');
    fwrite(buf[1], 1, *buf_idle - buf[1], stdout);

    done;
}

ok64 TTYtestPadding() {
    sane(1);
    a_pad(u8, buf, 128);

    a$str(hello, "hello");
    tty64 style = {.term = TTY_TERM_NONE, .width = 10};

    // Left pad
    style.pad = TTY_PAD_LEFT;
    call(TTYutf8sFeed, buf_idle, hello, style);
    u8sFeed1(buf_idle, '|');

    // Right pad
    style.pad = TTY_PAD_RIGHT;
    call(TTYutf8sFeed, buf_idle, hello, style);
    u8sFeed1(buf_idle, '|');

    u8sFeed1(buf_idle, '\n');

    printf("Padding (width=10): |");
    fwrite(buf[1], 1, *buf_idle - buf[1], stdout);

    // Verify left pad: 5 spaces + "hello"
    u8 *p = buf[1];
    want(p[0] == ' ' && p[4] == ' ' && p[5] == 'h');
    // Verify right pad: "hello" + 5 spaces (after |)
    want(p[11] == 'h' && p[15] == 'o' && p[16] == ' ');

    done;
}

ok64 TTYtestTrimming() {
    sane(1);
    a_pad(u8, buf, 128);

    a$str(long_text, "hello world");
    tty64 style = {.term = TTY_TERM_NONE, .width = 8};

    // Cut
    style.trim = TTY_TRIM_CUT;
    call(TTYutf8sFeed, buf_idle, long_text, style);
    u8sFeed1(buf_idle, '|');

    // Ellipsis
    style.trim = TTY_TRIM_ELLIPSIS;
    call(TTYutf8sFeed, buf_idle, long_text, style);
    u8sFeed1(buf_idle, '|');

    u8sFeed1(buf_idle, '\n');

    printf("Trimming (width=8): |");
    fwrite(buf[1], 1, *buf_idle - buf[1], stdout);

    // Verify cut: "hello wo"
    u8 *p = buf[1];
    want(p[0] == 'h' && p[7] == 'o');
    // Verify ellipsis: "hello..."
    want(p[9] == 'h' && p[14] == '.' && p[16] == '.');

    done;
}

ok64 TTYtest() {
    sane(1);
    call(TTYtestColors);
    call(TTYtestPadding);
    call(TTYtestTrimming);
    printf("TTY tests passed\n");
    done;
}

TEST(TTYtest);
