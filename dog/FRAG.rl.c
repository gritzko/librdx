
/* #line 1 "dog/FRAG.c.rl" */
#include "FRAG.h"
#include <string.h>


/* #line 61 "dog/FRAG.c.rl" */



/* #line 7 "dog/FRAG.rl.c" */
static const char _frag_actions[] = {
	0, 1, 0, 1, 1, 1, 3, 1, 
	4, 1, 5, 1, 6, 1, 7, 1, 
	8, 1, 9, 1, 10, 1, 11, 1, 
	12, 2, 2, 3, 2, 9, 10, 2, 
	10, 5, 2, 10, 6, 2, 10, 8, 
	2, 11, 12
};

static const char _frag_key_offsets[] = {
	0, 0, 9, 15, 17, 19, 19, 21, 
	23, 23, 29, 35, 39, 43, 45, 52, 
	56, 59, 69, 79, 85, 92, 98, 104, 
	105, 109
};

static const char _frag_trans_keys[] = {
	39, 47, 95, 48, 57, 65, 90, 97, 
	122, 48, 57, 65, 90, 97, 122, 48, 
	57, 48, 57, 47, 92, 47, 92, 48, 
	57, 65, 70, 97, 102, 48, 57, 65, 
	70, 97, 102, 39, 46, 58, 92, 39, 
	46, 58, 92, 46, 58, 46, 48, 57, 
	65, 90, 97, 122, 45, 46, 48, 57, 
	46, 48, 57, 39, 46, 58, 92, 48, 
	57, 65, 90, 97, 122, 39, 46, 58, 
	92, 48, 57, 65, 90, 97, 122, 39, 
	46, 58, 92, 48, 57, 39, 45, 46, 
	58, 92, 48, 57, 39, 46, 58, 92, 
	48, 57, 39, 46, 58, 92, 48, 57, 
	46, 45, 46, 48, 57, 37, 46, 58, 
	95, 48, 57, 65, 90, 97, 122, 0
};

static const char _frag_single_lengths[] = {
	0, 3, 0, 0, 0, 0, 2, 2, 
	0, 0, 0, 4, 4, 2, 1, 2, 
	1, 4, 4, 4, 5, 4, 4, 1, 
	2, 4
};

static const char _frag_range_lengths[] = {
	0, 3, 3, 1, 1, 0, 0, 0, 
	0, 3, 3, 0, 0, 0, 3, 1, 
	1, 3, 3, 1, 1, 1, 1, 0, 
	1, 3
};

static const unsigned char _frag_index_offsets[] = {
	0, 0, 7, 11, 13, 15, 16, 19, 
	22, 23, 27, 31, 36, 41, 44, 49, 
	53, 56, 64, 72, 78, 85, 91, 97, 
	99, 103
};

static const char _frag_indicies[] = {
	0, 2, 4, 3, 4, 4, 1, 5, 
	5, 5, 1, 6, 1, 7, 1, 8, 
	10, 11, 9, 13, 14, 12, 12, 15, 
	15, 15, 1, 16, 16, 16, 1, 18, 
	19, 20, 21, 17, 22, 23, 24, 25, 
	8, 26, 27, 1, 28, 29, 29, 29, 
	1, 30, 31, 32, 1, 33, 34, 1, 
	22, 23, 24, 25, 35, 35, 35, 8, 
	22, 36, 24, 25, 37, 37, 37, 8, 
	22, 23, 24, 25, 38, 8, 22, 39, 
	40, 24, 25, 41, 8, 22, 23, 24, 
	25, 42, 8, 22, 43, 24, 25, 44, 
	8, 26, 1, 45, 46, 47, 1, 48, 
	49, 50, 16, 16, 16, 16, 1, 0
};

static const char _frag_trans_targs[] = {
	11, 0, 6, 24, 25, 14, 15, 16, 
	12, 7, 23, 8, 7, 23, 8, 10, 
	25, 12, 13, 17, 19, 5, 13, 17, 
	19, 5, 2, 3, 2, 14, 4, 2, 
	15, 2, 16, 18, 17, 18, 20, 21, 
	17, 20, 22, 17, 22, 4, 2, 24, 
	9, 2, 3
};

static const char _frag_trans_actions[] = {
	0, 0, 0, 25, 1, 13, 25, 25, 
	0, 21, 40, 21, 0, 23, 0, 0, 
	0, 17, 28, 28, 28, 17, 19, 19, 
	19, 0, 0, 0, 15, 0, 9, 9, 
	5, 11, 5, 13, 37, 0, 25, 9, 
	31, 5, 25, 34, 5, 7, 7, 5, 
	0, 3, 3
};

static const char _frag_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 28, 19, 0, 15, 9, 
	11, 19, 37, 19, 31, 19, 34, 0, 
	7, 3
};

static const int frag_start = 1;

static const int frag_en_main = 1;


/* #line 64 "dog/FRAG.c.rl" */

ok64 FRAGu8sDrain(u8cs input, fragp f) {
    if (f == NULL) return FRAGBAD;
    memset(f, 0, sizeof(frag));
    if (input[0] == NULL || input[0] >= input[1]) return OK;

    u8cp p = input[0];
    u8cp pe = input[1];
    u8cp eof = pe;
    u8cp mark = NULL;
    u8cp ext_mark = NULL;
    u32 nval = 0;
    int cs = 0;

    
/* #line 124 "dog/FRAG.rl.c" */
	{
	cs = frag_start;
	}

/* #line 79 "dog/FRAG.c.rl" */
    
/* #line 127 "dog/FRAG.rl.c" */
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _frag_trans_keys + _frag_key_offsets[cs];
	_trans = _frag_index_offsets[cs];

	_klen = _frag_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
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

	_klen = _frag_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
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
	_trans = _frag_indicies[_trans];
	cs = _frag_trans_targs[_trans];

	if ( _frag_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _frag_actions + _frag_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
/* #line 7 "dog/FRAG.c.rl" */
	{ mark = p; }
	break;
	case 1:
/* #line 8 "dog/FRAG.c.rl" */
	{ f->type = FRAG_IDENT; f->body[0] = mark; f->body[1] = p; }
	break;
	case 2:
/* #line 9 "dog/FRAG.c.rl" */
	{ nval = 0; }
	break;
	case 3:
/* #line 10 "dog/FRAG.c.rl" */
	{ nval = nval * 10 + (*p - '0'); }
	break;
	case 4:
/* #line 11 "dog/FRAG.c.rl" */
	{ if (f->line == 0) f->line = nval; else f->line_end = nval; }
	break;
	case 5:
/* #line 12 "dog/FRAG.c.rl" */
	{ f->line = nval; nval = 0; }
	break;
	case 6:
/* #line 13 "dog/FRAG.c.rl" */
	{ f->line_end = nval; }
	break;
	case 7:
/* #line 14 "dog/FRAG.c.rl" */
	{ ext_mark = p; }
	break;
	case 8:
/* #line 15 "dog/FRAG.c.rl" */
	{
        if (f->nexts < FRAG_MAX_EXTS) {
            f->exts[f->nexts][0] = ext_mark;
            f->exts[f->nexts][1] = p;
            f->nexts++;
        }
    }
	break;
	case 9:
/* #line 22 "dog/FRAG.c.rl" */
	{ mark = p; }
	break;
	case 10:
/* #line 23 "dog/FRAG.c.rl" */
	{ f->type = FRAG_SPOT; f->body[0] = mark; f->body[1] = p; }
	break;
	case 11:
/* #line 24 "dog/FRAG.c.rl" */
	{ mark = p; }
	break;
	case 12:
/* #line 25 "dog/FRAG.c.rl" */
	{ f->type = FRAG_PCRE; f->body[0] = mark; f->body[1] = p; }
	break;
/* #line 245 "dog/FRAG.rl.c" */
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _frag_actions + _frag_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 1:
/* #line 8 "dog/FRAG.c.rl" */
	{ f->type = FRAG_IDENT; f->body[0] = mark; f->body[1] = p; }
	break;
	case 4:
/* #line 11 "dog/FRAG.c.rl" */
	{ if (f->line == 0) f->line = nval; else f->line_end = nval; }
	break;
	case 5:
/* #line 12 "dog/FRAG.c.rl" */
	{ f->line = nval; nval = 0; }
	break;
	case 6:
/* #line 13 "dog/FRAG.c.rl" */
	{ f->line_end = nval; }
	break;
	case 8:
/* #line 15 "dog/FRAG.c.rl" */
	{
        if (f->nexts < FRAG_MAX_EXTS) {
            f->exts[f->nexts][0] = ext_mark;
            f->exts[f->nexts][1] = p;
            f->nexts++;
        }
    }
	break;
	case 9:
/* #line 22 "dog/FRAG.c.rl" */
	{ mark = p; }
	break;
	case 10:
/* #line 23 "dog/FRAG.c.rl" */
	{ f->type = FRAG_SPOT; f->body[0] = mark; f->body[1] = p; }
	break;
/* #line 287 "dog/FRAG.rl.c" */
		}
	}
	}

	_out: {}
	}

/* #line 80 "dog/FRAG.c.rl" */

    if (cs < 11) {
        // If parser didn't reach a final state but we got a type,
        // it's a tolerant parse (e.g., unclosed spot quote).
        if (f->type == FRAG_SPOT && f->body[0] != NULL) {
            // Unclosed spot: body extends to end of input
            f->body[1] = pe;
            return OK;
        }
        return FRAGFAIL;
    }

    // Line-only: digit-started fragment, type not set by ident/spot/pcre
    if (f->type == FRAG_NONE && f->line > 0)
        f->type = FRAG_LINE;

    return OK;
}

// Is `c` legal in a URI fragment?
// Printable ASCII (0x20-0x7E) except '#' (fragment delimiter) and '%' (pct prefix).
static const u8 FRAG_CHAR[256] = {
    // 0x00-0x1F: control chars — illegal
    [0x20] = 1,  // space (unwise, but legal per URI.lex)
    [0x21] = 1,  // !
    [0x22] = 1,  // "
    // 0x23 '#' — fragment delimiter, must escape
    [0x24] = 1,  // $
    // 0x25 '%' — pct-encoded prefix, must escape
    [0x26] = 1,  // &
    [0x27] = 1,  // '
    [0x28] = 1,  // (
    [0x29] = 1,  // )
    [0x2A] = 1,  // *
    [0x2B] = 1,  // +
    [0x2C] = 1,  // ,
    [0x2D] = 1,  // -
    [0x2E] = 1,  // .
    [0x2F] = 1,  // /
    ['0'] = 1, ['1'] = 1, ['2'] = 1, ['3'] = 1, ['4'] = 1,
    ['5'] = 1, ['6'] = 1, ['7'] = 1, ['8'] = 1, ['9'] = 1,
    [0x3A] = 1,  // :
    [0x3B] = 1,  // ;
    [0x3C] = 1,  // <
    [0x3D] = 1,  // =
    [0x3E] = 1,  // >
    [0x3F] = 1,  // ?
    [0x40] = 1,  // @
    ['A'] = 1, ['B'] = 1, ['C'] = 1, ['D'] = 1, ['E'] = 1, ['F'] = 1,
    ['G'] = 1, ['H'] = 1, ['I'] = 1, ['J'] = 1, ['K'] = 1, ['L'] = 1,
    ['M'] = 1, ['N'] = 1, ['O'] = 1, ['P'] = 1, ['Q'] = 1, ['R'] = 1,
    ['S'] = 1, ['T'] = 1, ['U'] = 1, ['V'] = 1, ['W'] = 1, ['X'] = 1,
    ['Y'] = 1, ['Z'] = 1,
    [0x5B] = 1,  // [
    [0x5C] = 1,  // backslash
    [0x5D] = 1,  // ]
    [0x5E] = 1,  // ^
    [0x5F] = 1,  // _
    [0x60] = 1,  // `
    ['a'] = 1, ['b'] = 1, ['c'] = 1, ['d'] = 1, ['e'] = 1, ['f'] = 1,
    ['g'] = 1, ['h'] = 1, ['i'] = 1, ['j'] = 1, ['k'] = 1, ['l'] = 1,
    ['m'] = 1, ['n'] = 1, ['o'] = 1, ['p'] = 1, ['q'] = 1, ['r'] = 1,
    ['s'] = 1, ['t'] = 1, ['u'] = 1, ['v'] = 1, ['w'] = 1, ['x'] = 1,
    ['y'] = 1, ['z'] = 1,
    [0x7B] = 1,  // {
    [0x7C] = 1,  // |
    [0x7D] = 1,  // }
    [0x7E] = 1,  // ~
    // 0x7F DEL — control, illegal
    // 0x80-0xFF — non-ASCII, illegal
};

con u8c FRAG_HEX[16] = "0123456789ABCDEF";

ok64 FRAGu8sEsc(u8s into, u8cs raw) {
    if (into[0] == NULL || into[0] >= into[1]) return FRAGFAIL;
    if (raw[0] == NULL || raw[0] >= raw[1]) return OK;
    u8cp p = raw[0];
    u8cp end = raw[1];
    while (p < end) {
        u8 c = *p++;
        if (FRAG_CHAR[c]) {
            if (into[0] >= into[1]) return FRAGFAIL;
            *into[0]++ = c;
        } else {
            if (into[0] + 3 > into[1]) return FRAGFAIL;
            *into[0]++ = '%';
            *into[0]++ = FRAG_HEX[(c >> 4) & 0xF];
            *into[0]++ = FRAG_HEX[c & 0xF];
        }
    }
    return OK;
}

static int frag_hexval(u8 c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

ok64 FRAGu8sUnesc(u8s into, u8cs esc) {
    if (into[0] == NULL || into[0] >= into[1]) return FRAGFAIL;
    if (esc[0] == NULL || esc[0] >= esc[1]) return OK;
    u8cp p = esc[0];
    u8cp end = esc[1];
    while (p < end) {
        if (into[0] >= into[1]) return FRAGFAIL;
        u8 c = *p++;
        if (c == '%' && p + 2 <= end) {
            int hi = frag_hexval(p[0]);
            int lo = frag_hexval(p[1]);
            if (hi >= 0 && lo >= 0) {
                *into[0]++ = (u8)((hi << 4) | lo);
                p += 2;
                continue;
            }
        }
        *into[0]++ = c;
    }
    return OK;
}
