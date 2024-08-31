
#line 1 "MARK.rl"
#include "MARK.rl.h"

#line 147 "MARK.rl"

#line 7 "MARK.rl.c"
static const char _MARK_actions[] = {
    0,  2,  1,  21, 2,  3,  21, 2,  5,  21, 2,  7,  21, 2,  9,  21, 2,  11, 21,
    2,  13, 21, 2,  15, 21, 2,  17, 21, 2,  19, 21, 2,  23, 25, 2,  24, 25, 3,
    3,  4,  21, 3,  3,  16, 21, 3,  3,  18, 21, 3,  17, 4,  21, 3,  17, 16, 21,
    3,  17, 18, 21, 4,  3,  0,  6,  21, 4,  17, 0,  6,  21, 4,  23, 22, 20, 21,
    4,  24, 22, 20, 21, 5,  23, 22, 20, 4,  21, 5,  23, 22, 20, 16, 21, 5,  23,
    22, 20, 18, 21, 5,  24, 22, 20, 4,  21, 5,  24, 22, 20, 16, 21, 5,  24, 22,
    20, 18, 21, 6,  3,  8,  10, 12, 14, 21, 6,  17, 8,  10, 12, 14, 21, 6,  23,
    22, 20, 0,  6,  21, 6,  24, 22, 20, 0,  6,  21, 8,  23, 22, 20, 8,  10, 12,
    14, 21, 8,  24, 22, 20, 8,  10, 12, 14, 21, 9,  3,  2,  16, 8,  10, 12, 4,
    6,  21, 9,  17, 2,  16, 8,  10, 12, 4,  6,  21, 11, 23, 22, 20, 2,  16, 8,
    10, 12, 4,  6,  21, 11, 24, 22, 20, 2,  16, 8,  10, 12, 4,  6,  21};

static const char _MARK_key_offsets[] = {
    0,  1,  8,  15, 20, 28, 31, 33,  35,  36,  39,  41,  42, 45,
    46, 47, 50, 52, 54, 55, 57, 59,  60,  64,  66,  68,  69, 73,
    75, 77, 79, 81, 89, 96, 98, 100, 101, 104, 107, 110, 118};

static const unsigned char _MARK_trans_keys[] = {
    10u, 10u, 32u, 35u, 45u, 62u,  48u, 57u, 10u, 32u, 35u, 45u, 62u, 48u, 57u,
    10u, 32u, 35u, 45u, 62u, 10u,  32u, 35u, 45u, 62u, 91u, 48u, 57u, 10u, 32u,
    35u, 10u, 32u, 10u, 32u, 10u,  10u, 32u, 35u, 10u, 32u, 10u, 10u, 32u, 35u,
    10u, 10u, 10u, 32u, 45u, 10u,  32u, 10u, 32u, 10u, 10u, 45u, 10u, 45u, 10u,
    10u, 46u, 48u, 57u, 10u, 32u,  10u, 32u, 10u, 10u, 46u, 48u, 57u, 10u, 46u,
    10u, 32u, 10u, 32u, 10u, 32u,  10u, 32u, 35u, 45u, 62u, 91u, 48u, 57u, 10u,
    48u, 57u, 65u, 90u, 97u, 122u, 10u, 93u, 10u, 58u, 10u, 10u, 32u, 35u, 10u,
    32u, 35u, 10u, 32u, 35u, 10u,  32u, 35u, 45u, 62u, 91u, 48u, 57u, 10u, 32u,
    35u, 45u, 62u, 91u, 48u, 57u,  0};

static const char _MARK_single_lengths[] = {
    1, 5, 5, 5, 6, 3, 2, 2, 1, 3, 2, 1, 3, 1, 1, 3, 2, 2, 1, 2, 2,
    1, 2, 2, 2, 1, 2, 2, 2, 2, 2, 6, 1, 2, 2, 1, 3, 3, 3, 6, 6};

static const char _MARK_range_lengths[] = {
    0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 3, 0, 0, 0, 0, 0, 0, 1, 1};

static const unsigned char _MARK_index_offsets[] = {
    0,  2,   9,   16,  22,  30,  34,  37,  40,  42,  46,  49,  51, 55,
    57, 59,  63,  66,  69,  71,  74,  77,  79,  83,  86,  89,  91, 95,
    98, 101, 104, 107, 115, 120, 123, 126, 128, 132, 136, 140, 148};

static const char _MARK_trans_targs[] = {
    40, 0,  40, 2,  37, 16, 29, 26, 0,  40, 3,  36, 17, 30, 27, 0,  40, 4,
    8,  18, 31, 0,  40, 1,  5,  15, 28, 32, 22, 0,  40, 6,  9,  0,  40, 7,
    0,  40, 8,  0,  40, 0,  40, 10, 12, 0,  40, 11, 0,  40, 0,  40, 13, 14,
    0,  40, 0,  40, 0,  40, 16, 19, 0,  40, 17, 0,  40, 18, 0,  40, 0,  40,
    20, 0,  40, 21, 0,  40, 0,  40, 23, 26, 0,  40, 24, 0,  40, 25, 0,  40,
    0,  40, 24, 27, 0,  40, 25, 0,  40, 29, 0,  40, 30, 0,  40, 31, 0,  40,
    1,  5,  15, 28, 32, 22, 0,  40, 33, 33, 33, 0,  40, 34, 0,  40, 35, 0,
    40, 0,  40, 8,  11, 0,  40, 7,  38, 0,  40, 11, 13, 0,  40, 1,  5,  15,
    28, 32, 22, 0,  40, 1,  5,  15, 28, 32, 22, 0,  0};

static const unsigned char _MARK_trans_actions[] = {
    0,   0,   0,  0,  0,  0,   0,   0,   0,   0,  0,  0,  0,   0,   0,   0,
    0,   0,   0,  0,  0,  0,   4,   163, 117, 61, 41, 45, 37,  4,   0,   0,
    0,   0,   0,  0,  0,  0,   0,   0,   13,  13, 0,  0,  0,   0,   0,   0,
    0,   16,  16, 0,  0,  0,   0,   19,  19,  22, 22, 0,  0,   0,   0,   0,
    0,   0,   0,  0,  0,  10,  10,  0,   0,   0,  0,  0,  0,   1,   1,   0,
    0,   0,   0,  0,  0,  0,   0,   0,   0,   7,  7,  0,  0,   0,   0,   0,
    0,   0,   0,  0,  0,  0,   0,   0,   0,   0,  0,  25, 173, 124, 66,  53,
    57,  49,  25, 0,  0,  0,   0,   0,   0,   0,  0,  0,  0,   0,   28,  28,
    0,   0,   0,  0,  0,  0,   0,   0,   0,   0,  0,  0,  76,  195, 154, 138,
    105, 111, 99, 76, 71, 183, 145, 131, 87,  93, 81, 71, 0};

static const unsigned char _MARK_eof_actions[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 34, 31};

static const int MARK_start = 39;
static const int MARK_first_final = 39;
static const int MARK_error = -1;

static const int MARK_en_main = 39;

#line 150 "MARK.rl"

pro(MARKlexer, MARKstate *state) {
    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = 0;
    int res = 0;
    u8c *p = (u8c *)text[0];
    u8c *pe = (u8c *)text[1];
    u8c *eof = pe;
    u8c *pb = p;
    u64 mark0[64] = {};

    u32 sp = 2;
    $u8c tok = {p, p};

#line 169 "MARK.rl.c"
    { cs = MARK_start; }

#line 168 "MARK.rl"

#line 172 "MARK.rl.c"
    {
        int _klen;
        unsigned int _trans;
        const char *_acts;
        unsigned int _nacts;
        const unsigned char *_keys;

        if (p == pe) goto _test_eof;
    _resume:
        _keys = _MARK_trans_keys + _MARK_key_offsets[cs];
        _trans = _MARK_index_offsets[cs];

        _klen = _MARK_single_lengths[cs];
        if (_klen > 0) {
            const unsigned char *_lower = _keys;
            const unsigned char *_mid;
            const unsigned char *_upper = _keys + _klen - 1;
            while (1) {
                if (_upper < _lower) break;

                _mid = _lower + ((_upper - _lower) >> 1);
                if ((*p) < *_mid)
                    _upper = _mid - 1;
                else if ((*p) > *_mid)
                    _lower = _mid + 1;
                else {
                    _trans += (unsigned int)(_mid - _keys);
                    goto _match;
                }
            }
            _keys += _klen;
            _trans += _klen;
        }

        _klen = _MARK_range_lengths[cs];
        if (_klen > 0) {
            const unsigned char *_lower = _keys;
            const unsigned char *_mid;
            const unsigned char *_upper = _keys + (_klen << 1) - 2;
            while (1) {
                if (_upper < _lower) break;

                _mid = _lower + (((_upper - _lower) >> 1) & ~1);
                if ((*p) < _mid[0])
                    _upper = _mid - 2;
                else if ((*p) > _mid[1])
                    _lower = _mid + 2;
                else {
                    _trans += (unsigned int)((_mid - _keys) >> 1);
                    goto _match;
                }
            }
            _trans += _klen;
        }

    _match:
        cs = _MARK_trans_targs[_trans];

        if (_MARK_trans_actions[_trans] == 0) goto _again;

        _acts = _MARK_actions + _MARK_trans_actions[_trans];
        _nacts = (unsigned int)*_acts++;
        while (_nacts-- > 0) {
            switch (*_acts++) {
                case 0:
#line 10 "MARK.rl"
                {
                    mark0[MARKHLine] = p - text[0];
                } break;
                case 1:
#line 11 "MARK.rl"
                {
                    tok[0] = text[0] + mark0[MARKHLine];
                    tok[1] = p;
                    call(MARKonHLine, tok, state);
                } break;
                case 2:
#line 16 "MARK.rl"
                {
                    mark0[MARKIndent] = p - text[0];
                } break;
                case 3:
#line 17 "MARK.rl"
                {
                    tok[0] = text[0] + mark0[MARKIndent];
                    tok[1] = p;
                    call(MARKonIndent, tok, state);
                } break;
                case 4:
#line 22 "MARK.rl"
                {
                    mark0[MARKOList] = p - text[0];
                } break;
                case 5:
#line 23 "MARK.rl"
                {
                    tok[0] = text[0] + mark0[MARKOList];
                    tok[1] = p;
                    call(MARKonOList, tok, state);
                } break;
                case 6:
#line 28 "MARK.rl"
                {
                    mark0[MARKUList] = p - text[0];
                } break;
                case 7:
#line 29 "MARK.rl"
                {
                    tok[0] = text[0] + mark0[MARKUList];
                    tok[1] = p;
                    call(MARKonUList, tok, state);
                } break;
                case 8:
#line 34 "MARK.rl"
                {
                    mark0[MARKH1] = p - text[0];
                } break;
                case 9:
#line 35 "MARK.rl"
                {
                    tok[0] = text[0] + mark0[MARKH1];
                    tok[1] = p;
                    call(MARKonH1, tok, state);
                } break;
                case 10:
#line 40 "MARK.rl"
                {
                    mark0[MARKH2] = p - text[0];
                } break;
                case 11:
#line 41 "MARK.rl"
                {
                    tok[0] = text[0] + mark0[MARKH2];
                    tok[1] = p;
                    call(MARKonH2, tok, state);
                } break;
                case 12:
#line 46 "MARK.rl"
                {
                    mark0[MARKH3] = p - text[0];
                } break;
                case 13:
#line 47 "MARK.rl"
                {
                    tok[0] = text[0] + mark0[MARKH3];
                    tok[1] = p;
                    call(MARKonH3, tok, state);
                } break;
                case 14:
#line 52 "MARK.rl"
                {
                    mark0[MARKH4] = p - text[0];
                } break;
                case 15:
#line 53 "MARK.rl"
                {
                    tok[0] = text[0] + mark0[MARKH4];
                    tok[1] = p;
                    call(MARKonH4, tok, state);
                } break;
                case 16:
#line 64 "MARK.rl"
                {
                    mark0[MARKQuote] = p - text[0];
                } break;
                case 17:
#line 65 "MARK.rl"
                {
                    tok[0] = text[0] + mark0[MARKQuote];
                    tok[1] = p;
                    call(MARKonQuote, tok, state);
                } break;
                case 18:
#line 70 "MARK.rl"
                {
                    mark0[MARKLink] = p - text[0];
                } break;
                case 19:
#line 71 "MARK.rl"
                {
                    tok[0] = text[0] + mark0[MARKLink];
                    tok[1] = p;
                    call(MARKonLink, tok, state);
                } break;
                case 20:
#line 76 "MARK.rl"
                {
                    mark0[MARKDiv] = p - text[0];
                } break;
                case 21:
#line 77 "MARK.rl"
                {
                    tok[0] = text[0] + mark0[MARKDiv];
                    tok[1] = p;
                    call(MARKonDiv, tok, state);
                } break;
                case 22:
#line 82 "MARK.rl"
                {
                    mark0[MARKLine] = p - text[0];
                } break;
                case 23:
#line 83 "MARK.rl"
                {
                    tok[0] = text[0] + mark0[MARKLine];
                    tok[1] = p;
                    call(MARKonLine, tok, state);
                } break;
                case 24:
#line 88 "MARK.rl"
                {
                    mark0[MARKRoot] = p - text[0];
                } break;
#line 365 "MARK.rl.c"
            }
        }

    _again:
        if (++p != pe) goto _resume;
    _test_eof: {}
        if (p == eof) {
            const char *__acts = _MARK_actions + _MARK_eof_actions[cs];
            unsigned int __nacts = (unsigned int)*__acts++;
            while (__nacts-- > 0) {
                switch (*__acts++) {
                    case 23:
#line 83 "MARK.rl"
                    {
                        tok[0] = text[0] + mark0[MARKLine];
                        tok[1] = p;
                        call(MARKonLine, tok, state);
                    } break;
                    case 24:
#line 88 "MARK.rl"
                    {
                        mark0[MARKRoot] = p - text[0];
                    } break;
                    case 25:
#line 89 "MARK.rl"
                    {
                        tok[0] = text[0] + mark0[MARKRoot];
                        tok[1] = p;
                        call(MARKonRoot, tok, state);
                    } break;
#line 395 "MARK.rl.c"
                }
            }
        }
    }

#line 169 "MARK.rl"

    if (p != text[1] || cs < MARK_first_final) {
        fail(MARKfail);
        state->text[0] = p;
    }
    done;
}
