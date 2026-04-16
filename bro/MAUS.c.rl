// MAUS parses one specific CSI variant (SGR mouse,
// \033[<Btn;Col;RowM/m).  abc/ANSI writes SGR sequences but does not
// parse them — they live on opposite sides of the wire.  If more
// incoming CSI sequences appear (cursor-pos reports, OSC titles,
// function keys), factor out abc/CSI.c.rl with the common shell
// `\033[` + optional `<` private flag + `;`-separated decimal params
// + final byte; MAUS would become a thin consumer keyed on
// final='M'/'m' with private='<'.
#include "MAUS.h"

#include <unistd.h>

#define MAUS_SEQ_ON  "\033[?1000h\033[?1002h\033[?1006h"
#define MAUS_SEQ_OFF "\033[?1006l\033[?1002l\033[?1000l"

void MAUSEnable(int fd) {
    (void)write(fd, MAUS_SEQ_ON, sizeof(MAUS_SEQ_ON) - 1);
}

void MAUSDisable(int fd) {
    (void)write(fd, MAUS_SEQ_OFF, sizeof(MAUS_SEQ_OFF) - 1);
}

%%{
    machine maus;
    alphtype unsigned char;

    action btn_d { btn = btn * 10 + (u32)(*p - '0'); }
    action col_d { col = col * 10 + (u32)(*p - '0'); }
    action row_d { row = row * 10 + (u32)(*p - '0'); }
    action rel_M { released = 0; }
    action rel_m { released = 1; }

    main := 0x1B '[' '<'
            ( digit @btn_d )+ ';'
            ( digit @col_d )+ ';'
            ( digit @row_d )+
            ( 'M' @rel_M | 'm' @rel_m ) ;
}%%

%% write data nofinal noerror;

// Parse an SGR mouse sequence (\033[<Btn;Col;Row[Mm]) from input.
// Returns bytes consumed, 0 on incomplete/invalid input.
int MAUSParse(MAUSevent *ev, u8 const *buf, int len) {
    if (ev == NULL || buf == NULL || len < 9) return 0;

    u8c *p = buf;
    u8c *pe = buf + len;
    u8c *eof = pe;
    int cs = 0;
    u32 btn = 0, col = 0, row = 0;
    int released = 0;

    %% write init;
    %% write exec;

    if (cs < %%{ write first_final; }%%) return 0;

    ev->row = (u16)row;
    ev->col = (u16)col;
    if (btn >= 64) {
        ev->type   = MAUS_WHEEL;
        ev->button = (u8)btn;
    } else if (released) {
        ev->type   = MAUS_RELEASE;
        ev->button = (u8)(btn & 3);
    } else if (btn & 32) {
        ev->type   = MAUS_DRAG;
        ev->button = (u8)(btn & 3);
    } else {
        ev->type   = MAUS_PRESS;
        ev->button = (u8)(btn & 3);
    }
    return (int)(p - buf);
}
