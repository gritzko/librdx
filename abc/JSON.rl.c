
#line 1 "JSON.rl"
#include "JSON.rl.h"



#line 135 "JSON.rl"



#line 7 "JSON.rl.c"
static const char _JSON_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8, 1, 9, 1, 10, 1, 
	11, 1, 12, 1, 13, 1, 14, 1, 
	15, 1, 16, 1, 17, 2, 1, 6, 
	2, 1, 8, 2, 1, 10, 2, 1, 
	12, 2, 1, 14, 2, 1, 16, 2, 
	3, 6, 2, 3, 8, 2, 3, 10, 
	2, 3, 12, 2, 3, 14, 2, 3, 
	16, 2, 5, 6, 2, 5, 8, 2, 
	5, 10, 2, 5, 12, 2, 5, 14, 
	2, 5, 16, 2, 7, 0, 2, 7, 
	2, 2, 7, 4, 2, 7, 6, 2, 
	7, 8, 2, 7, 10, 2, 7, 12, 
	2, 7, 14, 2, 7, 16, 2, 9, 
	0, 2, 9, 2, 2, 9, 4, 2, 
	9, 6, 2, 9, 8, 2, 9, 10, 
	2, 9, 12, 2, 9, 14, 2, 9, 
	16, 2, 11, 0, 2, 11, 2, 2, 
	11, 4, 2, 11, 6, 2, 11, 8, 
	2, 11, 10, 2, 11, 12, 2, 11, 
	14, 2, 11, 16, 2, 13, 0, 2, 
	13, 2, 2, 13, 4, 2, 13, 6, 
	2, 13, 8, 2, 13, 10, 2, 13, 
	12, 2, 13, 14, 2, 13, 16, 2, 
	15, 0, 2, 15, 2, 2, 15, 4, 
	2, 15, 6, 2, 15, 8, 2, 15, 
	10, 2, 15, 12, 2, 15, 14, 2, 
	15, 16, 2, 17, 0, 2, 17, 2, 
	2, 17, 4, 2, 17, 6, 2, 17, 
	8, 2, 17, 10, 2, 17, 12, 2, 
	17, 14, 2, 17, 16, 2, 19, 21, 
	2, 20, 18, 3, 1, 19, 21, 3, 
	3, 19, 21, 3, 5, 19, 21, 3, 
	7, 19, 21, 3, 9, 19, 21, 3, 
	11, 19, 21, 3, 13, 19, 21, 3, 
	15, 19, 21, 3, 17, 19, 21, 3, 
	20, 18, 0, 3, 20, 18, 2, 3, 
	20, 18, 4, 3, 20, 18, 6, 3, 
	20, 18, 8, 3, 20, 18, 10, 3, 
	20, 18, 12, 3, 20, 18, 14, 3, 
	20, 18, 16, 4, 20, 18, 19, 21
	
};

static const short _JSON_key_offsets[] = {
	0, 0, 4, 7, 9, 13, 15, 16, 
	17, 18, 19, 20, 21, 22, 23, 24, 
	33, 39, 45, 51, 57, 75, 93, 103, 
	113, 131, 144, 158, 176, 191, 203, 221, 
	239, 249, 267
};

static const unsigned char _JSON_trans_keys[] = {
	34u, 92u, 0u, 31u, 48u, 49u, 57u, 48u, 
	57u, 43u, 45u, 48u, 57u, 48u, 57u, 97u, 
	108u, 115u, 101u, 117u, 108u, 108u, 114u, 117u, 
	34u, 47u, 92u, 98u, 102u, 110u, 114u, 116u, 
	117u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 48u, 57u, 65u, 
	70u, 97u, 102u, 48u, 57u, 65u, 70u, 97u, 
	102u, 13u, 32u, 34u, 44u, 45u, 48u, 58u, 
	91u, 93u, 102u, 110u, 116u, 123u, 125u, 9u, 
	10u, 49u, 57u, 13u, 32u, 34u, 44u, 45u, 
	48u, 58u, 91u, 93u, 102u, 110u, 116u, 123u, 
	125u, 9u, 10u, 49u, 57u, 13u, 32u, 44u, 
	58u, 91u, 93u, 123u, 125u, 9u, 10u, 13u, 
	32u, 44u, 58u, 91u, 93u, 123u, 125u, 9u, 
	10u, 13u, 32u, 34u, 44u, 45u, 48u, 58u, 
	91u, 93u, 102u, 110u, 116u, 123u, 125u, 9u, 
	10u, 49u, 57u, 13u, 32u, 44u, 46u, 58u, 
	69u, 91u, 93u, 101u, 123u, 125u, 9u, 10u, 
	13u, 32u, 44u, 58u, 69u, 91u, 93u, 101u, 
	123u, 125u, 9u, 10u, 48u, 57u, 13u, 32u, 
	34u, 44u, 45u, 48u, 58u, 91u, 93u, 102u, 
	110u, 116u, 123u, 125u, 9u, 10u, 49u, 57u, 
	13u, 32u, 44u, 46u, 58u, 69u, 91u, 93u, 
	101u, 123u, 125u, 9u, 10u, 48u, 57u, 13u, 
	32u, 44u, 58u, 91u, 93u, 123u, 125u, 9u, 
	10u, 48u, 57u, 13u, 32u, 34u, 44u, 45u, 
	48u, 58u, 91u, 93u, 102u, 110u, 116u, 123u, 
	125u, 9u, 10u, 49u, 57u, 13u, 32u, 34u, 
	44u, 45u, 48u, 58u, 91u, 93u, 102u, 110u, 
	116u, 123u, 125u, 9u, 10u, 49u, 57u, 13u, 
	32u, 44u, 58u, 91u, 93u, 123u, 125u, 9u, 
	10u, 13u, 32u, 34u, 44u, 45u, 48u, 58u, 
	91u, 93u, 102u, 110u, 116u, 123u, 125u, 9u, 
	10u, 49u, 57u, 13u, 32u, 34u, 44u, 45u, 
	48u, 58u, 91u, 93u, 102u, 110u, 116u, 123u, 
	125u, 9u, 10u, 49u, 57u, 0
};

static const char _JSON_single_lengths[] = {
	0, 2, 1, 0, 2, 0, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 9, 
	0, 0, 0, 0, 14, 14, 8, 8, 
	14, 11, 10, 14, 11, 8, 14, 14, 
	8, 14, 14
};

static const char _JSON_range_lengths[] = {
	0, 1, 1, 1, 1, 1, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	3, 3, 3, 3, 2, 2, 1, 1, 
	2, 1, 2, 2, 2, 2, 2, 2, 
	1, 2, 2
};

static const short _JSON_index_offsets[] = {
	0, 0, 4, 7, 9, 13, 15, 17, 
	19, 21, 23, 25, 27, 29, 31, 33, 
	43, 47, 51, 55, 59, 76, 93, 103, 
	113, 130, 143, 156, 173, 187, 198, 215, 
	232, 242, 259
};

static const unsigned char _JSON_indicies[] = {
	2, 3, 1, 0, 4, 5, 1, 6, 
	1, 7, 7, 8, 1, 8, 1, 9, 
	1, 10, 1, 11, 1, 12, 1, 13, 
	1, 14, 1, 12, 1, 15, 1, 11, 
	1, 0, 0, 0, 0, 0, 0, 0, 
	0, 16, 1, 17, 17, 17, 1, 18, 
	18, 18, 1, 19, 19, 19, 1, 0, 
	0, 0, 1, 20, 20, 21, 22, 23, 
	24, 26, 27, 28, 29, 30, 31, 32, 
	33, 20, 25, 1, 34, 34, 35, 36, 
	37, 38, 40, 41, 42, 43, 44, 45, 
	46, 47, 34, 39, 1, 48, 48, 49, 
	50, 51, 52, 53, 54, 48, 1, 55, 
	55, 36, 40, 41, 42, 46, 47, 55, 
	1, 56, 56, 57, 58, 59, 60, 62, 
	63, 64, 65, 66, 67, 68, 69, 56, 
	61, 1, 70, 70, 71, 72, 73, 74, 
	75, 76, 74, 77, 78, 70, 1, 70, 
	70, 71, 73, 74, 75, 76, 74, 77, 
	78, 70, 6, 1, 79, 79, 80, 81, 
	82, 83, 85, 86, 87, 88, 89, 90, 
	91, 92, 79, 84, 1, 70, 70, 71, 
	72, 73, 74, 75, 76, 74, 77, 78, 
	70, 5, 1, 70, 70, 71, 73, 75, 
	76, 77, 78, 70, 8, 1, 93, 93, 
	94, 95, 96, 97, 99, 100, 101, 102, 
	103, 104, 105, 106, 93, 98, 1, 107, 
	107, 108, 109, 110, 111, 113, 114, 115, 
	116, 117, 118, 119, 120, 107, 112, 1, 
	121, 121, 122, 123, 124, 125, 126, 127, 
	121, 1, 128, 128, 129, 130, 131, 132, 
	134, 135, 136, 137, 138, 139, 140, 141, 
	128, 133, 1, 142, 142, 143, 144, 145, 
	146, 148, 149, 150, 151, 152, 153, 154, 
	155, 142, 147, 1, 0
};

static const char _JSON_trans_targs[] = {
	1, 0, 22, 15, 25, 28, 26, 5, 
	29, 7, 8, 9, 32, 11, 12, 14, 
	16, 17, 18, 19, 21, 1, 24, 2, 
	25, 28, 27, 30, 31, 6, 10, 13, 
	33, 34, 21, 1, 24, 2, 25, 28, 
	27, 30, 31, 6, 10, 13, 33, 34, 
	23, 24, 27, 30, 31, 33, 34, 23, 
	21, 1, 24, 2, 25, 28, 27, 30, 
	31, 6, 10, 13, 33, 34, 23, 24, 
	3, 27, 4, 30, 31, 33, 34, 21, 
	1, 24, 2, 25, 28, 27, 30, 31, 
	6, 10, 13, 33, 34, 21, 1, 24, 
	2, 25, 28, 27, 30, 31, 6, 10, 
	13, 33, 34, 21, 1, 24, 2, 25, 
	28, 27, 30, 31, 6, 10, 13, 33, 
	34, 23, 24, 27, 30, 31, 33, 34, 
	21, 1, 24, 2, 25, 28, 27, 30, 
	31, 6, 10, 13, 33, 34, 21, 1, 
	24, 2, 25, 28, 27, 30, 31, 6, 
	10, 13, 33, 34
};

static const short _JSON_trans_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 256, 299, 323, 303, 
	303, 303, 327, 315, 319, 295, 295, 295, 
	307, 311, 0, 5, 29, 9, 9, 9, 
	33, 21, 25, 1, 1, 1, 13, 17, 
	7, 67, 70, 61, 64, 55, 58, 0, 
	31, 202, 220, 205, 205, 205, 223, 214, 
	217, 199, 199, 199, 208, 211, 11, 85, 
	0, 88, 0, 79, 82, 73, 76, 35, 
	229, 247, 232, 232, 232, 250, 241, 244, 
	226, 226, 226, 235, 238, 23, 148, 166, 
	151, 151, 151, 169, 160, 163, 145, 145, 
	145, 154, 157, 27, 175, 193, 178, 178, 
	178, 196, 187, 190, 172, 172, 172, 181, 
	184, 3, 49, 52, 43, 46, 37, 40, 
	15, 94, 112, 97, 97, 97, 115, 106, 
	109, 91, 91, 91, 100, 103, 19, 121, 
	139, 124, 124, 124, 142, 133, 136, 118, 
	118, 118, 127, 130
};

static const short _JSON_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 331, 253, 263, 253, 
	287, 267, 267, 291, 267, 267, 279, 283, 
	259, 271, 275
};

static const int JSON_start = 20;
static const int JSON_first_final = 20;
static const int JSON_error = 0;

static const int JSON_en_main = 20;


#line 138 "JSON.rl"

ok64 JSONlexer(JSONstate* state) {

    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = 0;
    int res = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = pe;
    u8c *pb = p;
    u64 mark0[64] = {};

    u32 sp = 2;
    $u8c tok = {p, p};

    
#line 239 "JSON.rl.c"
	{
	cs = JSON_start;
	}

#line 156 "JSON.rl"
    
#line 242 "JSON.rl.c"
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
	_keys = _JSON_trans_keys + _JSON_key_offsets[cs];
	_trans = _JSON_index_offsets[cs];

	_klen = _JSON_single_lengths[cs];
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

	_klen = _JSON_range_lengths[cs];
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
	_trans = _JSON_indicies[_trans];
	cs = _JSON_trans_targs[_trans];

	if ( _JSON_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _JSON_actions + _JSON_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 10 "JSON.rl"
	{ mark0[JSONLiteral] = p - text[0]; }
	break;
	case 1:
#line 11 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONLiteral];
    tok[1] = p;
    call(JSONonLiteral, tok, state); 
}
	break;
	case 2:
#line 16 "JSON.rl"
	{ mark0[JSONString] = p - text[0]; }
	break;
	case 3:
#line 17 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONString];
    tok[1] = p;
    call(JSONonString, tok, state); 
}
	break;
	case 4:
#line 22 "JSON.rl"
	{ mark0[JSONNumber] = p - text[0]; }
	break;
	case 5:
#line 23 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONNumber];
    tok[1] = p;
    call(JSONonNumber, tok, state); 
}
	break;
	case 6:
#line 28 "JSON.rl"
	{ mark0[JSONOpenObject] = p - text[0]; }
	break;
	case 7:
#line 29 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONOpenObject];
    tok[1] = p;
    call(JSONonOpenObject, tok, state); 
}
	break;
	case 8:
#line 34 "JSON.rl"
	{ mark0[JSONCloseObject] = p - text[0]; }
	break;
	case 9:
#line 35 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONCloseObject];
    tok[1] = p;
    call(JSONonCloseObject, tok, state); 
}
	break;
	case 10:
#line 40 "JSON.rl"
	{ mark0[JSONOpenArray] = p - text[0]; }
	break;
	case 11:
#line 41 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONOpenArray];
    tok[1] = p;
    call(JSONonOpenArray, tok, state); 
}
	break;
	case 12:
#line 46 "JSON.rl"
	{ mark0[JSONCloseArray] = p - text[0]; }
	break;
	case 13:
#line 47 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONCloseArray];
    tok[1] = p;
    call(JSONonCloseArray, tok, state); 
}
	break;
	case 14:
#line 52 "JSON.rl"
	{ mark0[JSONComma] = p - text[0]; }
	break;
	case 15:
#line 53 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONComma];
    tok[1] = p;
    call(JSONonComma, tok, state); 
}
	break;
	case 16:
#line 58 "JSON.rl"
	{ mark0[JSONColon] = p - text[0]; }
	break;
	case 17:
#line 59 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONColon];
    tok[1] = p;
    call(JSONonColon, tok, state); 
}
	break;
	case 18:
#line 64 "JSON.rl"
	{ mark0[JSONJSON] = p - text[0]; }
	break;
	case 20:
#line 70 "JSON.rl"
	{ mark0[JSONRoot] = p - text[0]; }
	break;
#line 411 "JSON.rl.c"
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
	const char *__acts = _JSON_actions + _JSON_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 1:
#line 11 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONLiteral];
    tok[1] = p;
    call(JSONonLiteral, tok, state); 
}
	break;
	case 3:
#line 17 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONString];
    tok[1] = p;
    call(JSONonString, tok, state); 
}
	break;
	case 5:
#line 23 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONNumber];
    tok[1] = p;
    call(JSONonNumber, tok, state); 
}
	break;
	case 7:
#line 29 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONOpenObject];
    tok[1] = p;
    call(JSONonOpenObject, tok, state); 
}
	break;
	case 9:
#line 35 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONCloseObject];
    tok[1] = p;
    call(JSONonCloseObject, tok, state); 
}
	break;
	case 11:
#line 41 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONOpenArray];
    tok[1] = p;
    call(JSONonOpenArray, tok, state); 
}
	break;
	case 13:
#line 47 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONCloseArray];
    tok[1] = p;
    call(JSONonCloseArray, tok, state); 
}
	break;
	case 15:
#line 53 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONComma];
    tok[1] = p;
    call(JSONonComma, tok, state); 
}
	break;
	case 17:
#line 59 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONColon];
    tok[1] = p;
    call(JSONonColon, tok, state); 
}
	break;
	case 18:
#line 64 "JSON.rl"
	{ mark0[JSONJSON] = p - text[0]; }
	break;
	case 19:
#line 65 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONJSON];
    tok[1] = p;
    call(JSONonJSON, tok, state); 
}
	break;
	case 20:
#line 70 "JSON.rl"
	{ mark0[JSONRoot] = p - text[0]; }
	break;
	case 21:
#line 71 "JSON.rl"
	{
    tok[0] = text[0] + mark0[JSONRoot];
    tok[1] = p;
    call(JSONonRoot, tok, state); 
}
	break;
#line 509 "JSON.rl.c"
		}
	}
	}

	_out: {}
	}

#line 157 "JSON.rl"

    state->text[0] = p;
    if (p!=text[1] || cs < JSON_first_final) {
        return JSONfail;
    }
    done;
}
