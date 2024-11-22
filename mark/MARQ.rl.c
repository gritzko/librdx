
#line 1 "MARQ.rl"
#include "MARQ.rl.h"

#line 116 "MARQ.rl"

#line 7 "MARQ.rl.c"
static const char _MARQ_actions[] = {
    0,  1,  17, 2,  1,  17, 2,  3,  17, 2,  5,  17, 2,  7,  17, 2,  9,  17, 2,
    11, 17, 2,  13, 17, 2,  15, 17, 2,  16, 17, 3,  1,  11, 17, 3,  5,  7,  17,
    3,  5,  11, 17, 3,  5,  15, 17, 3,  6,  2,  10, 3,  6,  14, 2,  3,  13, 11,
    17, 3,  15, 1,  17, 4,  1,  6,  2,  10, 4,  1,  6,  14, 2,  4,  3,  6,  2,
    10, 4,  3,  6,  14, 2,  4,  4,  12, 0,  10, 4,  5,  6,  2,  10, 4,  5,  6,
    14, 2,  4,  6,  7,  2,  10, 4,  6,  7,  14, 2,  4,  6,  14, 2,  10, 4,  7,
    6,  2,  10, 4,  7,  6,  14, 2,  4,  9,  6,  2,  10, 4,  9,  6,  14, 2,  4,
    11, 6,  2,  10, 4,  11, 6,  14, 2,  4,  13, 6,  2,  10, 4,  13, 6,  14, 2,
    4,  15, 6,  2,  10, 4,  15, 6,  14, 2,  4,  16, 6,  2,  10, 4,  16, 6,  14,
    2,  5,  1,  4,  12, 0,  10, 5,  1,  6,  14, 2,  10, 5,  1,  11, 6,  2,  10,
    5,  1,  11, 6,  14, 2,  5,  3,  4,  12, 0,  10, 5,  3,  6,  14, 2,  10, 5,
    4,  7,  12, 0,  10, 5,  5,  4,  12, 0,  10, 5,  5,  6,  14, 2,  10, 5,  5,
    7,  6,  2,  10, 5,  5,  7,  6,  14, 2,  5,  5,  11, 6,  2,  10, 5,  5,  11,
    6,  14, 2,  5,  5,  15, 6,  2,  10, 5,  5,  15, 6,  14, 2,  5,  6,  7,  14,
    2,  10, 5,  6,  8,  14, 2,  10, 5,  7,  4,  12, 0,  10, 5,  7,  6,  14, 2,
    10, 5,  9,  4,  12, 0,  10, 5,  9,  6,  14, 2,  10, 5,  11, 4,  12, 0,  10,
    5,  11, 6,  14, 2,  10, 5,  13, 4,  12, 0,  10, 5,  13, 6,  14, 2,  10, 5,
    13, 11, 6,  2,  10, 5,  13, 11, 6,  14, 2,  5,  15, 1,  6,  2,  10, 5,  15,
    1,  6,  14, 2,  5,  15, 4,  12, 0,  10, 5,  15, 6,  14, 2,  10, 5,  16, 4,
    12, 0,  10, 5,  16, 6,  14, 2,  10, 6,  1,  6,  8,  14, 2,  10, 6,  1,  11,
    4,  12, 0,  10, 6,  1,  11, 6,  14, 2,  10, 6,  3,  6,  8,  14, 2,  10, 6,
    5,  6,  8,  14, 2,  10, 6,  5,  7,  4,  12, 0,  10, 6,  5,  7,  6,  14, 2,
    10, 6,  5,  11, 4,  12, 0,  10, 6,  5,  11, 6,  14, 2,  10, 6,  5,  15, 4,
    12, 0,  10, 6,  5,  15, 6,  14, 2,  10, 6,  6,  7,  8,  14, 2,  10, 6,  9,
    6,  8,  14, 2,  10, 6,  11, 6,  8,  14, 2,  10, 6,  13, 6,  8,  14, 2,  10,
    6,  13, 11, 4,  12, 0,  10, 6,  13, 11, 6,  14, 2,  10, 6,  15, 1,  4,  12,
    0,  10, 6,  15, 1,  6,  14, 2,  10, 6,  15, 6,  8,  14, 2,  10, 6,  16, 6,
    8,  14, 2,  10, 7,  1,  11, 6,  8,  14, 2,  10, 7,  4,  6,  12, 14, 0,  2,
    10, 7,  13, 11, 6,  8,  14, 2,  10, 7,  15, 1,  6,  8,  14, 2,  10, 8,  1,
    4,  6,  12, 14, 0,  2,  10, 8,  3,  4,  6,  12, 14, 0,  2,  10, 8,  4,  6,
    7,  12, 14, 0,  2,  10, 8,  5,  4,  6,  12, 14, 0,  2,  10, 8,  7,  4,  6,
    12, 14, 0,  2,  10, 8,  9,  4,  6,  12, 14, 0,  2,  10, 8,  11, 4,  6,  12,
    14, 0,  2,  10, 8,  13, 4,  6,  12, 14, 0,  2,  10, 8,  15, 4,  6,  12, 14,
    0,  2,  10, 8,  16, 4,  6,  12, 14, 0,  2,  10, 9,  1,  11, 4,  6,  12, 14,
    0,  2,  10, 9,  5,  7,  4,  6,  12, 14, 0,  2,  10, 9,  5,  11, 4,  6,  12,
    14, 0,  2,  10, 9,  5,  15, 4,  6,  12, 14, 0,  2,  10, 9,  13, 11, 4,  6,
    12, 14, 0,  2,  10, 9,  15, 1,  4,  6,  12, 14, 0,  2,  10};

static const short _MARQ_key_offsets[] = {
    0,    16,   34,   52,   71,   89,   107,  125,  143,  160,  179,
    203,  221,  240,  258,  276,  294,  313,  331,  349,  367,  385,
    402,  421,  445,  463,  482,  498,  516,  534,  552,  570,  589,
    607,  625,  643,  662,  680,  697,  716,  734,  752,  769,  788,
    806,  824,  842,  861,  879,  896,  915,  933,  952,  970,  988,
    1007, 1025, 1043, 1061, 1080, 1098, 1117, 1135, 1153, 1170, 1189,
    1207, 1225, 1243, 1260, 1279, 1297, 1315};

static const unsigned char _MARQ_trans_keys[] = {
    13u, 32u, 42u, 63u, 92u, 95u, 9u,  10u,  33u, 34u,  39u, 41u, 44u, 46u, 58u,
    59u, 13u, 32u, 42u, 63u, 92u, 93u, 95u,  96u, 9u,   10u, 33u, 34u, 39u, 41u,
    44u, 46u, 58u, 59u, 13u, 32u, 42u, 63u,  91u, 92u,  95u, 96u, 9u,  10u, 33u,
    34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u,  32u, 42u,  63u, 91u, 92u, 93u, 95u,
    96u, 9u,  10u, 33u, 34u, 39u, 41u, 44u,  46u, 58u,  59u, 13u, 32u, 42u, 63u,
    92u, 93u, 95u, 96u, 9u,  10u, 33u, 34u,  39u, 41u,  44u, 46u, 58u, 59u, 13u,
    32u, 42u, 63u, 92u, 93u, 95u, 96u, 9u,   10u, 33u,  34u, 39u, 41u, 44u, 46u,
    58u, 59u, 13u, 32u, 42u, 63u, 92u, 93u,  95u, 96u,  9u,  10u, 33u, 34u, 39u,
    41u, 44u, 46u, 58u, 59u, 13u, 32u, 42u,  63u, 92u,  93u, 95u, 96u, 9u,  10u,
    33u, 34u, 39u, 41u, 44u, 46u, 58u, 59u,  13u, 32u,  42u, 63u, 92u, 93u, 95u,
    9u,  10u, 33u, 34u, 39u, 41u, 44u, 46u,  58u, 59u,  13u, 32u, 42u, 63u, 91u,
    92u, 93u, 95u, 96u, 9u,  10u, 33u, 34u,  39u, 41u,  44u, 46u, 58u, 59u, 13u,
    32u, 42u, 63u, 92u, 93u, 95u, 96u, 9u,   10u, 33u,  34u, 39u, 41u, 44u, 46u,
    48u, 57u, 58u, 59u, 65u, 90u, 97u, 122u, 13u, 32u,  42u, 63u, 92u, 93u, 95u,
    96u, 9u,  10u, 33u, 34u, 39u, 41u, 44u,  46u, 58u,  59u, 13u, 32u, 42u, 63u,
    91u, 92u, 93u, 95u, 96u, 9u,  10u, 33u,  34u, 39u,  41u, 44u, 46u, 58u, 59u,
    13u, 32u, 42u, 63u, 92u, 93u, 95u, 96u,  9u,  10u,  33u, 34u, 39u, 41u, 44u,
    46u, 58u, 59u, 13u, 32u, 42u, 63u, 92u,  93u, 95u,  96u, 9u,  10u, 33u, 34u,
    39u, 41u, 44u, 46u, 58u, 59u, 13u, 32u,  42u, 63u,  91u, 92u, 95u, 96u, 9u,
    10u, 33u, 34u, 39u, 41u, 44u, 46u, 58u,  59u, 13u,  32u, 42u, 63u, 91u, 92u,
    93u, 95u, 96u, 9u,  10u, 33u, 34u, 39u,  41u, 44u,  46u, 58u, 59u, 13u, 32u,
    42u, 63u, 92u, 93u, 95u, 96u, 9u,  10u,  33u, 34u,  39u, 41u, 44u, 46u, 58u,
    59u, 13u, 32u, 42u, 63u, 92u, 93u, 95u,  96u, 9u,   10u, 33u, 34u, 39u, 41u,
    44u, 46u, 58u, 59u, 13u, 32u, 42u, 63u,  92u, 93u,  95u, 96u, 9u,  10u, 33u,
    34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u,  32u, 42u,  63u, 92u, 93u, 95u, 96u,
    9u,  10u, 33u, 34u, 39u, 41u, 44u, 46u,  58u, 59u,  13u, 32u, 42u, 63u, 92u,
    93u, 95u, 9u,  10u, 33u, 34u, 39u, 41u,  44u, 46u,  58u, 59u, 13u, 32u, 42u,
    63u, 91u, 92u, 93u, 95u, 96u, 9u,  10u,  33u, 34u,  39u, 41u, 44u, 46u, 58u,
    59u, 13u, 32u, 42u, 63u, 92u, 93u, 95u,  96u, 9u,   10u, 33u, 34u, 39u, 41u,
    44u, 46u, 48u, 57u, 58u, 59u, 65u, 90u,  97u, 122u, 13u, 32u, 42u, 63u, 92u,
    93u, 95u, 96u, 9u,  10u, 33u, 34u, 39u,  41u, 44u,  46u, 58u, 59u, 13u, 32u,
    42u, 63u, 91u, 92u, 93u, 95u, 96u, 9u,   10u, 33u,  34u, 39u, 41u, 44u, 46u,
    58u, 59u, 13u, 32u, 42u, 63u, 92u, 95u,  9u,  10u,  33u, 34u, 39u, 41u, 44u,
    46u, 58u, 59u, 13u, 32u, 42u, 63u, 92u,  93u, 95u,  96u, 9u,  10u, 33u, 34u,
    39u, 41u, 44u, 46u, 58u, 59u, 13u, 32u,  42u, 63u,  92u, 93u, 95u, 96u, 9u,
    10u, 33u, 34u, 39u, 41u, 44u, 46u, 58u,  59u, 13u,  32u, 42u, 63u, 91u, 92u,
    95u, 96u, 9u,  10u, 33u, 34u, 39u, 41u,  44u, 46u,  58u, 59u, 13u, 32u, 42u,
    63u, 92u, 93u, 95u, 96u, 9u,  10u, 33u,  34u, 39u,  41u, 44u, 46u, 58u, 59u,
    13u, 32u, 42u, 63u, 91u, 92u, 93u, 95u,  96u, 9u,   10u, 33u, 34u, 39u, 41u,
    44u, 46u, 58u, 59u, 13u, 32u, 42u, 63u,  92u, 93u,  95u, 96u, 9u,  10u, 33u,
    34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u,  32u, 42u,  63u, 92u, 93u, 95u, 96u,
    9u,  10u, 33u, 34u, 39u, 41u, 44u, 46u,  58u, 59u,  13u, 32u, 42u, 63u, 92u,
    93u, 95u, 96u, 9u,  10u, 33u, 34u, 39u,  41u, 44u,  46u, 58u, 59u, 13u, 32u,
    42u, 63u, 91u, 92u, 93u, 95u, 96u, 9u,   10u, 33u,  34u, 39u, 41u, 44u, 46u,
    58u, 59u, 13u, 32u, 42u, 63u, 92u, 93u,  95u, 96u,  9u,  10u, 33u, 34u, 39u,
    41u, 44u, 46u, 58u, 59u, 13u, 32u, 42u,  63u, 92u,  93u, 95u, 9u,  10u, 33u,
    34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u,  32u, 42u,  63u, 91u, 92u, 93u, 95u,
    96u, 9u,  10u, 33u, 34u, 39u, 41u, 44u,  46u, 58u,  59u, 13u, 32u, 42u, 63u,
    92u, 93u, 95u, 96u, 9u,  10u, 33u, 34u,  39u, 41u,  44u, 46u, 58u, 59u, 13u,
    32u, 42u, 63u, 92u, 93u, 95u, 96u, 9u,   10u, 33u,  34u, 39u, 41u, 44u, 46u,
    58u, 59u, 13u, 32u, 42u, 63u, 92u, 93u,  95u, 9u,   10u, 33u, 34u, 39u, 41u,
    44u, 46u, 58u, 59u, 13u, 32u, 42u, 63u,  91u, 92u,  93u, 95u, 96u, 9u,  10u,
    33u, 34u, 39u, 41u, 44u, 46u, 58u, 59u,  13u, 32u,  42u, 63u, 92u, 93u, 95u,
    96u, 9u,  10u, 33u, 34u, 39u, 41u, 44u,  46u, 58u,  59u, 13u, 32u, 42u, 63u,
    92u, 93u, 95u, 96u, 9u,  10u, 33u, 34u,  39u, 41u,  44u, 46u, 58u, 59u, 13u,
    32u, 42u, 63u, 92u, 93u, 95u, 96u, 9u,   10u, 33u,  34u, 39u, 41u, 44u, 46u,
    58u, 59u, 13u, 32u, 42u, 63u, 91u, 92u,  93u, 95u,  96u, 9u,  10u, 33u, 34u,
    39u, 41u, 44u, 46u, 58u, 59u, 13u, 32u,  42u, 63u,  92u, 93u, 95u, 96u, 9u,
    10u, 33u, 34u, 39u, 41u, 44u, 46u, 58u,  59u, 13u,  32u, 42u, 63u, 92u, 93u,
    95u, 9u,  10u, 33u, 34u, 39u, 41u, 44u,  46u, 58u,  59u, 13u, 32u, 42u, 63u,
    91u, 92u, 93u, 95u, 96u, 9u,  10u, 33u,  34u, 39u,  41u, 44u, 46u, 58u, 59u,
    13u, 32u, 42u, 63u, 92u, 93u, 95u, 96u,  9u,  10u,  33u, 34u, 39u, 41u, 44u,
    46u, 58u, 59u, 13u, 32u, 42u, 63u, 91u,  92u, 93u,  95u, 96u, 9u,  10u, 33u,
    34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u,  32u, 42u,  63u, 91u, 92u, 95u, 96u,
    9u,  10u, 33u, 34u, 39u, 41u, 44u, 46u,  58u, 59u,  13u, 32u, 42u, 63u, 92u,
    93u, 95u, 96u, 9u,  10u, 33u, 34u, 39u,  41u, 44u,  46u, 58u, 59u, 13u, 32u,
    42u, 63u, 91u, 92u, 93u, 95u, 96u, 9u,   10u, 33u,  34u, 39u, 41u, 44u, 46u,
    58u, 59u, 13u, 32u, 42u, 63u, 92u, 93u,  95u, 96u,  9u,  10u, 33u, 34u, 39u,
    41u, 44u, 46u, 58u, 59u, 13u, 32u, 42u,  63u, 92u,  93u, 95u, 96u, 9u,  10u,
    33u, 34u, 39u, 41u, 44u, 46u, 58u, 59u,  13u, 32u,  42u, 63u, 92u, 93u, 95u,
    96u, 9u,  10u, 33u, 34u, 39u, 41u, 44u,  46u, 58u,  59u, 13u, 32u, 42u, 63u,
    91u, 92u, 93u, 95u, 96u, 9u,  10u, 33u,  34u, 39u,  41u, 44u, 46u, 58u, 59u,
    13u, 32u, 42u, 63u, 92u, 93u, 95u, 96u,  9u,  10u,  33u, 34u, 39u, 41u, 44u,
    46u, 58u, 59u, 13u, 32u, 42u, 63u, 91u,  92u, 93u,  95u, 96u, 9u,  10u, 33u,
    34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u,  32u, 42u,  63u, 92u, 93u, 95u, 96u,
    9u,  10u, 33u, 34u, 39u, 41u, 44u, 46u,  58u, 59u,  13u, 32u, 42u, 63u, 92u,
    93u, 95u, 96u, 9u,  10u, 33u, 34u, 39u,  41u, 44u,  46u, 58u, 59u, 13u, 32u,
    42u, 63u, 92u, 93u, 95u, 9u,  10u, 33u,  34u, 39u,  41u, 44u, 46u, 58u, 59u,
    13u, 32u, 42u, 63u, 91u, 92u, 93u, 95u,  96u, 9u,   10u, 33u, 34u, 39u, 41u,
    44u, 46u, 58u, 59u, 13u, 32u, 42u, 63u,  92u, 93u,  95u, 96u, 9u,  10u, 33u,
    34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u,  32u, 42u,  63u, 92u, 93u, 95u, 96u,
    9u,  10u, 33u, 34u, 39u, 41u, 44u, 46u,  58u, 59u,  13u, 32u, 42u, 63u, 92u,
    93u, 95u, 96u, 9u,  10u, 33u, 34u, 39u,  41u, 44u,  46u, 58u, 59u, 13u, 32u,
    42u, 63u, 92u, 93u, 95u, 9u,  10u, 33u,  34u, 39u,  41u, 44u, 46u, 58u, 59u,
    13u, 32u, 42u, 63u, 91u, 92u, 93u, 95u,  96u, 9u,   10u, 33u, 34u, 39u, 41u,
    44u, 46u, 58u, 59u, 13u, 32u, 42u, 63u,  92u, 93u,  95u, 96u, 9u,  10u, 33u,
    34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u,  32u, 42u,  63u, 92u, 93u, 95u, 96u,
    9u,  10u, 33u, 34u, 39u, 41u, 44u, 46u,  58u, 59u,  13u, 32u, 42u, 63u, 92u,
    93u, 95u, 96u, 9u,  10u, 33u, 34u, 39u,  41u, 44u,  46u, 58u, 59u, 0};

static const char _MARQ_single_lengths[] = {
    6, 8, 8, 9, 8, 8, 8, 8, 7, 9, 8, 8, 9, 8, 8, 8, 9, 8, 8, 8, 8, 7, 9, 8, 8,
    9, 6, 8, 8, 8, 8, 9, 8, 8, 8, 9, 8, 7, 9, 8, 8, 7, 9, 8, 8, 8, 9, 8, 7, 9,
    8, 9, 8, 8, 9, 8, 8, 8, 9, 8, 9, 8, 8, 7, 9, 8, 8, 8, 7, 9, 8, 8, 8};

static const char _MARQ_range_lengths[] = {
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 8, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 8, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5};

static const short _MARQ_index_offsets[] = {
    0,   12,  26,  40,  55,  69,  83,  97,  111, 124, 139, 156,  170, 185, 199,
    213, 227, 242, 256, 270, 284, 298, 311, 326, 343, 357, 372,  384, 398, 412,
    426, 440, 455, 469, 483, 497, 512, 526, 539, 554, 568, 582,  595, 610, 624,
    638, 652, 667, 681, 694, 709, 723, 738, 752, 766, 781, 795,  809, 823, 838,
    852, 867, 881, 895, 908, 923, 937, 951, 965, 978, 993, 1007, 1021};

static const short _MARQ_indicies[] = {
    1,   1,   3,   2,   4,   5,   1,   2,   2,   2,   2,   0,   7,   7,   9,
    8,   10,  11,  12,  13,  7,   8,   8,   8,   8,   6,   7,   7,   14,  8,
    15,  10,  16,  13,  7,   8,   8,   8,   8,   6,   7,   7,   17,  8,   15,
    10,  11,  18,  13,  7,   8,   8,   8,   8,   6,   20,  20,  22,  21,  23,
    24,  25,  26,  20,  21,  21,  21,  21,  19,  28,  28,  30,  29,  31,  32,
    33,  34,  28,  29,  29,  29,  29,  27,  20,  20,  37,  36,  38,  39,  40,
    41,  20,  36,  36,  36,  36,  35,  7,   7,   42,  8,   10,  11,  12,  13,
    7,   8,   8,   8,   8,   6,   7,   7,   9,   8,   10,  11,  12,  7,   8,
    8,   8,   8,   6,   7,   7,   9,   8,   43,  10,  11,  12,  13,  7,   8,
    8,   8,   8,   6,   7,   7,   9,   8,   10,  11,  12,  13,  7,   8,   8,
    8,   44,  8,   44,  44,  6,   7,   7,   9,   8,   10,  45,  12,  13,  7,
    8,   8,   8,   8,   6,   47,  47,  49,  48,  50,  51,  52,  53,  54,  47,
    48,  48,  48,  48,  46,  56,  56,  58,  57,  59,  60,  61,  62,  56,  57,
    57,  57,  57,  55,  63,  63,  58,  64,  59,  60,  65,  62,  63,  64,  64,
    64,  64,  55,  63,  63,  66,  64,  67,  59,  68,  62,  63,  64,  64,  64,
    64,  55,  63,  63,  69,  64,  67,  59,  60,  65,  62,  63,  64,  64,  64,
    64,  55,  71,  71,  73,  72,  74,  75,  65,  76,  71,  72,  72,  72,  72,
    70,  78,  78,  80,  79,  81,  82,  65,  83,  78,  79,  79,  79,  79,  77,
    71,  71,  86,  85,  87,  88,  65,  89,  71,  85,  85,  85,  85,  84,  63,
    63,  90,  64,  59,  60,  65,  62,  63,  64,  64,  64,  64,  55,  63,  63,
    58,  64,  59,  60,  61,  63,  64,  64,  64,  64,  55,  63,  63,  58,  64,
    91,  59,  60,  65,  62,  63,  64,  64,  64,  64,  55,  63,  63,  58,  64,
    59,  60,  65,  62,  63,  64,  64,  64,  92,  64,  92,  92,  55,  63,  63,
    58,  64,  59,  93,  65,  62,  63,  64,  64,  64,  64,  55,  95,  95,  97,
    96,  98,  99,  100, 65,  101, 95,  96,  96,  96,  96,  94,  103, 103, 105,
    104, 106, 107, 103, 104, 104, 104, 104, 102, 7,   7,   58,  64,  59,  60,
    61,  62,  7,   64,  64,  64,  64,  55,  108, 108, 58,  57,  59,  60,  65,
    62,  108, 57,  57,  57,  57,  55,  110, 110, 112, 111, 113, 114, 115, 116,
    110, 111, 111, 111, 111, 109, 63,  63,  119, 118, 120, 121, 65,  122, 63,
    118, 118, 118, 118, 117, 78,  78,  123, 79,  124, 81,  82,  65,  83,  78,
    79,  79,  79,  79,  77,  63,  63,  127, 126, 128, 129, 65,  130, 63,  126,
    126, 126, 126, 125, 132, 132, 134, 133, 135, 136, 65,  137, 132, 133, 133,
    133, 133, 131, 139, 139, 141, 140, 142, 143, 65,  144, 139, 140, 140, 140,
    140, 138, 132, 132, 145, 133, 146, 135, 136, 65,  137, 132, 133, 133, 133,
    133, 131, 148, 148, 150, 149, 151, 152, 65,  153, 148, 149, 149, 149, 149,
    147, 132, 132, 134, 133, 135, 136, 154, 132, 133, 133, 133, 133, 131, 132,
    132, 134, 133, 155, 135, 136, 65,  137, 132, 133, 133, 133, 133, 131, 157,
    157, 159, 158, 160, 161, 65,  162, 157, 158, 158, 158, 158, 156, 78,  78,
    163, 79,  81,  82,  65,  83,  78,  79,  79,  79,  79,  77,  78,  78,  80,
    79,  81,  82,  164, 78,  79,  79,  79,  79,  77,  78,  78,  80,  79,  165,
    81,  82,  65,  83,  78,  79,  79,  79,  79,  77,  167, 167, 169, 168, 170,
    171, 65,  172, 167, 168, 168, 168, 168, 166, 63,  63,  175, 174, 176, 177,
    65,  178, 63,  174, 174, 174, 174, 173, 180, 180, 182, 181, 183, 184, 65,
    185, 180, 181, 181, 181, 181, 179, 180, 180, 186, 181, 187, 183, 184, 65,
    185, 180, 181, 181, 181, 181, 179, 189, 189, 191, 190, 192, 193, 65,  194,
    189, 190, 190, 190, 190, 188, 180, 180, 182, 181, 183, 184, 195, 180, 181,
    181, 181, 181, 179, 180, 180, 182, 181, 196, 183, 184, 65,  185, 180, 181,
    181, 181, 181, 179, 198, 198, 200, 199, 201, 202, 65,  203, 198, 199, 199,
    199, 199, 197, 205, 205, 207, 206, 208, 209, 210, 65,  211, 205, 206, 206,
    206, 206, 204, 213, 213, 215, 214, 216, 217, 218, 219, 213, 214, 214, 214,
    214, 212, 7,   7,   222, 221, 223, 224, 225, 226, 7,   221, 221, 221, 221,
    220, 28,  28,  227, 29,  228, 31,  32,  229, 34,  28,  29,  29,  29,  29,
    27,  7,   7,   232, 231, 233, 234, 235, 236, 7,   231, 231, 231, 231, 230,
    238, 238, 240, 239, 241, 242, 243, 244, 238, 239, 239, 239, 239, 237, 246,
    246, 248, 247, 249, 250, 251, 252, 246, 247, 247, 247, 247, 245, 238, 238,
    253, 239, 254, 241, 242, 255, 244, 238, 239, 239, 239, 239, 237, 56,  56,
    175, 256, 176, 177, 257, 178, 56,  256, 256, 256, 256, 173, 259, 259, 261,
    260, 262, 263, 264, 65,  265, 259, 260, 260, 260, 260, 258, 266, 266, 182,
    267, 183, 184, 65,  185, 266, 267, 267, 267, 267, 179, 269, 269, 271, 270,
    272, 273, 274, 275, 269, 270, 270, 270, 270, 268, 238, 238, 240, 239, 241,
    242, 243, 238, 239, 239, 239, 239, 237, 238, 238, 240, 239, 276, 241, 242,
    243, 244, 238, 239, 239, 239, 239, 237, 277, 277, 134, 278, 135, 136, 154,
    137, 277, 278, 278, 278, 278, 131, 280, 280, 282, 281, 283, 284, 285, 286,
    280, 281, 281, 281, 281, 279, 28,  28,  287, 29,  31,  32,  33,  34,  28,
    29,  29,  29,  29,  27,  28,  28,  30,  29,  31,  32,  33,  28,  29,  29,
    29,  29,  27,  28,  28,  30,  29,  288, 31,  32,  33,  34,  28,  29,  29,
    29,  29,  27,  289, 289, 80,  290, 81,  82,  164, 83,  289, 290, 290, 290,
    290, 77,  292, 292, 294, 293, 295, 296, 297, 298, 292, 293, 293, 293, 293,
    291, 7,   7,   175, 174, 176, 177, 257, 178, 7,   174, 174, 174, 174, 173,
    0};

static const char _MARQ_trans_targs[] = {
    1,  2,  3,  7,  8,  27, 1,  2,  3,  6,  8,  9,  13, 57, 53, 55, 72, 4,  59,
    5,  2,  54, 67, 68, 69, 70, 71, 1,  2,  3,  6,  8,  9,  13, 57, 1,  3,  7,
    8,  9,  13, 57, 7,  10, 11, 12, 1,  2,  3,  6,  10, 8,  9,  13, 57, 14, 52,
    51, 19, 21, 22, 28, 34, 15, 16, 26, 30, 32, 44, 17, 18, 15, 31, 40, 41, 42,
    43, 14, 15, 16, 19, 21, 22, 34, 14, 16, 20, 21, 22, 34, 20, 23, 24, 25, 14,
    15, 16, 19, 23, 21, 22, 34, 1,  2,  3,  7,  8,  27, 29, 14, 15, 16, 30, 32,
    21, 44, 34, 18, 31, 40, 41, 42, 43, 17, 32, 33, 35, 36, 37, 38, 39, 14, 15,
    16, 19, 21, 22, 34, 14, 15, 16, 19, 21, 22, 34, 17, 32, 14, 15, 16, 20, 21,
    22, 34, 28, 23, 14, 15, 16, 19, 21, 22, 34, 20, 28, 23, 14, 15, 16, 19, 21,
    22, 34, 45, 46, 47, 48, 49, 50, 14, 15, 16, 19, 21, 22, 34, 17, 32, 14, 15,
    16, 20, 21, 22, 34, 28, 23, 14, 15, 16, 19, 21, 22, 34, 14, 15, 16, 17, 32,
    21, 22, 34, 1,  2,  3,  53, 55, 8,  72, 57, 5,  54, 67, 68, 69, 70, 71, 4,
    55, 59, 56, 58, 62, 63, 64, 65, 66, 1,  2,  3,  6,  8,  9,  13, 57, 1,  2,
    3,  6,  8,  9,  13, 57, 4,  55, 59, 60, 61, 14, 15, 16, 17, 32, 21, 22, 34,
    29, 51, 1,  2,  3,  7,  8,  9,  13, 57, 10, 52, 51, 1,  2,  3,  6,  8,  9,
    13, 57, 7,  10, 52, 51, 1,  2,  3,  6,  8,  9,  13, 57};

static const short _MARQ_trans_actions[] = {
    364, 358, 630, 162, 167, 510, 107, 82,  525, 46,  50,  107, 268, 107, 46,
    107, 268, 46,  268, 352, 346, 621, 152, 157, 352, 503, 352, 316, 310, 612,
    142, 147, 316, 468, 316, 352, 621, 152, 157, 352, 503, 352, 46,  107, 107,
    107, 202, 196, 558, 72,  202, 77,  202, 391, 202, 107, 82,  525, 46,  50,
    107, 268, 107, 82,  525, 0,   46,  107, 268, 46,  352, 346, 621, 152, 157,
    352, 352, 316, 310, 612, 142, 147, 316, 316, 352, 621, 152, 157, 352, 352,
    46,  107, 107, 107, 202, 196, 558, 72,  202, 77,  202, 202, 292, 286, 594,
    122, 127, 454, 82,  262, 208, 567, 97,  262, 102, 447, 262, 107, 525, 46,
    50,  107, 107, 142, 316, 107, 525, 46,  50,  107, 107, 178, 172, 549, 62,
    67,  178, 178, 304, 298, 603, 132, 137, 304, 304, 62,  178, 496, 489, 689,
    334, 340, 496, 496, 370, 178, 384, 377, 639, 184, 190, 384, 384, 142, 468,
    316, 482, 475, 679, 322, 328, 482, 482, 107, 525, 46,  50,  107, 107, 220,
    214, 576, 87,  92,  220, 220, 87,  220, 440, 433, 669, 250, 256, 440, 440,
    398, 220, 426, 419, 659, 238, 244, 426, 426, 280, 274, 585, 112, 280, 117,
    280, 280, 262, 208, 567, 97,  262, 102, 447, 262, 107, 525, 46,  50,  107,
    268, 107, 142, 316, 468, 107, 525, 46,  50,  107, 268, 107, 178, 172, 549,
    62,  67,  178, 370, 178, 304, 298, 603, 132, 137, 304, 461, 304, 62,  178,
    370, 525, 268, 412, 405, 649, 226, 412, 232, 412, 412, 214, 576, 496, 489,
    689, 334, 340, 496, 541, 496, 178, 172, 549, 384, 377, 639, 184, 190, 384,
    517, 384, 142, 316, 310, 612, 482, 475, 679, 322, 328, 482, 533, 482};

static const short _MARQ_eof_actions[] = {
    27, 1,  1,  1,  24, 21, 24, 1,  1, 1,  1,  1,  6,  1,  1,  1,  1,  24, 21,
    24, 1,  1,  1,  1,  1,  6,  15, 1, 1,  12, 1,  21, 1,  3,  18, 3,  58, 3,
    3,  30, 21, 21, 21, 54, 1,  9,  9, 42, 9,  9,  38, 12, 12, 1,  21, 1,  3,
    18, 3,  1,  34, 9,  58, 3,  3,  3, 30, 21, 21, 21, 21, 54, 1};

static const int MARQ_start = 0;
static const int MARQ_first_final = 0;
static const int MARQ_error = -1;

static const int MARQ_en_main = 0;

#line 119 "MARQ.rl"

pro(MARQlexer, MARQstate *state) {
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

#line 571 "MARQ.rl.c"
    { cs = MARQ_start; }

#line 137 "MARQ.rl"

#line 574 "MARQ.rl.c"
    {
        int _klen;
        unsigned int _trans;
        const char *_acts;
        unsigned int _nacts;
        const unsigned char *_keys;

        if (p == pe) goto _test_eof;
    _resume:
        _keys = _MARQ_trans_keys + _MARQ_key_offsets[cs];
        _trans = _MARQ_index_offsets[cs];

        _klen = _MARQ_single_lengths[cs];
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

        _klen = _MARQ_range_lengths[cs];
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
        _trans = _MARQ_indicies[_trans];
        cs = _MARQ_trans_targs[_trans];

        if (_MARQ_trans_actions[_trans] == 0) goto _again;

        _acts = _MARQ_actions + _MARQ_trans_actions[_trans];
        _nacts = (unsigned int)*_acts++;
        while (_nacts-- > 0) {
            switch (*_acts++) {
                case 0:
#line 10 "MARQ.rl"
                {
                    mark0[MARQRef0] = p - text[0];
                } break;
                case 1:
#line 11 "MARQ.rl"
                {
                    tok[0] = text[0] + mark0[MARQRef0];
                    tok[1] = p;
                    call(MARQonRef0, tok, state);
                } break;
                case 2:
#line 16 "MARQ.rl"
                {
                    mark0[MARQRef1] = p - text[0];
                } break;
                case 3:
#line 17 "MARQ.rl"
                {
                    tok[0] = text[0] + mark0[MARQRef1];
                    tok[1] = p;
                    call(MARQonRef1, tok, state);
                } break;
                case 4:
#line 22 "MARQ.rl"
                {
                    mark0[MARQEm0] = p - text[0];
                } break;
                case 5:
#line 23 "MARQ.rl"
                {
                    tok[0] = text[0] + mark0[MARQEm0];
                    tok[1] = p;
                    call(MARQonEm0, tok, state);
                } break;
                case 6:
#line 28 "MARQ.rl"
                {
                    mark0[MARQEm1] = p - text[0];
                } break;
                case 7:
#line 29 "MARQ.rl"
                {
                    tok[0] = text[0] + mark0[MARQEm1];
                    tok[1] = p;
                    call(MARQonEm1, tok, state);
                } break;
                case 8:
#line 34 "MARQ.rl"
                {
                    mark0[MARQEm] = p - text[0];
                } break;
                case 9:
#line 35 "MARQ.rl"
                {
                    tok[0] = text[0] + mark0[MARQEm];
                    tok[1] = p;
                    call(MARQonEm, tok, state);
                } break;
                case 10:
#line 40 "MARQ.rl"
                {
                    mark0[MARQCode01] = p - text[0];
                } break;
                case 11:
#line 41 "MARQ.rl"
                {
                    tok[0] = text[0] + mark0[MARQCode01];
                    tok[1] = p;
                    call(MARQonCode01, tok, state);
                } break;
                case 12:
#line 46 "MARQ.rl"
                {
                    mark0[MARQSt0] = p - text[0];
                } break;
                case 13:
#line 47 "MARQ.rl"
                {
                    tok[0] = text[0] + mark0[MARQSt0];
                    tok[1] = p;
                    call(MARQonSt0, tok, state);
                } break;
                case 14:
#line 52 "MARQ.rl"
                {
                    mark0[MARQSt1] = p - text[0];
                } break;
                case 15:
#line 53 "MARQ.rl"
                {
                    tok[0] = text[0] + mark0[MARQSt1];
                    tok[1] = p;
                    call(MARQonSt1, tok, state);
                } break;
                case 16:
#line 64 "MARQ.rl"
                {
                    mark0[MARQRoot] = p - text[0];
                } break;
#line 728 "MARQ.rl.c"
            }
        }

    _again:
        if (++p != pe) goto _resume;
    _test_eof: {}
        if (p == eof) {
            const char *__acts = _MARQ_actions + _MARQ_eof_actions[cs];
            unsigned int __nacts = (unsigned int)*__acts++;
            while (__nacts-- > 0) {
                switch (*__acts++) {
                    case 1:
#line 11 "MARQ.rl"
                    {
                        tok[0] = text[0] + mark0[MARQRef0];
                        tok[1] = p;
                        call(MARQonRef0, tok, state);
                    } break;
                    case 3:
#line 17 "MARQ.rl"
                    {
                        tok[0] = text[0] + mark0[MARQRef1];
                        tok[1] = p;
                        call(MARQonRef1, tok, state);
                    } break;
                    case 5:
#line 23 "MARQ.rl"
                    {
                        tok[0] = text[0] + mark0[MARQEm0];
                        tok[1] = p;
                        call(MARQonEm0, tok, state);
                    } break;
                    case 7:
#line 29 "MARQ.rl"
                    {
                        tok[0] = text[0] + mark0[MARQEm1];
                        tok[1] = p;
                        call(MARQonEm1, tok, state);
                    } break;
                    case 9:
#line 35 "MARQ.rl"
                    {
                        tok[0] = text[0] + mark0[MARQEm];
                        tok[1] = p;
                        call(MARQonEm, tok, state);
                    } break;
                    case 11:
#line 41 "MARQ.rl"
                    {
                        tok[0] = text[0] + mark0[MARQCode01];
                        tok[1] = p;
                        call(MARQonCode01, tok, state);
                    } break;
                    case 13:
#line 47 "MARQ.rl"
                    {
                        tok[0] = text[0] + mark0[MARQSt0];
                        tok[1] = p;
                        call(MARQonSt0, tok, state);
                    } break;
                    case 15:
#line 53 "MARQ.rl"
                    {
                        tok[0] = text[0] + mark0[MARQSt1];
                        tok[1] = p;
                        call(MARQonSt1, tok, state);
                    } break;
                    case 16:
#line 64 "MARQ.rl"
                    {
                        mark0[MARQRoot] = p - text[0];
                    } break;
                    case 17:
#line 65 "MARQ.rl"
                    {
                        tok[0] = text[0] + mark0[MARQRoot];
                        tok[1] = p;
                        call(MARQonRoot, tok, state);
                    } break;
#line 807 "MARQ.rl.c"
                }
            }
        }
    }

#line 138 "MARQ.rl"

    if (p != text[1] || cs < MARQ_first_final) {
        fail(MARQfail);
        state->text[0] = p;
    }
    done;
}