
/* #line 1 "CSST.c.rl" */
#include "abc/INT.h"
#include "abc/PRO.h"
#include "CSST.h"

ok64 CSSTonComment (u8cs tok, CSSTstate* state);
ok64 CSSTonString (u8cs tok, CSSTstate* state);
ok64 CSSTonNumber (u8cs tok, CSSTstate* state);
ok64 CSSTonAtRule (u8cs tok, CSSTstate* state);
ok64 CSSTonWord (u8cs tok, CSSTstate* state);
ok64 CSSTonPunct (u8cs tok, CSSTstate* state);
ok64 CSSTonSpace (u8cs tok, CSSTstate* state);


/* #line 104 "CSST.c.rl" */



/* #line 16 "CSST.rl.c" */
static const char _CSST_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8, 1, 9, 1, 10, 1, 
	11, 1, 12, 1, 13, 1, 14, 1, 
	15, 1, 16, 1, 17, 1, 18
};

static const unsigned char _CSST_key_offsets[] = {
	0, 0, 2, 2, 8, 14, 20, 22, 
	22, 30, 31, 33, 35, 40, 57, 60, 
	66, 72, 78, 84, 90, 98, 100, 107, 
	112, 113, 121, 126, 133, 138
};

static const unsigned char _CSST_trans_keys[] = {
	34u, 92u, 48u, 57u, 65u, 70u, 97u, 102u, 
	48u, 57u, 65u, 70u, 97u, 102u, 48u, 57u, 
	65u, 70u, 97u, 102u, 39u, 92u, 45u, 95u, 
	48u, 57u, 65u, 90u, 97u, 122u, 42u, 42u, 
	47u, 48u, 57u, 45u, 65u, 90u, 97u, 122u, 
	32u, 34u, 35u, 39u, 45u, 46u, 47u, 64u, 
	95u, 9u, 13u, 48u, 57u, 65u, 90u, 97u, 
	122u, 32u, 9u, 13u, 48u, 57u, 65u, 70u, 
	97u, 102u, 48u, 57u, 65u, 70u, 97u, 102u, 
	48u, 57u, 65u, 70u, 97u, 102u, 48u, 57u, 
	65u, 70u, 97u, 102u, 48u, 57u, 65u, 70u, 
	97u, 102u, 45u, 95u, 48u, 57u, 65u, 90u, 
	97u, 122u, 48u, 57u, 37u, 48u, 57u, 65u, 
	90u, 97u, 122u, 37u, 65u, 90u, 97u, 122u, 
	42u, 37u, 46u, 48u, 57u, 65u, 90u, 97u, 
	122u, 37u, 65u, 90u, 97u, 122u, 37u, 48u, 
	57u, 65u, 90u, 97u, 122u, 45u, 65u, 90u, 
	97u, 122u, 45u, 95u, 48u, 57u, 65u, 90u, 
	97u, 122u, 0
};

static const char _CSST_single_lengths[] = {
	0, 2, 0, 0, 0, 0, 2, 0, 
	2, 1, 2, 0, 1, 9, 1, 0, 
	0, 0, 0, 0, 2, 0, 1, 1, 
	1, 2, 1, 1, 1, 2
};

static const char _CSST_range_lengths[] = {
	0, 0, 0, 3, 3, 3, 0, 0, 
	3, 0, 0, 1, 2, 4, 1, 3, 
	3, 3, 3, 3, 3, 1, 3, 2, 
	0, 3, 2, 3, 2, 3
};

static const unsigned char _CSST_index_offsets[] = {
	0, 0, 3, 4, 8, 12, 16, 19, 
	20, 26, 28, 31, 33, 37, 51, 54, 
	58, 62, 66, 70, 74, 80, 82, 87, 
	91, 93, 99, 103, 108, 112
};

static const char _CSST_indicies[] = {
	1, 2, 0, 0, 3, 3, 3, 4, 
	5, 5, 5, 4, 6, 6, 6, 4, 
	8, 9, 7, 7, 10, 10, 10, 10, 
	10, 4, 13, 12, 13, 14, 12, 16, 
	15, 17, 17, 17, 4, 19, 0, 20, 
	7, 21, 22, 23, 25, 26, 19, 24, 
	26, 26, 18, 19, 19, 27, 29, 29, 
	29, 28, 30, 30, 30, 28, 31, 31, 
	31, 28, 32, 32, 32, 28, 33, 33, 
	33, 28, 10, 10, 10, 10, 10, 34, 
	36, 35, 38, 36, 38, 38, 37, 38, 
	38, 38, 37, 12, 39, 41, 42, 24, 
	41, 41, 40, 41, 41, 41, 40, 41, 
	16, 41, 41, 40, 17, 17, 17, 43, 
	26, 26, 26, 26, 26, 44, 0
};

static const char _CSST_trans_targs[] = {
	1, 13, 2, 4, 0, 5, 15, 6, 
	13, 7, 20, 13, 9, 10, 13, 13, 
	27, 28, 13, 14, 3, 8, 21, 24, 
	25, 12, 29, 13, 13, 16, 17, 18, 
	19, 13, 13, 13, 22, 13, 23, 13, 
	13, 26, 11, 13, 13
};

static const char _CSST_trans_actions[] = {
	0, 9, 0, 0, 0, 0, 0, 0, 
	11, 0, 0, 37, 0, 0, 7, 35, 
	0, 0, 15, 0, 0, 0, 0, 5, 
	5, 0, 0, 33, 17, 0, 0, 0, 
	0, 13, 27, 31, 0, 21, 0, 29, 
	19, 0, 0, 23, 25
};

static const char _CSST_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 1, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0
};

static const char _CSST_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 3, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0
};

static const unsigned char _CSST_eof_trans[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 12, 12, 16, 0, 0, 28, 29, 
	29, 29, 29, 29, 35, 36, 38, 38, 
	40, 41, 41, 41, 44, 45
};

static const int CSST_start = 13;
static const int CSST_first_final = 13;
static const int CSST_error = 0;

static const int CSST_en_main = 13;


/* #line 107 "CSST.c.rl" */

ok64 CSSTLexer(CSSTstate* state) {

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

    
/* #line 154 "CSST.rl.c" */
	{
	cs = CSST_start;
	ts = 0;
	te = 0;
	act = 0;
	}

/* #line 125 "CSST.c.rl" */
    
/* #line 160 "CSST.rl.c" */
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
	_acts = _CSST_actions + _CSST_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 1:
/* #line 1 "NONE" */
	{ts = p;}
	break;
/* #line 179 "CSST.rl.c" */
		}
	}

	_keys = _CSST_trans_keys + _CSST_key_offsets[cs];
	_trans = _CSST_index_offsets[cs];

	_klen = _CSST_single_lengths[cs];
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

	_klen = _CSST_range_lengths[cs];
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
	_trans = _CSST_indicies[_trans];
_eof_trans:
	cs = _CSST_trans_targs[_trans];

	if ( _CSST_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _CSST_actions + _CSST_trans_actions[_trans];
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
/* #line 28 "CSST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 4:
/* #line 34 "CSST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 5:
/* #line 34 "CSST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 6:
/* #line 40 "CSST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 7:
/* #line 58 "CSST.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 8:
/* #line 40 "CSST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 9:
/* #line 40 "CSST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 10:
/* #line 40 "CSST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 11:
/* #line 46 "CSST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonAtRule(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 12:
/* #line 52 "CSST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 13:
/* #line 52 "CSST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 14:
/* #line 58 "CSST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 15:
/* #line 58 "CSST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 16:
/* #line 64 "CSST.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonSpace(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 17:
/* #line 40 "CSST.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 18:
/* #line 58 "CSST.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = CSSTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
/* #line 375 "CSST.rl.c" */
		}
	}

_again:
	_acts = _CSST_actions + _CSST_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
/* #line 1 "NONE" */
	{ts = 0;}
	break;
/* #line 386 "CSST.rl.c" */
		}
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _CSST_eof_trans[cs] > 0 ) {
		_trans = _CSST_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

/* #line 126 "CSST.c.rl" */

    state->data[0] = p;
    if (o==OK && cs < CSST_first_final)
        o = CSSTBAD;

    return o;
}
