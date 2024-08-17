
#line 1 "MARK.rl"
#include "MARK.rl.h"



#line 138 "MARK.rl"



#line 7 "MARK.rl.c"
static const char _MARK_actions[] = {
	0, 2, 1, 19, 2, 3, 4, 2, 
	3, 16, 2, 3, 19, 2, 5, 4, 
	2, 5, 16, 2, 5, 19, 2, 7, 
	4, 2, 7, 16, 2, 7, 19, 2, 
	9, 19, 2, 11, 19, 2, 13, 19, 
	2, 15, 19, 2, 17, 19, 2, 21, 
	23, 2, 22, 23, 3, 3, 6, 0, 
	3, 5, 6, 0, 3, 7, 6, 0, 
	4, 21, 20, 18, 4, 4, 21, 20, 
	18, 16, 4, 21, 20, 18, 19, 4, 
	22, 20, 18, 4, 4, 22, 20, 18, 
	16, 4, 22, 20, 18, 19, 5, 3, 
	8, 10, 12, 14, 5, 5, 8, 10, 
	12, 14, 5, 7, 8, 10, 12, 14, 
	5, 21, 20, 18, 6, 0, 5, 22, 
	20, 18, 6, 0, 7, 3, 2, 4, 
	6, 8, 10, 12, 7, 5, 2, 4, 
	6, 8, 10, 12, 7, 7, 2, 4, 
	6, 8, 10, 12, 7, 21, 20, 18, 
	8, 10, 12, 14, 7, 22, 20, 18, 
	8, 10, 12, 14, 9, 21, 20, 18, 
	2, 4, 6, 8, 10, 12, 9, 22, 
	20, 18, 2, 4, 6, 8, 10, 12
	
};

static const char _MARK_key_offsets[] = {
	0, 0, 1, 6, 11, 14, 21, 23, 
	24, 25, 26, 28, 29, 30, 32, 33, 
	34, 36, 37, 38, 45, 48, 49, 50, 
	57, 63, 64, 65, 66, 69, 70, 71, 
	72, 73, 75, 77, 79, 86
};

static const unsigned char _MARK_trans_keys[] = {
	10u, 32u, 35u, 45u, 48u, 57u, 32u, 35u, 
	45u, 48u, 57u, 32u, 35u, 45u, 10u, 32u, 
	35u, 45u, 91u, 48u, 57u, 32u, 35u, 32u, 
	32u, 10u, 32u, 35u, 32u, 10u, 32u, 35u, 
	10u, 10u, 32u, 45u, 32u, 32u, 10u, 32u, 
	35u, 45u, 91u, 48u, 57u, 46u, 48u, 57u, 
	32u, 32u, 10u, 32u, 35u, 45u, 91u, 48u, 
	57u, 48u, 57u, 65u, 90u, 97u, 122u, 93u, 
	58u, 10u, 46u, 48u, 57u, 46u, 45u, 45u, 
	10u, 32u, 35u, 32u, 35u, 32u, 35u, 10u, 
	32u, 35u, 45u, 91u, 48u, 57u, 10u, 32u, 
	35u, 45u, 91u, 48u, 57u, 0
};

static const char _MARK_single_lengths[] = {
	0, 1, 3, 3, 3, 5, 2, 1, 
	1, 1, 2, 1, 1, 2, 1, 1, 
	2, 1, 1, 5, 1, 1, 1, 5, 
	0, 1, 1, 1, 1, 1, 1, 1, 
	1, 2, 2, 2, 5, 5
};

static const char _MARK_range_lengths[] = {
	0, 0, 1, 1, 0, 1, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 1, 1, 0, 0, 1, 
	3, 0, 0, 0, 1, 0, 0, 0, 
	0, 0, 0, 0, 1, 1
};

static const unsigned char _MARK_index_offsets[] = {
	0, 0, 2, 7, 12, 16, 23, 26, 
	28, 30, 32, 35, 37, 39, 42, 44, 
	46, 49, 51, 53, 60, 63, 65, 67, 
	74, 78, 80, 82, 84, 87, 89, 91, 
	93, 95, 98, 101, 104, 111
};

static const char _MARK_trans_targs[] = {
	37, 1, 3, 34, 17, 28, 0, 4, 
	33, 18, 29, 0, 5, 9, 19, 0, 
	37, 2, 6, 16, 24, 20, 1, 7, 
	10, 0, 8, 0, 9, 0, 37, 1, 
	11, 13, 0, 12, 0, 37, 1, 14, 
	15, 0, 37, 1, 37, 1, 17, 30, 
	0, 18, 0, 19, 0, 37, 2, 6, 
	16, 24, 20, 1, 21, 28, 0, 22, 
	0, 23, 0, 37, 2, 6, 16, 24, 
	20, 1, 25, 25, 25, 0, 26, 0, 
	27, 0, 37, 1, 22, 29, 0, 23, 
	0, 31, 0, 32, 0, 37, 1, 9, 
	12, 0, 8, 35, 0, 12, 14, 0, 
	37, 2, 6, 16, 24, 20, 1, 37, 
	2, 6, 16, 24, 20, 1, 0
};

static const unsigned char _MARK_trans_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	10, 124, 94, 52, 7, 4, 10, 0, 
	0, 0, 0, 0, 0, 0, 31, 31, 
	0, 0, 0, 0, 0, 34, 34, 0, 
	0, 0, 37, 37, 40, 40, 0, 0, 
	0, 0, 0, 0, 0, 28, 140, 106, 
	60, 25, 22, 28, 0, 0, 0, 0, 
	0, 0, 0, 19, 132, 100, 56, 16, 
	13, 19, 0, 0, 0, 0, 0, 0, 
	0, 0, 43, 43, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 1, 1, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	89, 174, 156, 118, 84, 79, 89, 74, 
	164, 148, 112, 69, 64, 74, 0
};

static const unsigned char _MARK_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 49, 46
};

static const int MARK_start = 36;
static const int MARK_first_final = 36;
static const int MARK_error = 0;

static const int MARK_en_main = 36;


#line 141 "MARK.rl"

pro(MARKlexer, MARKstate* state) {
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

    
#line 146 "MARK.rl.c"
	{
	cs = MARK_start;
	}

#line 157 "MARK.rl"
    
#line 149 "MARK.rl.c"
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
	_keys = _MARK_trans_keys + _MARK_key_offsets[cs];
	_trans = _MARK_index_offsets[cs];

	_klen = _MARK_single_lengths[cs];
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

	_klen = _MARK_range_lengths[cs];
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
	cs = _MARK_trans_targs[_trans];

	if ( _MARK_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _MARK_actions + _MARK_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 10 "MARK.rl"
	{ state->mark0[MARKHLine] = p - state->doc[0]; }
	break;
	case 1:
#line 11 "MARK.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARKHLine];
    tok[1] = p;
    call(MARKonHLine, tok, state); 
}
	break;
	case 2:
#line 16 "MARK.rl"
	{ state->mark0[MARKIndent] = p - state->doc[0]; }
	break;
	case 3:
#line 17 "MARK.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARKIndent];
    tok[1] = p;
    call(MARKonIndent, tok, state); 
}
	break;
	case 4:
#line 22 "MARK.rl"
	{ state->mark0[MARKOList] = p - state->doc[0]; }
	break;
	case 5:
#line 23 "MARK.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARKOList];
    tok[1] = p;
    call(MARKonOList, tok, state); 
}
	break;
	case 6:
#line 28 "MARK.rl"
	{ state->mark0[MARKUList] = p - state->doc[0]; }
	break;
	case 7:
#line 29 "MARK.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARKUList];
    tok[1] = p;
    call(MARKonUList, tok, state); 
}
	break;
	case 8:
#line 34 "MARK.rl"
	{ state->mark0[MARKH1] = p - state->doc[0]; }
	break;
	case 9:
#line 35 "MARK.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARKH1];
    tok[1] = p;
    call(MARKonH1, tok, state); 
}
	break;
	case 10:
#line 40 "MARK.rl"
	{ state->mark0[MARKH2] = p - state->doc[0]; }
	break;
	case 11:
#line 41 "MARK.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARKH2];
    tok[1] = p;
    call(MARKonH2, tok, state); 
}
	break;
	case 12:
#line 46 "MARK.rl"
	{ state->mark0[MARKH3] = p - state->doc[0]; }
	break;
	case 13:
#line 47 "MARK.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARKH3];
    tok[1] = p;
    call(MARKonH3, tok, state); 
}
	break;
	case 14:
#line 52 "MARK.rl"
	{ state->mark0[MARKH4] = p - state->doc[0]; }
	break;
	case 15:
#line 53 "MARK.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARKH4];
    tok[1] = p;
    call(MARKonH4, tok, state); 
}
	break;
	case 16:
#line 64 "MARK.rl"
	{ state->mark0[MARKLink] = p - state->doc[0]; }
	break;
	case 17:
#line 65 "MARK.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARKLink];
    tok[1] = p;
    call(MARKonLink, tok, state); 
}
	break;
	case 18:
#line 70 "MARK.rl"
	{ state->mark0[MARKDiv] = p - state->doc[0]; }
	break;
	case 19:
#line 71 "MARK.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARKDiv];
    tok[1] = p;
    call(MARKonDiv, tok, state); 
}
	break;
	case 20:
#line 76 "MARK.rl"
	{ state->mark0[MARKLine] = p - state->doc[0]; }
	break;
	case 21:
#line 77 "MARK.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARKLine];
    tok[1] = p;
    call(MARKonLine, tok, state); 
}
	break;
	case 22:
#line 82 "MARK.rl"
	{ state->mark0[MARKRoot] = p - state->doc[0]; }
	break;
#line 334 "MARK.rl.c"
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
	const char *__acts = _MARK_actions + _MARK_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 21:
#line 77 "MARK.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARKLine];
    tok[1] = p;
    call(MARKonLine, tok, state); 
}
	break;
	case 22:
#line 82 "MARK.rl"
	{ state->mark0[MARKRoot] = p - state->doc[0]; }
	break;
	case 23:
#line 83 "MARK.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARKRoot];
    tok[1] = p;
    call(MARKonRoot, tok, state); 
}
	break;
#line 366 "MARK.rl.c"
		}
	}
	}

	_out: {}
	}

#line 158 "MARK.rl"

    test(p==text[1], MARKfail);

    if (state->tbc) {
        test(cs != MARK_error, MARKfail);
        state->cs = cs;
    } else {
        test(cs >= MARK_first_final, MARKfail);
    }

    nedo(
        state->text[0] = p;
    );
}
