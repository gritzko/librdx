
/* #line 1 "spot/RXLITS.c.rl" */
// RXLITS: walk a regex pattern, calling a callback for each
// literal character and on each meta-boundary "flush".  Used by
// CAPOPcreGrep to extract trigram-indexable substrings.
#include "RXLITS.h"


/* #line 36 "spot/RXLITS.c.rl" */



/* #line 9 "spot/RXLITS.rl.c" */
static const char _rxlits_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 2, 
	2, 0
};

static const char _rxlits_key_offsets[] = {
	0, 3, 5, 5, 11, 13, 19, 29, 
	39, 50
};

static const unsigned char _rxlits_trans_keys[] = {
	92u, 93u, 94u, 92u, 93u, 68u, 83u, 87u, 
	100u, 115u, 119u, 92u, 93u, 68u, 83u, 87u, 
	100u, 115u, 119u, 36u, 46u, 63u, 91u, 92u, 
	94u, 40u, 43u, 123u, 125u, 36u, 46u, 63u, 
	91u, 92u, 94u, 40u, 43u, 123u, 125u, 36u, 
	46u, 63u, 91u, 92u, 93u, 94u, 40u, 43u, 
	123u, 125u, 36u, 46u, 63u, 91u, 92u, 93u, 
	94u, 40u, 43u, 123u, 125u, 0
};

static const char _rxlits_single_lengths[] = {
	3, 2, 0, 6, 2, 6, 6, 6, 
	7, 7
};

static const char _rxlits_range_lengths[] = {
	0, 0, 0, 0, 0, 0, 2, 2, 
	2, 2
};

static const char _rxlits_index_offsets[] = {
	0, 4, 7, 8, 15, 18, 25, 34, 
	43, 53
};

static const char _rxlits_indicies[] = {
	1, 2, 3, 0, 1, 4, 0, 0, 
	2, 2, 2, 2, 2, 2, 5, 1, 
	2, 0, 4, 4, 4, 4, 4, 4, 
	6, 4, 4, 4, 8, 9, 4, 4, 
	4, 7, 11, 11, 11, 12, 13, 11, 
	11, 11, 10, 15, 15, 15, 12, 16, 
	17, 15, 15, 15, 14, 2, 2, 2, 
	8, 19, 20, 2, 2, 2, 18, 0
};

static const char _rxlits_trans_targs[] = {
	1, 2, 8, 4, 7, 9, 6, 6, 
	0, 5, 6, 7, 0, 5, 9, 8, 
	3, 7, 9, 3, 7
};

static const char _rxlits_trans_actions[] = {
	0, 0, 0, 0, 0, 3, 3, 1, 
	0, 0, 7, 5, 5, 5, 7, 5, 
	5, 7, 1, 0, 1
};

static const char _rxlits_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 5, 
	5, 0
};

static const int rxlits_start = 6;

static const int rxlits_en_main = 6;


/* #line 39 "spot/RXLITS.c.rl" */

void RXLITSu8sDrain(u8csc pattern, rxlits_cb cb, void *ctx) {
    if (pattern[0] == NULL || pattern[0] >= pattern[1] || cb == NULL) {
        if (cb) (void)cb(ctx, 0, YES);
        return;
    }
    u8c *p = pattern[0];
    u8c *pe = pattern[1];
    u8c *eof = pe;
    int cs = 0;
    
/* #line 87 "spot/RXLITS.rl.c" */
	{
	cs = rxlits_start;
	}

/* #line 50 "spot/RXLITS.c.rl" */
    
/* #line 90 "spot/RXLITS.rl.c" */
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const unsigned char *_keys;

	if ( p == pe )
		goto _test_eof;
_resume:
	_keys = _rxlits_trans_keys + _rxlits_key_offsets[cs];
	_trans = _rxlits_index_offsets[cs];

	_klen = _rxlits_single_lengths[cs];
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

	_klen = _rxlits_range_lengths[cs];
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
	_trans = _rxlits_indicies[_trans];
	cs = _rxlits_trans_targs[_trans];

	if ( _rxlits_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _rxlits_actions + _rxlits_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
/* #line 10 "spot/RXLITS.c.rl" */
	{ (void)cb(ctx, *p, NO); }
	break;
	case 1:
/* #line 11 "spot/RXLITS.c.rl" */
	{ (void)cb(ctx, *p, NO); }
	break;
	case 2:
/* #line 12 "spot/RXLITS.c.rl" */
	{ (void)cb(ctx, 0, YES); }
	break;
/* #line 170 "spot/RXLITS.rl.c" */
		}
	}

_again:
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _rxlits_actions + _rxlits_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 2:
/* #line 12 "spot/RXLITS.c.rl" */
	{ (void)cb(ctx, 0, YES); }
	break;
/* #line 186 "spot/RXLITS.rl.c" */
		}
	}
	}

	}

/* #line 51 "spot/RXLITS.c.rl" */
    (void)cs;  // tolerate trailing partial token
    (void)cb(ctx, 0, YES);  // final flush
}
