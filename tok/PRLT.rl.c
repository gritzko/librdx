
/* #line 1 "PRLT.c.rl" */
#include "abc/INT.h"
#include "abc/PRO.h"
#include "PRLT.h"

ok64 PRLTonComment (u8cs tok, PRLTstate* state);
ok64 PRLTonString (u8cs tok, PRLTstate* state);
ok64 PRLTonNumber (u8cs tok, PRLTstate* state);
ok64 PRLTonWord (u8cs tok, PRLTstate* state);
ok64 PRLTonPunct (u8cs tok, PRLTstate* state);
ok64 PRLTonSpace (u8cs tok, PRLTstate* state);


/* #line 129 "PRLT.c.rl" */



/* #line 15 "PRLT.rl.c" */
static const char _PRLT_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	10, 1, 11, 1, 12, 1, 13, 1, 
	14, 1, 15, 1, 16, 1, 17, 1, 
	18, 1, 19, 1, 20, 1, 21, 1, 
	22, 1, 23, 1, 24, 1, 25, 1, 
	26, 1, 27, 1, 28, 1, 29, 1, 
	30, 1, 31, 1, 32, 1, 33, 1, 
	34, 1, 35, 1, 36, 1, 37, 1, 
	38, 1, 39, 1, 40, 1, 41, 2, 
	2, 3, 2, 2, 4, 2, 2, 5, 
	2, 2, 6, 2, 2, 7, 2, 2, 
	8, 2, 2, 9
};

static const short _PRLT_key_offsets[] = {
	0, 0, 2, 2, 17, 23, 25, 25, 
	29, 31, 33, 37, 39, 41, 45, 47, 
	49, 53, 55, 61, 63, 64, 65, 66, 
	67, 68, 69, 70, 71, 73, 74, 75, 
	76, 77, 78, 79, 80, 81, 82, 83, 
	85, 86, 87, 88, 89, 90, 91, 92, 
	97, 98, 103, 105, 105, 111, 118, 118, 
	124, 126, 128, 128, 128, 164, 167, 169, 
	170, 177, 179, 180, 182, 184, 187, 191, 
	192, 197, 200, 202, 214, 218, 223, 226, 
	234, 240, 243, 246, 253, 255, 256, 267, 
	268, 270, 278, 286, 294, 303, 311, 319, 
	335, 349, 363, 372, 380
};

static const unsigned char _PRLT_trans_keys[] = {
	34u, 92u, 33u, 59u, 64u, 92u, 95u, 38u, 
	39u, 43u, 44u, 46u, 57u, 65u, 90u, 97u, 
	122u, 61u, 95u, 65u, 90u, 97u, 122u, 39u, 
	92u, 43u, 45u, 48u, 57u, 48u, 57u, 48u, 
	57u, 43u, 45u, 48u, 57u, 48u, 57u, 48u, 
	57u, 43u, 45u, 48u, 57u, 48u, 57u, 48u, 
	57u, 48u, 55u, 56u, 57u, 48u, 49u, 48u, 
	57u, 65u, 70u, 97u, 102u, 97u, 101u, 99u, 
	107u, 103u, 105u, 110u, 117u, 116u, 110u, 99u, 
	100u, 111u, 100u, 105u, 110u, 103u, 111u, 114u, 
	101u, 97u, 100u, 49u, 52u, 116u, 101u, 109u, 
	118u, 101u, 111u, 100u, 95u, 65u, 90u, 97u, 
	122u, 58u, 95u, 65u, 90u, 97u, 122u, 47u, 
	92u, 33u, 41u, 47u, 93u, 124u, 125u, 33u, 
	41u, 47u, 92u, 93u, 124u, 125u, 33u, 41u, 
	47u, 93u, 124u, 125u, 47u, 92u, 47u, 92u, 
	32u, 33u, 34u, 35u, 36u, 37u, 38u, 39u, 
	42u, 43u, 45u, 46u, 47u, 48u, 60u, 61u, 
	62u, 64u, 94u, 95u, 101u, 103u, 108u, 109u, 
	110u, 113u, 115u, 124u, 9u, 13u, 49u, 57u, 
	65u, 90u, 97u, 122u, 32u, 9u, 13u, 61u, 
	126u, 10u, 95u, 48u, 57u, 65u, 90u, 97u, 
	122u, 38u, 61u, 61u, 42u, 61u, 43u, 61u, 
	45u, 61u, 62u, 46u, 61u, 48u, 57u, 46u, 
	69u, 95u, 101u, 48u, 57u, 95u, 48u, 57u, 
	47u, 61u, 46u, 66u, 69u, 88u, 95u, 98u, 
	101u, 120u, 48u, 55u, 56u, 57u, 69u, 101u, 
	48u, 57u, 69u, 95u, 101u, 48u, 57u, 95u, 
	48u, 57u, 46u, 69u, 95u, 101u, 48u, 55u, 
	56u, 57u, 46u, 69u, 95u, 101u, 48u, 57u, 
	95u, 48u, 57u, 95u, 48u, 49u, 95u, 48u, 
	57u, 65u, 70u, 97u, 102u, 60u, 61u, 62u, 
	98u, 99u, 101u, 102u, 104u, 105u, 111u, 112u, 
	126u, 61u, 62u, 10u, 61u, 62u, 58u, 95u, 
	48u, 57u, 65u, 90u, 97u, 122u, 58u, 95u, 
	48u, 57u, 65u, 90u, 97u, 122u, 58u, 95u, 
	48u, 57u, 65u, 90u, 97u, 122u, 47u, 58u, 
	95u, 48u, 57u, 65u, 90u, 97u, 122u, 99u, 
	101u, 103u, 105u, 109u, 112u, 115u, 120u, 58u, 
	95u, 48u, 57u, 65u, 90u, 97u, 122u, 33u, 
	40u, 47u, 58u, 91u, 95u, 113u, 119u, 48u, 
	57u, 65u, 90u, 97u, 122u, 123u, 124u, 33u, 
	40u, 47u, 58u, 91u, 95u, 48u, 57u, 65u, 
	90u, 97u, 122u, 123u, 124u, 33u, 40u, 47u, 
	58u, 91u, 95u, 48u, 57u, 65u, 90u, 97u, 
	122u, 123u, 124u, 47u, 58u, 95u, 48u, 57u, 
	65u, 90u, 97u, 122u, 99u, 101u, 103u, 105u, 
	109u, 112u, 115u, 120u, 61u, 124u, 0
};

static const char _PRLT_single_lengths[] = {
	0, 2, 0, 5, 2, 2, 0, 2, 
	0, 0, 2, 0, 0, 2, 0, 0, 
	0, 0, 0, 2, 1, 1, 1, 1, 
	1, 1, 1, 1, 2, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 0, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 2, 0, 4, 5, 0, 4, 
	2, 2, 0, 0, 28, 1, 2, 1, 
	1, 2, 1, 2, 2, 1, 2, 1, 
	3, 1, 2, 8, 2, 3, 1, 4, 
	4, 1, 1, 1, 2, 1, 9, 1, 
	2, 2, 2, 2, 3, 8, 2, 8, 
	6, 6, 3, 8, 2
};

static const char _PRLT_range_lengths[] = {
	0, 0, 0, 5, 2, 0, 0, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	2, 1, 3, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	0, 0, 0, 0, 0, 0, 0, 2, 
	0, 2, 0, 0, 1, 1, 0, 1, 
	0, 0, 0, 0, 4, 1, 0, 0, 
	3, 0, 0, 0, 0, 1, 1, 0, 
	1, 1, 0, 2, 1, 1, 1, 2, 
	1, 1, 1, 3, 0, 0, 1, 0, 
	0, 3, 3, 3, 3, 0, 3, 4, 
	4, 4, 3, 0, 0
};

static const short _PRLT_index_offsets[] = {
	0, 0, 3, 4, 15, 20, 23, 24, 
	28, 30, 32, 36, 38, 40, 44, 46, 
	48, 51, 53, 57, 60, 62, 64, 66, 
	68, 70, 72, 74, 76, 79, 81, 83, 
	85, 87, 89, 91, 93, 95, 97, 99, 
	101, 103, 105, 107, 109, 111, 113, 115, 
	119, 121, 125, 128, 129, 135, 142, 143, 
	149, 152, 155, 156, 157, 190, 193, 196, 
	198, 203, 206, 208, 211, 214, 217, 221, 
	223, 228, 231, 234, 245, 249, 254, 257, 
	264, 270, 273, 276, 281, 284, 286, 297, 
	299, 302, 308, 314, 320, 327, 336, 342, 
	355, 366, 377, 384, 393
};

static const unsigned char _PRLT_indicies[] = {
	1, 2, 0, 0, 3, 3, 3, 3, 
	5, 3, 3, 3, 5, 5, 4, 6, 
	5, 5, 5, 4, 8, 9, 7, 7, 
	11, 11, 12, 10, 12, 10, 13, 10, 
	15, 15, 16, 14, 16, 14, 17, 14, 
	19, 19, 20, 18, 20, 18, 22, 21, 
	24, 22, 23, 25, 18, 26, 26, 26, 
	18, 28, 29, 27, 30, 27, 31, 27, 
	32, 27, 33, 27, 31, 27, 34, 27, 
	31, 27, 35, 27, 36, 31, 27, 37, 
	27, 38, 27, 39, 27, 40, 27, 31, 
	27, 41, 27, 31, 27, 42, 27, 43, 
	27, 44, 27, 31, 27, 45, 27, 46, 
	27, 31, 27, 47, 27, 41, 27, 48, 
	27, 31, 27, 5, 5, 5, 4, 50, 
	49, 51, 51, 51, 49, 53, 54, 52, 
	52, 56, 56, 56, 56, 56, 55, 58, 
	58, 58, 59, 58, 58, 57, 57, 61, 
	61, 61, 61, 61, 60, 63, 64, 62, 
	65, 66, 63, 63, 62, 68, 69, 0, 
	70, 71, 72, 73, 7, 74, 75, 76, 
	77, 78, 79, 80, 81, 82, 83, 84, 
	51, 85, 86, 86, 87, 88, 89, 90, 
	91, 68, 22, 51, 51, 67, 68, 68, 
	92, 6, 6, 93, 94, 70, 5, 5, 
	5, 5, 95, 96, 6, 93, 6, 18, 
	96, 6, 93, 6, 6, 93, 6, 6, 
	93, 98, 6, 13, 97, 6, 99, 101, 
	102, 101, 13, 100, 11, 12, 100, 96, 
	6, 93, 104, 105, 106, 107, 108, 105, 
	106, 107, 24, 22, 103, 110, 110, 17, 
	109, 110, 111, 110, 17, 109, 15, 16, 
	109, 104, 106, 113, 106, 24, 22, 112, 
	104, 106, 108, 106, 22, 103, 19, 20, 
	114, 105, 25, 115, 107, 26, 26, 26, 
	116, 96, 117, 93, 6, 99, 118, 119, 
	120, 121, 122, 123, 124, 125, 6, 6, 
	93, 126, 31, 6, 96, 93, 128, 51, 
	51, 51, 51, 127, 128, 51, 51, 51, 
	51, 127, 128, 51, 51, 51, 51, 127, 
	52, 128, 51, 51, 51, 51, 127, 53, 
	53, 53, 53, 53, 53, 53, 53, 129, 
	128, 51, 51, 51, 51, 127, 55, 55, 
	55, 128, 55, 51, 130, 131, 51, 51, 
	51, 55, 127, 57, 57, 57, 128, 57, 
	51, 51, 51, 51, 57, 127, 60, 60, 
	60, 128, 60, 51, 51, 51, 51, 60, 
	127, 62, 128, 51, 51, 51, 51, 127, 
	65, 65, 65, 65, 65, 65, 65, 65, 
	132, 6, 96, 93, 0
};

static const char _PRLT_trans_targs[] = {
	1, 60, 2, 60, 0, 64, 60, 5, 
	60, 6, 60, 8, 73, 72, 60, 11, 
	78, 77, 60, 14, 81, 60, 80, 60, 
	79, 82, 83, 60, 20, 22, 21, 87, 
	23, 24, 26, 28, 29, 30, 31, 32, 
	33, 35, 37, 38, 39, 41, 42, 44, 
	46, 60, 49, 89, 50, 93, 51, 52, 
	60, 53, 60, 54, 55, 60, 56, 57, 
	59, 99, 58, 60, 61, 62, 63, 3, 
	4, 65, 67, 68, 69, 70, 74, 75, 
	84, 86, 88, 47, 66, 90, 91, 92, 
	94, 95, 98, 100, 60, 60, 60, 60, 
	66, 60, 71, 60, 60, 7, 9, 60, 
	76, 17, 13, 18, 15, 60, 10, 12, 
	60, 16, 60, 60, 60, 85, 19, 25, 
	27, 34, 36, 40, 43, 45, 60, 60, 
	48, 60, 96, 97, 60
};

static const char _PRLT_trans_actions[] = {
	0, 13, 0, 17, 0, 0, 19, 0, 
	15, 0, 61, 0, 5, 5, 59, 0, 
	5, 5, 69, 0, 80, 63, 83, 57, 
	77, 74, 71, 67, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 65, 0, 5, 0, 0, 0, 0, 
	11, 0, 9, 0, 0, 7, 0, 0, 
	0, 0, 0, 21, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 83, 
	0, 5, 0, 0, 89, 5, 5, 5, 
	5, 5, 5, 0, 55, 51, 23, 45, 
	86, 53, 0, 49, 39, 0, 0, 43, 
	5, 0, 0, 0, 0, 37, 0, 0, 
	35, 0, 41, 33, 31, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 25, 47, 
	0, 27, 5, 5, 29
};

static const char _PRLT_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0
};

static const char _PRLT_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 3, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0
};

static const short _PRLT_eof_trans[] = {
	0, 0, 0, 0, 0, 0, 0, 11, 
	11, 11, 15, 15, 15, 19, 19, 22, 
	24, 19, 19, 28, 28, 28, 28, 28, 
	28, 28, 28, 28, 28, 28, 28, 28, 
	28, 28, 28, 28, 28, 28, 28, 28, 
	28, 28, 28, 28, 28, 28, 28, 0, 
	50, 50, 50, 50, 50, 50, 50, 50, 
	50, 50, 50, 50, 0, 93, 94, 95, 
	96, 94, 19, 94, 94, 94, 98, 100, 
	101, 101, 94, 104, 110, 110, 110, 113, 
	104, 115, 116, 117, 94, 100, 94, 127, 
	94, 128, 128, 128, 128, 130, 128, 128, 
	128, 128, 128, 133, 94
};

static const int PRLT_start = 60;
static const int PRLT_first_final = 60;
static const int PRLT_error = 0;

static const int PRLT_en_main = 60;


/* #line 132 "PRLT.c.rl" */

ok64 PRLTLexer(PRLTstate* state) {

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

    
/* #line 309 "PRLT.rl.c" */
	{
	cs = PRLT_start;
	ts = 0;
	te = 0;
	act = 0;
	}

/* #line 150 "PRLT.c.rl" */
    
/* #line 315 "PRLT.rl.c" */
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
	_acts = _PRLT_actions + _PRLT_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 1:
/* #line 1 "NONE" */
	{ts = p;}
	break;
/* #line 334 "PRLT.rl.c" */
		}
	}

	_keys = _PRLT_trans_keys + _PRLT_key_offsets[cs];
	_trans = _PRLT_index_offsets[cs];

	_klen = _PRLT_single_lengths[cs];
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

	_klen = _PRLT_range_lengths[cs];
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
	_trans = _PRLT_indicies[_trans];
_eof_trans:
	cs = _PRLT_trans_targs[_trans];

	if ( _PRLT_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _PRLT_actions + _PRLT_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 2:
/* #line 1 "NONE" */
	{te = p+1;}
	break;
	case 3:
/* #line 42 "PRLT.c.rl" */
	{act = 10;}
	break;
	case 4:
/* #line 42 "PRLT.c.rl" */
	{act = 11;}
	break;
	case 5:
/* #line 42 "PRLT.c.rl" */
	{act = 12;}
	break;
	case 6:
/* #line 42 "PRLT.c.rl" */
	{act = 15;}
	break;
	case 7:
/* #line 42 "PRLT.c.rl" */
	{act = 16;}
	break;
	case 8:
/* #line 54 "PRLT.c.rl" */
	{act = 20;}
	break;
	case 9:
/* #line 54 "PRLT.c.rl" */
	{act = 21;}
	break;
	case 10:
/* #line 36 "PRLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 11:
/* #line 36 "PRLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 12:
/* #line 36 "PRLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 13:
/* #line 36 "PRLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 14:
/* #line 36 "PRLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 15:
/* #line 48 "PRLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 16:
/* #line 54 "PRLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 17:
/* #line 54 "PRLT.c.rl" */
	{te = p+1;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 18:
/* #line 30 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 19:
/* #line 30 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonComment(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 20:
/* #line 36 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 21:
/* #line 36 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonString(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 22:
/* #line 42 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 23:
/* #line 42 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 24:
/* #line 42 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 25:
/* #line 42 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 26:
/* #line 42 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 27:
/* #line 42 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 28:
/* #line 42 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 29:
/* #line 48 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 30:
/* #line 48 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 31:
/* #line 54 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 32:
/* #line 54 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 33:
/* #line 54 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 34:
/* #line 60 "PRLT.c.rl" */
	{te = p;p--;{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonSpace(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 35:
/* #line 42 "PRLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 36:
/* #line 42 "PRLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 37:
/* #line 42 "PRLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 38:
/* #line 42 "PRLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 39:
/* #line 48 "PRLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonWord(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 40:
/* #line 54 "PRLT.c.rl" */
	{{p = ((te))-1;}{
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}}
	break;
	case 41:
/* #line 1 "NONE" */
	{	switch( act ) {
	case 10:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 11:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 12:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 15:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 16:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonNumber(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 20:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	case 21:
	{{p = ((te))-1;}
    tok[0] = (u8c*)ts;
    tok[1] = (u8c*)te;
    o = PRLTonPunct(tok, state);
    if (o!=OK) {p++; goto _out; }
}
	break;
	}
	}
	break;
/* #line 732 "PRLT.rl.c" */
		}
	}

_again:
	_acts = _PRLT_actions + _PRLT_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
/* #line 1 "NONE" */
	{ts = 0;}
	break;
/* #line 743 "PRLT.rl.c" */
		}
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _PRLT_eof_trans[cs] > 0 ) {
		_trans = _PRLT_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

/* #line 151 "PRLT.c.rl" */

    state->data[0] = p;
    if (o==OK && cs < PRLT_first_final)
        o = PRLTBAD;

    return o;
}
