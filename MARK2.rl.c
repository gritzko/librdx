
#line 1 "MARK2.rl"
#include "PRO.h"
#include "MARK2.h"

enum {
	MARK2 = 0,
	MARK2plain = MARK2+5,
	MARK2ref = MARK2+6,
	MARK2tostress = MARK2+7,
	MARK2stress = MARK2+8,
	MARK2toemph = MARK2+9,
	MARK2emph = MARK2+10,
	MARK2inline = MARK2+11,
	MARK2root = MARK2+12,
};

#define MARK2maxnest 1024

fun ok64 popfails(u32* stack, u32* sp, u32 type) {
    while (*sp && stack[*sp]!=type) *sp -= 2;
    return *sp ? OK : MARK2fail;
}

#define lexpush(t) { \
    if (sp>=MARK2maxnest) fail(MARK2fail); \
    stack[++sp] = p - pb; \
    stack[++sp] = t; \
}
#define lexpop(t)  \
    if (stack[sp]!=t) call(popfails, stack, &sp, t); \
    tok[0] = *(text)+stack[sp-1]; \
    tok[1] = p; \
    sp -= 2;

ok64 _MARK2plain ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2ref ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2tostress ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2stress ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2toemph ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2emph ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2inline ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2root ($cu8c text, $cu8c tok, MARK2state* state);



#line 94 "MARK2.rl"



#line 47 "MARK2.rl.c"
static const char _MARK2_actions[] = {
	0, 1, 0, 1, 1, 2, 0, 1, 
	2, 1, 2, 2, 6, 7, 3, 1, 
	2, 0, 3, 3, 5, 7, 3, 6, 
	4, 0, 4, 3, 5, 4, 0, 5, 
	1, 3, 5, 4, 0
};

static const char _MARK2_key_offsets[] = {
	0, 0, 5, 10, 16, 22, 28, 40, 
	46, 58, 62
};

static const unsigned char _MARK2_trans_keys[] = {
	13u, 32u, 91u, 9u, 10u, 13u, 32u, 91u, 
	9u, 10u, 13u, 32u, 91u, 93u, 9u, 10u, 
	13u, 32u, 91u, 93u, 9u, 10u, 13u, 32u, 
	91u, 93u, 9u, 10u, 13u, 32u, 91u, 93u, 
	9u, 10u, 48u, 57u, 65u, 90u, 97u, 122u, 
	13u, 32u, 91u, 93u, 9u, 10u, 13u, 32u, 
	91u, 93u, 9u, 10u, 48u, 57u, 65u, 90u, 
	97u, 122u, 13u, 32u, 9u, 10u, 13u, 32u, 
	91u, 93u, 9u, 10u, 0
};

static const char _MARK2_single_lengths[] = {
	0, 3, 3, 4, 4, 4, 4, 4, 
	4, 2, 4
};

static const char _MARK2_range_lengths[] = {
	0, 1, 1, 1, 1, 1, 4, 1, 
	4, 1, 1
};

static const char _MARK2_index_offsets[] = {
	0, 0, 5, 10, 16, 22, 28, 37, 
	43, 52, 56
};

static const char _MARK2_indicies[] = {
	1, 1, 2, 1, 0, 1, 1, 4, 
	1, 3, 1, 1, 6, 7, 1, 5, 
	1, 1, 4, 8, 1, 3, 1, 1, 
	9, 7, 1, 5, 1, 1, 4, 8, 
	1, 10, 10, 10, 3, 1, 1, 6, 
	11, 1, 5, 1, 1, 6, 7, 1, 
	12, 12, 12, 5, 1, 1, 1, 13, 
	1, 1, 15, 16, 1, 14, 0
};

static const char _MARK2_trans_targs[] = {
	1, 0, 2, 3, 4, 3, 4, 5, 
	5, 6, 7, 10, 7, 1, 3, 8, 
	5
};

static const char _MARK2_trans_actions[] = {
	0, 0, 8, 1, 14, 0, 8, 3, 
	5, 8, 1, 3, 0, 22, 26, 26, 
	31
};

static const char _MARK2_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 11, 18
};

static const int MARK2_start = 9;
static const int MARK2_first_final = 9;
static const int MARK2_error = 0;

static const int MARK2_en_main = 9;


#line 97 "MARK2.rl"

pro(MARK2lexer, MARK2state* state) {
    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = state->cs;
    int res = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = state->tbc ? NULL : pe;
    u8c *pb = p;

    u32 stack[MARK2maxnest] = {0, MARK2};
    u32 sp = 2;
    $u8c tok = {p, p};

    
#line 137 "MARK2.rl.c"
	{
	cs = MARK2_start;
	}

#line 114 "MARK2.rl"
    
#line 140 "MARK2.rl.c"
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
#line 50 "MARK2.rl"
	{ lexpush(MARK2plain); }
	break;
	case 1:
#line 51 "MARK2.rl"
	{ lexpop(MARK2plain); call(_MARK2plain, text, tok, state); }
	break;
	case 2:
#line 52 "MARK2.rl"
	{ lexpush(MARK2ref); }
	break;
	case 3:
#line 53 "MARK2.rl"
	{ lexpop(MARK2ref); call(_MARK2ref, text, tok, state); }
	break;
	case 4:
#line 62 "MARK2.rl"
	{ lexpush(MARK2inline); }
	break;
	case 5:
#line 63 "MARK2.rl"
	{ lexpop(MARK2inline); call(_MARK2inline, text, tok, state); }
	break;
	case 6:
#line 64 "MARK2.rl"
	{ lexpush(MARK2root); }
	break;
#line 234 "MARK2.rl.c"
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
	const char *__acts = _MARK2_actions + _MARK2_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 3:
#line 53 "MARK2.rl"
	{ lexpop(MARK2ref); call(_MARK2ref, text, tok, state); }
	break;
	case 5:
#line 63 "MARK2.rl"
	{ lexpop(MARK2inline); call(_MARK2inline, text, tok, state); }
	break;
	case 6:
#line 64 "MARK2.rl"
	{ lexpush(MARK2root); }
	break;
	case 7:
#line 65 "MARK2.rl"
	{ lexpop(MARK2root); call(_MARK2root, text, tok, state); }
	break;
#line 261 "MARK2.rl.c"
		}
	}
	}

	_out: {}
	}

#line 115 "MARK2.rl"

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


