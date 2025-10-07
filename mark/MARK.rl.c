
#line 1 "MARK.rl"
#include "MARK.rl.h"



#line 156 "MARK.rl"



#line 7 "MARK.rl.c"
static const char _MARK_actions[] = {
	0, 2, 1, 23, 2, 3, 23, 2, 
	5, 23, 2, 7, 23, 2, 9, 23, 
	2, 11, 23, 2, 13, 23, 2, 15, 
	23, 2, 17, 23, 2, 19, 23, 2, 
	21, 23, 2, 25, 27, 2, 26, 27, 
	3, 3, 4, 23, 3, 3, 16, 23, 
	3, 3, 18, 23, 3, 3, 20, 23, 
	3, 17, 4, 23, 3, 17, 16, 23, 
	3, 17, 18, 23, 3, 17, 20, 23, 
	4, 3, 0, 6, 23, 4, 17, 0, 
	6, 23, 4, 25, 24, 22, 23, 4, 
	26, 24, 22, 23, 5, 25, 24, 22, 
	4, 23, 5, 25, 24, 22, 16, 23, 
	5, 25, 24, 22, 18, 23, 5, 25, 
	24, 22, 20, 23, 5, 26, 24, 22, 
	4, 23, 5, 26, 24, 22, 16, 23, 
	5, 26, 24, 22, 18, 23, 5, 26, 
	24, 22, 20, 23, 6, 3, 8, 10, 
	12, 14, 23, 6, 17, 8, 10, 12, 
	14, 23, 6, 25, 24, 22, 0, 6, 
	23, 6, 26, 24, 22, 0, 6, 23, 
	8, 25, 24, 22, 8, 10, 12, 14, 
	23, 8, 26, 24, 22, 8, 10, 12, 
	14, 23, 9, 3, 2, 16, 8, 10, 
	12, 4, 6, 23, 9, 17, 2, 16, 
	8, 10, 12, 4, 6, 23, 11, 25, 
	24, 22, 2, 16, 8, 10, 12, 4, 
	6, 23, 11, 26, 24, 22, 2, 16, 
	8, 10, 12, 4, 6, 23
};

static const unsigned char _MARK_key_offsets[] = {
	0, 1, 8, 15, 20, 29, 32, 34, 
	36, 37, 40, 42, 43, 46, 47, 48, 
	51, 53, 55, 56, 58, 60, 61, 65, 
	67, 69, 70, 74, 76, 78, 80, 82, 
	91, 98, 100, 102, 103, 105, 107, 110, 
	111, 114, 117, 120, 129
};

static const unsigned char _MARK_trans_keys[] = {
	10u, 10u, 32u, 35u, 45u, 62u, 48u, 57u, 
	10u, 32u, 35u, 45u, 62u, 48u, 57u, 10u, 
	32u, 35u, 45u, 62u, 10u, 32u, 35u, 45u, 
	62u, 91u, 96u, 48u, 57u, 10u, 32u, 35u, 
	10u, 32u, 10u, 32u, 10u, 10u, 32u, 35u, 
	10u, 32u, 10u, 10u, 32u, 35u, 10u, 10u, 
	10u, 32u, 45u, 10u, 32u, 10u, 32u, 10u, 
	10u, 45u, 10u, 45u, 10u, 10u, 46u, 48u, 
	57u, 10u, 32u, 10u, 32u, 10u, 10u, 46u, 
	48u, 57u, 10u, 46u, 10u, 32u, 10u, 32u, 
	10u, 32u, 10u, 32u, 35u, 45u, 62u, 91u, 
	96u, 48u, 57u, 10u, 48u, 57u, 65u, 90u, 
	97u, 122u, 10u, 93u, 10u, 58u, 10u, 10u, 
	96u, 10u, 96u, 10u, 32u, 96u, 10u, 10u, 
	32u, 35u, 10u, 32u, 35u, 10u, 32u, 35u, 
	10u, 32u, 35u, 45u, 62u, 91u, 96u, 48u, 
	57u, 10u, 32u, 35u, 45u, 62u, 91u, 96u, 
	48u, 57u, 0
};

static const char _MARK_single_lengths[] = {
	1, 5, 5, 5, 7, 3, 2, 2, 
	1, 3, 2, 1, 3, 1, 1, 3, 
	2, 2, 1, 2, 2, 1, 2, 2, 
	2, 1, 2, 2, 2, 2, 2, 7, 
	1, 2, 2, 1, 2, 2, 3, 1, 
	3, 3, 3, 7, 7
};

static const char _MARK_range_lengths[] = {
	0, 1, 1, 0, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 1, 0, 
	0, 0, 1, 0, 0, 0, 0, 1, 
	3, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 1, 1
};

static const unsigned char _MARK_index_offsets[] = {
	0, 2, 9, 16, 22, 31, 35, 38, 
	41, 43, 47, 50, 52, 56, 58, 60, 
	64, 67, 70, 72, 75, 78, 80, 84, 
	87, 90, 92, 96, 99, 102, 105, 108, 
	117, 122, 125, 128, 130, 133, 136, 140, 
	142, 146, 150, 154, 163
};

static const char _MARK_trans_targs[] = {
	44, 0, 44, 2, 41, 16, 29, 26, 
	0, 44, 3, 40, 17, 30, 27, 0, 
	44, 4, 8, 18, 31, 0, 44, 1, 
	5, 15, 28, 32, 36, 22, 0, 44, 
	6, 9, 0, 44, 7, 0, 44, 8, 
	0, 44, 0, 44, 10, 12, 0, 44, 
	11, 0, 44, 0, 44, 13, 14, 0, 
	44, 0, 44, 0, 44, 16, 19, 0, 
	44, 17, 0, 44, 18, 0, 44, 0, 
	44, 20, 0, 44, 21, 0, 44, 0, 
	44, 23, 26, 0, 44, 24, 0, 44, 
	25, 0, 44, 0, 44, 24, 27, 0, 
	44, 25, 0, 44, 29, 0, 44, 30, 
	0, 44, 31, 0, 44, 1, 5, 15, 
	28, 32, 36, 22, 0, 44, 33, 33, 
	33, 0, 44, 34, 0, 44, 35, 0, 
	44, 0, 44, 37, 0, 44, 38, 0, 
	44, 39, 39, 0, 44, 0, 44, 8, 
	11, 0, 44, 7, 42, 0, 44, 11, 
	13, 0, 44, 1, 5, 15, 28, 32, 
	36, 22, 0, 44, 1, 5, 15, 28, 
	32, 36, 22, 0, 0
};

static const unsigned char _MARK_trans_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 4, 186, 
	140, 72, 44, 52, 48, 40, 4, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 13, 13, 0, 0, 0, 0, 0, 
	0, 0, 16, 16, 0, 0, 0, 0, 
	19, 19, 22, 22, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 10, 10, 
	0, 0, 0, 0, 0, 0, 1, 1, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 7, 7, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 25, 196, 147, 77, 
	60, 68, 64, 56, 25, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	31, 31, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 28, 28, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 87, 218, 177, 161, 122, 134, 
	128, 116, 87, 82, 206, 168, 154, 98, 
	110, 104, 92, 82, 0
};

static const unsigned char _MARK_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 37, 34
};

static const int MARK_start = 43;
static const int MARK_first_final = 43;
static const int MARK_error = -1;

static const int MARK_en_main = 43;


#line 159 "MARK.rl"

pro(MARKlexer, MARKstate* state) {

    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = 0;
    int res = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = pe;
    u8c *pb = p;
    u64 mark0[64] = {};

    u32 sp = 2;
    u8cs tok = {p, p};

    
#line 178 "MARK.rl.c"
	{
	cs = MARK_start;
	}

#line 177 "MARK.rl"
    
#line 181 "MARK.rl.c"
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
	{ mark0[MARKHLine] = p - text[0]; }
	break;
	case 1:
#line 11 "MARK.rl"
	{
    tok[0] = text[0] + mark0[MARKHLine];
    tok[1] = p;
    call(MARKonHLine, tok, state); 
}
	break;
	case 2:
#line 16 "MARK.rl"
	{ mark0[MARKIndent] = p - text[0]; }
	break;
	case 3:
#line 17 "MARK.rl"
	{
    tok[0] = text[0] + mark0[MARKIndent];
    tok[1] = p;
    call(MARKonIndent, tok, state); 
}
	break;
	case 4:
#line 22 "MARK.rl"
	{ mark0[MARKOList] = p - text[0]; }
	break;
	case 5:
#line 23 "MARK.rl"
	{
    tok[0] = text[0] + mark0[MARKOList];
    tok[1] = p;
    call(MARKonOList, tok, state); 
}
	break;
	case 6:
#line 28 "MARK.rl"
	{ mark0[MARKUList] = p - text[0]; }
	break;
	case 7:
#line 29 "MARK.rl"
	{
    tok[0] = text[0] + mark0[MARKUList];
    tok[1] = p;
    call(MARKonUList, tok, state); 
}
	break;
	case 8:
#line 34 "MARK.rl"
	{ mark0[MARKH1] = p - text[0]; }
	break;
	case 9:
#line 35 "MARK.rl"
	{
    tok[0] = text[0] + mark0[MARKH1];
    tok[1] = p;
    call(MARKonH1, tok, state); 
}
	break;
	case 10:
#line 40 "MARK.rl"
	{ mark0[MARKH2] = p - text[0]; }
	break;
	case 11:
#line 41 "MARK.rl"
	{
    tok[0] = text[0] + mark0[MARKH2];
    tok[1] = p;
    call(MARKonH2, tok, state); 
}
	break;
	case 12:
#line 46 "MARK.rl"
	{ mark0[MARKH3] = p - text[0]; }
	break;
	case 13:
#line 47 "MARK.rl"
	{
    tok[0] = text[0] + mark0[MARKH3];
    tok[1] = p;
    call(MARKonH3, tok, state); 
}
	break;
	case 14:
#line 52 "MARK.rl"
	{ mark0[MARKH4] = p - text[0]; }
	break;
	case 15:
#line 53 "MARK.rl"
	{
    tok[0] = text[0] + mark0[MARKH4];
    tok[1] = p;
    call(MARKonH4, tok, state); 
}
	break;
	case 16:
#line 64 "MARK.rl"
	{ mark0[MARKQuote] = p - text[0]; }
	break;
	case 17:
#line 65 "MARK.rl"
	{
    tok[0] = text[0] + mark0[MARKQuote];
    tok[1] = p;
    call(MARKonQuote, tok, state); 
}
	break;
	case 18:
#line 70 "MARK.rl"
	{ mark0[MARKCode] = p - text[0]; }
	break;
	case 19:
#line 71 "MARK.rl"
	{
    tok[0] = text[0] + mark0[MARKCode];
    tok[1] = p;
    call(MARKonCode, tok, state); 
}
	break;
	case 20:
#line 76 "MARK.rl"
	{ mark0[MARKLink] = p - text[0]; }
	break;
	case 21:
#line 77 "MARK.rl"
	{
    tok[0] = text[0] + mark0[MARKLink];
    tok[1] = p;
    call(MARKonLink, tok, state); 
}
	break;
	case 22:
#line 82 "MARK.rl"
	{ mark0[MARKDiv] = p - text[0]; }
	break;
	case 23:
#line 83 "MARK.rl"
	{
    tok[0] = text[0] + mark0[MARKDiv];
    tok[1] = p;
    call(MARKonDiv, tok, state); 
}
	break;
	case 24:
#line 88 "MARK.rl"
	{ mark0[MARKLine] = p - text[0]; }
	break;
	case 25:
#line 89 "MARK.rl"
	{
    tok[0] = text[0] + mark0[MARKLine];
    tok[1] = p;
    call(MARKonLine, tok, state); 
}
	break;
	case 26:
#line 94 "MARK.rl"
	{ mark0[MARKRoot] = p - text[0]; }
	break;
#line 384 "MARK.rl.c"
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
	case 25:
#line 89 "MARK.rl"
	{
    tok[0] = text[0] + mark0[MARKLine];
    tok[1] = p;
    call(MARKonLine, tok, state); 
}
	break;
	case 26:
#line 94 "MARK.rl"
	{ mark0[MARKRoot] = p - text[0]; }
	break;
	case 27:
#line 95 "MARK.rl"
	{
    tok[0] = text[0] + mark0[MARKRoot];
    tok[1] = p;
    call(MARKonRoot, tok, state); 
}
	break;
#line 414 "MARK.rl.c"
		}
	}
	}

	}

#line 178 "MARK.rl"

    if (p!=text[1] || cs < MARK_first_final) {
        fail(MARKfail);
        state->text[0] = p;
    }
    done;
}
