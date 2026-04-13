
/* #line 1 "dog/FRAG.c.rl" */
#include "FRAG.h"
#include <string.h>


/* #line 60 "dog/FRAG.c.rl" */



/* #line 7 "dog/FRAG.rl.c" */
static const char _frag_actions[] = {
	0, 1, 0, 1, 1, 1, 3, 1, 
	4, 1, 5, 1, 6, 1, 7, 1, 
	8, 1, 9, 1, 10, 1, 11, 1, 
	12, 2, 2, 3, 2, 9, 10, 2, 
	10, 8, 2, 11, 12
};

static const char _frag_key_offsets[] = {
	0, 0, 9, 15, 15, 17, 19, 19, 
	21, 23, 26, 29, 30, 37, 46, 55, 
	59, 62, 71
};

static const char _frag_trans_keys[] = {
	39, 47, 95, 48, 57, 65, 90, 97, 
	122, 48, 57, 65, 90, 97, 122, 47, 
	92, 47, 92, 48, 57, 48, 57, 39, 
	46, 92, 39, 46, 92, 46, 46, 48, 
	57, 65, 90, 97, 122, 39, 46, 92, 
	48, 57, 65, 90, 97, 122, 39, 46, 
	92, 48, 57, 65, 90, 97, 122, 45, 
	46, 48, 57, 46, 48, 57, 46, 58, 
	95, 48, 57, 65, 90, 97, 122, 45, 
	46, 48, 57, 0
};

static const char _frag_single_lengths[] = {
	0, 3, 0, 0, 2, 2, 0, 0, 
	0, 3, 3, 1, 1, 3, 3, 2, 
	1, 3, 2
};

static const char _frag_range_lengths[] = {
	0, 3, 3, 0, 0, 0, 0, 1, 
	1, 0, 0, 0, 3, 3, 3, 1, 
	1, 3, 1
};

static const char _frag_index_offsets[] = {
	0, 0, 7, 11, 12, 15, 18, 19, 
	21, 23, 27, 31, 33, 38, 45, 52, 
	56, 59, 66
};

static const char _frag_trans_targs[] = {
	9, 4, 17, 15, 17, 17, 0, 12, 
	12, 12, 0, 10, 11, 6, 5, 11, 
	6, 5, 5, 16, 0, 18, 0, 11, 
	13, 3, 10, 11, 13, 3, 10, 2, 
	0, 2, 12, 12, 12, 0, 11, 13, 
	3, 14, 14, 14, 10, 11, 13, 3, 
	14, 14, 14, 10, 7, 2, 15, 0, 
	2, 16, 0, 2, 8, 17, 17, 17, 
	17, 0, 7, 2, 18, 0, 0
};

static const char _frag_trans_actions[] = {
	0, 0, 1, 25, 1, 1, 0, 13, 
	13, 13, 0, 0, 34, 21, 21, 23, 
	0, 0, 0, 25, 0, 25, 0, 28, 
	28, 17, 17, 19, 19, 0, 0, 0, 
	0, 15, 0, 0, 0, 0, 19, 19, 
	0, 13, 13, 13, 0, 19, 31, 0, 
	0, 0, 0, 0, 7, 7, 5, 0, 
	11, 5, 0, 3, 3, 0, 0, 0, 
	0, 0, 9, 9, 5, 0, 0
};

static const char _frag_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 28, 19, 0, 15, 19, 31, 7, 
	11, 3, 9
};

static const int frag_start = 1;

static const int frag_en_main = 1;


/* #line 63 "dog/FRAG.c.rl" */

ok64 FRAGu8sDrain(u8cs input, fragp f) {
    if (f == NULL) return FRAGBAD;
    memset(f, 0, sizeof(frag));
    if (input[0] == NULL || input[0] >= input[1]) return OK;

    u8cp p = input[0];
    u8cp pe = input[1];
    u8cp eof = pe;
    u8cp mark = NULL;
    u8cp ext_mark = NULL;
    u32 nval = 0;
    int cs = 0;

    
/* #line 100 "dog/FRAG.rl.c" */
	{
	cs = frag_start;
	}

/* #line 78 "dog/FRAG.c.rl" */
    
/* #line 103 "dog/FRAG.rl.c" */
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _frag_trans_keys + _frag_key_offsets[cs];
	_trans = _frag_index_offsets[cs];

	_klen = _frag_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
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

	_klen = _frag_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
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
	cs = _frag_trans_targs[_trans];

	if ( _frag_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _frag_actions + _frag_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
/* #line 7 "dog/FRAG.c.rl" */
	{ mark = p; }
	break;
	case 1:
/* #line 8 "dog/FRAG.c.rl" */
	{ f->type = FRAG_IDENT; f->body[0] = mark; f->body[1] = p; }
	break;
	case 2:
/* #line 9 "dog/FRAG.c.rl" */
	{ nval = 0; }
	break;
	case 3:
/* #line 10 "dog/FRAG.c.rl" */
	{ nval = nval * 10 + (*p - '0'); }
	break;
	case 4:
/* #line 11 "dog/FRAG.c.rl" */
	{ if (f->line == 0) f->line = nval; else f->line_end = nval; }
	break;
	case 5:
/* #line 12 "dog/FRAG.c.rl" */
	{ f->line = nval; nval = 0; }
	break;
	case 6:
/* #line 13 "dog/FRAG.c.rl" */
	{ f->line_end = nval; }
	break;
	case 7:
/* #line 14 "dog/FRAG.c.rl" */
	{ ext_mark = p; }
	break;
	case 8:
/* #line 15 "dog/FRAG.c.rl" */
	{
        if (f->nexts < FRAG_MAX_EXTS) {
            f->exts[f->nexts][0] = ext_mark;
            f->exts[f->nexts][1] = p;
            f->nexts++;
        }
    }
	break;
	case 9:
/* #line 22 "dog/FRAG.c.rl" */
	{ mark = p; }
	break;
	case 10:
/* #line 23 "dog/FRAG.c.rl" */
	{ f->type = FRAG_SPOT; f->body[0] = mark; f->body[1] = p; }
	break;
	case 11:
/* #line 24 "dog/FRAG.c.rl" */
	{ mark = p; }
	break;
	case 12:
/* #line 25 "dog/FRAG.c.rl" */
	{ f->type = FRAG_PCRE; f->body[0] = mark; f->body[1] = p; }
	break;
/* #line 220 "dog/FRAG.rl.c" */
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
	const char *__acts = _frag_actions + _frag_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 1:
/* #line 8 "dog/FRAG.c.rl" */
	{ f->type = FRAG_IDENT; f->body[0] = mark; f->body[1] = p; }
	break;
	case 4:
/* #line 11 "dog/FRAG.c.rl" */
	{ if (f->line == 0) f->line = nval; else f->line_end = nval; }
	break;
	case 5:
/* #line 12 "dog/FRAG.c.rl" */
	{ f->line = nval; nval = 0; }
	break;
	case 6:
/* #line 13 "dog/FRAG.c.rl" */
	{ f->line_end = nval; }
	break;
	case 8:
/* #line 15 "dog/FRAG.c.rl" */
	{
        if (f->nexts < FRAG_MAX_EXTS) {
            f->exts[f->nexts][0] = ext_mark;
            f->exts[f->nexts][1] = p;
            f->nexts++;
        }
    }
	break;
	case 9:
/* #line 22 "dog/FRAG.c.rl" */
	{ mark = p; }
	break;
	case 10:
/* #line 23 "dog/FRAG.c.rl" */
	{ f->type = FRAG_SPOT; f->body[0] = mark; f->body[1] = p; }
	break;
/* #line 262 "dog/FRAG.rl.c" */
		}
	}
	}

	_out: {}
	}

/* #line 79 "dog/FRAG.c.rl" */

    if (cs < 9) {
        // If parser didn't reach a final state but we got a type,
        // it's a tolerant parse (e.g., unclosed spot quote).
        if (f->type == FRAG_SPOT && f->body[0] != NULL) {
            // Unclosed spot: body extends to end of input
            f->body[1] = pe;
            return OK;
        }
        return FRAGFAIL;
    }

    // Line-only: digit-started fragment, type not set by ident/spot/pcre
    if (f->type == FRAG_NONE && f->line > 0)
        f->type = FRAG_LINE;

    return OK;
}
