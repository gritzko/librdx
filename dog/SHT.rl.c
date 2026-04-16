
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


/* #line 113 "SHT.c.rl" */



/* #line 16 "SHT.rl.c" */
static const char _SHT_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	5, 1, 6, 1, 7, 1, 8, 1, 
	9, 1, 10, 1, 11, 1, 12, 1, 
	13, 1, 14, 1, 15, 1, 16, 1, 
	17, 1, 18, 1, 19, 1, 20, 1, 
	21, 1, 22, 2, 2, 3, 2, 2, 
	4
};

static const char _SHT_key_offsets[] = {
	0, 2, 2, 4, 4, 5, 7, 13, 
	42, 45, 46, 48, 49, 66, 73, 74, 
	75, 76, 77, 89, 90, 91, 95, 97, 
	103, 105, 108, 109, 111, 118, 119, 120
};

static const unsigned char _SHT_trans_keys[] = {
	34u, 92u, 39u, 92u, 39u, 101u, 116u, 48u, 
	57u, 65u, 70u, 97u, 102u, 32u, 33u, 34u, 
	35u, 36u, 38u, 39u, 40u, 41u, 43u, 45u, 
	46u, 48u, 59u, 60u, 61u, 62u, 91u, 93u, 
	95u, 124u, 9u, 13u, 49u, 57u, 65u, 90u, 
	97u, 122u, 32u, 9u, 13u, 61u, 34u, 92u, 
	10u, 33u, 39u, 40u, 42u, 45u, 95u, 123u, 
	35u, 36u, 48u, 57u, 63u, 64u, 65u, 90u, 
	97u, 122u, 95u, 48u, 57u, 65u, 90u, 97u, 
	122u, 38u, 39u, 40u, 41u, 61u, 101u, 103u, 
	108u, 110u, 122u, 100u, 102u, 114u, 115u, 119u, 
	120u, 113u, 101u, 88u, 120u, 48u, 57u, 48u, 
	57u, 48u, 57u, 65u, 70u, 97u, 102u, 38u, 
	59u, 38u, 60u, 62u, 60u, 38u, 62u, 95u, 
	48u, 57u, 65u, 90u, 97u, 122u, 91u, 93u, 
	38u, 124u, 0
};

static const char _SHT_single_lengths[] = {
	2, 0, 2, 0, 1, 2, 0, 21, 
	1, 1, 2, 1, 7, 1, 1, 1, 
	1, 1, 6, 1, 1, 2, 0, 0, 
	2, 3, 1, 2, 1, 1, 1, 2
};

static const char _SHT_range_lengths[] = {
	0, 0, 0, 0, 0, 0, 3, 4, 
	1, 0, 0, 0, 5, 3, 0, 0, 
	0, 0, 3, 0, 0, 1, 1, 3, 
	0, 0, 0, 0, 3, 0, 0, 0
};

static const unsigned char _SHT_index_offsets[] = {
	0, 3, 4, 7, 8, 10, 13, 17, 
	43, 46, 48, 51, 53, 66, 71, 73, 
	75, 77, 79, 89, 91, 93, 97, 99, 
	103, 106, 110, 112, 115, 120, 122, 124
};

static const char _SHT_indicies[] = {
	2, 3, 1, 1, 5, 6, 4, 4, 
	8, 7, 9, 9, 0, 11, 11, 11, 
	10, 13, 14, 15, 16, 17, 18, 19, 
	20, 21, 14, 22, 23, 24, 26, 27, 
	14, 28, 30, 31, 29, 32, 13, 25, 
	29, 29, 12, 13, 13, 33, 9, 34, 
	2, 3, 1, 35, 16, 36, 4, 9, 
	36, 36, 37, 9, 36, 36, 36, 37, 
	37, 34, 37, 37, 37, 37, 38, 9, 
	39, 8, 7, 9, 34, 9, 34, 9, 
	40, 41, 41, 42, 9, 9, 9, 9, 
	34, 9, 43, 9, 43, 45, 45, 25, 
	44, 25, 44, 11, 11, 11, 46, 9, 
	47, 34, 9, 48, 9, 34, 9, 43, 
	9, 9, 34, 29, 29, 29, 29, 49, 
	9, 34, 9, 34, 9, 9, 34, 0
};

static const char _SHT_trans_targs[] = {
	7, 0, 7, 1, 2, 7, 3, 4, 
	7, 7, 7, 23, 7, 8, 9, 10, 
	11, 12, 14, 15, 16, 17, 18, 7, 
	21, 22, 24, 25, 27, 28, 29, 30, 
	31, 7, 7, 7, 7, 13, 7, 7, 
	19, 5, 20, 7, 7, 6, 7, 14, 
	26, 7
};

static const char _SHT_trans_actions[] = {
	39, 0, 9, 0, 0, 11, 0, 0, 
	7, 15, 37, 0, 17, 0, 0, 5, 
	0, 5, 46, 5, 0, 0, 5, 19, 
	5, 0, 0, 0, 0, 0, 0, 0, 
	0, 35, 33, 21, 13, 0, 27, 41, 
	0, 0, 0, 31, 23, 0, 25, 43, 
	0, 29
};

static const char _SHT_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 1, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0
};

static const char _SHT_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 3, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0
};

static const unsigned char _SHT_eof_trans[] = {
	1, 1, 1, 1, 1, 1, 11, 0, 
	34, 35, 35, 36, 35, 39, 40, 35, 
	35, 35, 35, 44, 44, 45, 45, 47, 
	35, 35, 44, 35, 50, 35, 35, 35
};

static const int SHT_start = 7;
static const int SHT_first_final = 7;
static const int SHT_error = -1;

static const int SHT_en_main = 7;


/* #line 116 "SHT.c.rl" */

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

    
/* #line 156 "SHT.rl.c" */
	{
	cs = SHT_start;
	ts = 0;
	te = 0;
	act = 0;
	}

/* #line 134 "SHT.c.rl" */
    
/* #line 162 "SHT.rl.c" */
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const unsigned char *_keys;

	if ( p == pe )
		goto _test_eof;
_resume:
	_acts = _SHT_actions + _SHT_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 1:
/* #line 1 "NONE" */
	{ts = p;}
	break;
/* #line 179 "SHT.rl.c" */
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
	{act = 10;}
	break;
	case 4:
/* #line 56 "SHT.c.rl" */
	{act = 11;}
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
/* #line 56 "SHT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 10:
/* #line 56 "SHT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonPunct(tok, state);
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
/* #line 26 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 13:
/* #line 38 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 14:
/* #line 38 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 15:
/* #line 50 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonVar(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 16:
/* #line 44 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 17:
/* #line 56 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 18:
/* #line 56 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 19:
/* #line 62 "SHT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonSpace(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 20:
/* #line 38 "SHT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 21:
/* #line 56 "SHT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 22:
/* #line 1 "NONE" */
	{	switch( act ) {
	case 10:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = SHTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 11:
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
/* #line 410 "SHT.rl.c" */
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
/* #line 421 "SHT.rl.c" */
		}
	}

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

/* #line 135 "SHT.c.rl" */

    state->data[0] = p;
    if (o==OK && cs < SHT_first_final)
        o = SHTBAD;

    return o;
}
