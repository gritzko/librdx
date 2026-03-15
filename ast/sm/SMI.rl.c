
/* #line 1 "SMI.c.rl" */
#include "abc/INT.h"
#include "abc/PRO.h"
#include "SMI.h"

// user functions (callbacks) for the parser
ok64 SMIonStars (u8cs tok, SMIstate* state);
ok64 SMIonTildes (u8cs tok, SMIstate* state);
ok64 SMIonCode (u8cs tok, SMIstate* state);
ok64 SMIonWord (u8cs tok, SMIstate* state);
ok64 SMIonSpace (u8cs tok, SMIstate* state);
ok64 SMIonOpen (u8cs tok, SMIstate* state);
ok64 SMIonClose (u8cs tok, SMIstate* state);


/* #line 68 "SMI.c.rl" */



/* #line 17 "SMI.rl.c" */
static const char _SMI_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8
};

static const char _SMI_key_offsets[] = {
	0, 0, 8, 16, 18, 19
};

static const unsigned char _SMI_trans_keys[] = {
	9u, 10u, 32u, 42u, 91u, 93u, 96u, 126u, 
	32u, 42u, 91u, 93u, 96u, 126u, 9u, 10u, 
	9u, 32u, 42u, 126u, 0
};

static const char _SMI_single_lengths[] = {
	0, 8, 6, 2, 1, 1
};

static const char _SMI_range_lengths[] = {
	0, 0, 1, 0, 0, 0
};

static const char _SMI_index_offsets[] = {
	0, 0, 9, 17, 20, 22
};

static const char _SMI_trans_targs[] = {
	3, 0, 3, 4, 1, 1, 1, 5, 
	2, 1, 1, 1, 1, 1, 1, 1, 
	2, 3, 3, 1, 4, 1, 5, 1, 
	1, 1, 1, 1, 0
};

static const char _SMI_trans_actions[] = {
	0, 0, 0, 0, 7, 9, 5, 0, 
	0, 15, 15, 15, 15, 15, 15, 15, 
	0, 0, 0, 17, 0, 11, 0, 13, 
	15, 17, 11, 13, 0
};

static const char _SMI_to_state_actions[] = {
	0, 1, 0, 0, 0, 0
};

static const char _SMI_from_state_actions[] = {
	0, 3, 0, 0, 0, 0
};

static const char _SMI_eof_trans[] = {
	0, 0, 25, 26, 27, 28
};

static const int SMI_start = 1;
static const int SMI_first_final = 1;
static const int SMI_error = 0;

static const int SMI_en_main = 1;


/* #line 71 "SMI.c.rl" */

// the public API function
ok64 SMILexer(SMIstate* state) {

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

    
/* #line 95 "SMI.rl.c" */
	{
	cs = SMI_start;
	ts = 0;
	te = 0;
	act = 0;
	}

/* #line 90 "SMI.c.rl" */
    
/* #line 101 "SMI.rl.c" */
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
	_acts = _SMI_actions + _SMI_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 1:
/* #line 1 "NONE" */
	{ts = p;}
	break;
/* #line 120 "SMI.rl.c" */
		}
	}

	_keys = _SMI_trans_keys + _SMI_key_offsets[cs];
	_trans = _SMI_index_offsets[cs];

	_klen = _SMI_single_lengths[cs];
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

	_klen = _SMI_range_lengths[cs];
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
_eof_trans:
	cs = _SMI_trans_targs[_trans];

	if ( _SMI_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _SMI_actions + _SMI_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 2:
/* #line 36 "SMI.c.rl" */
	{te = p+1;{
        tok[0] = ts;
        tok[1] = te;
        o = SMIonCode(tok, state);
        if (o != OK) goto _out;
    }}
	break;
	case 3:
/* #line 42 "SMI.c.rl" */
	{te = p+1;{
        tok[0] = ts;
        tok[1] = te;
        o = SMIonOpen(tok, state);
        if (o != OK) goto _out;
    }}
	break;
	case 4:
/* #line 48 "SMI.c.rl" */
	{te = p+1;{
        tok[0] = ts;
        tok[1] = te;
        o = SMIonClose(tok, state);
        if (o != OK) goto _out;
    }}
	break;
	case 5:
/* #line 24 "SMI.c.rl" */
	{te = p;p--;{
        tok[0] = ts;
        tok[1] = te;
        o = SMIonStars(tok, state);
        if (o != OK) goto _out;
    }}
	break;
	case 6:
/* #line 30 "SMI.c.rl" */
	{te = p;p--;{
        tok[0] = ts;
        tok[1] = te;
        o = SMIonTildes(tok, state);
        if (o != OK) goto _out;
    }}
	break;
	case 7:
/* #line 54 "SMI.c.rl" */
	{te = p;p--;{
        tok[0] = ts;
        tok[1] = te;
        o = SMIonWord(tok, state);
        if (o != OK) goto _out;
    }}
	break;
	case 8:
/* #line 60 "SMI.c.rl" */
	{te = p;p--;{
        tok[0] = ts;
        tok[1] = te;
        o = SMIonSpace(tok, state);
        if (o != OK) goto _out;
    }}
	break;
/* #line 240 "SMI.rl.c" */
		}
	}

_again:
	_acts = _SMI_actions + _SMI_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
/* #line 1 "NONE" */
	{ts = 0;}
	break;
/* #line 251 "SMI.rl.c" */
		}
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _SMI_eof_trans[cs] > 0 ) {
		_trans = _SMI_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

/* #line 91 "SMI.c.rl" */

    state->data[0] = p;
    if (o==OK && cs < SMI_first_final)
        o = SMIBAD;

    return o;
}
