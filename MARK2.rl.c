
#line 1 "MARK2.rl"
#include "PRO.h"
#include "MARK2.h"

enum {
	MARK2 = 0,
	MARK2plain = MARK2+4,
	MARK2ref = MARK2+5,
	MARK2em = MARK2+7,
	MARK2inline = MARK2+8,
	MARK2root = MARK2+9,
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
ok64 _MARK2em ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2inline ($cu8c text, $cu8c tok, MARK2state* state);
ok64 _MARK2root ($cu8c text, $cu8c tok, MARK2state* state);



#line 75 "MARK2.rl"



#line 41 "MARK2.rl.c"
static const char _MARK2_actions[] = {
	0, 1, 6, 1, 7, 2, 1, 5, 
	2, 4, 0, 2, 4, 2, 2, 6, 
	7, 3, 1, 3, 5, 3, 1, 5, 
	7, 3, 6, 4, 0, 3, 6, 4, 
	2, 4, 1, 3, 5, 7, 4, 1, 
	5, 4, 0, 4, 1, 5, 4, 2, 
	5, 1, 3, 5, 4, 0, 5, 1, 
	3, 5, 4, 2
};

static const char _MARK2_key_offsets[] = {
	0, 4, 8, 12, 17, 23, 30
};

static const unsigned char _MARK2_trans_keys[] = {
	13u, 32u, 9u, 10u, 13u, 32u, 9u, 10u, 
	13u, 32u, 9u, 10u, 13u, 32u, 42u, 9u, 
	10u, 13u, 32u, 9u, 10u, 97u, 122u, 13u, 
	32u, 42u, 9u, 10u, 97u, 122u, 13u, 32u, 
	9u, 10u, 0
};

static const char _MARK2_single_lengths[] = {
	2, 2, 2, 3, 2, 3, 2
};

static const char _MARK2_range_lengths[] = {
	1, 1, 1, 1, 2, 2, 1
};

static const char _MARK2_index_offsets[] = {
	0, 4, 8, 12, 17, 22, 28
};

static const char _MARK2_indicies[] = {
	1, 2, 1, 0, 4, 5, 4, 3, 
	7, 8, 7, 6, 7, 8, 9, 7, 
	6, 4, 5, 4, 10, 3, 4, 5, 
	11, 4, 10, 3, 13, 14, 13, 12, 
	0
};

static const char _MARK2_trans_targs[] = {
	1, 2, 3, 1, 2, 3, 1, 2, 
	3, 4, 5, 6, 1, 2, 3
};

static const char _MARK2_trans_actions[] = {
	25, 1, 29, 38, 5, 43, 8, 0, 
	11, 8, 38, 38, 48, 17, 54
};

static const char _MARK2_eof_actions[] = {
	14, 21, 3, 3, 21, 21, 33
};

static const int MARK2_start = 0;
static const int MARK2_first_final = 0;
static const int MARK2_error = -1;

static const int MARK2_en_main = 0;


#line 78 "MARK2.rl"

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

    
#line 120 "MARK2.rl.c"
	{
	cs = MARK2_start;
	}

#line 95 "MARK2.rl"
    
#line 123 "MARK2.rl.c"
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
#line 44 "MARK2.rl"
	{ lexpush(MARK2plain); }
	break;
	case 1:
#line 45 "MARK2.rl"
	{ lexpop(MARK2plain); call(_MARK2plain, text, tok, state); }
	break;
	case 2:
#line 48 "MARK2.rl"
	{ lexpush(MARK2em); }
	break;
	case 3:
#line 49 "MARK2.rl"
	{ lexpop(MARK2em); call(_MARK2em, text, tok, state); }
	break;
	case 4:
#line 50 "MARK2.rl"
	{ lexpush(MARK2inline); }
	break;
	case 5:
#line 51 "MARK2.rl"
	{ lexpop(MARK2inline); call(_MARK2inline, text, tok, state); }
	break;
	case 6:
#line 52 "MARK2.rl"
	{ lexpush(MARK2root); }
	break;
#line 215 "MARK2.rl.c"
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
#line 45 "MARK2.rl"
	{ lexpop(MARK2plain); call(_MARK2plain, text, tok, state); }
	break;
	case 3:
#line 49 "MARK2.rl"
	{ lexpop(MARK2em); call(_MARK2em, text, tok, state); }
	break;
	case 5:
#line 51 "MARK2.rl"
	{ lexpop(MARK2inline); call(_MARK2inline, text, tok, state); }
	break;
	case 6:
#line 52 "MARK2.rl"
	{ lexpush(MARK2root); }
	break;
	case 7:
#line 53 "MARK2.rl"
	{ lexpop(MARK2root); call(_MARK2root, text, tok, state); }
	break;
#line 243 "MARK2.rl.c"
		}
	}
	}

	}

#line 96 "MARK2.rl"

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


