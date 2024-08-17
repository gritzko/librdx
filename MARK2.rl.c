
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
	4, 8, 2, 3, 5, 1, 2, 3, 
	5, 1, 11, 3, 5, 6, 0, 3, 
	5, 7, 2, 3, 5, 7, 11, 3, 
	5, 8, 2, 3, 7, 6, 0, 3, 
	7, 8, 2, 3, 9, 1, 2, 3, 
	9, 1, 11, 3, 9, 6, 0, 3, 
	9, 8, 2, 3, 10, 6, 0, 3, 
	10, 8, 2, 4, 1, 4, 8, 2, 
	4, 3, 4, 8, 2, 4, 5, 1, 
	6, 0, 4, 5, 1, 8, 2, 4, 
	5, 4, 8, 2, 4, 5, 7, 6, 
	0, 4, 5, 7, 8, 2, 4, 7, 
	4, 8, 2, 4, 9, 1, 6, 0, 
	4, 9, 1, 8, 2, 4, 9, 4, 
	8, 2, 4, 10, 4, 8, 2, 5, 
	5, 1, 4, 8, 2, 5, 5, 7, 
	4, 8, 2, 5, 9, 1, 4, 8, 
	2
};

static const short _MARK2_key_offsets[] = {
	0, 6, 13, 20, 27, 34, 41, 48, 
	56, 69, 76, 84, 92, 100, 108, 116, 
	124, 132, 140, 148, 157, 171, 179, 188, 
	196, 204, 212, 220, 229, 237, 245, 253, 
	261, 269, 278, 286, 293, 301, 309, 316, 
	323, 330, 338
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
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 92u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 92u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 92u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 92u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 91u, 
	92u, 93u, 95u, 9u, 10u, 13u, 32u, 42u, 
	92u, 93u, 95u, 9u, 10u, 13u, 32u, 42u, 
	92u, 93u, 95u, 9u, 10u, 13u, 32u, 42u, 
	92u, 93u, 95u, 9u, 10u, 13u, 32u, 42u, 
	92u, 93u, 95u, 9u, 10u, 13u, 32u, 42u, 
	92u, 93u, 95u, 9u, 10u, 13u, 32u, 42u, 
	91u, 92u, 93u, 95u, 9u, 10u, 13u, 32u, 
	42u, 92u, 93u, 95u, 9u, 10u, 13u, 32u, 
	42u, 93u, 95u, 9u, 10u, 13u, 32u, 42u, 
	91u, 93u, 95u, 9u, 10u, 13u, 32u, 42u, 
	92u, 93u, 95u, 9u, 10u, 13u, 32u, 42u, 
	93u, 95u, 9u, 10u, 13u, 32u, 42u, 93u, 
	95u, 9u, 10u, 13u, 32u, 42u, 93u, 95u, 
	9u, 10u, 13u, 32u, 42u, 91u, 93u, 95u, 
	9u, 10u, 13u, 32u, 42u, 92u, 93u, 95u, 
	9u, 10u, 0
};

static const char _MARK2_single_lengths[] = {
	4, 5, 5, 5, 5, 5, 5, 6, 
	5, 5, 6, 6, 6, 6, 6, 6, 
	6, 6, 6, 7, 6, 6, 7, 6, 
	6, 6, 6, 7, 6, 6, 6, 6, 
	6, 7, 6, 5, 6, 6, 5, 5, 
	5, 6, 6
};

static const char _MARK2_range_lengths[] = {
	1, 1, 1, 1, 1, 1, 1, 1, 
	4, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 4, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1
};

static const short _MARK2_index_offsets[] = {
	0, 6, 13, 20, 27, 34, 41, 48, 
	56, 66, 73, 81, 89, 97, 105, 113, 
	121, 129, 137, 145, 154, 165, 173, 182, 
	190, 198, 206, 214, 223, 231, 239, 247, 
	255, 263, 272, 280, 287, 295, 303, 310, 
	317, 324, 332
};

static const unsigned char _MARK2_indicies[] = {
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
	29, 5, 5, 36, 37, 38, 8, 5, 
	35, 39, 39, 36, 37, 38, 40, 39, 
	35, 39, 39, 41, 42, 37, 8, 39, 
	35, 39, 39, 44, 45, 46, 47, 39, 
	43, 49, 49, 50, 51, 52, 53, 49, 
	48, 55, 55, 56, 57, 58, 59, 55, 
	54, 39, 39, 60, 37, 38, 40, 39, 
	35, 39, 39, 36, 37, 38, 61, 39, 
	35, 39, 39, 36, 62, 37, 38, 40, 
	39, 35, 39, 39, 36, 37, 38, 40, 
	39, 63, 63, 63, 35, 39, 39, 36, 
	37, 64, 40, 39, 35, 66, 66, 67, 
	68, 69, 70, 71, 66, 65, 73, 73, 
	74, 75, 76, 77, 73, 72, 78, 78, 
	74, 75, 76, 79, 78, 72, 49, 49, 
	80, 51, 52, 53, 49, 48, 49, 49, 
	50, 51, 52, 81, 49, 48, 49, 49, 
	50, 82, 51, 52, 53, 49, 48, 84, 
	84, 85, 86, 87, 88, 84, 83, 39, 
	39, 90, 91, 92, 93, 39, 89, 95, 
	95, 96, 97, 98, 99, 95, 94, 101, 
	101, 102, 103, 104, 105, 101, 100, 95, 
	95, 96, 97, 98, 106, 95, 94, 95, 
	95, 96, 107, 97, 98, 99, 95, 94, 
	109, 109, 110, 111, 112, 113, 109, 108, 
	16, 16, 114, 18, 19, 16, 15, 16, 
	16, 17, 115, 18, 19, 16, 15, 16, 
	16, 50, 51, 52, 19, 16, 48, 5, 
	5, 117, 118, 119, 5, 116, 121, 121, 
	122, 123, 124, 121, 120, 126, 126, 127, 
	128, 129, 126, 125, 121, 121, 122, 130, 
	123, 124, 121, 120, 121, 121, 96, 97, 
	98, 124, 121, 94, 0
};

static const char _MARK2_trans_targs[] = {
	1, 2, 6, 11, 1, 2, 5, 7, 
	11, 3, 38, 4, 35, 36, 37, 1, 
	2, 5, 7, 11, 1, 2, 6, 7, 
	11, 6, 8, 9, 10, 1, 2, 5, 
	8, 7, 11, 12, 16, 18, 19, 13, 
	23, 14, 29, 15, 25, 26, 27, 28, 
	12, 13, 16, 18, 19, 23, 12, 13, 
	17, 18, 19, 23, 17, 24, 20, 21, 
	22, 12, 13, 16, 20, 18, 19, 23, 
	12, 2, 16, 18, 19, 11, 13, 23, 
	17, 24, 20, 12, 2, 16, 18, 19, 
	11, 30, 31, 32, 33, 34, 12, 13, 
	16, 18, 19, 23, 12, 13, 17, 18, 
	19, 23, 24, 20, 12, 2, 16, 18, 
	19, 11, 6, 8, 39, 40, 41, 42, 
	1, 2, 5, 7, 11, 1, 2, 6, 
	7, 11, 8
};

static const unsigned char _MARK2_trans_actions[] = {
	119, 115, 41, 178, 32, 23, 1, 32, 
	63, 1, 32, 32, 1, 32, 63, 95, 
	91, 26, 95, 158, 111, 107, 35, 111, 
	173, 1, 32, 32, 32, 59, 55, 11, 
	59, 59, 128, 32, 1, 32, 32, 23, 
	63, 1, 32, 32, 1, 32, 32, 63, 
	95, 91, 26, 95, 95, 158, 111, 107, 
	35, 111, 111, 173, 1, 63, 32, 32, 
	32, 59, 55, 11, 59, 59, 59, 128, 
	87, 75, 17, 87, 87, 143, 75, 143, 
	26, 158, 95, 153, 148, 79, 153, 153, 
	189, 32, 1, 32, 32, 63, 51, 47, 
	5, 51, 51, 123, 168, 163, 99, 168, 
	168, 195, 123, 51, 138, 133, 67, 138, 
	138, 183, 26, 95, 32, 1, 32, 63, 
	51, 47, 5, 51, 123, 168, 163, 99, 
	168, 195, 51
};

static const unsigned char _MARK2_eof_actions[] = {
	44, 3, 3, 3, 29, 38, 3, 3, 
	3, 3, 14, 3, 3, 3, 3, 29, 
	38, 3, 3, 3, 3, 3, 14, 20, 
	20, 29, 29, 29, 83, 3, 8, 103, 
	8, 8, 71, 29, 29, 29, 3, 8, 
	103, 8, 8
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

    
#line 235 "MARK2.rl.c"
	{
	cs = MARK2_start;
	}

#line 100 "MARK2.rl"
    
#line 238 "MARK2.rl.c"
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
#line 362 "MARK2.rl.c"
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
#line 420 "MARK2.rl.c"
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
