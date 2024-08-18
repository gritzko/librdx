
#line 1 "MARK2.rl"
#include "MARK2.rl.h"



#line 81 "MARK2.rl"



#line 7 "MARK2.rl.c"
static const char _MARK2_actions[] = {
	0, 1, 2, 1, 11, 2, 1, 2, 
	2, 1, 11, 2, 3, 2, 2, 3, 
	11, 2, 5, 2, 2, 5, 11, 2, 
	6, 0, 2, 7, 2, 2, 7, 11, 
	2, 8, 2, 2, 9, 2, 2, 9, 
	11, 2, 10, 2, 2, 10, 11, 3, 
	1, 6, 0, 3, 1, 8, 2, 3, 
	3, 6, 0, 3, 3, 8, 2, 3, 
	4, 8, 2, 3, 5, 6, 0, 3, 
	5, 8, 2, 3, 7, 6, 0, 3, 
	7, 8, 2, 3, 9, 1, 2, 3, 
	9, 1, 11, 3, 9, 6, 0, 3, 
	9, 8, 2, 3, 10, 6, 0, 3, 
	10, 8, 2, 4, 1, 4, 8, 2, 
	4, 3, 4, 8, 2, 4, 5, 4, 
	8, 2, 4, 7, 4, 8, 2, 4, 
	9, 1, 6, 0, 4, 9, 1, 8, 
	2, 4, 9, 4, 8, 2, 4, 10, 
	4, 8, 2, 5, 9, 1, 4, 8, 
	2
};

static const short _MARK2_key_offsets[] = {
	0, 6, 13, 20, 27, 34, 41, 48, 
	56, 69, 76, 84, 92, 100, 108, 116, 
	124, 132, 140, 148, 157, 171, 179, 188, 
	194, 202, 210, 219, 227, 235, 243, 251, 
	260, 267, 275, 283, 290, 297, 304, 312
};

static const unsigned char _MARK2_trans_keys[] = {
	13u, 32u, 42u, 95u, 9u, 10u, 13u, 32u, 
	42u, 93u, 95u, 9u, 10u, 13u, 32u, 42u, 
	91u, 95u, 9u, 10u, 13u, 32u, 42u, 93u, 
	95u, 9u, 10u, 13u, 32u, 42u, 93u, 95u, 
	9u, 10u, 13u, 32u, 42u, 93u, 95u, 9u, 
	10u, 13u, 32u, 42u, 93u, 95u, 9u, 10u, 
	13u, 32u, 42u, 91u, 93u, 95u, 9u, 10u, 
	13u, 32u, 42u, 93u, 95u, 9u, 10u, 48u, 
	57u, 65u, 90u, 97u, 122u, 13u, 32u, 42u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 91u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 92u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 92u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 91u, 
	92u, 95u, 9u, 10u, 13u, 32u, 42u, 92u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 92u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 92u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 92u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 92u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 91u, 
	92u, 93u, 95u, 9u, 10u, 13u, 32u, 42u, 
	92u, 93u, 95u, 9u, 10u, 48u, 57u, 65u, 
	90u, 97u, 122u, 13u, 32u, 42u, 92u, 93u, 
	95u, 9u, 10u, 13u, 32u, 42u, 91u, 92u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 95u, 
	9u, 10u, 13u, 32u, 42u, 92u, 93u, 95u, 
	9u, 10u, 13u, 32u, 42u, 92u, 93u, 95u, 
	9u, 10u, 13u, 32u, 42u, 91u, 92u, 93u, 
	95u, 9u, 10u, 13u, 32u, 42u, 92u, 93u, 
	95u, 9u, 10u, 13u, 32u, 42u, 92u, 93u, 
	95u, 9u, 10u, 13u, 32u, 42u, 92u, 93u, 
	95u, 9u, 10u, 13u, 32u, 42u, 92u, 93u, 
	95u, 9u, 10u, 13u, 32u, 42u, 91u, 92u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 93u, 
	95u, 9u, 10u, 13u, 32u, 42u, 91u, 93u, 
	95u, 9u, 10u, 13u, 32u, 42u, 92u, 93u, 
	95u, 9u, 10u, 13u, 32u, 42u, 93u, 95u, 
	9u, 10u, 13u, 32u, 42u, 93u, 95u, 9u, 
	10u, 13u, 32u, 42u, 93u, 95u, 9u, 10u, 
	13u, 32u, 42u, 91u, 93u, 95u, 9u, 10u, 
	13u, 32u, 42u, 92u, 93u, 95u, 9u, 10u, 
	0
};

static const char _MARK2_single_lengths[] = {
	4, 5, 5, 5, 5, 5, 5, 6, 
	5, 5, 6, 6, 6, 6, 6, 6, 
	6, 6, 6, 7, 6, 6, 7, 4, 
	6, 6, 7, 6, 6, 6, 6, 7, 
	5, 6, 6, 5, 5, 5, 6, 6
};

static const char _MARK2_range_lengths[] = {
	1, 1, 1, 1, 1, 1, 1, 1, 
	4, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 4, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1
};

static const short _MARK2_index_offsets[] = {
	0, 6, 13, 20, 27, 34, 41, 48, 
	56, 66, 73, 81, 89, 97, 105, 113, 
	121, 129, 137, 145, 154, 165, 173, 182, 
	188, 196, 204, 213, 221, 229, 237, 245, 
	254, 261, 269, 277, 284, 291, 298, 306
};

static const char _MARK2_indicies[] = {
	1, 1, 2, 3, 1, 0, 5, 5, 
	6, 7, 8, 5, 4, 5, 5, 9, 
	10, 8, 5, 4, 5, 5, 12, 13, 
	14, 5, 11, 16, 16, 17, 18, 19, 
	16, 15, 21, 21, 22, 23, 24, 21, 
	20, 5, 5, 25, 7, 8, 5, 4, 
	5, 5, 6, 26, 7, 8, 5, 4, 
	5, 5, 6, 7, 8, 5, 27, 27, 
	27, 4, 5, 5, 6, 28, 8, 5, 
	4, 30, 30, 31, 32, 33, 34, 30, 
	29, 5, 5, 36, 37, 38, 39, 5, 
	35, 40, 40, 36, 37, 38, 41, 40, 
	35, 40, 40, 42, 43, 37, 39, 40, 
	35, 40, 40, 45, 46, 47, 41, 40, 
	44, 49, 49, 50, 51, 52, 41, 49, 
	48, 54, 54, 55, 56, 57, 41, 54, 
	53, 40, 40, 58, 37, 38, 41, 40, 
	35, 40, 40, 36, 37, 38, 39, 40, 
	35, 40, 40, 36, 59, 37, 38, 41, 
	40, 35, 40, 40, 36, 37, 38, 41, 
	40, 60, 60, 60, 35, 40, 40, 36, 
	37, 61, 41, 40, 35, 63, 63, 64, 
	65, 66, 67, 41, 63, 62, 69, 69, 
	70, 71, 69, 68, 49, 49, 72, 51, 
	52, 41, 49, 48, 49, 49, 50, 51, 
	52, 73, 49, 48, 49, 49, 50, 74, 
	51, 52, 41, 49, 48, 40, 40, 76, 
	77, 78, 41, 40, 75, 80, 80, 81, 
	82, 83, 41, 80, 79, 85, 85, 86, 
	87, 88, 41, 85, 84, 80, 80, 81, 
	82, 83, 89, 80, 79, 80, 80, 81, 
	90, 82, 83, 41, 80, 79, 16, 16, 
	91, 18, 19, 16, 15, 16, 16, 17, 
	92, 18, 19, 16, 15, 16, 16, 50, 
	51, 52, 73, 16, 48, 5, 5, 94, 
	95, 96, 5, 93, 98, 98, 99, 100, 
	101, 98, 97, 103, 103, 104, 105, 106, 
	103, 102, 98, 98, 99, 107, 100, 101, 
	98, 97, 98, 98, 81, 82, 83, 89, 
	98, 79, 0
};

static const char _MARK2_trans_targs[] = {
	1, 2, 6, 11, 1, 2, 5, 7, 
	11, 3, 35, 4, 32, 33, 34, 1, 
	2, 5, 7, 11, 1, 2, 6, 7, 
	11, 6, 8, 9, 10, 1, 2, 5, 
	8, 7, 11, 12, 16, 18, 19, 12, 
	13, 23, 14, 27, 15, 24, 25, 26, 
	12, 13, 16, 18, 19, 12, 13, 17, 
	18, 19, 17, 20, 21, 22, 12, 13, 
	16, 20, 18, 19, 1, 2, 6, 11, 
	17, 12, 20, 28, 29, 30, 31, 12, 
	13, 16, 18, 19, 12, 13, 17, 18, 
	19, 12, 20, 6, 8, 36, 37, 38, 
	39, 1, 2, 5, 7, 11, 1, 2, 
	6, 7, 11, 8
};

static const unsigned char _MARK2_trans_actions[] = {
	103, 99, 41, 142, 32, 23, 1, 32, 
	63, 1, 32, 32, 1, 32, 63, 79, 
	75, 26, 79, 122, 95, 91, 35, 95, 
	137, 1, 32, 32, 32, 59, 55, 11, 
	59, 59, 112, 32, 1, 32, 32, 63, 
	23, 0, 1, 32, 32, 1, 32, 32, 
	79, 75, 26, 79, 79, 95, 91, 35, 
	95, 95, 1, 32, 32, 32, 59, 55, 
	11, 59, 59, 59, 71, 67, 17, 117, 
	26, 122, 79, 32, 1, 32, 32, 51, 
	47, 5, 51, 51, 132, 127, 83, 132, 
	132, 107, 51, 26, 79, 32, 1, 32, 
	63, 51, 47, 5, 51, 107, 132, 127, 
	83, 132, 147, 51
};

static const unsigned char _MARK2_eof_actions[] = {
	44, 3, 3, 3, 29, 38, 3, 3, 
	3, 3, 14, 3, 3, 3, 3, 29, 
	38, 3, 3, 3, 3, 3, 14, 20, 
	29, 29, 29, 3, 8, 87, 8, 8, 
	29, 29, 29, 3, 8, 87, 8, 8
};

static const int MARK2_start = 0;
static const int MARK2_first_final = 0;
static const int MARK2_error = -1;

static const int MARK2_en_main = 0;


#line 84 "MARK2.rl"

pro(MARK2lexer, MARK2state* state) {
    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = state->cs;
    int res = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = state->tbc ? NULL : pe;
    u8c *pb = p;

    u32 sp = 2;
    $u8c tok = {p, p};

    
#line 212 "MARK2.rl.c"
	{
	cs = MARK2_start;
	}

#line 100 "MARK2.rl"
    
#line 215 "MARK2.rl.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const unsigned char *_keys;

	if ( p == pe )
		goto _test_eof;
_resume:
	_keys = _MARK2_trans_keys + _MARK2_key_offsets[cs];
	_trans = _MARK2_index_offsets[cs];

	_klen = _MARK2_single_lengths[cs];
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

	_klen = _MARK2_range_lengths[cs];
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
	_trans = _MARK2_indicies[_trans];
	cs = _MARK2_trans_targs[_trans];

	if ( _MARK2_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _MARK2_actions + _MARK2_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 10 "MARK2.rl"
	{ state->mark0[MARK2Ref0] = p - state->doc[0]; }
	break;
	case 1:
#line 11 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Ref0];
    tok[1] = p;
    call(MARK2onRef0, tok, state); 
}
	break;
	case 2:
#line 16 "MARK2.rl"
	{ state->mark0[MARK2Ref1] = p - state->doc[0]; }
	break;
	case 3:
#line 17 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Ref1];
    tok[1] = p;
    call(MARK2onRef1, tok, state); 
}
	break;
	case 4:
#line 22 "MARK2.rl"
	{ state->mark0[MARK2Em] = p - state->doc[0]; }
	break;
	case 5:
#line 23 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Em];
    tok[1] = p;
    call(MARK2onEm, tok, state); 
}
	break;
	case 6:
#line 28 "MARK2.rl"
	{ state->mark0[MARK2StA0] = p - state->doc[0]; }
	break;
	case 7:
#line 29 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2StA0];
    tok[1] = p;
    call(MARK2onStA0, tok, state); 
}
	break;
	case 8:
#line 34 "MARK2.rl"
	{ state->mark0[MARK2StA1] = p - state->doc[0]; }
	break;
	case 9:
#line 35 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2StA1];
    tok[1] = p;
    call(MARK2onStA1, tok, state); 
}
	break;
	case 10:
#line 40 "MARK2.rl"
	{ state->mark0[MARK2Root] = p - state->doc[0]; }
	break;
#line 339 "MARK2.rl.c"
		}
	}

_again:
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _MARK2_actions + _MARK2_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 1:
#line 11 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Ref0];
    tok[1] = p;
    call(MARK2onRef0, tok, state); 
}
	break;
	case 3:
#line 17 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Ref1];
    tok[1] = p;
    call(MARK2onRef1, tok, state); 
}
	break;
	case 5:
#line 23 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Em];
    tok[1] = p;
    call(MARK2onEm, tok, state); 
}
	break;
	case 7:
#line 29 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2StA0];
    tok[1] = p;
    call(MARK2onStA0, tok, state); 
}
	break;
	case 9:
#line 35 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2StA1];
    tok[1] = p;
    call(MARK2onStA1, tok, state); 
}
	break;
	case 10:
#line 40 "MARK2.rl"
	{ state->mark0[MARK2Root] = p - state->doc[0]; }
	break;
	case 11:
#line 41 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Root];
    tok[1] = p;
    call(MARK2onRoot, tok, state); 
}
	break;
#line 397 "MARK2.rl.c"
		}
	}
	}

	}

#line 101 "MARK2.rl"

    test(p==text[1], MARK2fail);

    if (state->tbc) {
        test(cs != MARK2_error, MARK2fail);
        state->cs = cs;
    } else {
        test(cs >= MARK2_first_final, MARK2fail);
    }

    nedo(
        state->text[0] = p;
    );
}
