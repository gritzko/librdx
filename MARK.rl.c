
#line 1 "MARK.rl"
#include "MARK.rl.h"



#line 138 "MARK.rl"



#line 7 "MARK.rl.c"
static const char _MARK_actions[] = {
	0, 2, 1, 19, 2, 3, 19, 2, 
	5, 19, 2, 7, 19, 2, 9, 19, 
	2, 11, 19, 2, 13, 19, 2, 15, 
	19, 2, 17, 19, 2, 21, 23, 2, 
	22, 23, 3, 3, 4, 19, 3, 3, 
	16, 19, 3, 5, 4, 19, 3, 5, 
	16, 19, 3, 7, 4, 19, 3, 7, 
	16, 19, 4, 3, 6, 0, 19, 4, 
	5, 6, 0, 19, 4, 7, 6, 0, 
	19, 4, 21, 20, 18, 19, 4, 22, 
	20, 18, 19, 5, 21, 20, 18, 4, 
	19, 5, 21, 20, 18, 16, 19, 5, 
	22, 20, 18, 4, 19, 5, 22, 20, 
	18, 16, 19, 6, 3, 8, 10, 12, 
	14, 19, 6, 5, 8, 10, 12, 14, 
	19, 6, 7, 8, 10, 12, 14, 19, 
	6, 21, 20, 18, 6, 0, 19, 6, 
	22, 20, 18, 6, 0, 19, 8, 3, 
	2, 4, 6, 8, 10, 12, 19, 8, 
	5, 2, 4, 6, 8, 10, 12, 19, 
	8, 7, 2, 4, 6, 8, 10, 12, 
	19, 8, 21, 20, 18, 8, 10, 12, 
	14, 19, 8, 22, 20, 18, 8, 10, 
	12, 14, 19, 10, 21, 20, 18, 2, 
	4, 6, 8, 10, 12, 19, 10, 22, 
	20, 18, 2, 4, 6, 8, 10, 12, 
	19
};

static const char _MARK_key_offsets[] = {
	0, 1, 7, 13, 17, 24, 27, 29, 
	31, 32, 35, 37, 38, 41, 42, 43, 
	46, 48, 50, 57, 61, 63, 65, 72, 
	79, 81, 83, 84, 88, 90, 92, 94, 
	95, 98, 101, 104, 111
};

static const unsigned char _MARK_trans_keys[] = {
	10u, 10u, 32u, 35u, 45u, 48u, 57u, 10u, 
	32u, 35u, 45u, 48u, 57u, 10u, 32u, 35u, 
	45u, 10u, 32u, 35u, 45u, 91u, 48u, 57u, 
	10u, 32u, 35u, 10u, 32u, 10u, 32u, 10u, 
	10u, 32u, 35u, 10u, 32u, 10u, 10u, 32u, 
	35u, 10u, 10u, 10u, 32u, 45u, 10u, 32u, 
	10u, 32u, 10u, 32u, 35u, 45u, 91u, 48u, 
	57u, 10u, 46u, 48u, 57u, 10u, 32u, 10u, 
	32u, 10u, 32u, 35u, 45u, 91u, 48u, 57u, 
	10u, 48u, 57u, 65u, 90u, 97u, 122u, 10u, 
	93u, 10u, 58u, 10u, 10u, 46u, 48u, 57u, 
	10u, 46u, 10u, 45u, 10u, 45u, 10u, 10u, 
	32u, 35u, 10u, 32u, 35u, 10u, 32u, 35u, 
	10u, 32u, 35u, 45u, 91u, 48u, 57u, 10u, 
	32u, 35u, 45u, 91u, 48u, 57u, 0
};

static const char _MARK_single_lengths[] = {
	1, 4, 4, 4, 5, 3, 2, 2, 
	1, 3, 2, 1, 3, 1, 1, 3, 
	2, 2, 5, 2, 2, 2, 5, 1, 
	2, 2, 1, 2, 2, 2, 2, 1, 
	3, 3, 3, 5, 5
};

static const char _MARK_range_lengths[] = {
	0, 1, 1, 0, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 1, 1, 0, 0, 1, 3, 
	0, 0, 0, 1, 0, 0, 0, 0, 
	0, 0, 0, 1, 1
};

static const unsigned char _MARK_index_offsets[] = {
	0, 2, 8, 14, 19, 26, 30, 33, 
	36, 38, 42, 45, 47, 51, 53, 55, 
	59, 62, 65, 72, 76, 79, 82, 89, 
	94, 97, 100, 102, 106, 109, 112, 115, 
	117, 121, 125, 129, 136
};

static const char _MARK_trans_targs[] = {
	36, 0, 36, 2, 33, 16, 27, 0, 
	36, 3, 32, 17, 28, 0, 36, 4, 
	8, 18, 0, 36, 1, 5, 15, 23, 
	19, 0, 36, 6, 9, 0, 36, 7, 
	0, 36, 8, 0, 36, 0, 36, 10, 
	12, 0, 36, 11, 0, 36, 0, 36, 
	13, 14, 0, 36, 0, 36, 0, 36, 
	16, 29, 0, 36, 17, 0, 36, 18, 
	0, 36, 1, 5, 15, 23, 19, 0, 
	36, 20, 27, 0, 36, 21, 0, 36, 
	22, 0, 36, 1, 5, 15, 23, 19, 
	0, 36, 24, 24, 24, 0, 36, 25, 
	0, 36, 26, 0, 36, 0, 36, 21, 
	28, 0, 36, 22, 0, 36, 30, 0, 
	36, 31, 0, 36, 0, 36, 8, 11, 
	0, 36, 7, 34, 0, 36, 11, 13, 
	0, 36, 1, 5, 15, 23, 19, 0, 
	36, 1, 5, 15, 23, 19, 0, 0
};

static const unsigned char _MARK_trans_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 4, 142, 107, 58, 38, 
	34, 4, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 13, 13, 0, 0, 
	0, 0, 0, 0, 0, 16, 16, 0, 
	0, 0, 0, 19, 19, 22, 22, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 10, 160, 121, 68, 54, 50, 10, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 7, 151, 114, 63, 46, 42, 
	7, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 25, 25, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 1, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 78, 198, 178, 135, 101, 95, 78, 
	73, 187, 169, 128, 89, 83, 73, 0
};

static const unsigned char _MARK_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 31, 28
};

static const int MARK_start = 35;
static const int MARK_first_final = 35;
static const int MARK_error = -1;

static const int MARK_en_main = 35;


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

    
#line 158 "MARK.rl.c"
	{
	cs = MARK_start;
	}

#line 157 "MARK.rl"
    
#line 161 "MARK.rl.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const unsigned char *_keys;

	if ( p == pe )
		goto _test_eof;
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
#line 344 "MARK.rl.c"
		}
	}

_again:
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
#line 374 "MARK.rl.c"
		}
	}
	}

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
