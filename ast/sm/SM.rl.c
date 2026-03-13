
/* #line 1 "SM.c.rl" */
#include "abc/INT.h"
#include "abc/PRO.h"
#include "SM.h"

// action indices for the parser
#define SMenum 0
enum {
	SMHLine = SMenum+2,
	SMIndent = SMenum+3,
	SMOList = SMenum+4,
	SMUList = SMenum+5,
	SMH1 = SMenum+6,
	SMH2 = SMenum+7,
	SMH3 = SMenum+8,
	SMH4 = SMenum+9,
	SMH = SMenum+10,
	SMQuote = SMenum+11,
	SMCode = SMenum+12,
	SMTodo = SMenum+13,
	SMLink = SMenum+15,
	SMDiv = SMenum+18,
	SMLine = SMenum+19,
	SMRoot = SMenum+20,
};

// user functions (callbacks) for the parser
ok64 SMonHLine (u8cs tok, SMstate* state);
ok64 SMonIndent (u8cs tok, SMstate* state);
ok64 SMonOList (u8cs tok, SMstate* state);
ok64 SMonUList (u8cs tok, SMstate* state);
ok64 SMonH1 (u8cs tok, SMstate* state);
ok64 SMonH2 (u8cs tok, SMstate* state);
ok64 SMonH3 (u8cs tok, SMstate* state);
ok64 SMonH4 (u8cs tok, SMstate* state);
ok64 SMonH (u8cs tok, SMstate* state);
ok64 SMonQuote (u8cs tok, SMstate* state);
ok64 SMonCode (u8cs tok, SMstate* state);
ok64 SMonTodo (u8cs tok, SMstate* state);
ok64 SMonLink (u8cs tok, SMstate* state);
ok64 SMonDiv (u8cs tok, SMstate* state);
ok64 SMonLine (u8cs tok, SMstate* state);
ok64 SMonRoot (u8cs tok, SMstate* state);




/* #line 225 "SM.c.rl" */



/* #line 49 "SM.rl.c" */
static const char _SM_actions[] = {
	0, 2, 1, 25, 2, 3, 25, 2, 
	5, 25, 2, 7, 25, 2, 9, 25, 
	2, 11, 25, 2, 13, 25, 2, 15, 
	25, 2, 17, 25, 2, 19, 25, 2, 
	21, 25, 2, 23, 25, 2, 27, 29, 
	2, 28, 29, 3, 3, 4, 25, 3, 
	3, 16, 25, 3, 3, 18, 25, 3, 
	17, 4, 25, 3, 17, 16, 25, 3, 
	17, 18, 25, 4, 3, 0, 6, 25, 
	4, 3, 22, 20, 25, 4, 17, 0, 
	6, 25, 4, 17, 22, 20, 25, 4, 
	27, 26, 24, 25, 4, 28, 26, 24, 
	25, 5, 27, 26, 24, 4, 25, 5, 
	27, 26, 24, 16, 25, 5, 27, 26, 
	24, 18, 25, 5, 28, 26, 24, 4, 
	25, 5, 28, 26, 24, 16, 25, 5, 
	28, 26, 24, 18, 25, 6, 3, 8, 
	10, 12, 14, 25, 6, 17, 8, 10, 
	12, 14, 25, 6, 27, 26, 24, 0, 
	6, 25, 6, 27, 26, 24, 22, 20, 
	25, 6, 28, 26, 24, 0, 6, 25, 
	6, 28, 26, 24, 22, 20, 25, 8, 
	27, 26, 24, 8, 10, 12, 14, 25, 
	8, 28, 26, 24, 8, 10, 12, 14, 
	25, 9, 3, 2, 16, 8, 10, 12, 
	4, 6, 25, 9, 17, 2, 16, 8, 
	10, 12, 4, 6, 25, 11, 27, 26, 
	24, 2, 16, 8, 10, 12, 4, 6, 
	25, 11, 28, 26, 24, 2, 16, 8, 
	10, 12, 4, 6, 25
};

static const unsigned char _SM_key_offsets[] = {
	0, 1, 8, 15, 20, 29, 32, 34, 
	36, 37, 40, 42, 43, 46, 47, 48, 
	51, 53, 55, 56, 58, 60, 61, 65, 
	67, 69, 70, 74, 76, 78, 80, 82, 
	91, 101, 103, 105, 106, 108, 110, 111, 
	113, 116, 118, 120, 123, 124, 127, 130, 
	133, 142
};

static const unsigned char _SM_trans_keys[] = {
	10u, 10u, 32u, 35u, 45u, 62u, 48u, 57u, 
	10u, 32u, 35u, 45u, 62u, 48u, 57u, 10u, 
	32u, 35u, 45u, 62u, 10u, 32u, 35u, 45u, 
	62u, 91u, 96u, 48u, 57u, 10u, 32u, 35u, 
	10u, 32u, 10u, 32u, 10u, 10u, 32u, 35u, 
	10u, 32u, 10u, 10u, 32u, 35u, 10u, 10u, 
	10u, 32u, 45u, 10u, 32u, 10u, 32u, 10u, 
	10u, 45u, 10u, 45u, 10u, 10u, 46u, 48u, 
	57u, 10u, 32u, 10u, 32u, 10u, 10u, 46u, 
	48u, 57u, 10u, 46u, 10u, 32u, 10u, 32u, 
	10u, 32u, 10u, 32u, 35u, 45u, 62u, 91u, 
	96u, 48u, 57u, 10u, 32u, 88u, 120u, 48u, 
	57u, 65u, 90u, 97u, 122u, 10u, 93u, 10u, 
	32u, 10u, 10u, 93u, 10u, 58u, 10u, 10u, 
	93u, 10u, 32u, 58u, 10u, 96u, 10u, 96u, 
	10u, 32u, 96u, 10u, 10u, 32u, 35u, 10u, 
	32u, 35u, 10u, 32u, 35u, 10u, 32u, 35u, 
	45u, 62u, 91u, 96u, 48u, 57u, 10u, 32u, 
	35u, 45u, 62u, 91u, 96u, 48u, 57u, 0
};

static const char _SM_single_lengths[] = {
	1, 5, 5, 5, 7, 3, 2, 2, 
	1, 3, 2, 1, 3, 1, 1, 3, 
	2, 2, 1, 2, 2, 1, 2, 2, 
	2, 1, 2, 2, 2, 2, 2, 7, 
	4, 2, 2, 1, 2, 2, 1, 2, 
	3, 2, 2, 3, 1, 3, 3, 3, 
	7, 7
};

static const char _SM_range_lengths[] = {
	0, 1, 1, 0, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 1, 0, 
	0, 0, 1, 0, 0, 0, 0, 1, 
	3, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 1
};

static const unsigned char _SM_index_offsets[] = {
	0, 2, 9, 16, 22, 31, 35, 38, 
	41, 43, 47, 50, 52, 56, 58, 60, 
	64, 67, 70, 72, 75, 78, 80, 84, 
	87, 90, 92, 96, 99, 102, 105, 108, 
	117, 125, 128, 131, 133, 136, 139, 141, 
	144, 148, 151, 154, 158, 160, 164, 168, 
	172, 181
};

static const char _SM_trans_targs[] = {
	49, 0, 49, 2, 46, 16, 29, 26, 
	0, 49, 3, 45, 17, 30, 27, 0, 
	49, 4, 8, 18, 31, 0, 49, 1, 
	5, 15, 28, 32, 41, 22, 0, 49, 
	6, 9, 0, 49, 7, 0, 49, 8, 
	0, 49, 0, 49, 10, 12, 0, 49, 
	11, 0, 49, 0, 49, 13, 14, 0, 
	49, 0, 49, 0, 49, 16, 19, 0, 
	49, 17, 0, 49, 18, 0, 49, 0, 
	49, 20, 0, 49, 21, 0, 49, 0, 
	49, 23, 26, 0, 49, 24, 0, 49, 
	25, 0, 49, 0, 49, 24, 27, 0, 
	49, 25, 0, 49, 29, 0, 49, 30, 
	0, 49, 31, 0, 49, 1, 5, 15, 
	28, 32, 41, 22, 0, 49, 33, 39, 
	39, 36, 36, 36, 0, 49, 34, 0, 
	49, 35, 0, 49, 0, 49, 37, 0, 
	49, 38, 0, 49, 0, 49, 40, 0, 
	49, 35, 38, 0, 49, 42, 0, 49, 
	43, 0, 49, 44, 44, 0, 49, 0, 
	49, 8, 11, 0, 49, 7, 47, 0, 
	49, 11, 13, 0, 49, 1, 5, 15, 
	28, 32, 41, 22, 0, 49, 1, 5, 
	15, 28, 32, 41, 22, 0, 0
};

static const unsigned char _SM_trans_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 4, 193, 
	133, 67, 47, 72, 51, 43, 4, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 13, 13, 0, 0, 0, 0, 0, 
	0, 0, 16, 16, 0, 0, 0, 0, 
	19, 19, 22, 22, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 10, 10, 
	0, 0, 0, 0, 0, 0, 1, 1, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 7, 7, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 25, 203, 140, 77, 
	59, 82, 63, 55, 25, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 31, 31, 0, 0, 0, 
	0, 0, 0, 34, 34, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 28, 28, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 92, 225, 184, 161, 
	121, 168, 127, 115, 92, 87, 213, 175, 
	147, 103, 154, 109, 97, 87, 0
};

static const unsigned char _SM_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	40, 37
};

static const int SM_start = 48;
static const int SM_first_final = 48;
static const int SM_error = -1;

static const int SM_en_main = 48;


/* #line 228 "SM.c.rl" */

// the public API function
ok64 SMLexer(SMstate* state) {

    a_dup(u8c, data, state->data);
    sane($ok(data));

    int cs = 0;
    u8c *p = (u8c*) data[0];
    u8c *pe = (u8c*) data[1];
    u8c *eof = pe;
    u64 mark0[64] = {};
    ok64 o = OK;

    u8cs tok = {p, p};

    
/* #line 230 "SM.rl.c" */
	{
	cs = SM_start;
	}

/* #line 245 "SM.c.rl" */
    
/* #line 233 "SM.rl.c" */
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const unsigned char *_keys;

	if ( p == pe )
		goto _test_eof;
_resume:
	_keys = _SM_trans_keys + _SM_key_offsets[cs];
	_trans = _SM_index_offsets[cs];

	_klen = _SM_single_lengths[cs];
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

	_klen = _SM_range_lengths[cs];
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
	cs = _SM_trans_targs[_trans];

	if ( _SM_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _SM_actions + _SM_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
/* #line 53 "SM.c.rl" */
	{ mark0[SMHLine] = p - data[0]; }
	break;
	case 1:
/* #line 54 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMHLine];
    tok[1] = p;
    o = SMonHLine(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 2:
/* #line 62 "SM.c.rl" */
	{ mark0[SMIndent] = p - data[0]; }
	break;
	case 3:
/* #line 63 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMIndent];
    tok[1] = p;
    o = SMonIndent(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 4:
/* #line 71 "SM.c.rl" */
	{ mark0[SMOList] = p - data[0]; }
	break;
	case 5:
/* #line 72 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMOList];
    tok[1] = p;
    o = SMonOList(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 6:
/* #line 80 "SM.c.rl" */
	{ mark0[SMUList] = p - data[0]; }
	break;
	case 7:
/* #line 81 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMUList];
    tok[1] = p;
    o = SMonUList(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 8:
/* #line 89 "SM.c.rl" */
	{ mark0[SMH1] = p - data[0]; }
	break;
	case 9:
/* #line 90 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMH1];
    tok[1] = p;
    o = SMonH1(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 10:
/* #line 98 "SM.c.rl" */
	{ mark0[SMH2] = p - data[0]; }
	break;
	case 11:
/* #line 99 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMH2];
    tok[1] = p;
    o = SMonH2(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 12:
/* #line 107 "SM.c.rl" */
	{ mark0[SMH3] = p - data[0]; }
	break;
	case 13:
/* #line 108 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMH3];
    tok[1] = p;
    o = SMonH3(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 14:
/* #line 116 "SM.c.rl" */
	{ mark0[SMH4] = p - data[0]; }
	break;
	case 15:
/* #line 117 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMH4];
    tok[1] = p;
    o = SMonH4(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 16:
/* #line 134 "SM.c.rl" */
	{ mark0[SMQuote] = p - data[0]; }
	break;
	case 17:
/* #line 135 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMQuote];
    tok[1] = p;
    o = SMonQuote(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 18:
/* #line 143 "SM.c.rl" */
	{ mark0[SMCode] = p - data[0]; }
	break;
	case 19:
/* #line 144 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMCode];
    tok[1] = p;
    o = SMonCode(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 20:
/* #line 152 "SM.c.rl" */
	{ mark0[SMTodo] = p - data[0]; }
	break;
	case 21:
/* #line 153 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMTodo];
    tok[1] = p;
    o = SMonTodo(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 22:
/* #line 161 "SM.c.rl" */
	{ mark0[SMLink] = p - data[0]; }
	break;
	case 23:
/* #line 162 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMLink];
    tok[1] = p;
    o = SMonLink(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 24:
/* #line 170 "SM.c.rl" */
	{ mark0[SMDiv] = p - data[0]; }
	break;
	case 25:
/* #line 171 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMDiv];
    tok[1] = p;
    o = SMonDiv(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 26:
/* #line 179 "SM.c.rl" */
	{ mark0[SMLine] = p - data[0]; }
	break;
	case 27:
/* #line 180 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMLine];
    tok[1] = p;
    o = SMonLine(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 28:
/* #line 188 "SM.c.rl" */
	{ mark0[SMRoot] = p - data[0]; }
	break;
/* #line 488 "SM.rl.c" */
		}
	}

_again:
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _SM_actions + _SM_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 27:
/* #line 180 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMLine];
    tok[1] = p;
    o = SMonLine(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 28:
/* #line 188 "SM.c.rl" */
	{ mark0[SMRoot] = p - data[0]; }
	break;
	case 29:
/* #line 189 "SM.c.rl" */
	{
    tok[0] = data[0] + mark0[SMRoot];
    tok[1] = p;
    o = SMonRoot(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
/* #line 524 "SM.rl.c" */
		}
	}
	}

	}

/* #line 246 "SM.c.rl" */

_out:
    state->data[0] = p;
    if (o==OK && cs < SM_first_final) 
        o = SMBAD;
    
    return o;
}
