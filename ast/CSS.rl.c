
/* #line 1 "CSS.c.rl" */
#include "abc/INT.h"
#include "abc/PRO.h"
#include "CSS.h"

// action indices for the parser
#define CSSenum 0
enum {
	CSSIdent = CSSenum+4,
	CSSDot = CSSenum+5,
	CSSStar = CSSenum+6,
	CSSChild = CSSenum+7,
	CSSAdjacent = CSSenum+8,
	CSSSibling = CSSenum+9,
	CSSHas = CSSenum+10,
	CSSNot = CSSenum+11,
	CSSClose = CSSenum+12,
	CSSLine = CSSenum+13,
	CSSRoot = CSSenum+17,
};

// user functions (callbacks) for the parser
ok64 CSSonIdent (u8cs tok, CSSstate* state);
ok64 CSSonDot (u8cs tok, CSSstate* state);
ok64 CSSonStar (u8cs tok, CSSstate* state);
ok64 CSSonChild (u8cs tok, CSSstate* state);
ok64 CSSonAdjacent (u8cs tok, CSSstate* state);
ok64 CSSonSibling (u8cs tok, CSSstate* state);
ok64 CSSonHas (u8cs tok, CSSstate* state);
ok64 CSSonNot (u8cs tok, CSSstate* state);
ok64 CSSonClose (u8cs tok, CSSstate* state);
ok64 CSSonLine (u8cs tok, CSSstate* state);
ok64 CSSonRoot (u8cs tok, CSSstate* state);




/* #line 164 "CSS.c.rl" */



/* #line 39 "CSS.rl.c" */
static const char _CSS_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8, 1, 9, 1, 10, 1, 
	11, 1, 13, 1, 15, 1, 16, 1, 
	17, 1, 19, 1, 20, 1, 21, 2, 
	0, 18, 2, 1, 2, 2, 1, 4, 
	2, 1, 6, 2, 1, 8, 2, 1, 
	10, 2, 1, 16, 2, 1, 19, 2, 
	1, 21, 2, 3, 0, 2, 3, 2, 
	2, 3, 4, 2, 3, 6, 2, 3, 
	8, 2, 3, 10, 2, 3, 16, 2, 
	3, 21, 2, 5, 0, 2, 5, 2, 
	2, 5, 4, 2, 5, 6, 2, 5, 
	8, 2, 5, 10, 2, 5, 16, 2, 
	5, 21, 2, 7, 0, 2, 7, 2, 
	2, 7, 4, 2, 7, 6, 2, 7, 
	8, 2, 7, 10, 2, 7, 16, 2, 
	7, 21, 2, 9, 0, 2, 9, 2, 
	2, 9, 4, 2, 9, 6, 2, 9, 
	8, 2, 9, 10, 2, 9, 16, 2, 
	9, 21, 2, 11, 0, 2, 11, 2, 
	2, 11, 4, 2, 11, 6, 2, 11, 
	8, 2, 11, 10, 2, 11, 16, 2, 
	11, 21, 2, 12, 14, 2, 13, 0, 
	2, 13, 2, 2, 13, 4, 2, 13, 
	6, 2, 13, 8, 2, 13, 10, 2, 
	13, 16, 2, 13, 21, 2, 15, 0, 
	2, 15, 2, 2, 15, 4, 2, 15, 
	6, 2, 15, 8, 2, 15, 10, 2, 
	15, 16, 2, 15, 21, 2, 17, 0, 
	2, 17, 2, 2, 17, 4, 2, 17, 
	6, 2, 17, 8, 2, 17, 10, 2, 
	17, 16, 2, 17, 21, 2, 19, 2, 
	2, 19, 4, 2, 19, 6, 2, 19, 
	8, 2, 19, 10, 2, 19, 16, 2, 
	19, 21, 2, 20, 0, 2, 20, 2, 
	2, 20, 4, 2, 20, 6, 2, 20, 
	8, 2, 20, 10, 2, 20, 16, 2, 
	20, 21, 3, 1, 12, 14, 3, 1, 
	19, 2, 3, 1, 19, 4, 3, 1, 
	19, 6, 3, 1, 19, 8, 3, 1, 
	19, 10, 3, 1, 19, 16, 3, 1, 
	19, 21, 3, 3, 0, 18, 3, 3, 
	12, 14, 3, 5, 0, 18, 3, 5, 
	12, 14, 3, 7, 0, 18, 3, 7, 
	12, 14, 3, 9, 0, 18, 3, 9, 
	12, 14, 3, 11, 0, 18, 3, 11, 
	12, 14, 3, 13, 0, 18, 3, 13, 
	12, 14, 3, 15, 0, 18, 3, 15, 
	12, 14, 3, 17, 0, 18, 3, 17, 
	12, 14, 3, 19, 12, 14, 3, 20, 
	0, 18, 3, 20, 12, 14, 4, 1, 
	19, 12, 14
};

static const unsigned char _CSS_key_offsets[] = {
	0, 0, 2, 3, 4, 5, 7, 8, 
	9, 10, 25, 40, 55, 70, 85, 100, 
	115, 130, 146, 161, 177, 194, 205
};

static const unsigned char _CSS_trans_keys[] = {
	104u, 110u, 97u, 115u, 40u, 48u, 57u, 111u, 
	116u, 40u, 9u, 32u, 41u, 42u, 43u, 46u, 
	58u, 62u, 76u, 95u, 126u, 65u, 90u, 97u, 
	122u, 9u, 32u, 41u, 42u, 43u, 46u, 58u, 
	62u, 76u, 95u, 126u, 65u, 90u, 97u, 122u, 
	9u, 32u, 41u, 42u, 43u, 46u, 58u, 62u, 
	76u, 95u, 126u, 65u, 90u, 97u, 122u, 9u, 
	32u, 41u, 42u, 43u, 46u, 58u, 62u, 76u, 
	95u, 126u, 65u, 90u, 97u, 122u, 9u, 32u, 
	41u, 42u, 43u, 46u, 58u, 62u, 76u, 95u, 
	126u, 65u, 90u, 97u, 122u, 9u, 32u, 41u, 
	42u, 43u, 46u, 58u, 62u, 76u, 95u, 126u, 
	65u, 90u, 97u, 122u, 9u, 32u, 41u, 42u, 
	43u, 46u, 58u, 62u, 76u, 95u, 126u, 65u, 
	90u, 97u, 122u, 9u, 32u, 41u, 42u, 43u, 
	46u, 58u, 62u, 76u, 95u, 126u, 65u, 90u, 
	97u, 122u, 9u, 32u, 41u, 42u, 43u, 46u, 
	58u, 62u, 95u, 126u, 48u, 57u, 65u, 90u, 
	97u, 122u, 9u, 32u, 41u, 42u, 43u, 46u, 
	58u, 62u, 76u, 95u, 126u, 65u, 90u, 97u, 
	122u, 9u, 32u, 41u, 42u, 43u, 46u, 58u, 
	62u, 95u, 126u, 48u, 57u, 65u, 90u, 97u, 
	122u, 9u, 32u, 41u, 42u, 43u, 45u, 46u, 
	58u, 62u, 95u, 126u, 48u, 57u, 65u, 90u, 
	97u, 122u, 9u, 32u, 41u, 42u, 43u, 46u, 
	58u, 62u, 126u, 48u, 57u, 9u, 32u, 41u, 
	42u, 43u, 46u, 58u, 62u, 76u, 95u, 126u, 
	65u, 90u, 97u, 122u, 0
};

static const char _CSS_single_lengths[] = {
	0, 2, 1, 1, 1, 0, 1, 1, 
	1, 11, 11, 11, 11, 11, 11, 11, 
	11, 10, 11, 10, 11, 9, 11
};

static const char _CSS_range_lengths[] = {
	0, 0, 0, 0, 0, 1, 0, 0, 
	0, 2, 2, 2, 2, 2, 2, 2, 
	2, 3, 2, 3, 3, 1, 2
};

static const unsigned char _CSS_index_offsets[] = {
	0, 0, 3, 5, 7, 9, 11, 13, 
	15, 17, 31, 45, 59, 73, 87, 101, 
	115, 129, 143, 157, 171, 186, 197
};

static const unsigned char _CSS_indicies[] = {
	0, 2, 1, 3, 1, 4, 1, 5, 
	1, 6, 1, 7, 1, 8, 1, 9, 
	1, 10, 10, 11, 12, 13, 14, 15, 
	16, 18, 17, 19, 17, 17, 1, 20, 
	20, 21, 22, 23, 24, 25, 26, 28, 
	27, 29, 27, 27, 1, 30, 30, 31, 
	32, 33, 34, 35, 36, 38, 37, 39, 
	37, 37, 1, 40, 40, 41, 42, 43, 
	44, 45, 46, 48, 47, 49, 47, 47, 
	1, 50, 50, 51, 52, 53, 54, 55, 
	56, 58, 57, 59, 57, 57, 1, 60, 
	60, 61, 62, 63, 64, 65, 66, 68, 
	67, 69, 67, 67, 1, 70, 70, 71, 
	72, 73, 74, 75, 76, 78, 77, 79, 
	77, 77, 1, 80, 80, 81, 82, 83, 
	84, 85, 86, 88, 87, 89, 87, 87, 
	1, 90, 90, 91, 92, 93, 94, 96, 
	97, 95, 98, 95, 95, 95, 1, 99, 
	99, 100, 101, 102, 103, 104, 105, 107, 
	106, 108, 106, 106, 1, 90, 90, 91, 
	92, 93, 94, 96, 97, 95, 98, 109, 
	95, 95, 1, 110, 110, 111, 112, 113, 
	114, 115, 116, 117, 95, 118, 109, 95, 
	95, 1, 119, 119, 120, 121, 122, 123, 
	124, 125, 126, 6, 1, 127, 127, 128, 
	129, 130, 131, 132, 133, 135, 134, 136, 
	134, 134, 1, 0
};

static const char _CSS_trans_targs[] = {
	2, 0, 6, 3, 4, 15, 21, 7, 
	8, 22, 10, 11, 12, 13, 14, 1, 
	16, 17, 19, 18, 10, 11, 12, 13, 
	14, 1, 16, 17, 19, 18, 10, 11, 
	12, 13, 14, 1, 16, 17, 19, 18, 
	10, 11, 12, 13, 14, 1, 16, 17, 
	19, 18, 10, 11, 12, 13, 14, 1, 
	16, 17, 19, 18, 10, 11, 12, 13, 
	14, 1, 16, 17, 19, 18, 10, 11, 
	12, 13, 14, 1, 16, 17, 19, 18, 
	10, 11, 12, 13, 14, 1, 16, 17, 
	19, 18, 10, 11, 12, 13, 14, 17, 
	1, 16, 18, 10, 11, 12, 13, 14, 
	1, 16, 17, 19, 18, 20, 10, 11, 
	12, 13, 5, 14, 1, 16, 18, 10, 
	11, 12, 13, 14, 1, 16, 18, 10, 
	11, 12, 13, 14, 1, 16, 17, 19, 
	18
};

static const short _CSS_trans_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 35, 300, 288, 294, 285, 410, 
	291, 282, 406, 297, 0, 29, 9, 17, 
	5, 186, 13, 1, 39, 21, 31, 255, 
	243, 249, 240, 398, 246, 237, 394, 252, 
	11, 108, 96, 102, 93, 350, 99, 90, 
	346, 105, 19, 156, 144, 150, 141, 366, 
	147, 138, 362, 153, 7, 84, 72, 78, 
	69, 342, 75, 66, 338, 81, 25, 207, 
	195, 201, 192, 382, 198, 189, 378, 204, 
	15, 132, 120, 126, 117, 358, 123, 114, 
	354, 129, 3, 57, 45, 51, 42, 0, 
	306, 48, 54, 23, 180, 168, 174, 165, 
	374, 171, 162, 370, 177, 0, 60, 330, 
	314, 322, 0, 310, 414, 318, 326, 33, 
	276, 264, 270, 261, 402, 267, 273, 27, 
	231, 219, 225, 216, 390, 222, 213, 386, 
	228
};

static const short _CSS_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 303, 37, 258, 111, 159, 87, 210, 
	135, 63, 183, 63, 334, 279, 234
};

static const int CSS_start = 9;
static const int CSS_first_final = 9;
static const int CSS_error = 0;

static const int CSS_en_main = 9;


/* #line 167 "CSS.c.rl" */

// the public API function
ok64 CSSLexer(CSSstate* state) {

    a_dup(u8c, data, state->data);
    sane($ok(data));

    int cs = 0;
    u8c *p = (u8c*) data[0];
    u8c *pe = (u8c*) data[1];
    u8c *eof = pe;
    u64 mark0[64] = {};
    ok64 o = OK;

    u8cs tok = {p, p};

    
/* #line 250 "CSS.rl.c" */
	{
	cs = CSS_start;
	}

/* #line 184 "CSS.c.rl" */
    
/* #line 253 "CSS.rl.c" */
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
	_keys = _CSS_trans_keys + _CSS_key_offsets[cs];
	_trans = _CSS_index_offsets[cs];

	_klen = _CSS_single_lengths[cs];
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

	_klen = _CSS_range_lengths[cs];
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
	_trans = _CSS_indicies[_trans];
	cs = _CSS_trans_targs[_trans];

	if ( _CSS_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _CSS_actions + _CSS_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
/* #line 43 "CSS.c.rl" */
	{ mark0[CSSIdent] = p - data[0]; }
	break;
	case 1:
/* #line 44 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSIdent];
    tok[1] = p;
    o = CSSonIdent(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 2:
/* #line 52 "CSS.c.rl" */
	{ mark0[CSSDot] = p - data[0]; }
	break;
	case 3:
/* #line 53 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSDot];
    tok[1] = p;
    o = CSSonDot(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 4:
/* #line 61 "CSS.c.rl" */
	{ mark0[CSSStar] = p - data[0]; }
	break;
	case 5:
/* #line 62 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSStar];
    tok[1] = p;
    o = CSSonStar(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 6:
/* #line 70 "CSS.c.rl" */
	{ mark0[CSSChild] = p - data[0]; }
	break;
	case 7:
/* #line 71 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSChild];
    tok[1] = p;
    o = CSSonChild(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 8:
/* #line 79 "CSS.c.rl" */
	{ mark0[CSSAdjacent] = p - data[0]; }
	break;
	case 9:
/* #line 80 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSAdjacent];
    tok[1] = p;
    o = CSSonAdjacent(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 10:
/* #line 88 "CSS.c.rl" */
	{ mark0[CSSSibling] = p - data[0]; }
	break;
	case 11:
/* #line 89 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSSibling];
    tok[1] = p;
    o = CSSonSibling(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 12:
/* #line 97 "CSS.c.rl" */
	{ mark0[CSSHas] = p - data[0]; }
	break;
	case 13:
/* #line 98 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSHas];
    tok[1] = p;
    o = CSSonHas(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 14:
/* #line 106 "CSS.c.rl" */
	{ mark0[CSSNot] = p - data[0]; }
	break;
	case 15:
/* #line 107 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSNot];
    tok[1] = p;
    o = CSSonNot(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 16:
/* #line 115 "CSS.c.rl" */
	{ mark0[CSSClose] = p - data[0]; }
	break;
	case 17:
/* #line 116 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSClose];
    tok[1] = p;
    o = CSSonClose(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 18:
/* #line 124 "CSS.c.rl" */
	{ mark0[CSSLine] = p - data[0]; }
	break;
	case 19:
/* #line 125 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSLine];
    tok[1] = p;
    o = CSSonLine(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 20:
/* #line 133 "CSS.c.rl" */
	{ mark0[CSSRoot] = p - data[0]; }
	break;
/* #line 459 "CSS.rl.c" */
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
	const char *__acts = _CSS_actions + _CSS_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 1:
/* #line 44 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSIdent];
    tok[1] = p;
    o = CSSonIdent(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 3:
/* #line 53 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSDot];
    tok[1] = p;
    o = CSSonDot(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 5:
/* #line 62 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSStar];
    tok[1] = p;
    o = CSSonStar(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 7:
/* #line 71 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSChild];
    tok[1] = p;
    o = CSSonChild(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 9:
/* #line 80 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSAdjacent];
    tok[1] = p;
    o = CSSonAdjacent(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 11:
/* #line 89 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSSibling];
    tok[1] = p;
    o = CSSonSibling(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 13:
/* #line 98 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSHas];
    tok[1] = p;
    o = CSSonHas(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 15:
/* #line 107 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSNot];
    tok[1] = p;
    o = CSSonNot(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 17:
/* #line 116 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSClose];
    tok[1] = p;
    o = CSSonClose(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 19:
/* #line 125 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSLine];
    tok[1] = p;
    o = CSSonLine(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
	case 20:
/* #line 133 "CSS.c.rl" */
	{ mark0[CSSRoot] = p - data[0]; }
	break;
	case 21:
/* #line 134 "CSS.c.rl" */
	{
    tok[0] = data[0] + mark0[CSSRoot];
    tok[1] = p;
    o = CSSonRoot(tok, state); 
    if (o!=OK) {
        goto _out;
    }
}
	break;
/* #line 587 "CSS.rl.c" */
		}
	}
	}

	_out: {}
	}

/* #line 185 "CSS.c.rl" */

    state->data[0] = p;
    if (o==OK && cs < CSS_first_final) 
        o = CSSBAD;
    
    return o;
}
