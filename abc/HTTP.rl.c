
#line 1 "HTTP.rl"
#include "HTTP.rl.h"



#line 199 "HTTP.rl"



#line 7 "HTTP.rl.c"
static const char _HTTP_actions[] = {
	0, 1, 1, 1, 2, 1, 3, 1, 
	4, 1, 5, 1, 7, 1, 9, 1, 
	10, 1, 11, 1, 13, 1, 16, 1, 
	20, 1, 21, 1, 22, 1, 23, 1, 
	25, 2, 10, 11, 2, 15, 16, 3, 
	7, 12, 8, 3, 13, 12, 8, 3, 
	14, 6, 0, 3, 25, 12, 8, 4, 
	17, 19, 29, 31, 4, 17, 27, 29, 
	31, 4, 30, 28, 18, 26, 5, 14, 
	6, 0, 24, 4, 5, 16, 17, 27, 
	29, 31, 6, 15, 16, 17, 19, 29, 
	31, 6, 30, 28, 18, 14, 6, 0, 
	9, 30, 28, 18, 14, 6, 0, 26, 
	24, 4
};

static const short _HTTP_key_offsets[] = {
	0, 0, 10, 20, 21, 23, 24, 25, 
	26, 27, 28, 29, 30, 34, 35, 36, 
	37, 38, 39, 40, 42, 43, 45, 46, 
	47, 63, 64, 80, 86, 87, 105, 111, 
	112, 130, 131, 132, 133, 134, 135, 136, 
	138, 139, 140, 141, 142, 143, 145, 146, 
	148, 149, 151, 153, 155, 156, 162, 168, 
	169, 187, 188, 204, 210, 211, 229, 235, 
	236, 254, 255, 257, 258, 259, 260, 261, 
	262, 263, 265, 266, 267, 268, 269, 269, 
	269, 269
};

static const unsigned char _HTTP_trans_keys[] = {
	9u, 13u, 32u, 67u, 68u, 71u, 72u, 79u, 
	80u, 84u, 9u, 13u, 32u, 67u, 68u, 71u, 
	72u, 79u, 80u, 84u, 10u, 9u, 32u, 79u, 
	78u, 78u, 69u, 67u, 84u, 32u, 13u, 32u, 
	9u, 10u, 32u, 72u, 84u, 84u, 80u, 47u, 
	48u, 57u, 46u, 48u, 57u, 13u, 10u, 13u, 
	33u, 124u, 126u, 35u, 39u, 42u, 43u, 45u, 
	46u, 48u, 57u, 65u, 90u, 94u, 122u, 10u, 
	33u, 58u, 124u, 126u, 35u, 39u, 42u, 43u, 
	45u, 46u, 48u, 57u, 65u, 90u, 94u, 122u, 
	9u, 13u, 32u, 127u, 0u, 31u, 10u, 9u, 
	13u, 32u, 33u, 124u, 126u, 35u, 39u, 42u, 
	43u, 45u, 46u, 48u, 57u, 65u, 90u, 94u, 
	122u, 13u, 127u, 0u, 8u, 10u, 31u, 10u, 
	9u, 13u, 32u, 33u, 124u, 126u, 35u, 39u, 
	42u, 43u, 45u, 46u, 48u, 57u, 65u, 90u, 
	94u, 122u, 69u, 76u, 69u, 84u, 69u, 69u, 
	69u, 84u, 65u, 68u, 84u, 80u, 47u, 48u, 
	57u, 46u, 48u, 57u, 32u, 48u, 57u, 48u, 
	57u, 48u, 57u, 32u, 13u, 127u, 0u, 8u, 
	10u, 31u, 13u, 127u, 0u, 8u, 10u, 31u, 
	10u, 9u, 13u, 32u, 33u, 124u, 126u, 35u, 
	39u, 42u, 43u, 45u, 46u, 48u, 57u, 65u, 
	90u, 94u, 122u, 10u, 33u, 58u, 124u, 126u, 
	35u, 39u, 42u, 43u, 45u, 46u, 48u, 57u, 
	65u, 90u, 94u, 122u, 9u, 13u, 32u, 127u, 
	0u, 31u, 10u, 9u, 13u, 32u, 33u, 124u, 
	126u, 35u, 39u, 42u, 43u, 45u, 46u, 48u, 
	57u, 65u, 90u, 94u, 122u, 13u, 127u, 0u, 
	8u, 10u, 31u, 10u, 9u, 13u, 32u, 33u, 
	124u, 126u, 35u, 39u, 42u, 43u, 45u, 46u, 
	48u, 57u, 65u, 90u, 94u, 122u, 10u, 9u, 
	32u, 80u, 84u, 73u, 79u, 78u, 83u, 79u, 
	85u, 83u, 82u, 65u, 67u, 0
};

static const char _HTTP_single_lengths[] = {
	0, 10, 10, 1, 2, 1, 1, 1, 
	1, 1, 1, 1, 2, 1, 1, 1, 
	1, 1, 1, 0, 1, 0, 1, 1, 
	4, 1, 4, 4, 1, 6, 2, 1, 
	6, 1, 1, 1, 1, 1, 1, 2, 
	1, 1, 1, 1, 1, 0, 1, 0, 
	1, 0, 0, 0, 1, 2, 2, 1, 
	6, 1, 4, 4, 1, 6, 2, 1, 
	6, 1, 2, 1, 1, 1, 1, 1, 
	1, 2, 1, 1, 1, 1, 0, 0, 
	0, 0
};

static const char _HTTP_range_lengths[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 1, 0, 0, 0, 
	0, 0, 0, 1, 0, 1, 0, 0, 
	6, 0, 6, 1, 0, 6, 2, 0, 
	6, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 1, 0, 1, 
	0, 1, 1, 1, 0, 2, 2, 0, 
	6, 0, 6, 1, 0, 6, 2, 0, 
	6, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0
};

static const short _HTTP_index_offsets[] = {
	0, 0, 11, 22, 24, 27, 29, 31, 
	33, 35, 37, 39, 41, 45, 47, 49, 
	51, 53, 55, 57, 59, 61, 63, 65, 
	67, 78, 80, 91, 97, 99, 112, 117, 
	119, 132, 134, 136, 138, 140, 142, 144, 
	147, 149, 151, 153, 155, 157, 159, 161, 
	163, 165, 167, 169, 171, 173, 178, 183, 
	185, 198, 200, 211, 217, 219, 232, 237, 
	239, 252, 254, 257, 259, 261, 263, 265, 
	267, 269, 272, 274, 276, 278, 280, 281, 
	282, 283
};

static const char _HTTP_indicies[] = {
	0, 2, 0, 3, 4, 5, 6, 7, 
	8, 9, 1, 10, 11, 10, 12, 13, 
	14, 15, 16, 17, 18, 1, 19, 1, 
	10, 10, 1, 20, 1, 21, 1, 22, 
	1, 23, 1, 24, 1, 25, 1, 26, 
	1, 1, 1, 1, 27, 28, 1, 29, 
	1, 30, 1, 31, 1, 32, 1, 33, 
	1, 34, 1, 35, 1, 36, 1, 37, 
	1, 38, 1, 39, 40, 40, 40, 40, 
	40, 40, 40, 40, 40, 1, 41, 1, 
	42, 43, 42, 42, 42, 42, 42, 42, 
	42, 42, 1, 44, 45, 44, 1, 1, 
	46, 47, 1, 48, 49, 48, 50, 50, 
	50, 50, 50, 50, 50, 50, 50, 1, 
	52, 1, 1, 1, 51, 53, 1, 51, 
	49, 51, 50, 50, 50, 50, 50, 50, 
	50, 50, 50, 1, 54, 1, 55, 1, 
	56, 1, 57, 1, 25, 1, 24, 1, 
	58, 59, 1, 60, 1, 25, 1, 61, 
	1, 62, 1, 63, 1, 64, 1, 65, 
	1, 66, 1, 67, 1, 68, 1, 69, 
	1, 70, 1, 71, 1, 73, 1, 1, 
	1, 72, 75, 1, 1, 1, 74, 76, 
	1, 74, 77, 74, 78, 78, 78, 78, 
	78, 78, 78, 78, 78, 1, 79, 1, 
	80, 81, 80, 80, 80, 80, 80, 80, 
	80, 80, 1, 82, 83, 82, 1, 1, 
	84, 85, 1, 86, 87, 86, 88, 88, 
	88, 88, 88, 88, 88, 88, 88, 1, 
	90, 1, 1, 1, 89, 91, 1, 89, 
	87, 89, 88, 88, 88, 88, 88, 88, 
	88, 88, 88, 1, 92, 1, 74, 74, 
	1, 93, 1, 94, 1, 95, 1, 96, 
	1, 97, 1, 25, 1, 98, 24, 1, 
	24, 1, 99, 1, 100, 1, 57, 1, 
	101, 102, 103, 104, 0
};

static const char _HTTP_trans_targs[] = {
	2, 0, 3, 5, 33, 38, 39, 67, 
	73, 75, 2, 3, 5, 33, 38, 39, 
	67, 73, 75, 4, 6, 7, 8, 9, 
	10, 11, 12, 13, 14, 15, 16, 17, 
	18, 19, 20, 21, 22, 23, 24, 25, 
	26, 78, 26, 27, 27, 28, 30, 29, 
	27, 25, 26, 30, 31, 32, 34, 35, 
	36, 37, 40, 42, 41, 43, 44, 45, 
	46, 47, 48, 49, 50, 51, 52, 53, 
	54, 65, 54, 55, 56, 57, 58, 80, 
	58, 59, 59, 60, 62, 61, 59, 57, 
	58, 62, 63, 64, 66, 68, 69, 70, 
	71, 72, 74, 76, 77, 79, 79, 81, 
	81
};

static const char _HTTP_trans_actions[] = {
	65, 0, 65, 89, 89, 89, 96, 89, 
	89, 89, 0, 0, 47, 47, 47, 70, 
	47, 47, 47, 0, 0, 0, 0, 0, 
	0, 0, 1, 3, 5, 7, 0, 0, 
	0, 0, 0, 0, 0, 9, 0, 11, 
	39, 0, 0, 13, 15, 33, 15, 0, 
	0, 19, 43, 0, 17, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 9, 23, 0, 0, 25, 
	27, 27, 0, 29, 0, 31, 51, 0, 
	0, 13, 15, 33, 15, 0, 0, 19, 
	43, 0, 17, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 36, 0, 21, 
	0
};

static const char _HTTP_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 82, 55, 
	76, 60
};

static const int HTTP_start = 1;
static const int HTTP_first_final = 78;
static const int HTTP_error = 0;

static const int HTTP_en_main = 1;


#line 202 "HTTP.rl"

ok64 HTTPlexer(HTTPstate* state) {

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

    
#line 227 "HTTP.rl.c"
	{
	cs = HTTP_start;
	}

#line 220 "HTTP.rl"
    
#line 230 "HTTP.rl.c"
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
	_keys = _HTTP_trans_keys + _HTTP_key_offsets[cs];
	_trans = _HTTP_index_offsets[cs];

	_klen = _HTTP_single_lengths[cs];
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

	_klen = _HTTP_range_lengths[cs];
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
	_trans = _HTTP_indicies[_trans];
	cs = _HTTP_trans_targs[_trans];

	if ( _HTTP_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _HTTP_actions + _HTTP_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 10 "HTTP.rl"
	{ mark0[HTTPMethod] = p - text[0]; }
	break;
	case 1:
#line 11 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPMethod];
    tok[1] = p;
    call(HTTPonMethod, tok, state); 
}
	break;
	case 2:
#line 16 "HTTP.rl"
	{ mark0[HTTPRequestURI] = p - text[0]; }
	break;
	case 3:
#line 17 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPRequestURI];
    tok[1] = p;
    call(HTTPonRequestURI, tok, state); 
}
	break;
	case 4:
#line 22 "HTTP.rl"
	{ mark0[HTTPHTTPVersion] = p - text[0]; }
	break;
	case 5:
#line 23 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPHTTPVersion];
    tok[1] = p;
    call(HTTPonHTTPVersion, tok, state); 
}
	break;
	case 6:
#line 28 "HTTP.rl"
	{ mark0[HTTPRequestLine] = p - text[0]; }
	break;
	case 7:
#line 29 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPRequestLine];
    tok[1] = p;
    call(HTTPonRequestLine, tok, state); 
}
	break;
	case 8:
#line 34 "HTTP.rl"
	{ mark0[HTTPFieldName] = p - text[0]; }
	break;
	case 9:
#line 35 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPFieldName];
    tok[1] = p;
    call(HTTPonFieldName, tok, state); 
}
	break;
	case 10:
#line 40 "HTTP.rl"
	{ mark0[HTTPFieldValue] = p - text[0]; }
	break;
	case 11:
#line 41 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPFieldValue];
    tok[1] = p;
    call(HTTPonFieldValue, tok, state); 
}
	break;
	case 12:
#line 46 "HTTP.rl"
	{ mark0[HTTPMessageHeader] = p - text[0]; }
	break;
	case 13:
#line 47 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPMessageHeader];
    tok[1] = p;
    call(HTTPonMessageHeader, tok, state); 
}
	break;
	case 14:
#line 52 "HTTP.rl"
	{ mark0[HTTPRequestHead] = p - text[0]; }
	break;
	case 15:
#line 53 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPRequestHead];
    tok[1] = p;
    call(HTTPonRequestHead, tok, state); 
}
	break;
	case 16:
#line 58 "HTTP.rl"
	{ mark0[HTTPBody] = p - text[0]; }
	break;
	case 18:
#line 64 "HTTP.rl"
	{ mark0[HTTPRequest] = p - text[0]; }
	break;
	case 20:
#line 70 "HTTP.rl"
	{ mark0[HTTPStatusCode] = p - text[0]; }
	break;
	case 21:
#line 71 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPStatusCode];
    tok[1] = p;
    call(HTTPonStatusCode, tok, state); 
}
	break;
	case 22:
#line 76 "HTTP.rl"
	{ mark0[HTTPReasonPhrase] = p - text[0]; }
	break;
	case 23:
#line 77 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPReasonPhrase];
    tok[1] = p;
    call(HTTPonReasonPhrase, tok, state); 
}
	break;
	case 24:
#line 82 "HTTP.rl"
	{ mark0[HTTPStatusLine] = p - text[0]; }
	break;
	case 25:
#line 83 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPStatusLine];
    tok[1] = p;
    call(HTTPonStatusLine, tok, state); 
}
	break;
	case 26:
#line 88 "HTTP.rl"
	{ mark0[HTTPResponse] = p - text[0]; }
	break;
	case 28:
#line 94 "HTTP.rl"
	{ mark0[HTTPMessage] = p - text[0]; }
	break;
	case 30:
#line 100 "HTTP.rl"
	{ mark0[HTTPRoot] = p - text[0]; }
	break;
#line 428 "HTTP.rl.c"
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
	const char *__acts = _HTTP_actions + _HTTP_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 15:
#line 53 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPRequestHead];
    tok[1] = p;
    call(HTTPonRequestHead, tok, state); 
}
	break;
	case 16:
#line 58 "HTTP.rl"
	{ mark0[HTTPBody] = p - text[0]; }
	break;
	case 17:
#line 59 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPBody];
    tok[1] = p;
    call(HTTPonBody, tok, state); 
}
	break;
	case 19:
#line 65 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPRequest];
    tok[1] = p;
    call(HTTPonRequest, tok, state); 
}
	break;
	case 27:
#line 89 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPResponse];
    tok[1] = p;
    call(HTTPonResponse, tok, state); 
}
	break;
	case 29:
#line 95 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPMessage];
    tok[1] = p;
    call(HTTPonMessage, tok, state); 
}
	break;
	case 31:
#line 101 "HTTP.rl"
	{
    tok[0] = text[0] + mark0[HTTPRoot];
    tok[1] = p;
    call(HTTPonRoot, tok, state); 
}
	break;
#line 488 "HTTP.rl.c"
		}
	}
	}

	_out: {}
	}

#line 221 "HTTP.rl"

    state->text[0] = p;
    if (p!=text[1] || cs < HTTP_first_final) {
        return HTTPfail;
    }
    done;
}
