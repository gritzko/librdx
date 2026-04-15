
/* #line 1 "dog/QURY.c.rl" */
#include "QURY.h"
#include <string.h>


/* #line 22 "dog/QURY.c.rl" */



/* #line 7 "dog/QURY.rl.c" */
static const char _qury_actions[] = {
	0, 1, 0, 1, 1, 1, 4, 2, 
	1, 2, 2, 1, 3
};

static const char _qury_key_offsets[] = {
	0, 0, 8, 16, 28
};

static const char _qury_trans_keys[] = {
	45, 95, 48, 57, 65, 90, 97, 122, 
	45, 95, 48, 57, 65, 90, 97, 122, 
	45, 94, 95, 126, 46, 47, 48, 57, 
	65, 90, 97, 122, 48, 57, 0
};

static const char _qury_single_lengths[] = {
	0, 2, 2, 4, 0
};

static const char _qury_range_lengths[] = {
	0, 3, 3, 4, 1
};

static const char _qury_index_offsets[] = {
	0, 0, 6, 12, 21
};

static const char _qury_indicies[] = {
	0, 0, 0, 0, 0, 1, 2, 2, 
	2, 2, 2, 1, 2, 4, 2, 5, 
	3, 2, 2, 2, 1, 6, 1, 0
};

static const char _qury_trans_targs[] = {
	3, 0, 3, 2, 4, 4, 4
};

static const char _qury_trans_actions[] = {
	1, 0, 0, 0, 10, 7, 5
};

static const char _qury_eof_actions[] = {
	0, 0, 0, 3, 0
};

static const int qury_start = 1;

static const int qury_en_main = 1;


/* #line 25 "dog/QURY.c.rl" */

static b8 qury_is_sha(u8cs s) {
    if ($len(s) < QURY_MIN_SHA) return NO;
    $for(u8c, p, s) {
        u8 c = *p;
        if ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F'))
            continue;
        return NO;
    }
    return YES;
}

ok64 QURYu8sDrain(u8cs input, qrefp out) {
    if (out == NULL) return QURYBAD;
    memset(out, 0, sizeof(qref));
    if (input[0] == NULL || input[0] >= input[1]) return OK;

    // Find the end of this spec (up to '&' or end of input)
    u8cp specend = input[0];
    while (specend < input[1] && *specend != '&') specend++;

    u8cp p = input[0];
    u8cp pe = specend;
    u8cp eof = pe;
    u8cp body_mark = NULL;
    int cs = 0;

    
/* #line 86 "dog/QURY.rl.c" */
	{
	cs = qury_start;
	}

/* #line 55 "dog/QURY.c.rl" */
    
/* #line 89 "dog/QURY.rl.c" */
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
	_keys = _qury_trans_keys + _qury_key_offsets[cs];
	_trans = _qury_index_offsets[cs];

	_klen = _qury_single_lengths[cs];
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

	_klen = _qury_range_lengths[cs];
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
	_trans = _qury_indicies[_trans];
	cs = _qury_trans_targs[_trans];

	if ( _qury_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _qury_actions + _qury_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
/* #line 7 "dog/QURY.c.rl" */
	{ body_mark = p; }
	break;
	case 1:
/* #line 8 "dog/QURY.c.rl" */
	{ out->body[0] = body_mark; out->body[1] = p; }
	break;
	case 2:
/* #line 9 "dog/QURY.c.rl" */
	{ out->anc_type = '~'; }
	break;
	case 3:
/* #line 10 "dog/QURY.c.rl" */
	{ out->anc_type = '^'; }
	break;
	case 4:
/* #line 11 "dog/QURY.c.rl" */
	{ out->ancestry = out->ancestry * 10 + (*p - '0'); }
	break;
/* #line 177 "dog/QURY.rl.c" */
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
	const char *__acts = _qury_actions + _qury_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 1:
/* #line 8 "dog/QURY.c.rl" */
	{ out->body[0] = body_mark; out->body[1] = p; }
	break;
/* #line 195 "dog/QURY.rl.c" */
		}
	}
	}

	_out: {}
	}

/* #line 56 "dog/QURY.c.rl" */

    // Advance input past spec and separator
    input[0] = (specend < input[1]) ? specend + 1 : specend;

    if (cs < 3)
        return QURYFAIL;

    out->type = qury_is_sha(out->body) ? QURY_SHA : QURY_REF;
    return OK;
}
