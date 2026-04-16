
/* #line 1 "bro/MAUS.c.rl" */
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


/* #line 39 "bro/MAUS.c.rl" */



/* #line 27 "bro/MAUS.rl.c" */
static const char _maus_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4
};

static const char _maus_key_offsets[] = {
	0, 0, 1, 2, 3, 5, 8, 10, 
	13, 15, 19
};

static const unsigned char _maus_trans_keys[] = {
	27u, 91u, 60u, 48u, 57u, 59u, 48u, 57u, 
	48u, 57u, 59u, 48u, 57u, 48u, 57u, 77u, 
	109u, 48u, 57u, 0
};

static const char _maus_single_lengths[] = {
	0, 1, 1, 1, 0, 1, 0, 1, 
	0, 2, 0
};

static const char _maus_range_lengths[] = {
	0, 0, 0, 0, 1, 1, 1, 1, 
	1, 1, 0
};

static const char _maus_index_offsets[] = {
	0, 0, 2, 4, 6, 8, 11, 13, 
	16, 18, 22
};

static const char _maus_indicies[] = {
	0, 1, 2, 1, 3, 1, 4, 1, 
	5, 4, 1, 6, 1, 7, 6, 1, 
	8, 1, 9, 10, 8, 1, 1, 0
};

static const char _maus_trans_targs[] = {
	2, 0, 3, 4, 5, 6, 7, 8, 
	9, 10, 10
};

static const char _maus_trans_actions[] = {
	0, 0, 0, 0, 1, 0, 3, 0, 
	5, 7, 9
};

static const int maus_start = 1;

static const int maus_en_main = 1;


/* #line 42 "bro/MAUS.c.rl" */

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

    
/* #line 91 "bro/MAUS.rl.c" */
	{
	cs = maus_start;
	}

/* #line 56 "bro/MAUS.c.rl" */
    
/* #line 94 "bro/MAUS.rl.c" */
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const unsigned char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _maus_trans_keys + _maus_key_offsets[cs];
	_trans = _maus_index_offsets[cs];

	_klen = _maus_single_lengths[cs];
	if ( _klen > 0 ) {
		const unsigned char *_lower = _keys;
		const unsigned char *_mid;
		const unsigned char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _maus_range_lengths[cs];
	if ( _klen > 0 ) {
		const unsigned char *_lower = _keys;
		const unsigned char *_mid;
		const unsigned char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _maus_indicies[_trans];
	cs = _maus_trans_targs[_trans];

	if ( _maus_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _maus_actions + _maus_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
/* #line 28 "bro/MAUS.c.rl" */
	{ btn = btn * 10 + (u32)(*p - '0'); }
	break;
	case 1:
/* #line 29 "bro/MAUS.c.rl" */
	{ col = col * 10 + (u32)(*p - '0'); }
	break;
	case 2:
/* #line 30 "bro/MAUS.c.rl" */
	{ row = row * 10 + (u32)(*p - '0'); }
	break;
	case 3:
/* #line 31 "bro/MAUS.c.rl" */
	{ released = 0; }
	break;
	case 4:
/* #line 32 "bro/MAUS.c.rl" */
	{ released = 1; }
	break;
/* #line 182 "bro/MAUS.rl.c" */
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}

/* #line 57 "bro/MAUS.c.rl" */

    if (cs < 10) return 0;

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
