
#line 1 "LEX.rl"
#include "LEX.h"

enum {
	LEX = 0,
	LEXspace = LEX+1,
	LEXname = LEX+2,
	LEXop = LEX+3,
	LEXclass = LEX+4,
	LEXstring = LEX+5,
	LEXentity = LEX+6,
	LEXexpr = LEX+7,
	LEXrulename = LEX+8,
	LEXeq = LEX+9,
	LEXline = LEX+10,
	LEXroot = LEX+11,
};

#define LEXmaxnest 1024

fun ok64 popfails(u32* stack, u32* sp, u32 type) {
    while (*sp && stack[*sp]!=type) *sp -= 2;
    return *sp ? OK : LEXfail;
}

#define lexpush(t) { \
    if (sp>=LEXmaxnest) fail(LEXfail); \
    stack[++sp] = p - pb; \
    stack[++sp] = t; \
}
#define lexpop(t)  \
    if (stack[sp]!=t) call(popfails, stack, &sp, t); \
    tok[0] = *(text)+stack[sp-1]; \
    tok[1] = p; \
    sp -= 2;

ok64 _LEXspace ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXname ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXop ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXclass ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXstring ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXentity ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXexpr ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXrulename ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXeq ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXline ($cu8c text, $cu8c tok, LEXstate* state);
ok64 _LEXroot ($cu8c text, $cu8c tok, LEXstate* state);



#line 103 "LEX.rl"



#line 52 "LEX.rl.c"
static const char _LEX_actions[] = {
	0, 1, 0, 1, 1, 2, 1, 0, 
	2, 5, 4, 2, 5, 13, 2, 19, 
	21, 2, 20, 21, 3, 1, 5, 4, 
	3, 1, 5, 13, 3, 1, 19, 21, 
	3, 3, 11, 4, 3, 3, 11, 13, 
	3, 3, 15, 16, 3, 5, 4, 0, 
	3, 5, 10, 2, 3, 5, 10, 6, 
	3, 5, 10, 8, 3, 7, 11, 4, 
	3, 7, 11, 13, 3, 9, 11, 4, 
	3, 9, 11, 13, 3, 17, 12, 4, 
	3, 17, 12, 13, 4, 1, 5, 4, 
	0, 4, 1, 5, 10, 2, 4, 1, 
	5, 10, 6, 4, 1, 5, 10, 8, 
	4, 3, 11, 4, 0, 4, 3, 15, 
	16, 0, 4, 7, 11, 4, 0, 4, 
	9, 11, 4, 0, 4, 17, 12, 4, 
	0, 4, 19, 18, 14, 2, 4, 20, 
	18, 14, 2, 5, 1, 19, 18, 14, 
	2
};

static const short _LEX_key_offsets[] = {
	0, 0, 12, 17, 27, 43, 45, 55, 
	71, 88, 90, 92, 104, 122, 125, 137, 
	155, 169, 176, 188, 198, 217, 220, 233, 
	251, 269, 284, 292, 305, 317, 334, 346, 
	363, 377, 384, 396, 398, 417, 437, 450, 
	454, 462, 470, 480, 490, 501, 512, 522
};

static const unsigned char _LEX_trans_keys[] = {
	13u, 32u, 61u, 95u, 9u, 10u, 48u, 57u, 
	65u, 90u, 97u, 122u, 13u, 32u, 61u, 9u, 
	10u, 13u, 32u, 45u, 59u, 63u, 124u, 9u, 
	10u, 40u, 43u, 13u, 32u, 34u, 45u, 59u, 
	63u, 91u, 124u, 9u, 10u, 40u, 43u, 65u, 
	90u, 97u, 122u, 34u, 92u, 13u, 32u, 45u, 
	59u, 63u, 124u, 9u, 10u, 40u, 43u, 13u, 
	32u, 34u, 45u, 59u, 63u, 91u, 124u, 9u, 
	10u, 40u, 43u, 65u, 90u, 97u, 122u, 13u, 
	32u, 45u, 59u, 63u, 95u, 124u, 9u, 10u, 
	40u, 43u, 48u, 57u, 65u, 90u, 97u, 122u, 
	92u, 93u, 92u, 93u, 13u, 32u, 45u, 59u, 
	63u, 92u, 93u, 124u, 9u, 10u, 40u, 43u, 
	13u, 32u, 34u, 45u, 59u, 63u, 91u, 92u, 
	93u, 124u, 9u, 10u, 40u, 43u, 65u, 90u, 
	97u, 122u, 34u, 92u, 93u, 13u, 32u, 45u, 
	59u, 63u, 92u, 93u, 124u, 9u, 10u, 40u, 
	43u, 13u, 32u, 34u, 45u, 59u, 63u, 91u, 
	92u, 93u, 124u, 9u, 10u, 40u, 43u, 65u, 
	90u, 97u, 122u, 13u, 32u, 61u, 92u, 93u, 
	95u, 9u, 10u, 48u, 57u, 65u, 90u, 97u, 
	122u, 13u, 32u, 61u, 92u, 93u, 9u, 10u, 
	13u, 32u, 45u, 59u, 63u, 92u, 93u, 124u, 
	9u, 10u, 40u, 43u, 13u, 32u, 45u, 59u, 
	63u, 124u, 9u, 10u, 40u, 43u, 13u, 32u, 
	45u, 59u, 63u, 92u, 93u, 95u, 124u, 9u, 
	10u, 40u, 43u, 48u, 57u, 65u, 90u, 97u, 
	122u, 34u, 92u, 93u, 13u, 32u, 34u, 45u, 
	59u, 63u, 92u, 93u, 124u, 9u, 10u, 40u, 
	43u, 13u, 32u, 34u, 45u, 59u, 63u, 91u, 
	92u, 93u, 124u, 9u, 10u, 40u, 43u, 65u, 
	90u, 97u, 122u, 13u, 32u, 34u, 45u, 59u, 
	63u, 91u, 92u, 93u, 124u, 9u, 10u, 40u, 
	43u, 65u, 90u, 97u, 122u, 13u, 32u, 34u, 
	61u, 92u, 93u, 95u, 9u, 10u, 48u, 57u, 
	65u, 90u, 97u, 122u, 13u, 32u, 34u, 61u, 
	92u, 93u, 9u, 10u, 13u, 32u, 34u, 45u, 
	59u, 63u, 92u, 93u, 124u, 9u, 10u, 40u, 
	43u, 13u, 32u, 34u, 45u, 59u, 63u, 92u, 
	124u, 9u, 10u, 40u, 43u, 13u, 32u, 34u, 
	45u, 59u, 63u, 91u, 92u, 124u, 9u, 10u, 
	40u, 43u, 65u, 90u, 97u, 122u, 13u, 32u, 
	34u, 45u, 59u, 63u, 92u, 124u, 9u, 10u, 
	40u, 43u, 13u, 32u, 34u, 45u, 59u, 63u, 
	91u, 92u, 124u, 9u, 10u, 40u, 43u, 65u, 
	90u, 97u, 122u, 13u, 32u, 34u, 61u, 92u, 
	95u, 9u, 10u, 48u, 57u, 65u, 90u, 97u, 
	122u, 13u, 32u, 34u, 61u, 92u, 9u, 10u, 
	13u, 32u, 34u, 45u, 59u, 63u, 92u, 124u, 
	9u, 10u, 40u, 43u, 34u, 92u, 13u, 32u, 
	34u, 45u, 59u, 63u, 92u, 95u, 124u, 9u, 
	10u, 40u, 43u, 48u, 57u, 65u, 90u, 97u, 
	122u, 13u, 32u, 34u, 45u, 59u, 63u, 92u, 
	93u, 95u, 124u, 9u, 10u, 40u, 43u, 48u, 
	57u, 65u, 90u, 97u, 122u, 13u, 32u, 34u, 
	45u, 59u, 63u, 92u, 93u, 124u, 9u, 10u, 
	40u, 43u, 65u, 90u, 97u, 122u, 13u, 32u, 
	9u, 10u, 65u, 90u, 97u, 122u, 13u, 32u, 
	9u, 10u, 65u, 90u, 97u, 122u, 13u, 32u, 
	92u, 93u, 9u, 10u, 65u, 90u, 97u, 122u, 
	13u, 32u, 92u, 93u, 9u, 10u, 65u, 90u, 
	97u, 122u, 13u, 32u, 34u, 92u, 93u, 9u, 
	10u, 65u, 90u, 97u, 122u, 13u, 32u, 34u, 
	92u, 93u, 9u, 10u, 65u, 90u, 97u, 122u, 
	13u, 32u, 34u, 92u, 9u, 10u, 65u, 90u, 
	97u, 122u, 13u, 32u, 34u, 92u, 9u, 10u, 
	65u, 90u, 97u, 122u, 0
};

static const char _LEX_single_lengths[] = {
	0, 4, 3, 6, 8, 2, 6, 8, 
	7, 2, 2, 8, 10, 3, 8, 10, 
	6, 5, 8, 6, 9, 3, 9, 10, 
	10, 7, 6, 9, 8, 9, 8, 9, 
	6, 5, 8, 2, 9, 10, 9, 0, 
	2, 2, 4, 4, 5, 5, 4, 4
};

static const char _LEX_range_lengths[] = {
	0, 4, 1, 2, 4, 0, 2, 4, 
	5, 0, 0, 2, 4, 0, 2, 4, 
	4, 1, 2, 2, 5, 0, 2, 4, 
	4, 4, 1, 2, 2, 4, 2, 4, 
	4, 1, 2, 0, 5, 5, 2, 2, 
	3, 3, 3, 3, 3, 3, 3, 3
};

static const short _LEX_index_offsets[] = {
	0, 0, 9, 14, 23, 36, 39, 48, 
	61, 74, 77, 80, 91, 106, 110, 121, 
	136, 147, 154, 165, 174, 189, 193, 205, 
	220, 235, 247, 255, 267, 278, 292, 303, 
	317, 328, 335, 346, 349, 364, 380, 392, 
	395, 401, 407, 415, 423, 432, 441, 449
};

static const unsigned char _LEX_indicies[] = {
	0, 0, 3, 2, 0, 2, 2, 2, 
	1, 4, 4, 5, 4, 1, 6, 6, 
	7, 8, 7, 7, 6, 7, 1, 9, 
	9, 10, 11, 12, 11, 14, 11, 9, 
	11, 13, 13, 1, 16, 17, 15, 18, 
	18, 19, 20, 19, 19, 18, 19, 1, 
	21, 21, 22, 23, 24, 23, 26, 23, 
	21, 23, 25, 25, 1, 27, 27, 28, 
	30, 28, 29, 28, 27, 28, 29, 29, 
	29, 1, 32, 33, 31, 32, 34, 31, 
	35, 35, 36, 37, 36, 32, 33, 36, 
	35, 36, 31, 38, 38, 39, 40, 41, 
	40, 14, 32, 33, 40, 38, 40, 42, 
	42, 31, 44, 45, 46, 43, 47, 47, 
	48, 49, 48, 32, 33, 48, 47, 48, 
	31, 50, 50, 51, 52, 53, 52, 26, 
	32, 33, 52, 50, 52, 54, 54, 31, 
	55, 55, 57, 32, 33, 56, 55, 56, 
	56, 56, 31, 58, 58, 59, 32, 33, 
	58, 31, 60, 60, 61, 62, 61, 32, 
	33, 61, 60, 61, 31, 63, 63, 64, 
	65, 64, 64, 63, 64, 1, 66, 66, 
	67, 69, 67, 32, 33, 68, 67, 66, 
	67, 68, 68, 68, 31, 70, 45, 71, 
	43, 72, 72, 44, 73, 74, 73, 45, 
	46, 73, 72, 73, 43, 75, 75, 76, 
	77, 78, 77, 80, 45, 46, 77, 75, 
	77, 79, 79, 43, 81, 81, 82, 83, 
	84, 83, 86, 45, 46, 83, 81, 83, 
	85, 85, 43, 87, 87, 44, 89, 45, 
	46, 88, 87, 88, 88, 88, 43, 90, 
	90, 44, 91, 45, 46, 90, 43, 92, 
	92, 44, 93, 94, 93, 45, 46, 93, 
	92, 93, 43, 95, 95, 16, 96, 97, 
	96, 17, 96, 95, 96, 15, 98, 98, 
	99, 100, 101, 100, 80, 17, 100, 98, 
	100, 102, 102, 15, 103, 103, 16, 104, 
	105, 104, 17, 104, 103, 104, 15, 106, 
	106, 107, 108, 109, 108, 86, 17, 108, 
	106, 108, 110, 110, 15, 111, 111, 16, 
	113, 17, 112, 111, 112, 112, 112, 15, 
	114, 114, 16, 115, 17, 114, 15, 116, 
	116, 16, 117, 118, 117, 17, 117, 116, 
	117, 15, 119, 17, 15, 120, 120, 16, 
	121, 123, 121, 17, 122, 121, 120, 121, 
	122, 122, 122, 15, 124, 124, 44, 125, 
	127, 125, 45, 46, 126, 125, 124, 125, 
	126, 126, 126, 43, 128, 128, 44, 129, 
	130, 129, 45, 46, 129, 128, 129, 43, 
	131, 131, 1, 132, 132, 132, 133, 133, 
	1, 134, 134, 134, 135, 135, 1, 136, 
	136, 32, 33, 136, 137, 137, 31, 138, 
	138, 32, 33, 138, 139, 139, 31, 140, 
	140, 44, 45, 46, 140, 141, 141, 43, 
	142, 142, 44, 45, 46, 142, 143, 143, 
	43, 144, 144, 16, 17, 144, 145, 145, 
	15, 146, 146, 16, 17, 146, 147, 147, 
	15, 0
};

static const char _LEX_trans_targs[] = {
	2, 0, 1, 3, 2, 3, 4, 7, 
	40, 4, 5, 7, 40, 8, 9, 5, 
	6, 35, 4, 7, 40, 4, 5, 7, 
	40, 8, 9, 4, 7, 8, 40, 9, 
	10, 19, 11, 12, 15, 42, 12, 13, 
	15, 42, 20, 13, 14, 21, 28, 12, 
	15, 42, 12, 13, 15, 42, 20, 17, 
	16, 18, 17, 18, 12, 15, 42, 4, 
	7, 40, 12, 15, 20, 42, 22, 38, 
	23, 24, 44, 23, 22, 24, 44, 37, 
	13, 23, 22, 24, 44, 37, 13, 26, 
	25, 27, 26, 27, 23, 24, 44, 29, 
	31, 46, 29, 30, 31, 46, 36, 29, 
	31, 46, 29, 30, 31, 46, 36, 33, 
	32, 34, 33, 34, 29, 31, 46, 30, 
	29, 31, 36, 46, 23, 24, 37, 44, 
	23, 24, 44, 1, 41, 1, 41, 1, 
	43, 16, 43, 16, 45, 25, 45, 25, 
	47, 32, 47, 32
};

static const unsigned char _LEX_trans_actions[] = {
	109, 0, 0, 40, 5, 3, 124, 76, 
	80, 84, 99, 20, 24, 89, 94, 0, 
	0, 0, 119, 68, 72, 44, 56, 8, 
	11, 48, 52, 104, 32, 0, 36, 0, 
	0, 0, 0, 114, 60, 64, 84, 99, 
	20, 24, 89, 0, 0, 0, 0, 119, 
	68, 72, 44, 56, 8, 11, 48, 109, 
	0, 40, 5, 3, 124, 76, 80, 114, 
	60, 64, 104, 32, 0, 36, 0, 0, 
	119, 68, 72, 84, 99, 20, 24, 89, 
	94, 44, 56, 8, 11, 48, 52, 109, 
	0, 40, 5, 3, 124, 76, 80, 114, 
	60, 64, 84, 99, 20, 24, 89, 119, 
	68, 72, 44, 56, 8, 11, 48, 109, 
	0, 40, 5, 3, 124, 76, 80, 0, 
	104, 32, 0, 36, 104, 32, 0, 36, 
	114, 60, 64, 134, 1, 129, 5, 139, 
	1, 129, 5, 139, 1, 129, 5, 139, 
	1, 129, 5, 139
};

static const unsigned char _LEX_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 17, 
	14, 28, 14, 28, 14, 28, 14, 28
};

static const int LEX_start = 39;
static const int LEX_first_final = 39;
static const int LEX_error = 0;

static const int LEX_en_main = 39;


#line 106 "LEX.rl"

pro(LEXlexer, LEXstate* state) {
    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = state->cs;
    int res = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = state->tbc ? NULL : pe;
    u8c *pb = p;

    u32 stack[LEXmaxnest] = {0, LEX};
    u32 sp = 2;
    $u8c tok = {p, p};

    
#line 316 "LEX.rl.c"
	{
	cs = LEX_start;
	}

#line 123 "LEX.rl"
    
#line 319 "LEX.rl.c"
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
	_keys = _LEX_trans_keys + _LEX_key_offsets[cs];
	_trans = _LEX_index_offsets[cs];

	_klen = _LEX_single_lengths[cs];
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

	_klen = _LEX_range_lengths[cs];
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
	_trans = _LEX_indicies[_trans];
	cs = _LEX_trans_targs[_trans];

	if ( _LEX_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _LEX_actions + _LEX_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 55 "LEX.rl"
	{ lexpush(LEXspace); }
	break;
	case 1:
#line 56 "LEX.rl"
	{ lexpop(LEXspace); call(_LEXspace, text, tok, state); }
	break;
	case 2:
#line 57 "LEX.rl"
	{ lexpush(LEXname); }
	break;
	case 3:
#line 58 "LEX.rl"
	{ lexpop(LEXname); call(_LEXname, text, tok, state); }
	break;
	case 4:
#line 59 "LEX.rl"
	{ lexpush(LEXop); }
	break;
	case 5:
#line 60 "LEX.rl"
	{ lexpop(LEXop); call(_LEXop, text, tok, state); }
	break;
	case 6:
#line 61 "LEX.rl"
	{ lexpush(LEXclass); }
	break;
	case 7:
#line 62 "LEX.rl"
	{ lexpop(LEXclass); call(_LEXclass, text, tok, state); }
	break;
	case 8:
#line 63 "LEX.rl"
	{ lexpush(LEXstring); }
	break;
	case 9:
#line 64 "LEX.rl"
	{ lexpop(LEXstring); call(_LEXstring, text, tok, state); }
	break;
	case 10:
#line 65 "LEX.rl"
	{ lexpush(LEXentity); }
	break;
	case 11:
#line 66 "LEX.rl"
	{ lexpop(LEXentity); call(_LEXentity, text, tok, state); }
	break;
	case 12:
#line 67 "LEX.rl"
	{ lexpush(LEXexpr); }
	break;
	case 13:
#line 68 "LEX.rl"
	{ lexpop(LEXexpr); call(_LEXexpr, text, tok, state); }
	break;
	case 14:
#line 69 "LEX.rl"
	{ lexpush(LEXrulename); }
	break;
	case 15:
#line 70 "LEX.rl"
	{ lexpop(LEXrulename); call(_LEXrulename, text, tok, state); }
	break;
	case 16:
#line 71 "LEX.rl"
	{ lexpush(LEXeq); }
	break;
	case 17:
#line 72 "LEX.rl"
	{ lexpop(LEXeq); call(_LEXeq, text, tok, state); }
	break;
	case 18:
#line 73 "LEX.rl"
	{ lexpush(LEXline); }
	break;
	case 19:
#line 74 "LEX.rl"
	{ lexpop(LEXline); call(_LEXline, text, tok, state); }
	break;
	case 20:
#line 75 "LEX.rl"
	{ lexpush(LEXroot); }
	break;
#line 455 "LEX.rl.c"
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
	const char *__acts = _LEX_actions + _LEX_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 1:
#line 56 "LEX.rl"
	{ lexpop(LEXspace); call(_LEXspace, text, tok, state); }
	break;
	case 19:
#line 74 "LEX.rl"
	{ lexpop(LEXline); call(_LEXline, text, tok, state); }
	break;
	case 20:
#line 75 "LEX.rl"
	{ lexpush(LEXroot); }
	break;
	case 21:
#line 76 "LEX.rl"
	{ lexpop(LEXroot); call(_LEXroot, text, tok, state); }
	break;
#line 482 "LEX.rl.c"
		}
	}
	}

	_out: {}
	}

#line 124 "LEX.rl"

    test(p==text[1], LEXfail);

    if (state->tbc) {
        test(cs != LEX_error, LEXfail);
        state->cs = cs;
    } else {
        test(cs >= LEX_first_final, LEXfail);
    }

    nedo(
        state->text[0] = p;
    );
}


