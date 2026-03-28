
/* #line 1 "TOMLT.c.rl" */
#include "abc/INT.h"
#include "abc/PRO.h"
#include "TOMLT.h"

ok64 TOMLTonComment (u8cs tok, TOMLTstate* state);
ok64 TOMLTonString (u8cs tok, TOMLTstate* state);
ok64 TOMLTonNumber (u8cs tok, TOMLTstate* state);
ok64 TOMLTonWord (u8cs tok, TOMLTstate* state);
ok64 TOMLTonPunct (u8cs tok, TOMLTstate* state);
ok64 TOMLTonSpace (u8cs tok, TOMLTstate* state);


/* #line 113 "TOMLT.c.rl" */



/* #line 15 "TOMLT.rl.c" */
static const char _TOMLT_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	11, 1, 12, 1, 13, 1, 14, 1, 
	15, 1, 16, 1, 17, 1, 18, 1, 
	19, 1, 20, 1, 21, 1, 22, 1, 
	23, 1, 24, 1, 25, 1, 26, 1, 
	27, 1, 28, 1, 29, 1, 30, 1, 
	31, 1, 32, 1, 33, 1, 34, 1, 
	35, 1, 36, 1, 37, 1, 38, 1, 
	39, 1, 40, 2, 2, 3, 2, 2, 
	4, 2, 2, 5, 2, 2, 6, 2, 
	2, 7, 2, 2, 8, 2, 2, 9, 
	2, 2, 10
};

static const short _TOMLT_key_offsets[] = {
	0, 0, 3, 6, 15, 21, 27, 33, 
	39, 45, 51, 57, 63, 65, 66, 67, 
	76, 82, 88, 94, 100, 106, 112, 118, 
	124, 126, 128, 129, 130, 131, 133, 137, 
	139, 143, 145, 147, 149, 151, 157, 158, 
	159, 160, 161, 163, 165, 166, 168, 170, 
	172, 174, 175, 177, 179, 181, 183, 184, 
	186, 188, 190, 192, 194, 213, 216, 217, 
	218, 219, 220, 221, 222, 223, 228, 240, 
	245, 248, 254, 257, 260, 263, 270, 282, 
	288, 294, 301, 303, 308, 313, 319, 325, 
	333, 342, 351, 360
};

static const unsigned char _TOMLT_trans_keys[] = {
	10u, 34u, 92u, 10u, 34u, 92u, 34u, 85u, 
	92u, 98u, 102u, 110u, 114u, 116u, 117u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 65u, 
	70u, 97u, 102u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 65u, 
	70u, 97u, 102u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 65u, 70u, 97u, 102u, 34u, 
	92u, 34u, 34u, 34u, 85u, 92u, 98u, 102u, 
	110u, 114u, 116u, 117u, 48u, 57u, 65u, 70u, 
	97u, 102u, 48u, 57u, 65u, 70u, 97u, 102u, 
	48u, 57u, 65u, 70u, 97u, 102u, 48u, 57u, 
	65u, 70u, 97u, 102u, 48u, 57u, 65u, 70u, 
	97u, 102u, 48u, 57u, 65u, 70u, 97u, 102u, 
	48u, 57u, 65u, 70u, 97u, 102u, 48u, 57u, 
	65u, 70u, 97u, 102u, 10u, 39u, 10u, 39u, 
	39u, 39u, 39u, 48u, 57u, 43u, 45u, 48u, 
	57u, 48u, 57u, 43u, 45u, 48u, 57u, 48u, 
	57u, 48u, 57u, 48u, 49u, 48u, 55u, 48u, 
	57u, 65u, 70u, 97u, 102u, 110u, 102u, 97u, 
	110u, 48u, 57u, 48u, 57u, 45u, 48u, 57u, 
	48u, 57u, 48u, 57u, 48u, 57u, 58u, 48u, 
	57u, 48u, 57u, 48u, 57u, 48u, 57u, 58u, 
	48u, 57u, 48u, 57u, 48u, 57u, 48u, 57u, 
	48u, 57u, 32u, 34u, 35u, 39u, 43u, 45u, 
	46u, 48u, 95u, 105u, 110u, 9u, 13u, 49u, 
	57u, 65u, 90u, 97u, 122u, 32u, 9u, 13u, 
	34u, 34u, 34u, 10u, 39u, 39u, 39u, 48u, 
	105u, 110u, 49u, 57u, 46u, 66u, 69u, 79u, 
	88u, 95u, 98u, 101u, 111u, 120u, 48u, 57u, 
	69u, 95u, 101u, 48u, 57u, 95u, 48u, 57u, 
	46u, 69u, 95u, 101u, 48u, 57u, 95u, 48u, 
	57u, 95u, 48u, 49u, 95u, 48u, 55u, 95u, 
	48u, 57u, 65u, 70u, 97u, 102u, 46u, 66u, 
	69u, 79u, 88u, 95u, 98u, 101u, 111u, 120u, 
	48u, 57u, 46u, 69u, 95u, 101u, 48u, 57u, 
	46u, 69u, 95u, 101u, 48u, 57u, 45u, 46u, 
	69u, 95u, 101u, 48u, 57u, 32u, 84u, 43u, 
	45u, 58u, 90u, 122u, 43u, 45u, 46u, 90u, 
	122u, 43u, 45u, 90u, 122u, 48u, 57u, 46u, 
	69u, 95u, 101u, 48u, 57u, 45u, 95u, 48u, 
	57u, 65u, 90u, 97u, 122u, 45u, 95u, 110u, 
	48u, 57u, 65u, 90u, 97u, 122u, 45u, 95u, 
	102u, 48u, 57u, 65u, 90u, 97u, 122u, 45u, 
	95u, 97u, 48u, 57u, 65u, 90u, 98u, 122u, 
	45u, 95u, 110u, 48u, 57u, 65u, 90u, 97u, 
	122u, 0
};

static const char _TOMLT_single_lengths[] = {
	0, 3, 3, 9, 0, 0, 0, 0, 
	0, 0, 0, 0, 2, 1, 1, 9, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	2, 2, 1, 1, 1, 0, 2, 0, 
	2, 0, 0, 0, 0, 0, 1, 1, 
	1, 1, 0, 0, 1, 0, 0, 0, 
	0, 1, 0, 0, 0, 0, 1, 0, 
	0, 0, 0, 0, 11, 1, 1, 1, 
	1, 1, 1, 1, 1, 3, 10, 3, 
	1, 4, 1, 1, 1, 1, 10, 4, 
	4, 5, 2, 5, 5, 4, 4, 2, 
	3, 3, 3, 3
};

static const char _TOMLT_range_lengths[] = {
	0, 0, 0, 0, 3, 3, 3, 3, 
	3, 3, 3, 3, 0, 0, 0, 0, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	0, 0, 0, 0, 0, 1, 1, 1, 
	1, 1, 1, 1, 1, 3, 0, 0, 
	0, 0, 1, 1, 0, 1, 1, 1, 
	1, 0, 1, 1, 1, 1, 0, 1, 
	1, 1, 1, 1, 4, 1, 0, 0, 
	0, 0, 0, 0, 0, 1, 1, 1, 
	1, 1, 1, 1, 1, 3, 1, 1, 
	1, 1, 0, 0, 0, 1, 1, 3, 
	3, 3, 3, 3
};

static const short _TOMLT_index_offsets[] = {
	0, 0, 4, 8, 18, 22, 26, 30, 
	34, 38, 42, 46, 50, 53, 55, 57, 
	67, 71, 75, 79, 83, 87, 91, 95, 
	99, 102, 105, 107, 109, 111, 113, 117, 
	119, 123, 125, 127, 129, 131, 135, 137, 
	139, 141, 143, 145, 147, 149, 151, 153, 
	155, 157, 159, 161, 163, 165, 167, 169, 
	171, 173, 175, 177, 179, 195, 198, 200, 
	202, 204, 206, 208, 210, 212, 217, 229, 
	234, 237, 243, 246, 249, 252, 257, 269, 
	275, 281, 288, 291, 297, 303, 309, 315, 
	321, 328, 335, 342
};

static const char _TOMLT_indicies[] = {
	1, 2, 3, 0, 1, 4, 3, 0, 
	0, 5, 0, 0, 0, 0, 0, 0, 
	6, 1, 7, 7, 7, 1, 8, 8, 
	8, 1, 9, 9, 9, 1, 6, 6, 
	6, 1, 10, 10, 10, 1, 11, 11, 
	11, 1, 12, 12, 12, 1, 0, 0, 
	0, 1, 15, 16, 14, 17, 14, 18, 
	14, 14, 19, 14, 14, 14, 14, 14, 
	14, 20, 13, 21, 21, 21, 13, 22, 
	22, 22, 13, 23, 23, 23, 13, 20, 
	20, 20, 13, 24, 24, 24, 13, 25, 
	25, 25, 13, 26, 26, 26, 13, 14, 
	14, 14, 13, 1, 28, 27, 1, 29, 
	27, 32, 31, 33, 31, 34, 31, 36, 
	35, 38, 38, 39, 37, 39, 37, 41, 
	41, 42, 40, 42, 35, 43, 40, 44, 
	35, 45, 35, 46, 46, 46, 35, 48, 
	47, 49, 47, 50, 47, 49, 47, 51, 
	40, 52, 40, 53, 40, 54, 40, 55, 
	40, 57, 56, 58, 56, 59, 56, 60, 
	56, 61, 56, 62, 56, 63, 56, 64, 
	56, 65, 56, 66, 56, 67, 56, 68, 
	56, 69, 56, 71, 72, 73, 74, 75, 
	75, 76, 77, 79, 80, 81, 71, 78, 
	79, 79, 70, 71, 71, 82, 14, 83, 
	85, 84, 86, 84, 87, 73, 31, 88, 
	90, 89, 91, 89, 93, 94, 95, 43, 
	92, 97, 98, 99, 100, 101, 102, 98, 
	99, 100, 101, 43, 96, 104, 97, 104, 
	36, 103, 38, 39, 103, 97, 99, 102, 
	99, 43, 96, 41, 42, 105, 98, 44, 
	106, 100, 45, 107, 101, 46, 46, 46, 
	108, 97, 98, 99, 100, 101, 102, 98, 
	99, 100, 101, 109, 96, 97, 99, 102, 
	99, 110, 96, 97, 99, 102, 99, 111, 
	96, 112, 97, 99, 102, 99, 43, 96, 
	114, 114, 113, 115, 115, 116, 66, 66, 
	113, 115, 115, 117, 66, 66, 113, 115, 
	115, 66, 66, 69, 113, 97, 99, 102, 
	99, 109, 96, 79, 79, 79, 79, 79, 
	35, 79, 79, 119, 79, 79, 79, 118, 
	79, 79, 120, 79, 79, 79, 118, 79, 
	79, 121, 79, 79, 79, 118, 79, 79, 
	120, 79, 79, 79, 118, 0
};

static const char _TOMLT_trans_targs[] = {
	2, 0, 62, 3, 60, 4, 8, 5, 
	6, 7, 9, 10, 11, 60, 12, 13, 
	15, 14, 63, 16, 20, 17, 18, 19, 
	21, 22, 23, 25, 66, 60, 60, 26, 
	27, 28, 67, 60, 71, 60, 31, 72, 
	60, 33, 74, 73, 75, 76, 77, 60, 
	39, 60, 41, 43, 44, 45, 46, 82, 
	60, 48, 49, 50, 51, 83, 53, 54, 
	55, 56, 60, 58, 84, 85, 60, 61, 
	1, 65, 24, 69, 60, 78, 86, 87, 
	88, 90, 60, 60, 60, 64, 60, 60, 
	60, 60, 68, 60, 60, 70, 38, 40, 
	60, 29, 35, 32, 36, 37, 34, 60, 
	30, 60, 60, 60, 60, 79, 80, 81, 
	42, 60, 47, 52, 57, 59, 60, 89, 
	87, 91
};

static const char _TOMLT_trans_actions[] = {
	0, 0, 5, 0, 11, 0, 0, 0, 
	0, 0, 0, 0, 0, 53, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 5, 13, 55, 0, 
	0, 0, 0, 65, 79, 57, 0, 5, 
	59, 0, 82, 85, 73, 70, 67, 63, 
	0, 15, 0, 0, 0, 0, 0, 5, 
	61, 0, 0, 0, 0, 5, 0, 0, 
	0, 0, 17, 0, 5, 5, 19, 0, 
	0, 0, 0, 5, 21, 85, 85, 88, 
	0, 0, 51, 29, 25, 0, 7, 23, 
	31, 27, 0, 9, 49, 85, 0, 0, 
	43, 0, 0, 0, 0, 0, 0, 39, 
	0, 41, 37, 35, 33, 85, 85, 85, 
	0, 45, 0, 0, 0, 0, 47, 0, 
	76, 0
};

static const char _TOMLT_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0
};

static const char _TOMLT_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 3, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0
};

static const short _TOMLT_eof_trans[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 14, 14, 14, 14, 
	14, 14, 14, 14, 14, 14, 14, 14, 
	0, 0, 31, 31, 31, 36, 38, 38, 
	41, 36, 41, 36, 36, 36, 48, 48, 
	48, 48, 41, 41, 41, 41, 41, 57, 
	57, 57, 57, 57, 57, 57, 57, 57, 
	57, 57, 57, 57, 0, 83, 84, 85, 
	85, 88, 89, 90, 90, 93, 97, 104, 
	104, 97, 106, 107, 108, 109, 97, 97, 
	97, 97, 114, 114, 114, 114, 97, 36, 
	119, 119, 119, 119
};

static const int TOMLT_start = 60;
static const int TOMLT_first_final = 60;
static const int TOMLT_error = 0;

static const int TOMLT_en_main = 60;


/* #line 116 "TOMLT.c.rl" */

ok64 TOMLTLexer(TOMLTstate* state) {

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

    
/* #line 293 "TOMLT.rl.c" */
	{
	cs = TOMLT_start;
	ts = 0;
	te = 0;
	act = 0;
	}

/* #line 134 "TOMLT.c.rl" */
    
/* #line 299 "TOMLT.rl.c" */
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
	_acts = _TOMLT_actions + _TOMLT_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 1:
/* #line 1 "NONE" */
	{ts = p;}
	break;
/* #line 318 "TOMLT.rl.c" */
		}
	}

	_keys = _TOMLT_trans_keys + _TOMLT_key_offsets[cs];
	_trans = _TOMLT_index_offsets[cs];

	_klen = _TOMLT_single_lengths[cs];
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

	_klen = _TOMLT_range_lengths[cs];
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
	_trans = _TOMLT_indicies[_trans];
_eof_trans:
	cs = _TOMLT_trans_targs[_trans];

	if ( _TOMLT_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _TOMLT_actions + _TOMLT_trans_actions[_trans];
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
/* #line 39 "TOMLT.c.rl" */
	{act = 6;}
	break;
	case 4:
/* #line 39 "TOMLT.c.rl" */
	{act = 7;}
	break;
	case 5:
/* #line 39 "TOMLT.c.rl" */
	{act = 8;}
	break;
	case 6:
/* #line 39 "TOMLT.c.rl" */
	{act = 9;}
	break;
	case 7:
/* #line 39 "TOMLT.c.rl" */
	{act = 10;}
	break;
	case 8:
/* #line 39 "TOMLT.c.rl" */
	{act = 11;}
	break;
	case 9:
/* #line 39 "TOMLT.c.rl" */
	{act = 12;}
	break;
	case 10:
/* #line 45 "TOMLT.c.rl" */
	{act = 14;}
	break;
	case 11:
/* #line 33 "TOMLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 12:
/* #line 33 "TOMLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 13:
/* #line 33 "TOMLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 14:
/* #line 33 "TOMLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 15:
/* #line 39 "TOMLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 16:
/* #line 45 "TOMLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 17:
/* #line 51 "TOMLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 18:
/* #line 51 "TOMLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 19:
/* #line 27 "TOMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 20:
/* #line 33 "TOMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 21:
/* #line 33 "TOMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 22:
/* #line 33 "TOMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 23:
/* #line 33 "TOMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 24:
/* #line 39 "TOMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 25:
/* #line 39 "TOMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 26:
/* #line 39 "TOMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 27:
/* #line 39 "TOMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 28:
/* #line 39 "TOMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 29:
/* #line 39 "TOMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 30:
/* #line 45 "TOMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 31:
/* #line 45 "TOMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 32:
/* #line 51 "TOMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 33:
/* #line 57 "TOMLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonSpace(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 34:
/* #line 33 "TOMLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 35:
/* #line 33 "TOMLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 36:
/* #line 39 "TOMLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 37:
/* #line 39 "TOMLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 38:
/* #line 45 "TOMLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 39:
/* #line 51 "TOMLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 40:
/* #line 1 "NONE" */
	{	switch( act ) {
	case 6:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 7:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 8:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 9:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 10:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 11:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 12:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 14:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = TOMLTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	}
	}
	break;
/* #line 711 "TOMLT.rl.c" */
		}
	}

_again:
	_acts = _TOMLT_actions + _TOMLT_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
/* #line 1 "NONE" */
	{ts = 0;}
	break;
/* #line 722 "TOMLT.rl.c" */
		}
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _TOMLT_eof_trans[cs] > 0 ) {
		_trans = _TOMLT_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

/* #line 135 "TOMLT.c.rl" */

    state->data[0] = p;
    if (o==OK && cs < TOMLT_first_final)
        o = TOMLTBAD;

    return o;
}
