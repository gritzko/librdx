
/* #line 1 "PYT.c.rl" */
#include "abc/INT.h"
#include "abc/PRO.h"
#include "PYT.h"

ok64 PYTonComment (u8cs tok, PYTstate* state);
ok64 PYTonString (u8cs tok, PYTstate* state);
ok64 PYTonNumber (u8cs tok, PYTstate* state);
ok64 PYTonDecorator (u8cs tok, PYTstate* state);
ok64 PYTonWord (u8cs tok, PYTstate* state);
ok64 PYTonPunct (u8cs tok, PYTstate* state);
ok64 PYTonSpace (u8cs tok, PYTstate* state);


/* #line 131 "PYT.c.rl" */



/* #line 16 "PYT.rl.c" */
static const char _PYT_actions[] = {
	0, 1, 2, 1, 3, 1, 12, 1, 
	13, 1, 14, 1, 15, 1, 16, 1, 
	17, 1, 18, 1, 19, 1, 20, 1, 
	21, 1, 22, 1, 23, 1, 24, 1, 
	25, 1, 26, 1, 27, 1, 28, 1, 
	29, 1, 30, 1, 31, 1, 32, 1, 
	33, 1, 34, 1, 35, 1, 36, 1, 
	37, 1, 38, 1, 39, 1, 40, 1, 
	41, 1, 42, 1, 43, 2, 0, 1, 
	2, 3, 4, 2, 3, 5, 2, 3, 
	6, 2, 3, 7, 2, 3, 8, 2, 
	3, 9, 2, 3, 10, 2, 3, 11
	
};

static const short _PYT_key_offsets[] = {
	0, 0, 2, 4, 21, 22, 27, 33, 
	39, 45, 51, 57, 63, 69, 75, 81, 
	82, 83, 84, 86, 88, 105, 106, 111, 
	117, 123, 129, 135, 141, 147, 153, 159, 
	165, 166, 167, 168, 169, 173, 175, 177, 
	181, 183, 185, 189, 191, 193, 195, 197, 
	203, 209, 246, 249, 250, 251, 252, 254, 
	255, 257, 259, 262, 269, 274, 276, 290, 
	296, 303, 308, 316, 321, 324, 327, 334, 
	336, 338, 346, 353, 370, 379
};

static const unsigned char _PYT_trans_keys[] = {
	34u, 92u, 34u, 92u, 10u, 34u, 39u, 48u, 
	78u, 85u, 92u, 110u, 114u, 117u, 120u, 97u, 
	98u, 101u, 102u, 116u, 118u, 123u, 32u, 65u, 
	90u, 97u, 122u, 32u, 125u, 65u, 90u, 97u, 
	122u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 65u, 
	70u, 97u, 102u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 65u, 
	70u, 97u, 102u, 48u, 57u, 65u, 70u, 97u, 
	102u, 34u, 34u, 34u, 39u, 92u, 39u, 92u, 
	10u, 34u, 39u, 48u, 78u, 85u, 92u, 110u, 
	114u, 117u, 120u, 97u, 98u, 101u, 102u, 116u, 
	118u, 123u, 32u, 65u, 90u, 97u, 122u, 32u, 
	125u, 65u, 90u, 97u, 122u, 48u, 57u, 65u, 
	70u, 97u, 102u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 65u, 
	70u, 97u, 102u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 39u, 39u, 39u, 
	46u, 43u, 45u, 48u, 57u, 48u, 57u, 48u, 
	57u, 43u, 45u, 48u, 57u, 48u, 57u, 48u, 
	57u, 43u, 45u, 48u, 57u, 48u, 57u, 48u, 
	57u, 48u, 49u, 48u, 55u, 48u, 57u, 65u, 
	70u, 97u, 102u, 61u, 95u, 65u, 90u, 97u, 
	122u, 32u, 33u, 34u, 35u, 37u, 38u, 39u, 
	42u, 43u, 45u, 46u, 47u, 48u, 58u, 60u, 
	61u, 62u, 64u, 66u, 70u, 82u, 85u, 94u, 
	95u, 98u, 102u, 114u, 117u, 124u, 9u, 13u, 
	49u, 57u, 65u, 90u, 97u, 122u, 32u, 9u, 
	13u, 61u, 34u, 10u, 38u, 61u, 39u, 42u, 
	61u, 61u, 62u, 46u, 48u, 57u, 69u, 74u, 
	95u, 101u, 106u, 48u, 57u, 74u, 95u, 106u, 
	48u, 57u, 47u, 61u, 46u, 66u, 69u, 74u, 
	79u, 88u, 95u, 98u, 101u, 106u, 111u, 120u, 
	48u, 57u, 69u, 74u, 101u, 106u, 48u, 57u, 
	69u, 74u, 95u, 101u, 106u, 48u, 57u, 74u, 
	95u, 106u, 48u, 57u, 46u, 69u, 74u, 95u, 
	101u, 106u, 48u, 57u, 74u, 95u, 106u, 48u, 
	57u, 95u, 48u, 49u, 95u, 48u, 55u, 95u, 
	48u, 57u, 65u, 70u, 97u, 102u, 60u, 61u, 
	61u, 62u, 46u, 95u, 48u, 57u, 65u, 90u, 
	97u, 122u, 95u, 48u, 57u, 65u, 90u, 97u, 
	122u, 34u, 39u, 66u, 70u, 82u, 85u, 95u, 
	98u, 102u, 114u, 117u, 48u, 57u, 65u, 90u, 
	97u, 122u, 34u, 39u, 95u, 48u, 57u, 65u, 
	90u, 97u, 122u, 61u, 124u, 0
};

static const char _PYT_single_lengths[] = {
	0, 2, 2, 11, 1, 1, 2, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	1, 1, 2, 2, 11, 1, 1, 2, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 1, 1, 1, 2, 0, 0, 2, 
	0, 0, 2, 0, 0, 0, 0, 0, 
	2, 29, 1, 1, 1, 1, 2, 1, 
	2, 0, 1, 5, 3, 2, 12, 4, 
	5, 3, 6, 3, 1, 1, 1, 2, 
	2, 2, 1, 11, 3, 2
};

static const char _PYT_range_lengths[] = {
	0, 0, 0, 3, 0, 2, 2, 3, 
	3, 3, 3, 3, 3, 3, 3, 0, 
	0, 0, 0, 0, 3, 0, 2, 2, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	0, 0, 0, 0, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 3, 
	2, 4, 1, 0, 0, 0, 0, 0, 
	0, 1, 1, 1, 1, 0, 1, 1, 
	1, 1, 1, 1, 1, 1, 3, 0, 
	0, 3, 3, 3, 3, 0
};

static const short _PYT_index_offsets[] = {
	0, 0, 3, 6, 21, 23, 27, 32, 
	36, 40, 44, 48, 52, 56, 60, 64, 
	66, 68, 70, 73, 76, 91, 93, 97, 
	102, 106, 110, 114, 118, 122, 126, 130, 
	134, 136, 138, 140, 142, 146, 148, 150, 
	154, 156, 158, 162, 164, 166, 168, 170, 
	174, 179, 213, 216, 218, 220, 222, 225, 
	227, 230, 232, 235, 242, 247, 250, 264, 
	270, 277, 282, 290, 295, 298, 301, 306, 
	309, 312, 318, 323, 338, 345
};

static const char _PYT_indicies[] = {
	2, 3, 1, 4, 3, 1, 1, 1, 
	1, 1, 5, 6, 1, 1, 1, 7, 
	8, 1, 1, 1, 0, 9, 0, 10, 
	10, 10, 0, 10, 1, 10, 10, 0, 
	11, 11, 11, 0, 12, 12, 12, 0, 
	13, 13, 13, 0, 7, 7, 7, 0, 
	14, 14, 14, 0, 8, 8, 8, 0, 
	15, 15, 15, 0, 1, 1, 1, 0, 
	18, 17, 19, 17, 20, 17, 22, 23, 
	21, 24, 23, 21, 21, 21, 21, 21, 
	25, 26, 21, 21, 21, 27, 28, 21, 
	21, 21, 0, 29, 0, 30, 30, 30, 
	0, 30, 21, 30, 30, 0, 31, 31, 
	31, 0, 32, 32, 32, 0, 33, 33, 
	33, 0, 27, 27, 27, 0, 34, 34, 
	34, 0, 28, 28, 28, 0, 35, 35, 
	35, 0, 21, 21, 21, 0, 38, 37, 
	39, 37, 40, 37, 42, 41, 44, 44, 
	45, 43, 45, 43, 46, 43, 48, 48, 
	49, 47, 49, 47, 50, 47, 52, 52, 
	53, 51, 53, 0, 54, 51, 55, 0, 
	56, 0, 57, 57, 57, 0, 42, 59, 
	59, 59, 58, 61, 62, 63, 64, 62, 
	65, 66, 67, 62, 68, 69, 70, 71, 
	62, 72, 62, 73, 74, 76, 76, 76, 
	76, 62, 75, 76, 76, 76, 76, 77, 
	61, 54, 75, 75, 60, 61, 61, 78, 
	42, 0, 17, 79, 80, 64, 42, 42, 
	81, 37, 82, 83, 42, 81, 42, 81, 
	85, 46, 84, 87, 88, 89, 87, 88, 
	46, 86, 88, 44, 88, 45, 86, 83, 
	42, 81, 91, 92, 93, 94, 95, 96, 
	97, 92, 93, 94, 95, 96, 54, 90, 
	99, 100, 99, 100, 50, 98, 99, 100, 
	101, 99, 100, 50, 98, 100, 48, 100, 
	49, 98, 91, 93, 94, 97, 93, 94, 
	54, 90, 103, 52, 103, 53, 102, 92, 
	55, 104, 95, 56, 105, 96, 57, 57, 
	57, 106, 83, 42, 81, 42, 83, 81, 
	59, 59, 59, 59, 59, 107, 75, 75, 
	75, 75, 108, 63, 66, 109, 109, 109, 
	109, 75, 109, 109, 109, 109, 75, 75, 
	75, 108, 63, 66, 75, 75, 75, 75, 
	108, 42, 42, 81, 0
};

static const char _PYT_trans_targs[] = {
	49, 2, 52, 3, 49, 4, 7, 11, 
	13, 5, 6, 8, 9, 10, 12, 14, 
	49, 15, 16, 17, 49, 19, 55, 20, 
	49, 21, 24, 28, 30, 22, 23, 25, 
	26, 27, 29, 31, 49, 32, 33, 34, 
	49, 49, 49, 49, 37, 60, 59, 49, 
	40, 65, 64, 49, 43, 67, 66, 68, 
	69, 70, 0, 73, 49, 50, 51, 1, 
	53, 54, 18, 56, 57, 58, 61, 62, 
	71, 72, 48, 74, 75, 77, 49, 49, 
	49, 49, 49, 51, 49, 35, 49, 36, 
	49, 38, 49, 63, 45, 42, 49, 46, 
	47, 44, 49, 39, 49, 41, 49, 49, 
	49, 49, 49, 49, 49, 76
};

static const char _PYT_trans_actions[] = {
	67, 0, 3, 0, 9, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	55, 0, 0, 0, 5, 0, 3, 0, 
	11, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 57, 0, 0, 0, 
	7, 65, 21, 61, 0, 3, 3, 59, 
	0, 3, 3, 63, 0, 81, 84, 78, 
	75, 72, 0, 0, 23, 0, 93, 0, 
	0, 0, 0, 0, 0, 3, 0, 84, 
	0, 0, 0, 0, 87, 0, 53, 27, 
	25, 49, 29, 90, 51, 0, 39, 0, 
	15, 0, 43, 3, 0, 0, 19, 0, 
	0, 0, 37, 0, 13, 0, 41, 17, 
	35, 33, 31, 45, 47, 87
};

static const char _PYT_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 69, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0
};

static const char _PYT_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 1, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0
};

static const short _PYT_eof_trans[] = {
	0, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 17, 
	17, 17, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	37, 37, 37, 42, 44, 44, 44, 48, 
	48, 48, 52, 1, 52, 1, 1, 1, 
	0, 0, 79, 1, 80, 81, 82, 83, 
	82, 82, 85, 87, 87, 82, 91, 99, 
	99, 99, 91, 103, 105, 106, 107, 82, 
	82, 108, 109, 109, 109, 82
};

static const int PYT_start = 49;
static const int PYT_first_final = 49;
static const int PYT_error = 0;

static const int PYT_en_main = 49;


/* #line 134 "PYT.c.rl" */

ok64 PYTLexer(PYTstate* state) {

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

    
/* #line 278 "PYT.rl.c" */
	{
	cs = PYT_start;
	ts = 0;
	te = 0;
	act = 0;
	}

/* #line 152 "PYT.c.rl" */
    
/* #line 284 "PYT.rl.c" */
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
	_acts = _PYT_actions + _PYT_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 2:
/* #line 1 "NONE" */
	{ts = p;}
	break;
/* #line 303 "PYT.rl.c" */
		}
	}

	_keys = _PYT_trans_keys + _PYT_key_offsets[cs];
	_trans = _PYT_index_offsets[cs];

	_klen = _PYT_single_lengths[cs];
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

	_klen = _PYT_range_lengths[cs];
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
	_trans = _PYT_indicies[_trans];
_eof_trans:
	cs = _PYT_trans_targs[_trans];

	if ( _PYT_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _PYT_actions + _PYT_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 3:
/* #line 1 "NONE" */
	{te = p+1;}
	break;
	case 4:
/* #line 49 "PYT.c.rl" */
	{act = 6;}
	break;
	case 5:
/* #line 49 "PYT.c.rl" */
	{act = 7;}
	break;
	case 6:
/* #line 49 "PYT.c.rl" */
	{act = 8;}
	break;
	case 7:
/* #line 49 "PYT.c.rl" */
	{act = 11;}
	break;
	case 8:
/* #line 49 "PYT.c.rl" */
	{act = 12;}
	break;
	case 9:
/* #line 61 "PYT.c.rl" */
	{act = 14;}
	break;
	case 10:
/* #line 67 "PYT.c.rl" */
	{act = 15;}
	break;
	case 11:
/* #line 67 "PYT.c.rl" */
	{act = 16;}
	break;
	case 12:
/* #line 43 "PYT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 13:
/* #line 43 "PYT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 14:
/* #line 43 "PYT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 15:
/* #line 43 "PYT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 16:
/* #line 49 "PYT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 17:
/* #line 49 "PYT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 18:
/* #line 49 "PYT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 19:
/* #line 49 "PYT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 20:
/* #line 67 "PYT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 21:
/* #line 67 "PYT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 22:
/* #line 37 "PYT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 23:
/* #line 43 "PYT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 24:
/* #line 43 "PYT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 25:
/* #line 49 "PYT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 26:
/* #line 49 "PYT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 27:
/* #line 49 "PYT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 28:
/* #line 49 "PYT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 29:
/* #line 49 "PYT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 30:
/* #line 49 "PYT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 31:
/* #line 49 "PYT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 32:
/* #line 55 "PYT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonDecorator(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 33:
/* #line 61 "PYT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 34:
/* #line 67 "PYT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 35:
/* #line 67 "PYT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 36:
/* #line 73 "PYT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonSpace(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 37:
/* #line 43 "PYT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 38:
/* #line 43 "PYT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 39:
/* #line 49 "PYT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 40:
/* #line 49 "PYT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 41:
/* #line 49 "PYT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 42:
/* #line 67 "PYT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 43:
/* #line 1 "NONE" */
	{	switch( act ) {
	case 0:
	{{cs = 0;goto _again;}}
	break;
	case 6:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 7:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 8:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 11:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 12:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 14:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 15:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 16:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PYTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	}
	}
	break;
/* #line 715 "PYT.rl.c" */
		}
	}

_again:
	_acts = _PYT_actions + _PYT_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
/* #line 1 "NONE" */
	{ts = 0;}
	break;
	case 1:
/* #line 1 "NONE" */
	{act = 0;}
	break;
/* #line 729 "PYT.rl.c" */
		}
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _PYT_eof_trans[cs] > 0 ) {
		_trans = _PYT_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

/* #line 153 "PYT.c.rl" */

    state->data[0] = p;
    if (o==OK && cs < PYT_first_final)
        o = PYTBAD;

    return o;
}
