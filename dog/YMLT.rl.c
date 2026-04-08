
/* #line 1 "YMLT.c.rl" */
#include "abc/INT.h"
#include "abc/PRO.h"
#include "YMLT.h"

ok64 YMLTonComment (u8cs tok, YMLTstate* state);
ok64 YMLTonString (u8cs tok, YMLTstate* state);
ok64 YMLTonNumber (u8cs tok, YMLTstate* state);
ok64 YMLTonWord (u8cs tok, YMLTstate* state);
ok64 YMLTonPunct (u8cs tok, YMLTstate* state);
ok64 YMLTonSpace (u8cs tok, YMLTstate* state);


/* #line 105 "YMLT.c.rl" */



/* #line 15 "YMLT.rl.c" */
static const char _YMLT_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	5, 1, 6, 1, 7, 1, 8, 1, 
	9, 1, 10, 1, 11, 1, 12, 1, 
	13, 1, 14, 1, 15, 1, 16, 1, 
	17, 1, 18, 1, 19, 1, 20, 1, 
	21, 1, 22, 1, 23, 1, 24, 1, 
	25, 1, 26, 1, 27, 1, 28, 1, 
	29, 2, 2, 3, 2, 2, 4
};

static const unsigned char _YMLT_key_offsets[] = {
	0, 0, 8, 15, 17, 17, 22, 23, 
	27, 31, 33, 35, 36, 37, 38, 42, 
	44, 48, 50, 52, 53, 54, 55, 57, 
	63, 85, 88, 95, 102, 103, 110, 113, 
	117, 119, 124, 128, 130, 132, 138, 147, 
	149, 155
};

static const unsigned char _YMLT_trans_keys[] = {
	33u, 95u, 48u, 57u, 65u, 90u, 97u, 122u, 
	95u, 48u, 57u, 65u, 90u, 97u, 122u, 34u, 
	92u, 95u, 65u, 90u, 97u, 122u, 39u, 73u, 
	105u, 48u, 57u, 43u, 45u, 48u, 57u, 48u, 
	57u, 78u, 110u, 70u, 102u, 110u, 43u, 45u, 
	48u, 57u, 48u, 57u, 43u, 45u, 48u, 57u, 
	48u, 57u, 65u, 97u, 78u, 97u, 110u, 48u, 
	55u, 48u, 57u, 65u, 70u, 97u, 102u, 32u, 
	33u, 34u, 35u, 38u, 39u, 42u, 43u, 45u, 
	46u, 48u, 62u, 95u, 124u, 9u, 13u, 49u, 
	57u, 65u, 90u, 97u, 122u, 32u, 9u, 13u, 
	95u, 48u, 57u, 65u, 90u, 97u, 122u, 95u, 
	48u, 57u, 65u, 90u, 97u, 122u, 10u, 95u, 
	48u, 57u, 65u, 90u, 97u, 122u, 46u, 48u, 
	57u, 69u, 101u, 48u, 57u, 48u, 57u, 46u, 
	69u, 101u, 48u, 57u, 69u, 101u, 48u, 57u, 
	48u, 57u, 48u, 57u, 73u, 78u, 105u, 110u, 
	48u, 57u, 46u, 69u, 79u, 88u, 101u, 111u, 
	120u, 48u, 57u, 48u, 55u, 48u, 57u, 65u, 
	70u, 97u, 102u, 95u, 48u, 57u, 65u, 90u, 
	97u, 122u, 0
};

static const char _YMLT_single_lengths[] = {
	0, 2, 1, 2, 0, 1, 1, 2, 
	2, 0, 2, 1, 1, 1, 2, 0, 
	2, 0, 2, 1, 1, 1, 0, 0, 
	14, 1, 1, 1, 1, 1, 1, 2, 
	0, 3, 2, 0, 0, 4, 7, 0, 
	0, 1
};

static const char _YMLT_range_lengths[] = {
	0, 3, 3, 0, 0, 2, 0, 1, 
	1, 1, 0, 0, 0, 0, 1, 1, 
	1, 1, 0, 0, 0, 0, 1, 3, 
	4, 1, 3, 3, 0, 3, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	3, 3
};

static const unsigned char _YMLT_index_offsets[] = {
	0, 0, 6, 11, 14, 15, 19, 21, 
	25, 29, 31, 34, 36, 38, 40, 44, 
	46, 50, 52, 55, 57, 59, 61, 63, 
	67, 86, 89, 94, 99, 101, 106, 109, 
	113, 115, 120, 124, 126, 128, 134, 143, 
	145, 149
};

static const char _YMLT_indicies[] = {
	0, 2, 2, 2, 2, 1, 3, 3, 
	3, 3, 1, 5, 6, 4, 4, 7, 
	7, 7, 1, 9, 8, 12, 13, 11, 
	10, 15, 15, 16, 14, 16, 14, 18, 
	19, 17, 20, 17, 20, 17, 19, 17, 
	22, 22, 23, 21, 23, 21, 25, 25, 
	26, 24, 26, 24, 28, 28, 27, 29, 
	27, 30, 27, 29, 27, 31, 24, 32, 
	32, 32, 24, 34, 35, 4, 36, 37, 
	8, 37, 38, 38, 39, 40, 42, 43, 
	42, 34, 41, 43, 43, 33, 34, 34, 
	44, 3, 3, 3, 3, 45, 2, 2, 
	2, 2, 46, 47, 36, 7, 7, 7, 
	7, 48, 50, 41, 49, 52, 52, 11, 
	51, 16, 51, 54, 55, 55, 41, 53, 
	57, 57, 54, 56, 23, 56, 26, 53, 
	12, 59, 13, 60, 11, 58, 54, 55, 
	61, 62, 55, 61, 62, 41, 53, 31, 
	63, 32, 32, 32, 64, 43, 43, 43, 
	43, 65, 0
};

static const char _YMLT_trans_targs[] = {
	2, 0, 27, 26, 3, 24, 4, 29, 
	6, 24, 24, 31, 10, 13, 24, 9, 
	32, 24, 11, 12, 24, 24, 15, 35, 
	24, 17, 36, 24, 19, 24, 21, 39, 
	40, 24, 25, 1, 28, 5, 30, 37, 
	38, 33, 24, 41, 24, 24, 24, 24, 
	24, 24, 7, 24, 8, 24, 34, 16, 
	24, 14, 24, 18, 20, 22, 23, 24, 
	24, 24
};

static const char _YMLT_trans_actions[] = {
	0, 0, 0, 0, 0, 7, 0, 0, 
	0, 9, 51, 5, 0, 0, 47, 0, 
	0, 55, 0, 0, 11, 45, 0, 0, 
	49, 0, 0, 53, 0, 13, 0, 0, 
	0, 17, 0, 0, 0, 0, 57, 60, 
	5, 5, 15, 0, 43, 35, 33, 19, 
	31, 39, 0, 27, 0, 29, 5, 0, 
	25, 0, 41, 0, 0, 0, 0, 23, 
	21, 37
};

static const char _YMLT_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0
};

static const char _YMLT_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	3, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0
};

static const unsigned char _YMLT_eof_trans[] = {
	0, 0, 0, 0, 0, 0, 0, 11, 
	15, 15, 18, 18, 18, 18, 22, 22, 
	25, 25, 28, 28, 28, 28, 25, 25, 
	0, 45, 46, 47, 48, 49, 50, 52, 
	52, 54, 57, 57, 54, 59, 54, 64, 
	65, 66
};

static const int YMLT_start = 24;
static const int YMLT_first_final = 24;
static const int YMLT_error = 0;

static const int YMLT_en_main = 24;


/* #line 108 "YMLT.c.rl" */

ok64 YMLTLexer(YMLTstate* state) {

    a_dup(u8c, data, state->data);
    sane($ok(data));

    int cs = 0;
    int act = 0;
    u8c *p = (u8c*) data[0];
    u8c *pe = (u8c*) data[1];
    u8c *eof = pe;
    u8c *ts = NULL;
    u8c *te = NULL;
    ok64 o = OK;

    u8cs tok = {p, p};

    
/* #line 183 "YMLT.rl.c" */
	{
	cs = YMLT_start;
	ts = 0;
	te = 0;
	act = 0;
	}

/* #line 126 "YMLT.c.rl" */
    
/* #line 189 "YMLT.rl.c" */
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
	_acts = _YMLT_actions + _YMLT_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 1:
/* #line 1 "NONE" */
	{ts = p;}
	break;
/* #line 208 "YMLT.rl.c" */
		}
	}

	_keys = _YMLT_trans_keys + _YMLT_key_offsets[cs];
	_trans = _YMLT_index_offsets[cs];

	_klen = _YMLT_single_lengths[cs];
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

	_klen = _YMLT_range_lengths[cs];
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
	_trans = _YMLT_indicies[_trans];
_eof_trans:
	cs = _YMLT_trans_targs[_trans];

	if ( _YMLT_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _YMLT_actions + _YMLT_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 2:
/* #line 1 "NONE" */
	{te = p+1;}
	break;
	case 3:
/* #line 50 "YMLT.c.rl" */
	{act = 16;}
	break;
	case 4:
/* #line 50 "YMLT.c.rl" */
	{act = 17;}
	break;
	case 5:
/* #line 32 "YMLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 6:
/* #line 32 "YMLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 7:
/* #line 38 "YMLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 8:
/* #line 38 "YMLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 9:
/* #line 50 "YMLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 10:
/* #line 50 "YMLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 11:
/* #line 26 "YMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 12:
/* #line 38 "YMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 13:
/* #line 38 "YMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 14:
/* #line 38 "YMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 15:
/* #line 38 "YMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 16:
/* #line 38 "YMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 17:
/* #line 50 "YMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 18:
/* #line 50 "YMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 19:
/* #line 50 "YMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 20:
/* #line 44 "YMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 21:
/* #line 50 "YMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 22:
/* #line 50 "YMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 23:
/* #line 56 "YMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonSpace(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 24:
/* #line 38 "YMLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 25:
/* #line 38 "YMLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 26:
/* #line 38 "YMLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 27:
/* #line 50 "YMLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 28:
/* #line 50 "YMLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 29:
/* #line 1 "NONE" */
	{	switch( act ) {
	case 16:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 17:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = YMLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	}
	}
	break;
/* #line 495 "YMLT.rl.c" */
		}
	}

_again:
	_acts = _YMLT_actions + _YMLT_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
/* #line 1 "NONE" */
	{ts = 0;}
	break;
/* #line 506 "YMLT.rl.c" */
		}
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _YMLT_eof_trans[cs] > 0 ) {
		_trans = _YMLT_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

/* #line 127 "YMLT.c.rl" */

    state->data[0] = p;
    if (o==OK && cs < YMLT_first_final)
        o = YMLTBAD;

    return o;
}
