
#line 1 "MARK.rl"
#include "PRO.h"
#include "MARK.h"

enum {
	MARK = 0,
	MARKhline = MARK+2,
	MARKindent = MARK+3,
	MARKolist = MARK+4,
	MARKulist = MARK+5,
	MARKh1 = MARK+6,
	MARKh2 = MARK+7,
	MARKh3 = MARK+8,
	MARKh4 = MARK+9,
	MARKh = MARK+10,
	MARKlndx = MARK+11,
	MARKlink = MARK+12,
	MARKnest = MARK+13,
	MARKterm = MARK+14,
	MARKdiv = MARK+15,
	MARKline = MARK+16,
	MARKroot = MARK+17,
};

#define MARKmaxnest 1024

fun ok64 popfails(u32* stack, u32* sp, u32 type) {
    while (*sp && stack[*sp]!=type) *sp -= 2;
    return *sp ? OK : MARKfail;
}

#define lexpush(t) { \
    if (sp>=MARKmaxnest) fail(MARKfail); \
    stack[++sp] = p - pb; \
    stack[++sp] = t; \
}
#define lexpop(t)  \
    if (stack[sp]!=t) call(popfails, stack, &sp, t); \
    tok[0] = *(text)+stack[sp-1]; \
    tok[1] = p; \
    sp -= 2;

ok64 _MARKhline ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKindent ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKolist ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKulist ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKh1 ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKh2 ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKh3 ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKh4 ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKh ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKlndx ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKlink ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKnest ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKterm ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKdiv ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKline ($cu8c text, $cu8c tok, MARKstate* state);
ok64 _MARKroot ($cu8c text, $cu8c tok, MARKstate* state);



#line 148 "MARK.rl"



#line 63 "MARK.rl.c"
static const char _MARK_actions[] = {
	0, 1, 16, 1, 17, 2, 27, 29, 
	2, 28, 29, 3, 1, 23, 25, 3, 
	3, 21, 25, 3, 5, 21, 25, 3, 
	7, 21, 25, 3, 9, 23, 25, 3, 
	11, 23, 25, 3, 13, 23, 25, 3, 
	15, 23, 25, 3, 19, 23, 25, 4, 
	3, 21, 20, 4, 4, 3, 21, 22, 
	18, 4, 5, 21, 20, 4, 4, 5, 
	21, 22, 18, 4, 7, 21, 20, 4, 
	4, 7, 21, 22, 18, 4, 27, 26, 
	24, 25, 4, 28, 26, 24, 25, 5, 
	27, 26, 24, 20, 4, 5, 27, 26, 
	24, 22, 18, 5, 28, 26, 24, 20, 
	4, 5, 28, 26, 24, 22, 18, 6, 
	3, 21, 20, 6, 22, 0, 6, 5, 
	21, 20, 6, 22, 0, 6, 7, 21, 
	20, 6, 22, 0, 7, 3, 21, 22, 
	8, 10, 12, 14, 7, 5, 21, 22, 
	8, 10, 12, 14, 7, 7, 21, 22, 
	8, 10, 12, 14, 7, 27, 26, 24, 
	20, 6, 22, 0, 7, 28, 26, 24, 
	20, 6, 22, 0, 8, 27, 26, 24, 
	22, 8, 10, 12, 14, 8, 28, 26, 
	24, 22, 8, 10, 12, 14, 10, 3, 
	21, 20, 2, 4, 6, 22, 8, 10, 
	12, 10, 5, 21, 20, 2, 4, 6, 
	22, 8, 10, 12, 10, 7, 21, 20, 
	2, 4, 6, 22, 8, 10, 12, 11, 
	27, 26, 24, 20, 2, 4, 6, 22, 
	8, 10, 12, 11, 28, 26, 24, 20, 
	2, 4, 6, 22, 8, 10, 12
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
	15, 190, 132, 111, 52, 47, 15, 0, 
	0, 0, 0, 0, 0, 0, 27, 27, 
	0, 0, 0, 0, 0, 31, 31, 0, 
	0, 0, 35, 35, 39, 39, 0, 0, 
	0, 0, 0, 0, 0, 23, 212, 148, 
	125, 72, 67, 23, 0, 0, 0, 0, 
	0, 0, 0, 19, 201, 140, 118, 62, 
	57, 19, 1, 1, 1, 0, 3, 0, 
	0, 0, 43, 43, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 11, 11, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	82, 235, 181, 164, 105, 99, 82, 77, 
	223, 172, 156, 93, 87, 77, 0
};

static const unsigned char _MARK_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 8, 5
};

static const int MARK_start = 36;
static const int MARK_first_final = 36;
static const int MARK_error = 0;

static const int MARK_en_main = 36;


#line 151 "MARK.rl"

pro(MARKlexer, MARKstate* state) {
    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = state->cs;
    int res = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = state->tbc ? NULL : pe;
    u8c *pb = p;

    u32 stack[MARKmaxnest] = {0, MARK};
    u32 sp = 2;
    $u8c tok = {p, p};

    
#line 210 "MARK.rl.c"
	{
	cs = MARK_start;
	}

#line 168 "MARK.rl"
    
#line 213 "MARK.rl.c"
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
#line 66 "MARK.rl"
	{ lexpush(MARKhline); }
	break;
	case 1:
#line 67 "MARK.rl"
	{ lexpop(MARKhline); call(_MARKhline, text, tok, state); }
	break;
	case 2:
#line 68 "MARK.rl"
	{ lexpush(MARKindent); }
	break;
	case 3:
#line 69 "MARK.rl"
	{ lexpop(MARKindent); call(_MARKindent, text, tok, state); }
	break;
	case 4:
#line 70 "MARK.rl"
	{ lexpush(MARKolist); }
	break;
	case 5:
#line 71 "MARK.rl"
	{ lexpop(MARKolist); call(_MARKolist, text, tok, state); }
	break;
	case 6:
#line 72 "MARK.rl"
	{ lexpush(MARKulist); }
	break;
	case 7:
#line 73 "MARK.rl"
	{ lexpop(MARKulist); call(_MARKulist, text, tok, state); }
	break;
	case 8:
#line 74 "MARK.rl"
	{ lexpush(MARKh1); }
	break;
	case 9:
#line 75 "MARK.rl"
	{ lexpop(MARKh1); call(_MARKh1, text, tok, state); }
	break;
	case 10:
#line 76 "MARK.rl"
	{ lexpush(MARKh2); }
	break;
	case 11:
#line 77 "MARK.rl"
	{ lexpop(MARKh2); call(_MARKh2, text, tok, state); }
	break;
	case 12:
#line 78 "MARK.rl"
	{ lexpush(MARKh3); }
	break;
	case 13:
#line 79 "MARK.rl"
	{ lexpop(MARKh3); call(_MARKh3, text, tok, state); }
	break;
	case 14:
#line 80 "MARK.rl"
	{ lexpush(MARKh4); }
	break;
	case 15:
#line 81 "MARK.rl"
	{ lexpop(MARKh4); call(_MARKh4, text, tok, state); }
	break;
	case 16:
#line 84 "MARK.rl"
	{ lexpush(MARKlndx); }
	break;
	case 17:
#line 85 "MARK.rl"
	{ lexpop(MARKlndx); call(_MARKlndx, text, tok, state); }
	break;
	case 18:
#line 86 "MARK.rl"
	{ lexpush(MARKlink); }
	break;
	case 19:
#line 87 "MARK.rl"
	{ lexpop(MARKlink); call(_MARKlink, text, tok, state); }
	break;
	case 20:
#line 88 "MARK.rl"
	{ lexpush(MARKnest); }
	break;
	case 21:
#line 89 "MARK.rl"
	{ lexpop(MARKnest); call(_MARKnest, text, tok, state); }
	break;
	case 22:
#line 90 "MARK.rl"
	{ lexpush(MARKterm); }
	break;
	case 23:
#line 91 "MARK.rl"
	{ lexpop(MARKterm); call(_MARKterm, text, tok, state); }
	break;
	case 24:
#line 92 "MARK.rl"
	{ lexpush(MARKdiv); }
	break;
	case 25:
#line 93 "MARK.rl"
	{ lexpop(MARKdiv); call(_MARKdiv, text, tok, state); }
	break;
	case 26:
#line 94 "MARK.rl"
	{ lexpush(MARKline); }
	break;
	case 27:
#line 95 "MARK.rl"
	{ lexpop(MARKline); call(_MARKline, text, tok, state); }
	break;
	case 28:
#line 96 "MARK.rl"
	{ lexpush(MARKroot); }
	break;
#line 372 "MARK.rl.c"
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
	case 27:
#line 95 "MARK.rl"
	{ lexpop(MARKline); call(_MARKline, text, tok, state); }
	break;
	case 28:
#line 96 "MARK.rl"
	{ lexpush(MARKroot); }
	break;
	case 29:
#line 97 "MARK.rl"
	{ lexpop(MARKroot); call(_MARKroot, text, tok, state); }
	break;
#line 396 "MARK.rl.c"
		}
	}
	}

	_out: {}
	}

#line 169 "MARK.rl"

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


