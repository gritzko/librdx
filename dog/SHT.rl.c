
/* #line 1 "SHT.c.rl" */
#include "abc/INT.h"
#include "abc/PRO.h"
#include "SHT.h"

ok64 SHTonComment (u8cs tok, SHTstate* state);
ok64 SHTonString (u8cs tok, SHTstate* state);
ok64 SHTonNumber (u8cs tok, SHTstate* state);
ok64 SHTonWord (u8cs tok, SHTstate* state);
ok64 SHTonVar (u8cs tok, SHTstate* state);
ok64 SHTonPunct (u8cs tok, SHTstate* state);
ok64 SHTonSpace (u8cs tok, SHTstate* state);


/* #line 114 "SHT.c.rl" */



/* #line 16 "SHT.rl.c" */
static const char _SHT_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	5, 1, 6, 1, 7, 1, 8, 1, 
	9, 1, 10, 1, 11, 1, 12, 1, 
	13, 1, 14, 1, 15, 1, 16, 1, 
	17, 1, 18, 1, 19, 1, 20, 1, 
	21, 1, 22, 1, 23, 1, 24, 2, 
	2, 3, 2, 2, 4
};

static const char _SHT_key_offsets[] = {
	0, 0, 2, 2, 19, 21, 21, 22, 
	23, 24, 26, 32, 61, 64, 65, 66, 
	73, 74, 75, 76, 88, 89, 90, 94, 
	96, 102, 104, 107, 108, 110, 117, 118, 
	119
};

static const unsigned char _SHT_trans_keys[] = {
	34u, 92u, 33u, 39u, 40u, 42u, 45u, 95u, 
	123u, 35u, 36u, 48u, 57u, 63u, 64u, 65u, 
	90u, 97u, 122u, 39u, 92u, 41u, 125u, 39u, 
	101u, 116u, 48u, 57u, 65u, 70u, 97u, 102u, 
	32u, 33u, 34u, 35u, 36u, 38u, 39u, 40u, 
	41u, 43u, 45u, 46u, 48u, 59u, 60u, 61u, 
	62u, 91u, 93u, 95u, 124u, 9u, 13u, 49u, 
	57u, 65u, 90u, 97u, 122u, 32u, 9u, 13u, 
	61u, 10u, 95u, 48u, 57u, 65u, 90u, 97u, 
	122u, 38u, 40u, 41u, 61u, 101u, 103u, 108u, 
	110u, 122u, 100u, 102u, 114u, 115u, 119u, 120u, 
	113u, 101u, 88u, 120u, 48u, 57u, 48u, 57u, 
	48u, 57u, 65u, 70u, 97u, 102u, 38u, 59u, 
	38u, 60u, 62u, 60u, 38u, 62u, 95u, 48u, 
	57u, 65u, 90u, 97u, 122u, 91u, 93u, 38u, 
	124u, 0
};

static const char _SHT_single_lengths[] = {
	0, 2, 0, 7, 2, 0, 1, 1, 
	1, 2, 0, 21, 1, 1, 1, 1, 
	1, 1, 1, 6, 1, 1, 2, 0, 
	0, 2, 3, 1, 2, 1, 1, 1, 
	2
};

static const char _SHT_range_lengths[] = {
	0, 0, 0, 5, 0, 0, 0, 0, 
	0, 0, 3, 4, 1, 0, 0, 3, 
	0, 0, 0, 3, 0, 0, 1, 1, 
	3, 0, 0, 0, 0, 3, 0, 0, 
	0
};

static const unsigned char _SHT_index_offsets[] = {
	0, 0, 3, 4, 17, 20, 21, 23, 
	25, 27, 30, 34, 60, 63, 65, 67, 
	72, 74, 76, 78, 88, 90, 92, 96, 
	98, 102, 105, 109, 111, 114, 119, 121, 
	123
};

static const char _SHT_indicies[] = {
	1, 2, 0, 0, 3, 5, 6, 3, 
	3, 7, 8, 3, 3, 3, 7, 7, 
	4, 9, 10, 5, 5, 11, 6, 12, 
	8, 14, 13, 16, 16, 15, 18, 18, 
	18, 17, 20, 21, 0, 22, 23, 24, 
	13, 25, 26, 21, 27, 28, 29, 31, 
	32, 21, 33, 35, 36, 34, 37, 20, 
	30, 34, 34, 19, 20, 20, 38, 16, 
	39, 40, 22, 7, 7, 7, 7, 41, 
	16, 42, 16, 39, 16, 39, 16, 43, 
	44, 44, 45, 16, 16, 16, 16, 39, 
	16, 46, 16, 46, 48, 48, 30, 47, 
	30, 47, 18, 18, 18, 49, 16, 50, 
	39, 16, 51, 16, 39, 16, 46, 16, 
	16, 39, 34, 34, 34, 34, 52, 16, 
	39, 16, 39, 16, 16, 39, 0
};

static const char _SHT_trans_targs[] = {
	1, 11, 2, 11, 0, 4, 6, 15, 
	7, 11, 5, 11, 11, 8, 11, 11, 
	11, 11, 24, 11, 12, 13, 14, 3, 
	16, 17, 18, 19, 11, 22, 23, 25, 
	26, 28, 29, 30, 31, 32, 11, 11, 
	11, 11, 11, 20, 9, 21, 11, 11, 
	10, 11, 16, 27, 11
};

static const char _SHT_trans_actions[] = {
	0, 9, 0, 17, 0, 0, 0, 0, 
	0, 11, 0, 15, 13, 0, 7, 43, 
	19, 41, 0, 21, 0, 0, 0, 0, 
	50, 0, 0, 5, 23, 5, 0, 0, 
	0, 0, 0, 0, 0, 0, 39, 37, 
	25, 31, 45, 0, 0, 0, 35, 27, 
	0, 29, 47, 0, 33
};

static const char _SHT_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 1, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0
};

static const char _SHT_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 3, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0
};

static const unsigned char _SHT_eof_trans[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 16, 18, 0, 39, 40, 41, 42, 
	43, 40, 40, 40, 47, 47, 48, 48, 
	50, 40, 40, 47, 40, 53, 40, 40, 
	40
};

static const int SHT_start = 11;
static const int SHT_first_final = 11;
static const int SHT_error = 0;

static const int SHT_en_main = 11;


/* #line 117 "SHT.c.rl" */

ok64 SHTLexer(SHTstate* state) {

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

    
/* #line 163 "SHT.rl.c" */
	{
	cs = SHT_start;
	ts = 0;
	te = 0;
	act = 0;
	}

/* #line 135 "SHT.c.rl" */
    
/* #line 169 "SHT.rl.c" */
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
	_acts = _SHT_actions + _SHT_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 1:
/* #line 1 "NONE" */
	{ts = p;}
	break;
/* #line 188 "SHT.rl.c" */
		}
	}

	_keys = _SHT_trans_keys + _SHT_key_offsets[cs];
	_trans = _SHT_index_offsets[cs];

	_klen = _SHT_single_lengths[cs];
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

	_klen = _SHT_range_lengths[cs];
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
	_trans = _SHT_indicies[_trans];
_eof_trans:
	cs = _SHT_trans_targs[_trans];

	if ( _SHT_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _SHT_actions + _SHT_trans_actions[_trans];
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
/* #line 56 "SHT.c.rl" */
	{act = 12;}
	break;
	case 4:
/* #line 56 "SHT.c.rl" */
	{act = 13;}
	break;
	case 5:
/* #line 32 "SHT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 6:
/* #line 32 "SHT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 7:
/* #line 32 "SHT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 8:
/* #line 50 "SHT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonVar(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 9:
/* #line 50 "SHT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonVar(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 10:
/* #line 50 "SHT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonVar(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 11:
/* #line 56 "SHT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 12:
/* #line 56 "SHT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 13:
/* #line 56 "SHT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 14:
/* #line 26 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 15:
/* #line 38 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 16:
/* #line 38 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 17:
/* #line 50 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonVar(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 18:
/* #line 44 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 19:
/* #line 56 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 20:
/* #line 56 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 21:
/* #line 62 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonSpace(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 22:
/* #line 38 "SHT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 23:
/* #line 56 "SHT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 24:
/* #line 1 "NONE" */
	{	switch( act ) {
	case 12:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 13:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	}
	}
	break;
/* #line 435 "SHT.rl.c" */
		}
	}

_again:
	_acts = _SHT_actions + _SHT_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
/* #line 1 "NONE" */
	{ts = 0;}
	break;
/* #line 446 "SHT.rl.c" */
		}
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _SHT_eof_trans[cs] > 0 ) {
		_trans = _SHT_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

/* #line 136 "SHT.c.rl" */

    state->data[0] = p;
    if (o==OK && cs < SHT_first_final)
        o = SHTBAD;

    return o;
}
