
#line 1 "JDR.c.rl"
#include "JDR.h"

#include "abc/INT.h"
#include "abc/PRO.h"

// action indices for the parser
#define JDRenum 0
enum {
    JDRNL = JDRenum + 1,
    JDRUtf8cp1 = JDRenum + 10,
    JDRUtf8cp2 = JDRenum + 11,
    JDRUtf8cp3 = JDRenum + 12,
    JDRUtf8cp4 = JDRenum + 13,
    JDRInt = JDRenum + 19,
    JDRFloat = JDRenum + 20,
    JDRTerm = JDRenum + 21,
    JDRRef = JDRenum + 22,
    JDRString = JDRenum + 23,
    JDRMLString = JDRenum + 24,
    JDRStamp = JDRenum + 25,
    JDRNoStamp = JDRenum + 26,
    JDROpenP = JDRenum + 27,
    JDRCloseP = JDRenum + 28,
    JDROpenL = JDRenum + 29,
    JDRCloseL = JDRenum + 30,
    JDROpenE = JDRenum + 31,
    JDRCloseE = JDRenum + 32,
    JDROpenX = JDRenum + 33,
    JDRCloseX = JDRenum + 34,
    JDRComma = JDRenum + 35,
    JDRColon = JDRenum + 36,
    JDROpen = JDRenum + 37,
    JDRClose = JDRenum + 38,
    JDRInter = JDRenum + 39,
    JDRFIRST = JDRenum + 40,
    JDRToken = JDRenum + 41,
    JDRRoot = JDRenum + 42,
};

// user functions (callbacks) for the parser
ok64 JDRonNL(utf8cs tok, JDRstate* state);
ok64 JDRonUtf8cp1(utf8cs tok, JDRstate* state);
ok64 JDRonUtf8cp2(utf8cs tok, JDRstate* state);
ok64 JDRonUtf8cp3(utf8cs tok, JDRstate* state);
ok64 JDRonUtf8cp4(utf8cs tok, JDRstate* state);
ok64 JDRonInt(utf8cs tok, JDRstate* state);
ok64 JDRonFloat(utf8cs tok, JDRstate* state);
ok64 JDRonTerm(utf8cs tok, JDRstate* state);
ok64 JDRonRef(utf8cs tok, JDRstate* state);
ok64 JDRonString(utf8cs tok, JDRstate* state);
ok64 JDRonMLString(utf8cs tok, JDRstate* state);
ok64 JDRonStamp(utf8cs tok, JDRstate* state);
ok64 JDRonNoStamp(utf8cs tok, JDRstate* state);
ok64 JDRonOpenP(utf8cs tok, JDRstate* state);
ok64 JDRonCloseP(utf8cs tok, JDRstate* state);
ok64 JDRonOpenL(utf8cs tok, JDRstate* state);
ok64 JDRonCloseL(utf8cs tok, JDRstate* state);
ok64 JDRonOpenE(utf8cs tok, JDRstate* state);
ok64 JDRonCloseE(utf8cs tok, JDRstate* state);
ok64 JDRonOpenX(utf8cs tok, JDRstate* state);
ok64 JDRonCloseX(utf8cs tok, JDRstate* state);
ok64 JDRonComma(utf8cs tok, JDRstate* state);
ok64 JDRonColon(utf8cs tok, JDRstate* state);
ok64 JDRonOpen(utf8cs tok, JDRstate* state);
ok64 JDRonClose(utf8cs tok, JDRstate* state);
ok64 JDRonInter(utf8cs tok, JDRstate* state);
ok64 JDRonFIRST(utf8cs tok, JDRstate* state);
ok64 JDRonToken(utf8cs tok, JDRstate* state);
ok64 JDRonRoot(utf8cs tok, JDRstate* state);

#line 389 "JDR.c.rl"

#line 75 "JDR.rl.c"
static const char _JDR_actions[] = {
    0,  1,  0,  1,  1,  1,  2,  1,  3,  1,  4,  1,  5,  1,  6,  1,  7,  1,  8,
    1,  9,  1,  11, 1,  13, 1,  15, 1,  17, 1,  19, 1,  21, 1,  22, 1,  27, 1,
    31, 1,  35, 1,  39, 1,  56, 1,  57, 2,  1,  0,  2,  1,  22, 2,  1,  57, 2,
    3,  2,  2,  3,  4,  2,  3,  6,  2,  3,  8,  2,  5,  2,  2,  5,  4,  2,  5,
    6,  2,  5,  8,  2,  7,  2,  2,  7,  4,  2,  7,  6,  2,  7,  8,  2,  9,  2,
    2,  9,  4,  2,  9,  6,  2,  9,  8,  2,  11, 0,  2,  11, 22, 2,  13, 0,  2,
    13, 22, 2,  15, 0,  2,  15, 22, 2,  17, 0,  2,  17, 22, 2,  19, 0,  2,  19,
    22, 2,  21, 0,  2,  21, 22, 2,  27, 0,  2,  27, 22, 2,  31, 0,  2,  31, 22,
    2,  35, 0,  2,  35, 22, 2,  39, 0,  2,  39, 22, 2,  56, 0,  2,  56, 57, 3,
    23, 47, 55, 3,  23, 53, 55, 3,  29, 49, 55, 3,  33, 49, 55, 3,  37, 49, 55,
    3,  41, 49, 55, 3,  43, 51, 55, 3,  45, 51, 55, 3,  54, 46, 26, 3,  54, 46,
    30, 3,  54, 46, 34, 3,  54, 46, 38, 3,  54, 48, 28, 3,  54, 48, 32, 3,  54,
    48, 36, 3,  54, 48, 40, 3,  54, 50, 42, 3,  54, 50, 44, 3,  54, 52, 18, 3,
    54, 52, 20, 4,  1,  54, 46, 26, 4,  1,  54, 46, 30, 4,  1,  54, 46, 34, 4,
    1,  54, 46, 38, 4,  1,  54, 48, 28, 4,  1,  54, 48, 32, 4,  1,  54, 48, 36,
    4,  1,  54, 48, 40, 4,  1,  54, 50, 42, 4,  1,  54, 50, 44, 4,  1,  54, 52,
    18, 4,  1,  54, 52, 20, 4,  23, 47, 55, 0,  4,  23, 47, 55, 57, 4,  23, 53,
    55, 0,  4,  23, 53, 55, 57, 4,  29, 49, 55, 0,  4,  29, 49, 55, 57, 4,  33,
    49, 55, 0,  4,  33, 49, 55, 57, 4,  37, 49, 55, 0,  4,  37, 49, 55, 57, 4,
    41, 49, 55, 0,  4,  41, 49, 55, 57, 4,  43, 51, 55, 0,  4,  43, 51, 55, 57,
    4,  45, 51, 55, 0,  4,  45, 51, 55, 57, 4,  54, 52, 12, 10, 4,  54, 52, 16,
    14, 4,  56, 54, 46, 26, 4,  56, 54, 46, 30, 4,  56, 54, 46, 34, 4,  56, 54,
    46, 38, 4,  56, 54, 48, 28, 4,  56, 54, 48, 32, 4,  56, 54, 48, 36, 4,  56,
    54, 48, 40, 4,  56, 54, 50, 42, 4,  56, 54, 50, 44, 4,  56, 54, 52, 18, 4,
    56, 54, 52, 20, 5,  1,  54, 52, 12, 10, 5,  1,  54, 52, 16, 14, 5,  24, 25,
    47, 55, 57, 5,  24, 25, 53, 55, 57, 5,  56, 54, 52, 12, 10, 5,  56, 54, 52,
    16, 14, 6,  1,  24, 25, 47, 55, 57, 6,  1,  24, 25, 53, 55, 57, 6,  11, 24,
    25, 53, 55, 57, 6,  13, 24, 25, 53, 55, 57, 6,  15, 24, 25, 53, 55, 57, 6,
    17, 24, 25, 53, 55, 57, 6,  19, 24, 25, 53, 55, 57, 6,  21, 24, 25, 53, 55,
    57, 6,  23, 47, 55, 54, 46, 26, 6,  23, 47, 55, 54, 46, 30, 6,  23, 47, 55,
    54, 46, 34, 6,  23, 47, 55, 54, 46, 38, 6,  23, 47, 55, 54, 48, 28, 6,  23,
    47, 55, 54, 48, 32, 6,  23, 47, 55, 54, 48, 36, 6,  23, 47, 55, 54, 48, 40,
    6,  23, 47, 55, 54, 50, 42, 6,  23, 47, 55, 54, 50, 44, 6,  23, 47, 55, 54,
    52, 18, 6,  23, 47, 55, 54, 52, 20, 6,  23, 53, 55, 54, 46, 26, 6,  23, 53,
    55, 54, 46, 30, 6,  23, 53, 55, 54, 46, 34, 6,  23, 53, 55, 54, 46, 38, 6,
    23, 53, 55, 54, 48, 28, 6,  23, 53, 55, 54, 48, 32, 6,  23, 53, 55, 54, 48,
    36, 6,  23, 53, 55, 54, 48, 40, 6,  23, 53, 55, 54, 50, 42, 6,  23, 53, 55,
    54, 50, 44, 6,  23, 53, 55, 54, 52, 18, 6,  23, 53, 55, 54, 52, 20, 6,  27,
    24, 25, 47, 55, 57, 6,  29, 49, 55, 54, 46, 26, 6,  29, 49, 55, 54, 46, 30,
    6,  29, 49, 55, 54, 46, 34, 6,  29, 49, 55, 54, 46, 38, 6,  29, 49, 55, 54,
    48, 28, 6,  29, 49, 55, 54, 48, 32, 6,  29, 49, 55, 54, 48, 36, 6,  29, 49,
    55, 54, 48, 40, 6,  29, 49, 55, 54, 50, 42, 6,  29, 49, 55, 54, 50, 44, 6,
    29, 49, 55, 54, 52, 18, 6,  29, 49, 55, 54, 52, 20, 6,  31, 24, 25, 47, 55,
    57, 6,  33, 49, 55, 54, 46, 26, 6,  33, 49, 55, 54, 46, 30, 6,  33, 49, 55,
    54, 46, 34, 6,  33, 49, 55, 54, 46, 38, 6,  33, 49, 55, 54, 48, 28, 6,  33,
    49, 55, 54, 48, 32, 6,  33, 49, 55, 54, 48, 36, 6,  33, 49, 55, 54, 48, 40,
    6,  33, 49, 55, 54, 50, 42, 6,  33, 49, 55, 54, 50, 44, 6,  33, 49, 55, 54,
    52, 18, 6,  33, 49, 55, 54, 52, 20, 6,  35, 24, 25, 47, 55, 57, 6,  37, 49,
    55, 54, 46, 26, 6,  37, 49, 55, 54, 46, 30, 6,  37, 49, 55, 54, 46, 34, 6,
    37, 49, 55, 54, 46, 38, 6,  37, 49, 55, 54, 48, 28, 6,  37, 49, 55, 54, 48,
    32, 6,  37, 49, 55, 54, 48, 36, 6,  37, 49, 55, 54, 48, 40, 6,  37, 49, 55,
    54, 50, 42, 6,  37, 49, 55, 54, 50, 44, 6,  37, 49, 55, 54, 52, 18, 6,  37,
    49, 55, 54, 52, 20, 6,  39, 24, 25, 47, 55, 57, 6,  41, 49, 55, 54, 46, 26,
    6,  41, 49, 55, 54, 46, 30, 6,  41, 49, 55, 54, 46, 34, 6,  41, 49, 55, 54,
    46, 38, 6,  41, 49, 55, 54, 48, 28, 6,  41, 49, 55, 54, 48, 32, 6,  41, 49,
    55, 54, 48, 36, 6,  41, 49, 55, 54, 48, 40, 6,  41, 49, 55, 54, 50, 42, 6,
    41, 49, 55, 54, 50, 44, 6,  41, 49, 55, 54, 52, 18, 6,  41, 49, 55, 54, 52,
    20, 6,  43, 51, 55, 54, 46, 26, 6,  43, 51, 55, 54, 46, 30, 6,  43, 51, 55,
    54, 46, 34, 6,  43, 51, 55, 54, 46, 38, 6,  43, 51, 55, 54, 48, 28, 6,  43,
    51, 55, 54, 48, 32, 6,  43, 51, 55, 54, 48, 36, 6,  43, 51, 55, 54, 48, 40,
    6,  43, 51, 55, 54, 50, 42, 6,  43, 51, 55, 54, 50, 44, 6,  43, 51, 55, 54,
    52, 18, 6,  43, 51, 55, 54, 52, 20, 6,  45, 51, 55, 54, 46, 26, 6,  45, 51,
    55, 54, 46, 30, 6,  45, 51, 55, 54, 46, 34, 6,  45, 51, 55, 54, 46, 38, 6,
    45, 51, 55, 54, 48, 28, 6,  45, 51, 55, 54, 48, 32, 6,  45, 51, 55, 54, 48,
    36, 6,  45, 51, 55, 54, 48, 40, 6,  45, 51, 55, 54, 50, 42, 6,  45, 51, 55,
    54, 50, 44, 6,  45, 51, 55, 54, 52, 18, 6,  45, 51, 55, 54, 52, 20, 6,  54,
    52, 12, 10, 16, 14, 7,  1,  54, 52, 12, 10, 16, 14, 7,  23, 47, 55, 54, 52,
    12, 10, 7,  23, 53, 55, 54, 52, 12, 10, 7,  24, 25, 47, 55, 54, 46, 26, 7,
    24, 25, 47, 55, 54, 46, 30, 7,  24, 25, 47, 55, 54, 46, 34, 7,  24, 25, 47,
    55, 54, 46, 38, 7,  24, 25, 47, 55, 54, 48, 28, 7,  24, 25, 47, 55, 54, 48,
    32, 7,  24, 25, 47, 55, 54, 48, 36, 7,  24, 25, 47, 55, 54, 48, 40, 7,  24,
    25, 47, 55, 54, 50, 42, 7,  24, 25, 47, 55, 54, 50, 44, 7,  24, 25, 47, 55,
    54, 52, 18, 7,  24, 25, 47, 55, 54, 52, 20, 7,  24, 25, 53, 55, 54, 46, 26,
    7,  24, 25, 53, 55, 54, 46, 30, 7,  24, 25, 53, 55, 54, 46, 34, 7,  24, 25,
    53, 55, 54, 46, 38, 7,  24, 25, 53, 55, 54, 48, 28, 7,  24, 25, 53, 55, 54,
    48, 32, 7,  24, 25, 53, 55, 54, 48, 36, 7,  24, 25, 53, 55, 54, 48, 40, 7,
    24, 25, 53, 55, 54, 50, 42, 7,  24, 25, 53, 55, 54, 50, 44, 7,  24, 25, 53,
    55, 54, 52, 18, 7,  24, 25, 53, 55, 54, 52, 20, 7,  29, 49, 55, 54, 52, 12,
    10, 7,  29, 49, 55, 54, 52, 16, 14, 7,  33, 49, 55, 54, 52, 12, 10, 7,  33,
    49, 55, 54, 52, 16, 14, 7,  37, 49, 55, 54, 52, 12, 10, 7,  37, 49, 55, 54,
    52, 16, 14, 7,  41, 49, 55, 54, 52, 12, 10, 7,  41, 49, 55, 54, 52, 16, 14,
    7,  43, 51, 55, 54, 52, 12, 10, 7,  43, 51, 55, 54, 52, 16, 14, 7,  45, 51,
    55, 54, 52, 12, 10, 7,  45, 51, 55, 54, 52, 16, 14, 7,  56, 54, 52, 12, 10,
    16, 14, 8,  1,  24, 25, 47, 55, 54, 46, 26, 8,  1,  24, 25, 47, 55, 54, 46,
    30, 8,  1,  24, 25, 47, 55, 54, 46, 34, 8,  1,  24, 25, 47, 55, 54, 46, 38,
    8,  1,  24, 25, 47, 55, 54, 48, 28, 8,  1,  24, 25, 47, 55, 54, 48, 32, 8,
    1,  24, 25, 47, 55, 54, 48, 36, 8,  1,  24, 25, 47, 55, 54, 48, 40, 8,  1,
    24, 25, 47, 55, 54, 50, 42, 8,  1,  24, 25, 47, 55, 54, 50, 44, 8,  1,  24,
    25, 47, 55, 54, 52, 18, 8,  1,  24, 25, 47, 55, 54, 52, 20, 8,  1,  24, 25,
    53, 55, 54, 46, 26, 8,  1,  24, 25, 53, 55, 54, 46, 30, 8,  1,  24, 25, 53,
    55, 54, 46, 34, 8,  1,  24, 25, 53, 55, 54, 46, 38, 8,  1,  24, 25, 53, 55,
    54, 48, 28, 8,  1,  24, 25, 53, 55, 54, 48, 32, 8,  1,  24, 25, 53, 55, 54,
    48, 36, 8,  1,  24, 25, 53, 55, 54, 48, 40, 8,  1,  24, 25, 53, 55, 54, 50,
    42, 8,  1,  24, 25, 53, 55, 54, 50, 44, 8,  1,  24, 25, 53, 55, 54, 52, 18,
    8,  1,  24, 25, 53, 55, 54, 52, 20, 8,  11, 24, 25, 53, 55, 54, 46, 26, 8,
    11, 24, 25, 53, 55, 54, 46, 30, 8,  11, 24, 25, 53, 55, 54, 46, 34, 8,  11,
    24, 25, 53, 55, 54, 46, 38, 8,  11, 24, 25, 53, 55, 54, 48, 28, 8,  11, 24,
    25, 53, 55, 54, 48, 32, 8,  11, 24, 25, 53, 55, 54, 48, 36, 8,  11, 24, 25,
    53, 55, 54, 48, 40, 8,  11, 24, 25, 53, 55, 54, 50, 42, 8,  11, 24, 25, 53,
    55, 54, 50, 44, 8,  11, 24, 25, 53, 55, 54, 52, 18, 8,  11, 24, 25, 53, 55,
    54, 52, 20, 8,  13, 24, 25, 53, 55, 54, 46, 26, 8,  13, 24, 25, 53, 55, 54,
    46, 30, 8,  13, 24, 25, 53, 55, 54, 46, 34, 8,  13, 24, 25, 53, 55, 54, 46,
    38, 8,  13, 24, 25, 53, 55, 54, 48, 28, 8,  13, 24, 25, 53, 55, 54, 48, 32,
    8,  13, 24, 25, 53, 55, 54, 48, 36, 8,  13, 24, 25, 53, 55, 54, 48, 40, 8,
    13, 24, 25, 53, 55, 54, 50, 42, 8,  13, 24, 25, 53, 55, 54, 50, 44, 8,  13,
    24, 25, 53, 55, 54, 52, 18, 8,  13, 24, 25, 53, 55, 54, 52, 20, 8,  15, 24,
    25, 53, 55, 54, 46, 26, 8,  15, 24, 25, 53, 55, 54, 46, 30, 8,  15, 24, 25,
    53, 55, 54, 46, 34, 8,  15, 24, 25, 53, 55, 54, 46, 38, 8,  15, 24, 25, 53,
    55, 54, 48, 28, 8,  15, 24, 25, 53, 55, 54, 48, 32, 8,  15, 24, 25, 53, 55,
    54, 48, 36, 8,  15, 24, 25, 53, 55, 54, 48, 40, 8,  15, 24, 25, 53, 55, 54,
    50, 42, 8,  15, 24, 25, 53, 55, 54, 50, 44, 8,  15, 24, 25, 53, 55, 54, 52,
    18, 8,  15, 24, 25, 53, 55, 54, 52, 20, 8,  17, 24, 25, 53, 55, 54, 46, 26,
    8,  17, 24, 25, 53, 55, 54, 46, 30, 8,  17, 24, 25, 53, 55, 54, 46, 34, 8,
    17, 24, 25, 53, 55, 54, 46, 38, 8,  17, 24, 25, 53, 55, 54, 48, 28, 8,  17,
    24, 25, 53, 55, 54, 48, 32, 8,  17, 24, 25, 53, 55, 54, 48, 36, 8,  17, 24,
    25, 53, 55, 54, 48, 40, 8,  17, 24, 25, 53, 55, 54, 50, 42, 8,  17, 24, 25,
    53, 55, 54, 50, 44, 8,  17, 24, 25, 53, 55, 54, 52, 18, 8,  17, 24, 25, 53,
    55, 54, 52, 20, 8,  19, 24, 25, 53, 55, 54, 46, 26, 8,  19, 24, 25, 53, 55,
    54, 46, 30, 8,  19, 24, 25, 53, 55, 54, 46, 34, 8,  19, 24, 25, 53, 55, 54,
    46, 38, 8,  19, 24, 25, 53, 55, 54, 48, 28, 8,  19, 24, 25, 53, 55, 54, 48,
    32, 8,  19, 24, 25, 53, 55, 54, 48, 36, 8,  19, 24, 25, 53, 55, 54, 48, 40,
    8,  19, 24, 25, 53, 55, 54, 50, 42, 8,  19, 24, 25, 53, 55, 54, 50, 44, 8,
    19, 24, 25, 53, 55, 54, 52, 18, 8,  19, 24, 25, 53, 55, 54, 52, 20, 8,  21,
    24, 25, 53, 55, 54, 46, 26, 8,  21, 24, 25, 53, 55, 54, 46, 30, 8,  21, 24,
    25, 53, 55, 54, 46, 34, 8,  21, 24, 25, 53, 55, 54, 46, 38, 8,  21, 24, 25,
    53, 55, 54, 48, 28, 8,  21, 24, 25, 53, 55, 54, 48, 32, 8,  21, 24, 25, 53,
    55, 54, 48, 36, 8,  21, 24, 25, 53, 55, 54, 48, 40, 8,  21, 24, 25, 53, 55,
    54, 50, 42, 8,  21, 24, 25, 53, 55, 54, 50, 44, 8,  21, 24, 25, 53, 55, 54,
    52, 18, 8,  21, 24, 25, 53, 55, 54, 52, 20, 8,  24, 25, 47, 55, 54, 52, 12,
    10, 8,  24, 25, 47, 55, 54, 52, 16, 14, 8,  24, 25, 53, 55, 54, 52, 12, 10,
    8,  24, 25, 53, 55, 54, 52, 16, 14, 8,  27, 24, 25, 47, 55, 54, 46, 26, 8,
    27, 24, 25, 47, 55, 54, 46, 30, 8,  27, 24, 25, 47, 55, 54, 46, 34, 8,  27,
    24, 25, 47, 55, 54, 46, 38, 8,  27, 24, 25, 47, 55, 54, 48, 28, 8,  27, 24,
    25, 47, 55, 54, 48, 32, 8,  27, 24, 25, 47, 55, 54, 48, 36, 8,  27, 24, 25,
    47, 55, 54, 48, 40, 8,  27, 24, 25, 47, 55, 54, 50, 42, 8,  27, 24, 25, 47,
    55, 54, 50, 44, 8,  27, 24, 25, 47, 55, 54, 52, 18, 8,  27, 24, 25, 47, 55,
    54, 52, 20, 8,  31, 24, 25, 47, 55, 54, 46, 26, 8,  31, 24, 25, 47, 55, 54,
    46, 30, 8,  31, 24, 25, 47, 55, 54, 46, 34, 8,  31, 24, 25, 47, 55, 54, 46,
    38, 8,  31, 24, 25, 47, 55, 54, 48, 28, 8,  31, 24, 25, 47, 55, 54, 48, 32,
    8,  31, 24, 25, 47, 55, 54, 48, 36, 8,  31, 24, 25, 47, 55, 54, 48, 40, 8,
    31, 24, 25, 47, 55, 54, 50, 42, 8,  31, 24, 25, 47, 55, 54, 50, 44, 8,  31,
    24, 25, 47, 55, 54, 52, 18, 8,  31, 24, 25, 47, 55, 54, 52, 20, 8,  35, 24,
    25, 47, 55, 54, 46, 26, 8,  35, 24, 25, 47, 55, 54, 46, 30, 8,  35, 24, 25,
    47, 55, 54, 46, 34, 8,  35, 24, 25, 47, 55, 54, 46, 38, 8,  35, 24, 25, 47,
    55, 54, 48, 28, 8,  35, 24, 25, 47, 55, 54, 48, 32, 8,  35, 24, 25, 47, 55,
    54, 48, 36, 8,  35, 24, 25, 47, 55, 54, 48, 40, 8,  35, 24, 25, 47, 55, 54,
    50, 42, 8,  35, 24, 25, 47, 55, 54, 50, 44, 8,  35, 24, 25, 47, 55, 54, 52,
    18, 8,  35, 24, 25, 47, 55, 54, 52, 20, 8,  39, 24, 25, 47, 55, 54, 46, 26,
    8,  39, 24, 25, 47, 55, 54, 46, 30, 8,  39, 24, 25, 47, 55, 54, 46, 34, 8,
    39, 24, 25, 47, 55, 54, 46, 38, 8,  39, 24, 25, 47, 55, 54, 48, 28, 8,  39,
    24, 25, 47, 55, 54, 48, 32, 8,  39, 24, 25, 47, 55, 54, 48, 36, 8,  39, 24,
    25, 47, 55, 54, 48, 40, 8,  39, 24, 25, 47, 55, 54, 50, 42, 8,  39, 24, 25,
    47, 55, 54, 50, 44, 8,  39, 24, 25, 47, 55, 54, 52, 18, 8,  39, 24, 25, 47,
    55, 54, 52, 20, 9,  1,  24, 25, 47, 55, 54, 52, 12, 10, 9,  1,  24, 25, 47,
    55, 54, 52, 16, 14, 9,  1,  24, 25, 53, 55, 54, 52, 12, 10, 9,  1,  24, 25,
    53, 55, 54, 52, 16, 14, 9,  11, 24, 25, 53, 55, 54, 52, 12, 10, 9,  11, 24,
    25, 53, 55, 54, 52, 16, 14, 9,  13, 24, 25, 53, 55, 54, 52, 12, 10, 9,  13,
    24, 25, 53, 55, 54, 52, 16, 14, 9,  17, 24, 25, 53, 55, 54, 52, 12, 10, 9,
    19, 24, 25, 53, 55, 54, 52, 12, 10, 9,  19, 24, 25, 53, 55, 54, 52, 16, 14,
    9,  21, 24, 25, 53, 55, 54, 52, 12, 10, 9,  21, 24, 25, 53, 55, 54, 52, 16,
    14, 9,  27, 24, 25, 47, 55, 54, 52, 12, 10, 9,  27, 24, 25, 47, 55, 54, 52,
    16, 14, 9,  29, 49, 55, 54, 52, 12, 10, 16, 14, 9,  31, 24, 25, 47, 55, 54,
    52, 12, 10, 9,  31, 24, 25, 47, 55, 54, 52, 16, 14, 9,  33, 49, 55, 54, 52,
    12, 10, 16, 14, 9,  35, 24, 25, 47, 55, 54, 52, 12, 10, 9,  35, 24, 25, 47,
    55, 54, 52, 16, 14, 9,  37, 49, 55, 54, 52, 12, 10, 16, 14, 9,  39, 24, 25,
    47, 55, 54, 52, 12, 10, 9,  39, 24, 25, 47, 55, 54, 52, 16, 14, 9,  41, 49,
    55, 54, 52, 12, 10, 16, 14, 9,  43, 51, 55, 54, 52, 12, 10, 16, 14, 9,  45,
    51, 55, 54, 52, 12, 10, 16, 14, 10, 24, 25, 47, 55, 54, 52, 12, 10, 16, 14,
    10, 24, 25, 53, 55, 54, 52, 12, 10, 16, 14, 11, 1,  24, 25, 47, 55, 54, 52,
    12, 10, 16, 14, 11, 1,  24, 25, 53, 55, 54, 52, 12, 10, 16, 14, 11, 11, 24,
    25, 53, 55, 54, 52, 12, 10, 16, 14, 11, 19, 24, 25, 53, 55, 54, 52, 12, 10,
    16, 14, 11, 21, 24, 25, 53, 55, 54, 52, 12, 10, 16, 14, 11, 27, 24, 25, 47,
    55, 54, 52, 12, 10, 16, 14, 11, 31, 24, 25, 47, 55, 54, 52, 12, 10, 16, 14,
    11, 35, 24, 25, 47, 55, 54, 52, 12, 10, 16, 14, 11, 39, 24, 25, 47, 55, 54,
    52, 12, 10, 16, 14};

static const short _JDR_eof_actions[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   167, 45,  53,  538, 478, 503, 720,
    472, 496, 335, 375, 510, 517, 385, 510, 531, 993, 510, 365, 524, 325, 325,
    811, 315, 315, 345, 545, 902, 355, 524, 517, 517, 517, 510};

static const int JDR_start = 38;
static const int JDR_first_final = 38;
static const int JDR_error = 0;

static const int JDR_en_main = 38;

#line 392 "JDR.c.rl"

// the public API function
pro(JDRlexer, JDRstate* state) {
    a_dup(u8c, data, state->data);
    sane($ok(data));

    int cs = 0;
    u8c* p = (u8c*)data[0];
    u8c* pe = (u8c*)data[1];
    u8c* eof = pe;
    u64 mark0[64] = {};
    ok64 o = OK;

    utf8cs tok = {p, p};

#line 526 "JDR.rl.c"
    {
        cs = JDR_start;
    }

#line 409 "JDR.c.rl"

#line 529 "JDR.rl.c"
    {
        const char* _acts;
        unsigned int _nacts;

        if (p == pe) goto _test_eof;
        if (cs == 0) goto _out;
    _resume:
        switch (cs) {
            case 38:
                switch ((*p)) {
                    case 9u:
                        goto tr84;
                    case 10u:
                        goto tr85;
                    case 13u:
                        goto tr84;
                    case 32u:
                        goto tr84;
                    case 34u:
                        goto tr86;
                    case 40u:
                        goto tr87;
                    case 41u:
                        goto tr88;
                    case 44u:
                        goto tr89;
                    case 45u:
                        goto tr90;
                    case 48u:
                        goto tr91;
                    case 58u:
                        goto tr93;
                    case 60u:
                        goto tr94;
                    case 62u:
                        goto tr95;
                    case 91u:
                        goto tr97;
                    case 93u:
                        goto tr98;
                    case 96u:
                        goto tr99;
                    case 123u:
                        goto tr100;
                    case 125u:
                        goto tr101;
                    case 126u:
                        goto tr96;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr92;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr96;
                } else
                    goto tr96;
                goto tr1;
            case 0:
                goto _out;
            case 39:
                switch ((*p)) {
                    case 9u:
                        goto tr102;
                    case 10u:
                        goto tr103;
                    case 13u:
                        goto tr102;
                    case 32u:
                        goto tr102;
                    case 34u:
                        goto tr104;
                    case 40u:
                        goto tr105;
                    case 41u:
                        goto tr106;
                    case 44u:
                        goto tr107;
                    case 45u:
                        goto tr108;
                    case 48u:
                        goto tr109;
                    case 58u:
                        goto tr111;
                    case 60u:
                        goto tr112;
                    case 62u:
                        goto tr113;
                    case 91u:
                        goto tr115;
                    case 93u:
                        goto tr116;
                    case 96u:
                        goto tr117;
                    case 123u:
                        goto tr118;
                    case 125u:
                        goto tr119;
                    case 126u:
                        goto tr114;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr110;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr114;
                } else
                    goto tr114;
                goto tr1;
            case 40:
                switch ((*p)) {
                    case 9u:
                        goto tr120;
                    case 10u:
                        goto tr121;
                    case 13u:
                        goto tr120;
                    case 32u:
                        goto tr120;
                    case 34u:
                        goto tr122;
                    case 40u:
                        goto tr123;
                    case 41u:
                        goto tr124;
                    case 44u:
                        goto tr125;
                    case 45u:
                        goto tr126;
                    case 48u:
                        goto tr127;
                    case 58u:
                        goto tr129;
                    case 60u:
                        goto tr130;
                    case 62u:
                        goto tr131;
                    case 91u:
                        goto tr133;
                    case 93u:
                        goto tr134;
                    case 96u:
                        goto tr135;
                    case 123u:
                        goto tr136;
                    case 125u:
                        goto tr137;
                    case 126u:
                        goto tr132;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr128;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr132;
                } else
                    goto tr132;
                goto tr1;
            case 1:
                switch ((*p)) {
                    case 10u:
                        goto tr1;
                    case 13u:
                        goto tr1;
                    case 34u:
                        goto tr2;
                    case 92u:
                        goto tr3;
                }
                if ((*p) < 224u) {
                    if ((*p) > 191u) {
                        if (192u <= (*p) && (*p) <= 223u) goto tr4;
                    } else if ((*p) >= 128u)
                        goto tr1;
                } else if ((*p) > 239u) {
                    if ((*p) > 247u) {
                        if (248u <= (*p)) goto tr1;
                    } else if ((*p) >= 240u)
                        goto tr6;
                } else
                    goto tr5;
                goto tr0;
            case 2:
                switch ((*p)) {
                    case 10u:
                        goto tr1;
                    case 13u:
                        goto tr1;
                    case 34u:
                        goto tr8;
                    case 92u:
                        goto tr9;
                }
                if ((*p) < 224u) {
                    if ((*p) > 191u) {
                        if (192u <= (*p) && (*p) <= 223u) goto tr10;
                    } else if ((*p) >= 128u)
                        goto tr1;
                } else if ((*p) > 239u) {
                    if ((*p) > 247u) {
                        if (248u <= (*p)) goto tr1;
                    } else if ((*p) >= 240u)
                        goto tr12;
                } else
                    goto tr11;
                goto tr7;
            case 41:
                switch ((*p)) {
                    case 9u:
                        goto tr138;
                    case 10u:
                        goto tr139;
                    case 13u:
                        goto tr138;
                    case 32u:
                        goto tr138;
                    case 34u:
                        goto tr140;
                    case 40u:
                        goto tr141;
                    case 41u:
                        goto tr142;
                    case 44u:
                        goto tr143;
                    case 45u:
                        goto tr144;
                    case 48u:
                        goto tr145;
                    case 58u:
                        goto tr147;
                    case 60u:
                        goto tr148;
                    case 62u:
                        goto tr149;
                    case 64u:
                        goto tr150;
                    case 91u:
                        goto tr152;
                    case 93u:
                        goto tr153;
                    case 96u:
                        goto tr154;
                    case 123u:
                        goto tr155;
                    case 125u:
                        goto tr156;
                    case 126u:
                        goto tr151;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr146;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr151;
                } else
                    goto tr151;
                goto tr1;
            case 42:
                switch ((*p)) {
                    case 9u:
                        goto tr157;
                    case 10u:
                        goto tr158;
                    case 13u:
                        goto tr157;
                    case 32u:
                        goto tr157;
                    case 34u:
                        goto tr159;
                    case 40u:
                        goto tr160;
                    case 41u:
                        goto tr161;
                    case 44u:
                        goto tr162;
                    case 45u:
                        goto tr163;
                    case 48u:
                        goto tr164;
                    case 58u:
                        goto tr166;
                    case 60u:
                        goto tr167;
                    case 62u:
                        goto tr168;
                    case 64u:
                        goto tr169;
                    case 91u:
                        goto tr171;
                    case 93u:
                        goto tr172;
                    case 96u:
                        goto tr173;
                    case 123u:
                        goto tr174;
                    case 125u:
                        goto tr175;
                    case 126u:
                        goto tr170;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr165;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr170;
                } else
                    goto tr170;
                goto tr1;
            case 43:
                switch ((*p)) {
                    case 9u:
                        goto tr176;
                    case 10u:
                        goto tr177;
                    case 13u:
                        goto tr176;
                    case 32u:
                        goto tr176;
                    case 34u:
                        goto tr178;
                    case 40u:
                        goto tr179;
                    case 41u:
                        goto tr180;
                    case 44u:
                        goto tr181;
                    case 45u:
                        goto tr182;
                    case 48u:
                        goto tr183;
                    case 58u:
                        goto tr185;
                    case 60u:
                        goto tr186;
                    case 62u:
                        goto tr187;
                    case 64u:
                        goto tr188;
                    case 91u:
                        goto tr190;
                    case 93u:
                        goto tr191;
                    case 96u:
                        goto tr192;
                    case 123u:
                        goto tr193;
                    case 125u:
                        goto tr194;
                    case 126u:
                        goto tr189;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr184;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr189;
                } else
                    goto tr189;
                goto tr1;
            case 44:
                switch ((*p)) {
                    case 9u:
                        goto tr195;
                    case 10u:
                        goto tr196;
                    case 13u:
                        goto tr195;
                    case 32u:
                        goto tr195;
                    case 34u:
                        goto tr197;
                    case 40u:
                        goto tr198;
                    case 41u:
                        goto tr199;
                    case 44u:
                        goto tr200;
                    case 45u:
                        goto tr201;
                    case 48u:
                        goto tr202;
                    case 58u:
                        goto tr204;
                    case 60u:
                        goto tr205;
                    case 62u:
                        goto tr206;
                    case 64u:
                        goto tr207;
                    case 91u:
                        goto tr209;
                    case 93u:
                        goto tr210;
                    case 96u:
                        goto tr211;
                    case 123u:
                        goto tr212;
                    case 125u:
                        goto tr213;
                    case 126u:
                        goto tr208;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr203;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr208;
                } else
                    goto tr208;
                goto tr1;
            case 45:
                switch ((*p)) {
                    case 9u:
                        goto tr214;
                    case 10u:
                        goto tr215;
                    case 13u:
                        goto tr214;
                    case 32u:
                        goto tr214;
                    case 34u:
                        goto tr216;
                    case 40u:
                        goto tr217;
                    case 41u:
                        goto tr218;
                    case 44u:
                        goto tr219;
                    case 45u:
                        goto tr220;
                    case 48u:
                        goto tr221;
                    case 58u:
                        goto tr223;
                    case 60u:
                        goto tr224;
                    case 62u:
                        goto tr225;
                    case 64u:
                        goto tr226;
                    case 91u:
                        goto tr228;
                    case 93u:
                        goto tr229;
                    case 96u:
                        goto tr230;
                    case 123u:
                        goto tr231;
                    case 125u:
                        goto tr232;
                    case 126u:
                        goto tr227;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr222;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr227;
                } else
                    goto tr227;
                goto tr1;
            case 46:
                switch ((*p)) {
                    case 9u:
                        goto tr233;
                    case 10u:
                        goto tr234;
                    case 13u:
                        goto tr233;
                    case 32u:
                        goto tr233;
                    case 34u:
                        goto tr235;
                    case 40u:
                        goto tr236;
                    case 41u:
                        goto tr237;
                    case 44u:
                        goto tr238;
                    case 45u:
                        goto tr239;
                    case 48u:
                        goto tr240;
                    case 58u:
                        goto tr242;
                    case 60u:
                        goto tr243;
                    case 62u:
                        goto tr244;
                    case 64u:
                        goto tr245;
                    case 91u:
                        goto tr247;
                    case 93u:
                        goto tr248;
                    case 96u:
                        goto tr249;
                    case 123u:
                        goto tr250;
                    case 125u:
                        goto tr251;
                    case 126u:
                        goto tr246;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr241;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr246;
                } else
                    goto tr246;
                goto tr1;
            case 47:
                switch ((*p)) {
                    case 9u:
                        goto tr252;
                    case 10u:
                        goto tr253;
                    case 13u:
                        goto tr252;
                    case 32u:
                        goto tr252;
                    case 34u:
                        goto tr254;
                    case 40u:
                        goto tr255;
                    case 41u:
                        goto tr256;
                    case 44u:
                        goto tr257;
                    case 45u:
                        goto tr258;
                    case 48u:
                        goto tr259;
                    case 58u:
                        goto tr261;
                    case 60u:
                        goto tr262;
                    case 62u:
                        goto tr263;
                    case 91u:
                        goto tr265;
                    case 93u:
                        goto tr266;
                    case 96u:
                        goto tr267;
                    case 123u:
                        goto tr268;
                    case 125u:
                        goto tr269;
                    case 126u:
                        goto tr264;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr260;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr264;
                } else
                    goto tr264;
                goto tr1;
            case 48:
                switch ((*p)) {
                    case 9u:
                        goto tr270;
                    case 10u:
                        goto tr271;
                    case 13u:
                        goto tr270;
                    case 32u:
                        goto tr270;
                    case 34u:
                        goto tr272;
                    case 40u:
                        goto tr273;
                    case 41u:
                        goto tr274;
                    case 44u:
                        goto tr275;
                    case 45u:
                        goto tr276;
                    case 48u:
                        goto tr277;
                    case 58u:
                        goto tr279;
                    case 60u:
                        goto tr280;
                    case 62u:
                        goto tr281;
                    case 91u:
                        goto tr283;
                    case 93u:
                        goto tr284;
                    case 96u:
                        goto tr285;
                    case 123u:
                        goto tr286;
                    case 125u:
                        goto tr287;
                    case 126u:
                        goto tr282;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr278;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr282;
                } else
                    goto tr282;
                goto tr1;
            case 3:
                if ((*p) == 48u) goto tr13;
                if (49u <= (*p) && (*p) <= 57u) goto tr14;
                goto tr1;
            case 49:
                switch ((*p)) {
                    case 9u:
                        goto tr288;
                    case 10u:
                        goto tr289;
                    case 13u:
                        goto tr288;
                    case 32u:
                        goto tr288;
                    case 34u:
                        goto tr290;
                    case 40u:
                        goto tr291;
                    case 41u:
                        goto tr292;
                    case 44u:
                        goto tr293;
                    case 45u:
                        goto tr294;
                    case 46u:
                        goto tr295;
                    case 48u:
                        goto tr296;
                    case 58u:
                        goto tr298;
                    case 60u:
                        goto tr299;
                    case 62u:
                        goto tr300;
                    case 64u:
                        goto tr301;
                    case 69u:
                        goto tr303;
                    case 91u:
                        goto tr304;
                    case 93u:
                        goto tr305;
                    case 96u:
                        goto tr306;
                    case 101u:
                        goto tr303;
                    case 123u:
                        goto tr307;
                    case 125u:
                        goto tr308;
                    case 126u:
                        goto tr302;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr297;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr302;
                } else
                    goto tr302;
                goto tr1;
            case 4:
                if (48u <= (*p) && (*p) <= 57u) goto tr15;
                goto tr1;
            case 50:
                switch ((*p)) {
                    case 9u:
                        goto tr309;
                    case 10u:
                        goto tr310;
                    case 13u:
                        goto tr309;
                    case 32u:
                        goto tr309;
                    case 34u:
                        goto tr311;
                    case 40u:
                        goto tr312;
                    case 41u:
                        goto tr313;
                    case 44u:
                        goto tr314;
                    case 45u:
                        goto tr315;
                    case 58u:
                        goto tr316;
                    case 60u:
                        goto tr317;
                    case 62u:
                        goto tr318;
                    case 64u:
                        goto tr319;
                    case 69u:
                        goto tr321;
                    case 91u:
                        goto tr322;
                    case 93u:
                        goto tr323;
                    case 96u:
                        goto tr324;
                    case 101u:
                        goto tr321;
                    case 123u:
                        goto tr325;
                    case 125u:
                        goto tr326;
                    case 126u:
                        goto tr320;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr15;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr320;
                } else
                    goto tr320;
                goto tr1;
            case 51:
                switch ((*p)) {
                    case 9u:
                        goto tr327;
                    case 10u:
                        goto tr328;
                    case 13u:
                        goto tr327;
                    case 32u:
                        goto tr327;
                    case 34u:
                        goto tr329;
                    case 40u:
                        goto tr330;
                    case 41u:
                        goto tr331;
                    case 44u:
                        goto tr332;
                    case 45u:
                        goto tr333;
                    case 48u:
                        goto tr334;
                    case 58u:
                        goto tr336;
                    case 60u:
                        goto tr337;
                    case 62u:
                        goto tr338;
                    case 91u:
                        goto tr340;
                    case 93u:
                        goto tr341;
                    case 96u:
                        goto tr342;
                    case 123u:
                        goto tr343;
                    case 125u:
                        goto tr344;
                    case 126u:
                        goto tr339;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr335;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr339;
                } else
                    goto tr339;
                goto tr1;
            case 52:
                switch ((*p)) {
                    case 9u:
                        goto tr288;
                    case 10u:
                        goto tr289;
                    case 13u:
                        goto tr288;
                    case 32u:
                        goto tr288;
                    case 34u:
                        goto tr290;
                    case 40u:
                        goto tr291;
                    case 41u:
                        goto tr292;
                    case 44u:
                        goto tr293;
                    case 45u:
                        goto tr345;
                    case 46u:
                        goto tr295;
                    case 58u:
                        goto tr298;
                    case 60u:
                        goto tr299;
                    case 62u:
                        goto tr300;
                    case 64u:
                        goto tr301;
                    case 69u:
                        goto tr347;
                    case 91u:
                        goto tr304;
                    case 93u:
                        goto tr305;
                    case 96u:
                        goto tr306;
                    case 101u:
                        goto tr347;
                    case 123u:
                        goto tr307;
                    case 125u:
                        goto tr308;
                    case 126u:
                        goto tr346;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr346;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr346;
                } else
                    goto tr346;
                goto tr1;
            case 5:
                switch ((*p)) {
                    case 95u:
                        goto tr16;
                    case 126u:
                        goto tr16;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr16;
                } else if ((*p) > 90u) {
                    if (97u <= (*p) && (*p) <= 122u) goto tr16;
                } else
                    goto tr16;
                goto tr1;
            case 53:
                switch ((*p)) {
                    case 9u:
                        goto tr348;
                    case 10u:
                        goto tr349;
                    case 13u:
                        goto tr348;
                    case 32u:
                        goto tr348;
                    case 34u:
                        goto tr350;
                    case 40u:
                        goto tr351;
                    case 41u:
                        goto tr352;
                    case 44u:
                        goto tr353;
                    case 45u:
                        goto tr354;
                    case 58u:
                        goto tr355;
                    case 60u:
                        goto tr356;
                    case 62u:
                        goto tr357;
                    case 64u:
                        goto tr358;
                    case 91u:
                        goto tr359;
                    case 93u:
                        goto tr360;
                    case 96u:
                        goto tr361;
                    case 123u:
                        goto tr362;
                    case 125u:
                        goto tr363;
                    case 126u:
                        goto tr16;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr16;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr16;
                } else
                    goto tr16;
                goto tr1;
            case 54:
                switch ((*p)) {
                    case 9u:
                        goto tr364;
                    case 10u:
                        goto tr365;
                    case 13u:
                        goto tr364;
                    case 32u:
                        goto tr364;
                    case 34u:
                        goto tr366;
                    case 40u:
                        goto tr367;
                    case 41u:
                        goto tr368;
                    case 44u:
                        goto tr369;
                    case 45u:
                        goto tr370;
                    case 48u:
                        goto tr371;
                    case 58u:
                        goto tr373;
                    case 60u:
                        goto tr374;
                    case 62u:
                        goto tr375;
                    case 64u:
                        goto tr376;
                    case 91u:
                        goto tr378;
                    case 93u:
                        goto tr379;
                    case 96u:
                        goto tr380;
                    case 123u:
                        goto tr381;
                    case 125u:
                        goto tr382;
                    case 126u:
                        goto tr377;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr372;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr377;
                } else
                    goto tr377;
                goto tr1;
            case 55:
                switch ((*p)) {
                    case 9u:
                        goto tr288;
                    case 10u:
                        goto tr289;
                    case 13u:
                        goto tr288;
                    case 32u:
                        goto tr288;
                    case 34u:
                        goto tr290;
                    case 40u:
                        goto tr291;
                    case 41u:
                        goto tr292;
                    case 44u:
                        goto tr293;
                    case 45u:
                        goto tr345;
                    case 46u:
                        goto tr295;
                    case 58u:
                        goto tr298;
                    case 60u:
                        goto tr299;
                    case 62u:
                        goto tr300;
                    case 64u:
                        goto tr301;
                    case 69u:
                        goto tr347;
                    case 91u:
                        goto tr304;
                    case 93u:
                        goto tr305;
                    case 96u:
                        goto tr306;
                    case 101u:
                        goto tr347;
                    case 123u:
                        goto tr307;
                    case 125u:
                        goto tr308;
                    case 126u:
                        goto tr346;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr383;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr346;
                } else
                    goto tr346;
                goto tr1;
            case 56:
                switch ((*p)) {
                    case 9u:
                        goto tr384;
                    case 10u:
                        goto tr385;
                    case 13u:
                        goto tr384;
                    case 32u:
                        goto tr384;
                    case 34u:
                        goto tr386;
                    case 40u:
                        goto tr387;
                    case 41u:
                        goto tr388;
                    case 44u:
                        goto tr389;
                    case 45u:
                        goto tr390;
                    case 48u:
                        goto tr391;
                    case 58u:
                        goto tr393;
                    case 60u:
                        goto tr394;
                    case 62u:
                        goto tr395;
                    case 91u:
                        goto tr397;
                    case 93u:
                        goto tr398;
                    case 96u:
                        goto tr399;
                    case 123u:
                        goto tr400;
                    case 125u:
                        goto tr401;
                    case 126u:
                        goto tr396;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr392;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr396;
                } else
                    goto tr396;
                goto tr1;
            case 57:
                switch ((*p)) {
                    case 9u:
                        goto tr402;
                    case 10u:
                        goto tr403;
                    case 13u:
                        goto tr402;
                    case 32u:
                        goto tr402;
                    case 34u:
                        goto tr404;
                    case 40u:
                        goto tr405;
                    case 41u:
                        goto tr406;
                    case 44u:
                        goto tr407;
                    case 45u:
                        goto tr345;
                    case 58u:
                        goto tr408;
                    case 60u:
                        goto tr409;
                    case 62u:
                        goto tr410;
                    case 64u:
                        goto tr411;
                    case 91u:
                        goto tr412;
                    case 93u:
                        goto tr413;
                    case 96u:
                        goto tr414;
                    case 123u:
                        goto tr415;
                    case 125u:
                        goto tr416;
                    case 126u:
                        goto tr346;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr346;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr346;
                } else
                    goto tr346;
                goto tr1;
            case 6:
                switch ((*p)) {
                    case 95u:
                        goto tr17;
                    case 126u:
                        goto tr17;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr17;
                } else if ((*p) > 90u) {
                    if (97u <= (*p) && (*p) <= 122u) goto tr17;
                } else
                    goto tr17;
                goto tr1;
            case 58:
                switch ((*p)) {
                    case 9u:
                        goto tr417;
                    case 10u:
                        goto tr418;
                    case 13u:
                        goto tr417;
                    case 32u:
                        goto tr417;
                    case 34u:
                        goto tr419;
                    case 40u:
                        goto tr420;
                    case 41u:
                        goto tr421;
                    case 44u:
                        goto tr422;
                    case 45u:
                        goto tr423;
                    case 58u:
                        goto tr424;
                    case 60u:
                        goto tr425;
                    case 62u:
                        goto tr426;
                    case 91u:
                        goto tr427;
                    case 93u:
                        goto tr428;
                    case 96u:
                        goto tr429;
                    case 123u:
                        goto tr430;
                    case 125u:
                        goto tr431;
                    case 126u:
                        goto tr17;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr17;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr17;
                } else
                    goto tr17;
                goto tr1;
            case 7:
                switch ((*p)) {
                    case 95u:
                        goto tr18;
                    case 126u:
                        goto tr18;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr18;
                } else if ((*p) > 90u) {
                    if (97u <= (*p) && (*p) <= 122u) goto tr18;
                } else
                    goto tr18;
                goto tr1;
            case 59:
                switch ((*p)) {
                    case 9u:
                        goto tr417;
                    case 10u:
                        goto tr418;
                    case 13u:
                        goto tr417;
                    case 32u:
                        goto tr417;
                    case 34u:
                        goto tr419;
                    case 40u:
                        goto tr420;
                    case 41u:
                        goto tr421;
                    case 44u:
                        goto tr422;
                    case 45u:
                        goto tr432;
                    case 58u:
                        goto tr424;
                    case 60u:
                        goto tr425;
                    case 62u:
                        goto tr426;
                    case 91u:
                        goto tr427;
                    case 93u:
                        goto tr428;
                    case 96u:
                        goto tr429;
                    case 123u:
                        goto tr430;
                    case 125u:
                        goto tr431;
                    case 126u:
                        goto tr18;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr18;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr18;
                } else
                    goto tr18;
                goto tr1;
            case 60:
                switch ((*p)) {
                    case 9u:
                        goto tr433;
                    case 10u:
                        goto tr434;
                    case 13u:
                        goto tr433;
                    case 32u:
                        goto tr433;
                    case 34u:
                        goto tr435;
                    case 40u:
                        goto tr436;
                    case 41u:
                        goto tr437;
                    case 44u:
                        goto tr438;
                    case 45u:
                        goto tr439;
                    case 48u:
                        goto tr440;
                    case 58u:
                        goto tr442;
                    case 60u:
                        goto tr443;
                    case 62u:
                        goto tr444;
                    case 64u:
                        goto tr445;
                    case 91u:
                        goto tr447;
                    case 93u:
                        goto tr448;
                    case 96u:
                        goto tr449;
                    case 123u:
                        goto tr450;
                    case 125u:
                        goto tr451;
                    case 126u:
                        goto tr446;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr441;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr446;
                } else
                    goto tr446;
                goto tr1;
            case 8:
                switch ((*p)) {
                    case 95u:
                        goto tr19;
                    case 126u:
                        goto tr19;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr19;
                } else if ((*p) > 90u) {
                    if (97u <= (*p) && (*p) <= 122u) goto tr19;
                } else
                    goto tr19;
                goto tr1;
            case 61:
                switch ((*p)) {
                    case 9u:
                        goto tr452;
                    case 10u:
                        goto tr453;
                    case 13u:
                        goto tr452;
                    case 32u:
                        goto tr452;
                    case 34u:
                        goto tr454;
                    case 40u:
                        goto tr455;
                    case 41u:
                        goto tr456;
                    case 44u:
                        goto tr457;
                    case 45u:
                        goto tr458;
                    case 58u:
                        goto tr459;
                    case 60u:
                        goto tr460;
                    case 62u:
                        goto tr461;
                    case 91u:
                        goto tr462;
                    case 93u:
                        goto tr463;
                    case 96u:
                        goto tr464;
                    case 123u:
                        goto tr465;
                    case 125u:
                        goto tr466;
                    case 126u:
                        goto tr19;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr19;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr19;
                } else
                    goto tr19;
                goto tr1;
            case 9:
                switch ((*p)) {
                    case 95u:
                        goto tr20;
                    case 126u:
                        goto tr20;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr20;
                } else if ((*p) > 90u) {
                    if (97u <= (*p) && (*p) <= 122u) goto tr20;
                } else
                    goto tr20;
                goto tr1;
            case 62:
                switch ((*p)) {
                    case 9u:
                        goto tr452;
                    case 10u:
                        goto tr453;
                    case 13u:
                        goto tr452;
                    case 32u:
                        goto tr452;
                    case 34u:
                        goto tr454;
                    case 40u:
                        goto tr455;
                    case 41u:
                        goto tr456;
                    case 44u:
                        goto tr457;
                    case 45u:
                        goto tr467;
                    case 58u:
                        goto tr459;
                    case 60u:
                        goto tr460;
                    case 62u:
                        goto tr461;
                    case 91u:
                        goto tr462;
                    case 93u:
                        goto tr463;
                    case 96u:
                        goto tr464;
                    case 123u:
                        goto tr465;
                    case 125u:
                        goto tr466;
                    case 126u:
                        goto tr20;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr20;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr20;
                } else
                    goto tr20;
                goto tr1;
            case 63:
                switch ((*p)) {
                    case 9u:
                        goto tr468;
                    case 10u:
                        goto tr469;
                    case 13u:
                        goto tr468;
                    case 32u:
                        goto tr468;
                    case 34u:
                        goto tr470;
                    case 40u:
                        goto tr471;
                    case 41u:
                        goto tr472;
                    case 44u:
                        goto tr473;
                    case 45u:
                        goto tr474;
                    case 48u:
                        goto tr475;
                    case 58u:
                        goto tr477;
                    case 60u:
                        goto tr478;
                    case 62u:
                        goto tr479;
                    case 91u:
                        goto tr481;
                    case 93u:
                        goto tr482;
                    case 96u:
                        goto tr483;
                    case 123u:
                        goto tr484;
                    case 125u:
                        goto tr485;
                    case 126u:
                        goto tr480;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr476;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr480;
                } else
                    goto tr480;
                goto tr1;
            case 10:
                if ((*p) == 96u) goto tr22;
                if ((*p) < 224u) {
                    if ((*p) > 191u) {
                        if (192u <= (*p) && (*p) <= 223u) goto tr23;
                    } else if ((*p) >= 128u)
                        goto tr1;
                } else if ((*p) > 239u) {
                    if ((*p) > 247u) {
                        if (248u <= (*p)) goto tr1;
                    } else if ((*p) >= 240u)
                        goto tr25;
                } else
                    goto tr24;
                goto tr21;
            case 11:
                if ((*p) == 96u) goto tr27;
                if ((*p) < 224u) {
                    if ((*p) > 191u) {
                        if (192u <= (*p) && (*p) <= 223u) goto tr28;
                    } else if ((*p) >= 128u)
                        goto tr1;
                } else if ((*p) > 239u) {
                    if ((*p) > 247u) {
                        if (248u <= (*p)) goto tr1;
                    } else if ((*p) >= 240u)
                        goto tr30;
                } else
                    goto tr29;
                goto tr26;
            case 64:
                switch ((*p)) {
                    case 9u:
                        goto tr486;
                    case 10u:
                        goto tr487;
                    case 13u:
                        goto tr486;
                    case 32u:
                        goto tr486;
                    case 34u:
                        goto tr488;
                    case 40u:
                        goto tr489;
                    case 41u:
                        goto tr490;
                    case 44u:
                        goto tr491;
                    case 45u:
                        goto tr492;
                    case 48u:
                        goto tr493;
                    case 58u:
                        goto tr495;
                    case 60u:
                        goto tr496;
                    case 62u:
                        goto tr497;
                    case 64u:
                        goto tr498;
                    case 91u:
                        goto tr500;
                    case 93u:
                        goto tr501;
                    case 96u:
                        goto tr502;
                    case 123u:
                        goto tr503;
                    case 125u:
                        goto tr504;
                    case 126u:
                        goto tr499;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr494;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr499;
                } else
                    goto tr499;
                goto tr1;
            case 65:
                switch ((*p)) {
                    case 9u:
                        goto tr505;
                    case 10u:
                        goto tr506;
                    case 13u:
                        goto tr505;
                    case 32u:
                        goto tr505;
                    case 34u:
                        goto tr507;
                    case 40u:
                        goto tr508;
                    case 41u:
                        goto tr509;
                    case 44u:
                        goto tr510;
                    case 45u:
                        goto tr511;
                    case 48u:
                        goto tr512;
                    case 58u:
                        goto tr514;
                    case 60u:
                        goto tr515;
                    case 62u:
                        goto tr516;
                    case 64u:
                        goto tr517;
                    case 91u:
                        goto tr519;
                    case 93u:
                        goto tr520;
                    case 96u:
                        goto tr521;
                    case 123u:
                        goto tr522;
                    case 125u:
                        goto tr523;
                    case 126u:
                        goto tr518;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr513;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr518;
                } else
                    goto tr518;
                goto tr1;
            case 66:
                switch ((*p)) {
                    case 9u:
                        goto tr524;
                    case 10u:
                        goto tr525;
                    case 13u:
                        goto tr524;
                    case 32u:
                        goto tr524;
                    case 34u:
                        goto tr526;
                    case 40u:
                        goto tr527;
                    case 41u:
                        goto tr528;
                    case 44u:
                        goto tr529;
                    case 45u:
                        goto tr530;
                    case 48u:
                        goto tr531;
                    case 58u:
                        goto tr533;
                    case 60u:
                        goto tr534;
                    case 62u:
                        goto tr535;
                    case 91u:
                        goto tr537;
                    case 93u:
                        goto tr538;
                    case 96u:
                        goto tr539;
                    case 123u:
                        goto tr540;
                    case 125u:
                        goto tr541;
                    case 126u:
                        goto tr536;
                }
                if ((*p) < 65u) {
                    if (49u <= (*p) && (*p) <= 57u) goto tr532;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr536;
                } else
                    goto tr536;
                goto tr1;
            case 12:
                if (128u <= (*p) && (*p) <= 191u) goto tr31;
                goto tr1;
            case 13:
                if ((*p) == 96u) goto tr33;
                if ((*p) < 224u) {
                    if ((*p) > 191u) {
                        if (192u <= (*p) && (*p) <= 223u) goto tr34;
                    } else if ((*p) >= 128u)
                        goto tr1;
                } else if ((*p) > 239u) {
                    if ((*p) > 247u) {
                        if (248u <= (*p)) goto tr1;
                    } else if ((*p) >= 240u)
                        goto tr36;
                } else
                    goto tr35;
                goto tr32;
            case 14:
                if (128u <= (*p) && (*p) <= 191u) goto tr37;
                goto tr1;
            case 15:
                if (128u <= (*p) && (*p) <= 191u) goto tr38;
                goto tr1;
            case 16:
                if ((*p) == 96u) goto tr40;
                if ((*p) < 224u) {
                    if ((*p) > 191u) {
                        if (192u <= (*p) && (*p) <= 223u) goto tr41;
                    } else if ((*p) >= 128u)
                        goto tr1;
                } else if ((*p) > 239u) {
                    if ((*p) > 247u) {
                        if (248u <= (*p)) goto tr1;
                    } else if ((*p) >= 240u)
                        goto tr43;
                } else
                    goto tr42;
                goto tr39;
            case 17:
                if (128u <= (*p) && (*p) <= 191u) goto tr44;
                goto tr1;
            case 18:
                if (128u <= (*p) && (*p) <= 191u) goto tr45;
                goto tr1;
            case 19:
                if (128u <= (*p) && (*p) <= 191u) goto tr46;
                goto tr1;
            case 20:
                if ((*p) == 96u) goto tr48;
                if ((*p) < 224u) {
                    if ((*p) > 191u) {
                        if (192u <= (*p) && (*p) <= 223u) goto tr49;
                    } else if ((*p) >= 128u)
                        goto tr1;
                } else if ((*p) > 239u) {
                    if ((*p) > 247u) {
                        if (248u <= (*p)) goto tr1;
                    } else if ((*p) >= 240u)
                        goto tr51;
                } else
                    goto tr50;
                goto tr47;
            case 67:
                switch ((*p)) {
                    case 9u:
                        goto tr402;
                    case 10u:
                        goto tr403;
                    case 13u:
                        goto tr402;
                    case 32u:
                        goto tr402;
                    case 34u:
                        goto tr404;
                    case 40u:
                        goto tr405;
                    case 41u:
                        goto tr406;
                    case 43u:
                        goto tr54;
                    case 44u:
                        goto tr407;
                    case 45u:
                        goto tr542;
                    case 58u:
                        goto tr408;
                    case 60u:
                        goto tr409;
                    case 62u:
                        goto tr410;
                    case 64u:
                        goto tr411;
                    case 91u:
                        goto tr412;
                    case 93u:
                        goto tr413;
                    case 96u:
                        goto tr414;
                    case 123u:
                        goto tr415;
                    case 125u:
                        goto tr416;
                    case 126u:
                        goto tr346;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr543;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr346;
                } else
                    goto tr346;
                goto tr1;
            case 21:
                if (48u <= (*p) && (*p) <= 57u) goto tr52;
                goto tr1;
            case 68:
                switch ((*p)) {
                    case 9u:
                        goto tr309;
                    case 10u:
                        goto tr310;
                    case 13u:
                        goto tr309;
                    case 32u:
                        goto tr309;
                    case 34u:
                        goto tr311;
                    case 40u:
                        goto tr312;
                    case 41u:
                        goto tr313;
                    case 44u:
                        goto tr314;
                    case 45u:
                        goto tr315;
                    case 58u:
                        goto tr316;
                    case 60u:
                        goto tr317;
                    case 62u:
                        goto tr318;
                    case 64u:
                        goto tr319;
                    case 91u:
                        goto tr322;
                    case 93u:
                        goto tr323;
                    case 96u:
                        goto tr324;
                    case 123u:
                        goto tr325;
                    case 125u:
                        goto tr326;
                    case 126u:
                        goto tr320;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr52;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr320;
                } else
                    goto tr320;
                goto tr1;
            case 22:
                switch ((*p)) {
                    case 95u:
                        goto tr16;
                    case 126u:
                        goto tr16;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr53;
                } else if ((*p) > 90u) {
                    if (97u <= (*p) && (*p) <= 122u) goto tr16;
                } else
                    goto tr16;
                goto tr1;
            case 69:
                switch ((*p)) {
                    case 9u:
                        goto tr309;
                    case 10u:
                        goto tr310;
                    case 13u:
                        goto tr309;
                    case 32u:
                        goto tr309;
                    case 34u:
                        goto tr311;
                    case 40u:
                        goto tr312;
                    case 41u:
                        goto tr313;
                    case 44u:
                        goto tr314;
                    case 45u:
                        goto tr315;
                    case 58u:
                        goto tr316;
                    case 60u:
                        goto tr317;
                    case 62u:
                        goto tr318;
                    case 64u:
                        goto tr319;
                    case 91u:
                        goto tr322;
                    case 93u:
                        goto tr323;
                    case 96u:
                        goto tr324;
                    case 123u:
                        goto tr325;
                    case 125u:
                        goto tr326;
                    case 126u:
                        goto tr16;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr53;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr16;
                } else
                    goto tr16;
                goto tr1;
            case 70:
                switch ((*p)) {
                    case 9u:
                        goto tr309;
                    case 10u:
                        goto tr310;
                    case 13u:
                        goto tr309;
                    case 32u:
                        goto tr309;
                    case 34u:
                        goto tr311;
                    case 40u:
                        goto tr312;
                    case 41u:
                        goto tr313;
                    case 44u:
                        goto tr314;
                    case 45u:
                        goto tr345;
                    case 58u:
                        goto tr316;
                    case 60u:
                        goto tr317;
                    case 62u:
                        goto tr318;
                    case 64u:
                        goto tr319;
                    case 91u:
                        goto tr322;
                    case 93u:
                        goto tr323;
                    case 96u:
                        goto tr324;
                    case 123u:
                        goto tr325;
                    case 125u:
                        goto tr326;
                    case 126u:
                        goto tr346;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr543;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr346;
                } else
                    goto tr346;
                goto tr1;
            case 23:
                switch ((*p)) {
                    case 43u:
                        goto tr54;
                    case 45u:
                        goto tr54;
                }
                if (48u <= (*p) && (*p) <= 57u) goto tr52;
                goto tr1;
            case 71:
                switch ((*p)) {
                    case 9u:
                        goto tr288;
                    case 10u:
                        goto tr289;
                    case 13u:
                        goto tr288;
                    case 32u:
                        goto tr288;
                    case 34u:
                        goto tr290;
                    case 40u:
                        goto tr291;
                    case 41u:
                        goto tr292;
                    case 44u:
                        goto tr293;
                    case 45u:
                        goto tr294;
                    case 46u:
                        goto tr295;
                    case 58u:
                        goto tr298;
                    case 60u:
                        goto tr299;
                    case 62u:
                        goto tr300;
                    case 64u:
                        goto tr301;
                    case 69u:
                        goto tr303;
                    case 91u:
                        goto tr304;
                    case 93u:
                        goto tr305;
                    case 96u:
                        goto tr306;
                    case 101u:
                        goto tr303;
                    case 123u:
                        goto tr307;
                    case 125u:
                        goto tr308;
                    case 126u:
                        goto tr302;
                }
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr14;
                } else if ((*p) > 90u) {
                    if (95u <= (*p) && (*p) <= 122u) goto tr302;
                } else
                    goto tr302;
                goto tr1;
            case 24:
                switch ((*p)) {
                    case 34u:
                        goto tr55;
                    case 47u:
                        goto tr55;
                    case 92u:
                        goto tr55;
                    case 98u:
                        goto tr55;
                    case 102u:
                        goto tr55;
                    case 110u:
                        goto tr55;
                    case 114u:
                        goto tr55;
                    case 116u:
                        goto tr55;
                    case 117u:
                        goto tr56;
                }
                goto tr1;
            case 25:
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr57;
                } else if ((*p) > 90u) {
                    if (97u <= (*p) && (*p) <= 102u) goto tr57;
                } else
                    goto tr57;
                goto tr1;
            case 26:
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr58;
                } else if ((*p) > 90u) {
                    if (97u <= (*p) && (*p) <= 102u) goto tr58;
                } else
                    goto tr58;
                goto tr1;
            case 27:
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr59;
                } else if ((*p) > 90u) {
                    if (97u <= (*p) && (*p) <= 102u) goto tr59;
                } else
                    goto tr59;
                goto tr1;
            case 28:
                if ((*p) < 65u) {
                    if (48u <= (*p) && (*p) <= 57u) goto tr55;
                } else if ((*p) > 90u) {
                    if (97u <= (*p) && (*p) <= 102u) goto tr55;
                } else
                    goto tr55;
                goto tr1;
            case 29:
                if (128u <= (*p) && (*p) <= 191u) goto tr60;
                goto tr1;
            case 30:
                switch ((*p)) {
                    case 10u:
                        goto tr1;
                    case 13u:
                        goto tr1;
                    case 34u:
                        goto tr62;
                    case 92u:
                        goto tr63;
                }
                if ((*p) < 224u) {
                    if ((*p) > 191u) {
                        if (192u <= (*p) && (*p) <= 223u) goto tr64;
                    } else if ((*p) >= 128u)
                        goto tr1;
                } else if ((*p) > 239u) {
                    if ((*p) > 247u) {
                        if (248u <= (*p)) goto tr1;
                    } else if ((*p) >= 240u)
                        goto tr66;
                } else
                    goto tr65;
                goto tr61;
            case 31:
                if (128u <= (*p) && (*p) <= 191u) goto tr67;
                goto tr1;
            case 32:
                if (128u <= (*p) && (*p) <= 191u) goto tr68;
                goto tr1;
            case 33:
                switch ((*p)) {
                    case 10u:
                        goto tr1;
                    case 13u:
                        goto tr1;
                    case 34u:
                        goto tr70;
                    case 92u:
                        goto tr71;
                }
                if ((*p) < 224u) {
                    if ((*p) > 191u) {
                        if (192u <= (*p) && (*p) <= 223u) goto tr72;
                    } else if ((*p) >= 128u)
                        goto tr1;
                } else if ((*p) > 239u) {
                    if ((*p) > 247u) {
                        if (248u <= (*p)) goto tr1;
                    } else if ((*p) >= 240u)
                        goto tr74;
                } else
                    goto tr73;
                goto tr69;
            case 34:
                if (128u <= (*p) && (*p) <= 191u) goto tr75;
                goto tr1;
            case 35:
                if (128u <= (*p) && (*p) <= 191u) goto tr76;
                goto tr1;
            case 36:
                if (128u <= (*p) && (*p) <= 191u) goto tr77;
                goto tr1;
            case 37:
                switch ((*p)) {
                    case 10u:
                        goto tr1;
                    case 13u:
                        goto tr1;
                    case 34u:
                        goto tr79;
                    case 92u:
                        goto tr80;
                }
                if ((*p) < 224u) {
                    if ((*p) > 191u) {
                        if (192u <= (*p) && (*p) <= 223u) goto tr81;
                    } else if ((*p) >= 128u)
                        goto tr1;
                } else if ((*p) > 239u) {
                    if ((*p) > 247u) {
                        if (248u <= (*p)) goto tr1;
                    } else if ((*p) >= 240u)
                        goto tr83;
                } else
                    goto tr82;
                goto tr78;
        }

    tr1:
        cs = 0;
        goto _again;
    tr55:
        cs = 1;
        goto _again;
    tr86:
        cs = 1;
        goto f27;
    tr104:
        cs = 1;
        goto f44;
    tr122:
        cs = 1;
        goto f62;
    tr140:
        cs = 1;
        goto f80;
    tr159:
        cs = 1;
        goto f97;
    tr178:
        cs = 1;
        goto f114;
    tr197:
        cs = 1;
        goto f133;
    tr216:
        cs = 1;
        goto f150;
    tr235:
        cs = 1;
        goto f166;
    tr254:
        cs = 1;
        goto f184;
    tr272:
        cs = 1;
        goto f202;
    tr290:
        cs = 1;
        goto f220;
    tr311:
        cs = 1;
        goto f239;
    tr329:
        cs = 1;
        goto f257;
    tr350:
        cs = 1;
        goto f275;
    tr366:
        cs = 1;
        goto f292;
    tr386:
        cs = 1;
        goto f311;
    tr404:
        cs = 1;
        goto f329;
    tr419:
        cs = 1;
        goto f345;
    tr435:
        cs = 1;
        goto f361;
    tr454:
        cs = 1;
        goto f380;
    tr470:
        cs = 1;
        goto f396;
    tr488:
        cs = 1;
        goto f414;
    tr507:
        cs = 1;
        goto f433;
    tr526:
        cs = 1;
        goto f452;
    tr0:
        cs = 2;
        goto f0;
    tr7:
        cs = 2;
        goto f4;
    tr61:
        cs = 2;
        goto f9;
    tr69:
        cs = 2;
        goto f14;
    tr78:
        cs = 2;
        goto f19;
    tr90:
        cs = 3;
        goto f31;
    tr108:
        cs = 3;
        goto f48;
    tr126:
        cs = 3;
        goto f66;
    tr144:
        cs = 3;
        goto f84;
    tr163:
        cs = 3;
        goto f101;
    tr182:
        cs = 3;
        goto f118;
    tr201:
        cs = 3;
        goto f137;
    tr220:
        cs = 3;
        goto f154;
    tr239:
        cs = 3;
        goto f170;
    tr258:
        cs = 3;
        goto f188;
    tr276:
        cs = 3;
        goto f206;
    tr294:
        cs = 3;
        goto f224;
    tr315:
        cs = 3;
        goto f243;
    tr333:
        cs = 3;
        goto f261;
    tr354:
        cs = 3;
        goto f279;
    tr370:
        cs = 3;
        goto f296;
    tr390:
        cs = 3;
        goto f315;
    tr432:
        cs = 3;
        goto f357;
    tr439:
        cs = 3;
        goto f365;
    tr467:
        cs = 3;
        goto f392;
    tr474:
        cs = 3;
        goto f400;
    tr492:
        cs = 3;
        goto f418;
    tr511:
        cs = 3;
        goto f437;
    tr530:
        cs = 3;
        goto f456;
    tr295:
        cs = 4;
        goto f218;
    tr345:
        cs = 5;
        goto _again;
    tr150:
        cs = 6;
        goto f89;
    tr169:
        cs = 6;
        goto f106;
    tr188:
        cs = 6;
        goto f123;
    tr301:
        cs = 6;
        goto f229;
    tr319:
        cs = 6;
        goto f247;
    tr358:
        cs = 6;
        goto f283;
    tr411:
        cs = 6;
        goto f336;
    tr498:
        cs = 6;
        goto f423;
    tr423:
        cs = 7;
        goto _again;
    tr226:
        cs = 8;
        goto f106;
    tr245:
        cs = 8;
        goto f123;
    tr207:
        cs = 8;
        goto f142;
    tr376:
        cs = 8;
        goto f301;
    tr445:
        cs = 8;
        goto f370;
    tr517:
        cs = 8;
        goto f442;
    tr458:
        cs = 9;
        goto _again;
    tr99:
        cs = 10;
        goto f39;
    tr117:
        cs = 10;
        goto f56;
    tr135:
        cs = 10;
        goto f74;
    tr154:
        cs = 10;
        goto f93;
    tr173:
        cs = 10;
        goto f110;
    tr192:
        cs = 10;
        goto f127;
    tr211:
        cs = 10;
        goto f146;
    tr230:
        cs = 10;
        goto f162;
    tr249:
        cs = 10;
        goto f178;
    tr267:
        cs = 10;
        goto f196;
    tr285:
        cs = 10;
        goto f214;
    tr306:
        cs = 10;
        goto f233;
    tr324:
        cs = 10;
        goto f251;
    tr342:
        cs = 10;
        goto f269;
    tr361:
        cs = 10;
        goto f286;
    tr380:
        cs = 10;
        goto f305;
    tr399:
        cs = 10;
        goto f323;
    tr414:
        cs = 10;
        goto f339;
    tr429:
        cs = 10;
        goto f354;
    tr449:
        cs = 10;
        goto f374;
    tr464:
        cs = 10;
        goto f389;
    tr483:
        cs = 10;
        goto f408;
    tr502:
        cs = 10;
        goto f427;
    tr521:
        cs = 10;
        goto f446;
    tr539:
        cs = 10;
        goto f464;
    tr21:
        cs = 11;
        goto f0;
    tr26:
        cs = 11;
        goto f4;
    tr32:
        cs = 11;
        goto f9;
    tr39:
        cs = 11;
        goto f14;
    tr47:
        cs = 11;
        goto f19;
    tr23:
        cs = 12;
        goto f1;
    tr28:
        cs = 12;
        goto f6;
    tr34:
        cs = 12;
        goto f11;
    tr41:
        cs = 12;
        goto f16;
    tr49:
        cs = 12;
        goto f21;
    tr31:
        cs = 13;
        goto _again;
    tr24:
        cs = 14;
        goto f2;
    tr29:
        cs = 14;
        goto f7;
    tr35:
        cs = 14;
        goto f12;
    tr42:
        cs = 14;
        goto f17;
    tr50:
        cs = 14;
        goto f22;
    tr37:
        cs = 15;
        goto _again;
    tr38:
        cs = 16;
        goto _again;
    tr25:
        cs = 17;
        goto f3;
    tr30:
        cs = 17;
        goto f8;
    tr36:
        cs = 17;
        goto f13;
    tr43:
        cs = 17;
        goto f18;
    tr51:
        cs = 17;
        goto f23;
    tr44:
        cs = 18;
        goto _again;
    tr45:
        cs = 19;
        goto _again;
    tr46:
        cs = 20;
        goto _again;
    tr54:
        cs = 21;
        goto _again;
    tr542:
        cs = 22;
        goto _again;
    tr321:
        cs = 23;
        goto _again;
    tr303:
        cs = 23;
        goto f218;
    tr3:
        cs = 24;
        goto _again;
    tr9:
        cs = 24;
        goto f5;
    tr63:
        cs = 24;
        goto f10;
    tr71:
        cs = 24;
        goto f15;
    tr80:
        cs = 24;
        goto f20;
    tr56:
        cs = 25;
        goto _again;
    tr57:
        cs = 26;
        goto _again;
    tr58:
        cs = 27;
        goto _again;
    tr59:
        cs = 28;
        goto _again;
    tr4:
        cs = 29;
        goto f1;
    tr10:
        cs = 29;
        goto f6;
    tr64:
        cs = 29;
        goto f11;
    tr72:
        cs = 29;
        goto f16;
    tr81:
        cs = 29;
        goto f21;
    tr60:
        cs = 30;
        goto _again;
    tr5:
        cs = 31;
        goto f2;
    tr11:
        cs = 31;
        goto f7;
    tr65:
        cs = 31;
        goto f12;
    tr73:
        cs = 31;
        goto f17;
    tr82:
        cs = 31;
        goto f22;
    tr67:
        cs = 32;
        goto _again;
    tr68:
        cs = 33;
        goto _again;
    tr6:
        cs = 34;
        goto f3;
    tr12:
        cs = 34;
        goto f8;
    tr66:
        cs = 34;
        goto f13;
    tr74:
        cs = 34;
        goto f18;
    tr83:
        cs = 34;
        goto f23;
    tr75:
        cs = 35;
        goto _again;
    tr76:
        cs = 36;
        goto _again;
    tr77:
        cs = 37;
        goto _again;
    tr102:
        cs = 39;
        goto _again;
    tr84:
        cs = 39;
        goto f25;
    tr120:
        cs = 39;
        goto f60;
    tr252:
        cs = 39;
        goto f182;
    tr270:
        cs = 39;
        goto f200;
    tr327:
        cs = 39;
        goto f255;
    tr384:
        cs = 39;
        goto f309;
    tr417:
        cs = 39;
        goto f343;
    tr452:
        cs = 39;
        goto f378;
    tr468:
        cs = 39;
        goto f394;
    tr524:
        cs = 39;
        goto f450;
    tr85:
        cs = 40;
        goto f26;
    tr103:
        cs = 40;
        goto f43;
    tr121:
        cs = 40;
        goto f61;
    tr253:
        cs = 40;
        goto f183;
    tr271:
        cs = 40;
        goto f201;
    tr328:
        cs = 40;
        goto f256;
    tr385:
        cs = 40;
        goto f310;
    tr418:
        cs = 40;
        goto f344;
    tr453:
        cs = 40;
        goto f379;
    tr469:
        cs = 40;
        goto f395;
    tr525:
        cs = 40;
        goto f451;
    tr2:
        cs = 41;
        goto _again;
    tr8:
        cs = 41;
        goto f5;
    tr62:
        cs = 41;
        goto f10;
    tr70:
        cs = 41;
        goto f15;
    tr79:
        cs = 41;
        goto f20;
    tr157:
        cs = 42;
        goto _again;
    tr176:
        cs = 42;
        goto f60;
    tr138:
        cs = 42;
        goto f78;
    tr288:
        cs = 42;
        goto f218;
    tr309:
        cs = 42;
        goto f237;
    tr348:
        cs = 42;
        goto f273;
    tr402:
        cs = 42;
        goto f327;
    tr486:
        cs = 42;
        goto f412;
    tr158:
        cs = 43;
        goto f43;
    tr177:
        cs = 43;
        goto f61;
    tr139:
        cs = 43;
        goto f79;
    tr289:
        cs = 43;
        goto f219;
    tr310:
        cs = 43;
        goto f238;
    tr349:
        cs = 43;
        goto f274;
    tr403:
        cs = 43;
        goto f328;
    tr487:
        cs = 43;
        goto f413;
    tr87:
        cs = 44;
        goto f28;
    tr105:
        cs = 44;
        goto f45;
    tr123:
        cs = 44;
        goto f63;
    tr141:
        cs = 44;
        goto f81;
    tr160:
        cs = 44;
        goto f98;
    tr179:
        cs = 44;
        goto f115;
    tr198:
        cs = 44;
        goto f134;
    tr217:
        cs = 44;
        goto f151;
    tr236:
        cs = 44;
        goto f167;
    tr255:
        cs = 44;
        goto f185;
    tr273:
        cs = 44;
        goto f203;
    tr291:
        cs = 44;
        goto f221;
    tr312:
        cs = 44;
        goto f240;
    tr330:
        cs = 44;
        goto f258;
    tr351:
        cs = 44;
        goto f276;
    tr367:
        cs = 44;
        goto f293;
    tr387:
        cs = 44;
        goto f312;
    tr405:
        cs = 44;
        goto f330;
    tr420:
        cs = 44;
        goto f346;
    tr436:
        cs = 44;
        goto f362;
    tr455:
        cs = 44;
        goto f381;
    tr471:
        cs = 44;
        goto f397;
    tr489:
        cs = 44;
        goto f415;
    tr508:
        cs = 44;
        goto f434;
    tr527:
        cs = 44;
        goto f453;
    tr214:
        cs = 45;
        goto _again;
    tr233:
        cs = 45;
        goto f60;
    tr195:
        cs = 45;
        goto f131;
    tr364:
        cs = 45;
        goto f290;
    tr433:
        cs = 45;
        goto f359;
    tr505:
        cs = 45;
        goto f431;
    tr215:
        cs = 46;
        goto f43;
    tr234:
        cs = 46;
        goto f61;
    tr196:
        cs = 46;
        goto f132;
    tr365:
        cs = 46;
        goto f291;
    tr434:
        cs = 46;
        goto f360;
    tr506:
        cs = 46;
        goto f432;
    tr88:
        cs = 47;
        goto f29;
    tr106:
        cs = 47;
        goto f46;
    tr124:
        cs = 47;
        goto f64;
    tr142:
        cs = 47;
        goto f82;
    tr161:
        cs = 47;
        goto f99;
    tr180:
        cs = 47;
        goto f116;
    tr199:
        cs = 47;
        goto f135;
    tr218:
        cs = 47;
        goto f152;
    tr237:
        cs = 47;
        goto f168;
    tr256:
        cs = 47;
        goto f186;
    tr274:
        cs = 47;
        goto f204;
    tr292:
        cs = 47;
        goto f222;
    tr313:
        cs = 47;
        goto f241;
    tr331:
        cs = 47;
        goto f259;
    tr352:
        cs = 47;
        goto f277;
    tr368:
        cs = 47;
        goto f294;
    tr388:
        cs = 47;
        goto f313;
    tr406:
        cs = 47;
        goto f331;
    tr421:
        cs = 47;
        goto f347;
    tr437:
        cs = 47;
        goto f363;
    tr456:
        cs = 47;
        goto f382;
    tr472:
        cs = 47;
        goto f398;
    tr490:
        cs = 47;
        goto f416;
    tr509:
        cs = 47;
        goto f435;
    tr528:
        cs = 47;
        goto f454;
    tr89:
        cs = 48;
        goto f30;
    tr107:
        cs = 48;
        goto f47;
    tr125:
        cs = 48;
        goto f65;
    tr143:
        cs = 48;
        goto f83;
    tr162:
        cs = 48;
        goto f100;
    tr181:
        cs = 48;
        goto f117;
    tr200:
        cs = 48;
        goto f136;
    tr219:
        cs = 48;
        goto f153;
    tr238:
        cs = 48;
        goto f169;
    tr257:
        cs = 48;
        goto f187;
    tr275:
        cs = 48;
        goto f205;
    tr293:
        cs = 48;
        goto f223;
    tr314:
        cs = 48;
        goto f242;
    tr332:
        cs = 48;
        goto f260;
    tr353:
        cs = 48;
        goto f278;
    tr369:
        cs = 48;
        goto f295;
    tr389:
        cs = 48;
        goto f314;
    tr407:
        cs = 48;
        goto f332;
    tr422:
        cs = 48;
        goto f348;
    tr438:
        cs = 48;
        goto f364;
    tr457:
        cs = 48;
        goto f383;
    tr473:
        cs = 48;
        goto f399;
    tr491:
        cs = 48;
        goto f417;
    tr510:
        cs = 48;
        goto f436;
    tr529:
        cs = 48;
        goto f455;
    tr13:
        cs = 49;
        goto _again;
    tr15:
        cs = 50;
        goto _again;
    tr93:
        cs = 51;
        goto f33;
    tr111:
        cs = 51;
        goto f50;
    tr129:
        cs = 51;
        goto f68;
    tr147:
        cs = 51;
        goto f86;
    tr166:
        cs = 51;
        goto f103;
    tr185:
        cs = 51;
        goto f120;
    tr204:
        cs = 51;
        goto f139;
    tr223:
        cs = 51;
        goto f156;
    tr242:
        cs = 51;
        goto f172;
    tr261:
        cs = 51;
        goto f190;
    tr279:
        cs = 51;
        goto f208;
    tr298:
        cs = 51;
        goto f226;
    tr316:
        cs = 51;
        goto f244;
    tr336:
        cs = 51;
        goto f263;
    tr355:
        cs = 51;
        goto f280;
    tr373:
        cs = 51;
        goto f298;
    tr393:
        cs = 51;
        goto f317;
    tr408:
        cs = 51;
        goto f333;
    tr424:
        cs = 51;
        goto f349;
    tr442:
        cs = 51;
        goto f367;
    tr459:
        cs = 51;
        goto f384;
    tr477:
        cs = 51;
        goto f402;
    tr495:
        cs = 51;
        goto f420;
    tr514:
        cs = 51;
        goto f439;
    tr533:
        cs = 51;
        goto f458;
    tr91:
        cs = 52;
        goto f32;
    tr109:
        cs = 52;
        goto f49;
    tr127:
        cs = 52;
        goto f67;
    tr145:
        cs = 52;
        goto f85;
    tr164:
        cs = 52;
        goto f102;
    tr183:
        cs = 52;
        goto f119;
    tr202:
        cs = 52;
        goto f138;
    tr221:
        cs = 52;
        goto f155;
    tr240:
        cs = 52;
        goto f171;
    tr259:
        cs = 52;
        goto f189;
    tr277:
        cs = 52;
        goto f207;
    tr296:
        cs = 52;
        goto f225;
    tr334:
        cs = 52;
        goto f262;
    tr371:
        cs = 52;
        goto f297;
    tr391:
        cs = 52;
        goto f316;
    tr440:
        cs = 52;
        goto f366;
    tr475:
        cs = 52;
        goto f401;
    tr493:
        cs = 52;
        goto f419;
    tr512:
        cs = 52;
        goto f438;
    tr531:
        cs = 52;
        goto f457;
    tr16:
        cs = 53;
        goto _again;
    tr94:
        cs = 54;
        goto f34;
    tr112:
        cs = 54;
        goto f51;
    tr130:
        cs = 54;
        goto f69;
    tr148:
        cs = 54;
        goto f87;
    tr167:
        cs = 54;
        goto f104;
    tr186:
        cs = 54;
        goto f121;
    tr205:
        cs = 54;
        goto f140;
    tr224:
        cs = 54;
        goto f157;
    tr243:
        cs = 54;
        goto f173;
    tr262:
        cs = 54;
        goto f191;
    tr280:
        cs = 54;
        goto f209;
    tr299:
        cs = 54;
        goto f227;
    tr317:
        cs = 54;
        goto f245;
    tr337:
        cs = 54;
        goto f264;
    tr356:
        cs = 54;
        goto f281;
    tr374:
        cs = 54;
        goto f299;
    tr394:
        cs = 54;
        goto f318;
    tr409:
        cs = 54;
        goto f334;
    tr425:
        cs = 54;
        goto f350;
    tr443:
        cs = 54;
        goto f368;
    tr460:
        cs = 54;
        goto f385;
    tr478:
        cs = 54;
        goto f403;
    tr496:
        cs = 54;
        goto f421;
    tr515:
        cs = 54;
        goto f440;
    tr534:
        cs = 54;
        goto f459;
    tr383:
        cs = 55;
        goto _again;
    tr92:
        cs = 55;
        goto f32;
    tr110:
        cs = 55;
        goto f49;
    tr128:
        cs = 55;
        goto f67;
    tr146:
        cs = 55;
        goto f85;
    tr165:
        cs = 55;
        goto f102;
    tr184:
        cs = 55;
        goto f119;
    tr203:
        cs = 55;
        goto f138;
    tr222:
        cs = 55;
        goto f155;
    tr241:
        cs = 55;
        goto f171;
    tr260:
        cs = 55;
        goto f189;
    tr278:
        cs = 55;
        goto f207;
    tr297:
        cs = 55;
        goto f225;
    tr335:
        cs = 55;
        goto f262;
    tr372:
        cs = 55;
        goto f297;
    tr392:
        cs = 55;
        goto f316;
    tr441:
        cs = 55;
        goto f366;
    tr476:
        cs = 55;
        goto f401;
    tr494:
        cs = 55;
        goto f419;
    tr513:
        cs = 55;
        goto f438;
    tr532:
        cs = 55;
        goto f457;
    tr95:
        cs = 56;
        goto f35;
    tr113:
        cs = 56;
        goto f52;
    tr131:
        cs = 56;
        goto f70;
    tr149:
        cs = 56;
        goto f88;
    tr168:
        cs = 56;
        goto f105;
    tr187:
        cs = 56;
        goto f122;
    tr206:
        cs = 56;
        goto f141;
    tr225:
        cs = 56;
        goto f158;
    tr244:
        cs = 56;
        goto f174;
    tr263:
        cs = 56;
        goto f192;
    tr281:
        cs = 56;
        goto f210;
    tr300:
        cs = 56;
        goto f228;
    tr318:
        cs = 56;
        goto f246;
    tr338:
        cs = 56;
        goto f265;
    tr357:
        cs = 56;
        goto f282;
    tr375:
        cs = 56;
        goto f300;
    tr395:
        cs = 56;
        goto f319;
    tr410:
        cs = 56;
        goto f335;
    tr426:
        cs = 56;
        goto f351;
    tr444:
        cs = 56;
        goto f369;
    tr461:
        cs = 56;
        goto f386;
    tr479:
        cs = 56;
        goto f404;
    tr497:
        cs = 56;
        goto f422;
    tr516:
        cs = 56;
        goto f441;
    tr535:
        cs = 56;
        goto f460;
    tr346:
        cs = 57;
        goto _again;
    tr96:
        cs = 57;
        goto f36;
    tr114:
        cs = 57;
        goto f53;
    tr132:
        cs = 57;
        goto f71;
    tr151:
        cs = 57;
        goto f90;
    tr170:
        cs = 57;
        goto f107;
    tr189:
        cs = 57;
        goto f124;
    tr208:
        cs = 57;
        goto f143;
    tr227:
        cs = 57;
        goto f159;
    tr246:
        cs = 57;
        goto f175;
    tr264:
        cs = 57;
        goto f193;
    tr282:
        cs = 57;
        goto f211;
    tr302:
        cs = 57;
        goto f230;
    tr320:
        cs = 57;
        goto f248;
    tr339:
        cs = 57;
        goto f266;
    tr377:
        cs = 57;
        goto f302;
    tr396:
        cs = 57;
        goto f320;
    tr446:
        cs = 57;
        goto f371;
    tr480:
        cs = 57;
        goto f405;
    tr499:
        cs = 57;
        goto f424;
    tr518:
        cs = 57;
        goto f443;
    tr536:
        cs = 57;
        goto f461;
    tr17:
        cs = 58;
        goto _again;
    tr18:
        cs = 59;
        goto _again;
    tr97:
        cs = 60;
        goto f37;
    tr115:
        cs = 60;
        goto f54;
    tr133:
        cs = 60;
        goto f72;
    tr152:
        cs = 60;
        goto f91;
    tr171:
        cs = 60;
        goto f108;
    tr190:
        cs = 60;
        goto f125;
    tr209:
        cs = 60;
        goto f144;
    tr228:
        cs = 60;
        goto f160;
    tr247:
        cs = 60;
        goto f176;
    tr265:
        cs = 60;
        goto f194;
    tr283:
        cs = 60;
        goto f212;
    tr304:
        cs = 60;
        goto f231;
    tr322:
        cs = 60;
        goto f249;
    tr340:
        cs = 60;
        goto f267;
    tr359:
        cs = 60;
        goto f284;
    tr378:
        cs = 60;
        goto f303;
    tr397:
        cs = 60;
        goto f321;
    tr412:
        cs = 60;
        goto f337;
    tr427:
        cs = 60;
        goto f352;
    tr447:
        cs = 60;
        goto f372;
    tr462:
        cs = 60;
        goto f387;
    tr481:
        cs = 60;
        goto f406;
    tr500:
        cs = 60;
        goto f425;
    tr519:
        cs = 60;
        goto f444;
    tr537:
        cs = 60;
        goto f462;
    tr19:
        cs = 61;
        goto _again;
    tr20:
        cs = 62;
        goto _again;
    tr98:
        cs = 63;
        goto f38;
    tr116:
        cs = 63;
        goto f55;
    tr134:
        cs = 63;
        goto f73;
    tr153:
        cs = 63;
        goto f92;
    tr172:
        cs = 63;
        goto f109;
    tr191:
        cs = 63;
        goto f126;
    tr210:
        cs = 63;
        goto f145;
    tr229:
        cs = 63;
        goto f161;
    tr248:
        cs = 63;
        goto f177;
    tr266:
        cs = 63;
        goto f195;
    tr284:
        cs = 63;
        goto f213;
    tr305:
        cs = 63;
        goto f232;
    tr323:
        cs = 63;
        goto f250;
    tr341:
        cs = 63;
        goto f268;
    tr360:
        cs = 63;
        goto f285;
    tr379:
        cs = 63;
        goto f304;
    tr398:
        cs = 63;
        goto f322;
    tr413:
        cs = 63;
        goto f338;
    tr428:
        cs = 63;
        goto f353;
    tr448:
        cs = 63;
        goto f373;
    tr463:
        cs = 63;
        goto f388;
    tr482:
        cs = 63;
        goto f407;
    tr501:
        cs = 63;
        goto f426;
    tr520:
        cs = 63;
        goto f445;
    tr538:
        cs = 63;
        goto f463;
    tr22:
        cs = 64;
        goto _again;
    tr27:
        cs = 64;
        goto f5;
    tr33:
        cs = 64;
        goto f10;
    tr40:
        cs = 64;
        goto f15;
    tr48:
        cs = 64;
        goto f20;
    tr100:
        cs = 65;
        goto f40;
    tr118:
        cs = 65;
        goto f57;
    tr136:
        cs = 65;
        goto f75;
    tr155:
        cs = 65;
        goto f94;
    tr174:
        cs = 65;
        goto f111;
    tr193:
        cs = 65;
        goto f128;
    tr212:
        cs = 65;
        goto f147;
    tr231:
        cs = 65;
        goto f163;
    tr250:
        cs = 65;
        goto f179;
    tr268:
        cs = 65;
        goto f197;
    tr286:
        cs = 65;
        goto f215;
    tr307:
        cs = 65;
        goto f234;
    tr325:
        cs = 65;
        goto f252;
    tr343:
        cs = 65;
        goto f270;
    tr362:
        cs = 65;
        goto f287;
    tr381:
        cs = 65;
        goto f306;
    tr400:
        cs = 65;
        goto f324;
    tr415:
        cs = 65;
        goto f340;
    tr430:
        cs = 65;
        goto f355;
    tr450:
        cs = 65;
        goto f375;
    tr465:
        cs = 65;
        goto f390;
    tr484:
        cs = 65;
        goto f409;
    tr503:
        cs = 65;
        goto f428;
    tr522:
        cs = 65;
        goto f447;
    tr540:
        cs = 65;
        goto f465;
    tr101:
        cs = 66;
        goto f41;
    tr119:
        cs = 66;
        goto f58;
    tr137:
        cs = 66;
        goto f76;
    tr156:
        cs = 66;
        goto f95;
    tr175:
        cs = 66;
        goto f112;
    tr194:
        cs = 66;
        goto f129;
    tr213:
        cs = 66;
        goto f148;
    tr232:
        cs = 66;
        goto f164;
    tr251:
        cs = 66;
        goto f180;
    tr269:
        cs = 66;
        goto f198;
    tr287:
        cs = 66;
        goto f216;
    tr308:
        cs = 66;
        goto f235;
    tr326:
        cs = 66;
        goto f253;
    tr344:
        cs = 66;
        goto f271;
    tr363:
        cs = 66;
        goto f288;
    tr382:
        cs = 66;
        goto f307;
    tr401:
        cs = 66;
        goto f325;
    tr416:
        cs = 66;
        goto f341;
    tr431:
        cs = 66;
        goto f356;
    tr451:
        cs = 66;
        goto f376;
    tr466:
        cs = 66;
        goto f391;
    tr485:
        cs = 66;
        goto f410;
    tr504:
        cs = 66;
        goto f429;
    tr523:
        cs = 66;
        goto f448;
    tr541:
        cs = 66;
        goto f466;
    tr347:
        cs = 67;
        goto f218;
    tr52:
        cs = 68;
        goto _again;
    tr53:
        cs = 69;
        goto _again;
    tr543:
        cs = 70;
        goto _again;
    tr14:
        cs = 71;
        goto _again;

    f43:
        _acts = _JDR_actions + 1;
        goto execFuncs;
    f60:
        _acts = _JDR_actions + 3;
        goto execFuncs;
    f0:
        _acts = _JDR_actions + 5;
        goto execFuncs;
    f5:
        _acts = _JDR_actions + 7;
        goto execFuncs;
    f1:
        _acts = _JDR_actions + 9;
        goto execFuncs;
    f10:
        _acts = _JDR_actions + 11;
        goto execFuncs;
    f2:
        _acts = _JDR_actions + 13;
        goto execFuncs;
    f15:
        _acts = _JDR_actions + 15;
        goto execFuncs;
    f3:
        _acts = _JDR_actions + 17;
        goto execFuncs;
    f20:
        _acts = _JDR_actions + 19;
        goto execFuncs;
    f218:
        _acts = _JDR_actions + 21;
        goto execFuncs;
    f237:
        _acts = _JDR_actions + 23;
        goto execFuncs;
    f327:
        _acts = _JDR_actions + 25;
        goto execFuncs;
    f273:
        _acts = _JDR_actions + 27;
        goto execFuncs;
    f78:
        _acts = _JDR_actions + 29;
        goto execFuncs;
    f412:
        _acts = _JDR_actions + 31;
        goto execFuncs;
    f106:
        _acts = _JDR_actions + 33;
        goto execFuncs;
    f131:
        _acts = _JDR_actions + 35;
        goto execFuncs;
    f359:
        _acts = _JDR_actions + 37;
        goto execFuncs;
    f431:
        _acts = _JDR_actions + 39;
        goto execFuncs;
    f290:
        _acts = _JDR_actions + 41;
        goto execFuncs;
    f25:
        _acts = _JDR_actions + 43;
        goto execFuncs;
    f61:
        _acts = _JDR_actions + 47;
        goto execFuncs;
    f123:
        _acts = _JDR_actions + 50;
        goto execFuncs;
    f4:
        _acts = _JDR_actions + 56;
        goto execFuncs;
    f6:
        _acts = _JDR_actions + 59;
        goto execFuncs;
    f7:
        _acts = _JDR_actions + 62;
        goto execFuncs;
    f8:
        _acts = _JDR_actions + 65;
        goto execFuncs;
    f9:
        _acts = _JDR_actions + 68;
        goto execFuncs;
    f11:
        _acts = _JDR_actions + 71;
        goto execFuncs;
    f12:
        _acts = _JDR_actions + 74;
        goto execFuncs;
    f13:
        _acts = _JDR_actions + 77;
        goto execFuncs;
    f14:
        _acts = _JDR_actions + 80;
        goto execFuncs;
    f16:
        _acts = _JDR_actions + 83;
        goto execFuncs;
    f17:
        _acts = _JDR_actions + 86;
        goto execFuncs;
    f18:
        _acts = _JDR_actions + 89;
        goto execFuncs;
    f19:
        _acts = _JDR_actions + 92;
        goto execFuncs;
    f21:
        _acts = _JDR_actions + 95;
        goto execFuncs;
    f22:
        _acts = _JDR_actions + 98;
        goto execFuncs;
    f23:
        _acts = _JDR_actions + 101;
        goto execFuncs;
    f219:
        _acts = _JDR_actions + 104;
        goto execFuncs;
    f229:
        _acts = _JDR_actions + 107;
        goto execFuncs;
    f238:
        _acts = _JDR_actions + 110;
        goto execFuncs;
    f247:
        _acts = _JDR_actions + 113;
        goto execFuncs;
    f328:
        _acts = _JDR_actions + 116;
        goto execFuncs;
    f336:
        _acts = _JDR_actions + 119;
        goto execFuncs;
    f274:
        _acts = _JDR_actions + 122;
        goto execFuncs;
    f283:
        _acts = _JDR_actions + 125;
        goto execFuncs;
    f79:
        _acts = _JDR_actions + 128;
        goto execFuncs;
    f89:
        _acts = _JDR_actions + 131;
        goto execFuncs;
    f413:
        _acts = _JDR_actions + 134;
        goto execFuncs;
    f423:
        _acts = _JDR_actions + 137;
        goto execFuncs;
    f132:
        _acts = _JDR_actions + 140;
        goto execFuncs;
    f142:
        _acts = _JDR_actions + 143;
        goto execFuncs;
    f360:
        _acts = _JDR_actions + 146;
        goto execFuncs;
    f370:
        _acts = _JDR_actions + 149;
        goto execFuncs;
    f432:
        _acts = _JDR_actions + 152;
        goto execFuncs;
    f442:
        _acts = _JDR_actions + 155;
        goto execFuncs;
    f291:
        _acts = _JDR_actions + 158;
        goto execFuncs;
    f301:
        _acts = _JDR_actions + 161;
        goto execFuncs;
    f26:
        _acts = _JDR_actions + 164;
        goto execFuncs;
    f378:
        _acts = _JDR_actions + 170;
        goto execFuncs;
    f343:
        _acts = _JDR_actions + 174;
        goto execFuncs;
    f182:
        _acts = _JDR_actions + 178;
        goto execFuncs;
    f394:
        _acts = _JDR_actions + 182;
        goto execFuncs;
    f450:
        _acts = _JDR_actions + 186;
        goto execFuncs;
    f309:
        _acts = _JDR_actions + 190;
        goto execFuncs;
    f200:
        _acts = _JDR_actions + 194;
        goto execFuncs;
    f255:
        _acts = _JDR_actions + 198;
        goto execFuncs;
    f45:
        _acts = _JDR_actions + 202;
        goto execFuncs;
    f54:
        _acts = _JDR_actions + 206;
        goto execFuncs;
    f57:
        _acts = _JDR_actions + 210;
        goto execFuncs;
    f51:
        _acts = _JDR_actions + 214;
        goto execFuncs;
    f46:
        _acts = _JDR_actions + 218;
        goto execFuncs;
    f55:
        _acts = _JDR_actions + 222;
        goto execFuncs;
    f58:
        _acts = _JDR_actions + 226;
        goto execFuncs;
    f52:
        _acts = _JDR_actions + 230;
        goto execFuncs;
    f47:
        _acts = _JDR_actions + 234;
        goto execFuncs;
    f50:
        _acts = _JDR_actions + 238;
        goto execFuncs;
    f44:
        _acts = _JDR_actions + 242;
        goto execFuncs;
    f56:
        _acts = _JDR_actions + 246;
        goto execFuncs;
    f63:
        _acts = _JDR_actions + 250;
        goto execFuncs;
    f72:
        _acts = _JDR_actions + 255;
        goto execFuncs;
    f75:
        _acts = _JDR_actions + 260;
        goto execFuncs;
    f69:
        _acts = _JDR_actions + 265;
        goto execFuncs;
    f64:
        _acts = _JDR_actions + 270;
        goto execFuncs;
    f73:
        _acts = _JDR_actions + 275;
        goto execFuncs;
    f76:
        _acts = _JDR_actions + 280;
        goto execFuncs;
    f70:
        _acts = _JDR_actions + 285;
        goto execFuncs;
    f65:
        _acts = _JDR_actions + 290;
        goto execFuncs;
    f68:
        _acts = _JDR_actions + 295;
        goto execFuncs;
    f62:
        _acts = _JDR_actions + 300;
        goto execFuncs;
    f74:
        _acts = _JDR_actions + 305;
        goto execFuncs;
    f379:
        _acts = _JDR_actions + 310;
        goto execFuncs;
    f344:
        _acts = _JDR_actions + 320;
        goto execFuncs;
    f183:
        _acts = _JDR_actions + 330;
        goto execFuncs;
    f395:
        _acts = _JDR_actions + 340;
        goto execFuncs;
    f451:
        _acts = _JDR_actions + 350;
        goto execFuncs;
    f310:
        _acts = _JDR_actions + 360;
        goto execFuncs;
    f201:
        _acts = _JDR_actions + 370;
        goto execFuncs;
    f256:
        _acts = _JDR_actions + 380;
        goto execFuncs;
    f48:
        _acts = _JDR_actions + 390;
        goto execFuncs;
    f53:
        _acts = _JDR_actions + 395;
        goto execFuncs;
    f28:
        _acts = _JDR_actions + 400;
        goto execFuncs;
    f37:
        _acts = _JDR_actions + 405;
        goto execFuncs;
    f40:
        _acts = _JDR_actions + 410;
        goto execFuncs;
    f34:
        _acts = _JDR_actions + 415;
        goto execFuncs;
    f29:
        _acts = _JDR_actions + 420;
        goto execFuncs;
    f38:
        _acts = _JDR_actions + 425;
        goto execFuncs;
    f41:
        _acts = _JDR_actions + 430;
        goto execFuncs;
    f35:
        _acts = _JDR_actions + 435;
        goto execFuncs;
    f30:
        _acts = _JDR_actions + 440;
        goto execFuncs;
    f33:
        _acts = _JDR_actions + 445;
        goto execFuncs;
    f27:
        _acts = _JDR_actions + 450;
        goto execFuncs;
    f39:
        _acts = _JDR_actions + 455;
        goto execFuncs;
    f66:
        _acts = _JDR_actions + 460;
        goto execFuncs;
    f71:
        _acts = _JDR_actions + 466;
        goto execFuncs;
    f31:
        _acts = _JDR_actions + 484;
        goto execFuncs;
    f36:
        _acts = _JDR_actions + 490;
        goto execFuncs;
    f381:
        _acts = _JDR_actions + 552;
        goto execFuncs;
    f387:
        _acts = _JDR_actions + 559;
        goto execFuncs;
    f390:
        _acts = _JDR_actions + 566;
        goto execFuncs;
    f385:
        _acts = _JDR_actions + 573;
        goto execFuncs;
    f382:
        _acts = _JDR_actions + 580;
        goto execFuncs;
    f388:
        _acts = _JDR_actions + 587;
        goto execFuncs;
    f391:
        _acts = _JDR_actions + 594;
        goto execFuncs;
    f386:
        _acts = _JDR_actions + 601;
        goto execFuncs;
    f383:
        _acts = _JDR_actions + 608;
        goto execFuncs;
    f384:
        _acts = _JDR_actions + 615;
        goto execFuncs;
    f380:
        _acts = _JDR_actions + 622;
        goto execFuncs;
    f389:
        _acts = _JDR_actions + 629;
        goto execFuncs;
    f346:
        _acts = _JDR_actions + 636;
        goto execFuncs;
    f352:
        _acts = _JDR_actions + 643;
        goto execFuncs;
    f355:
        _acts = _JDR_actions + 650;
        goto execFuncs;
    f350:
        _acts = _JDR_actions + 657;
        goto execFuncs;
    f347:
        _acts = _JDR_actions + 664;
        goto execFuncs;
    f353:
        _acts = _JDR_actions + 671;
        goto execFuncs;
    f356:
        _acts = _JDR_actions + 678;
        goto execFuncs;
    f351:
        _acts = _JDR_actions + 685;
        goto execFuncs;
    f348:
        _acts = _JDR_actions + 692;
        goto execFuncs;
    f349:
        _acts = _JDR_actions + 699;
        goto execFuncs;
    f345:
        _acts = _JDR_actions + 706;
        goto execFuncs;
    f354:
        _acts = _JDR_actions + 713;
        goto execFuncs;
    f185:
        _acts = _JDR_actions + 727;
        goto execFuncs;
    f194:
        _acts = _JDR_actions + 734;
        goto execFuncs;
    f197:
        _acts = _JDR_actions + 741;
        goto execFuncs;
    f191:
        _acts = _JDR_actions + 748;
        goto execFuncs;
    f186:
        _acts = _JDR_actions + 755;
        goto execFuncs;
    f195:
        _acts = _JDR_actions + 762;
        goto execFuncs;
    f198:
        _acts = _JDR_actions + 769;
        goto execFuncs;
    f192:
        _acts = _JDR_actions + 776;
        goto execFuncs;
    f187:
        _acts = _JDR_actions + 783;
        goto execFuncs;
    f190:
        _acts = _JDR_actions + 790;
        goto execFuncs;
    f184:
        _acts = _JDR_actions + 797;
        goto execFuncs;
    f196:
        _acts = _JDR_actions + 804;
        goto execFuncs;
    f397:
        _acts = _JDR_actions + 818;
        goto execFuncs;
    f406:
        _acts = _JDR_actions + 825;
        goto execFuncs;
    f409:
        _acts = _JDR_actions + 832;
        goto execFuncs;
    f403:
        _acts = _JDR_actions + 839;
        goto execFuncs;
    f398:
        _acts = _JDR_actions + 846;
        goto execFuncs;
    f407:
        _acts = _JDR_actions + 853;
        goto execFuncs;
    f410:
        _acts = _JDR_actions + 860;
        goto execFuncs;
    f404:
        _acts = _JDR_actions + 867;
        goto execFuncs;
    f399:
        _acts = _JDR_actions + 874;
        goto execFuncs;
    f402:
        _acts = _JDR_actions + 881;
        goto execFuncs;
    f396:
        _acts = _JDR_actions + 888;
        goto execFuncs;
    f408:
        _acts = _JDR_actions + 895;
        goto execFuncs;
    f453:
        _acts = _JDR_actions + 909;
        goto execFuncs;
    f462:
        _acts = _JDR_actions + 916;
        goto execFuncs;
    f465:
        _acts = _JDR_actions + 923;
        goto execFuncs;
    f459:
        _acts = _JDR_actions + 930;
        goto execFuncs;
    f454:
        _acts = _JDR_actions + 937;
        goto execFuncs;
    f463:
        _acts = _JDR_actions + 944;
        goto execFuncs;
    f466:
        _acts = _JDR_actions + 951;
        goto execFuncs;
    f460:
        _acts = _JDR_actions + 958;
        goto execFuncs;
    f455:
        _acts = _JDR_actions + 965;
        goto execFuncs;
    f458:
        _acts = _JDR_actions + 972;
        goto execFuncs;
    f452:
        _acts = _JDR_actions + 979;
        goto execFuncs;
    f464:
        _acts = _JDR_actions + 986;
        goto execFuncs;
    f312:
        _acts = _JDR_actions + 1000;
        goto execFuncs;
    f321:
        _acts = _JDR_actions + 1007;
        goto execFuncs;
    f324:
        _acts = _JDR_actions + 1014;
        goto execFuncs;
    f318:
        _acts = _JDR_actions + 1021;
        goto execFuncs;
    f313:
        _acts = _JDR_actions + 1028;
        goto execFuncs;
    f322:
        _acts = _JDR_actions + 1035;
        goto execFuncs;
    f325:
        _acts = _JDR_actions + 1042;
        goto execFuncs;
    f319:
        _acts = _JDR_actions + 1049;
        goto execFuncs;
    f314:
        _acts = _JDR_actions + 1056;
        goto execFuncs;
    f317:
        _acts = _JDR_actions + 1063;
        goto execFuncs;
    f311:
        _acts = _JDR_actions + 1070;
        goto execFuncs;
    f323:
        _acts = _JDR_actions + 1077;
        goto execFuncs;
    f203:
        _acts = _JDR_actions + 1084;
        goto execFuncs;
    f212:
        _acts = _JDR_actions + 1091;
        goto execFuncs;
    f215:
        _acts = _JDR_actions + 1098;
        goto execFuncs;
    f209:
        _acts = _JDR_actions + 1105;
        goto execFuncs;
    f204:
        _acts = _JDR_actions + 1112;
        goto execFuncs;
    f213:
        _acts = _JDR_actions + 1119;
        goto execFuncs;
    f216:
        _acts = _JDR_actions + 1126;
        goto execFuncs;
    f210:
        _acts = _JDR_actions + 1133;
        goto execFuncs;
    f205:
        _acts = _JDR_actions + 1140;
        goto execFuncs;
    f208:
        _acts = _JDR_actions + 1147;
        goto execFuncs;
    f202:
        _acts = _JDR_actions + 1154;
        goto execFuncs;
    f214:
        _acts = _JDR_actions + 1161;
        goto execFuncs;
    f258:
        _acts = _JDR_actions + 1168;
        goto execFuncs;
    f267:
        _acts = _JDR_actions + 1175;
        goto execFuncs;
    f270:
        _acts = _JDR_actions + 1182;
        goto execFuncs;
    f264:
        _acts = _JDR_actions + 1189;
        goto execFuncs;
    f259:
        _acts = _JDR_actions + 1196;
        goto execFuncs;
    f268:
        _acts = _JDR_actions + 1203;
        goto execFuncs;
    f271:
        _acts = _JDR_actions + 1210;
        goto execFuncs;
    f265:
        _acts = _JDR_actions + 1217;
        goto execFuncs;
    f260:
        _acts = _JDR_actions + 1224;
        goto execFuncs;
    f263:
        _acts = _JDR_actions + 1231;
        goto execFuncs;
    f257:
        _acts = _JDR_actions + 1238;
        goto execFuncs;
    f269:
        _acts = _JDR_actions + 1245;
        goto execFuncs;
    f49:
        _acts = _JDR_actions + 1252;
        goto execFuncs;
    f67:
        _acts = _JDR_actions + 1259;
        goto execFuncs;
    f392:
        _acts = _JDR_actions + 1267;
        goto execFuncs;
    f357:
        _acts = _JDR_actions + 1275;
        goto execFuncs;
    f151:
        _acts = _JDR_actions + 1283;
        goto execFuncs;
    f160:
        _acts = _JDR_actions + 1291;
        goto execFuncs;
    f163:
        _acts = _JDR_actions + 1299;
        goto execFuncs;
    f157:
        _acts = _JDR_actions + 1307;
        goto execFuncs;
    f152:
        _acts = _JDR_actions + 1315;
        goto execFuncs;
    f161:
        _acts = _JDR_actions + 1323;
        goto execFuncs;
    f164:
        _acts = _JDR_actions + 1331;
        goto execFuncs;
    f158:
        _acts = _JDR_actions + 1339;
        goto execFuncs;
    f153:
        _acts = _JDR_actions + 1347;
        goto execFuncs;
    f156:
        _acts = _JDR_actions + 1355;
        goto execFuncs;
    f150:
        _acts = _JDR_actions + 1363;
        goto execFuncs;
    f162:
        _acts = _JDR_actions + 1371;
        goto execFuncs;
    f98:
        _acts = _JDR_actions + 1379;
        goto execFuncs;
    f108:
        _acts = _JDR_actions + 1387;
        goto execFuncs;
    f111:
        _acts = _JDR_actions + 1395;
        goto execFuncs;
    f104:
        _acts = _JDR_actions + 1403;
        goto execFuncs;
    f99:
        _acts = _JDR_actions + 1411;
        goto execFuncs;
    f109:
        _acts = _JDR_actions + 1419;
        goto execFuncs;
    f112:
        _acts = _JDR_actions + 1427;
        goto execFuncs;
    f105:
        _acts = _JDR_actions + 1435;
        goto execFuncs;
    f100:
        _acts = _JDR_actions + 1443;
        goto execFuncs;
    f103:
        _acts = _JDR_actions + 1451;
        goto execFuncs;
    f97:
        _acts = _JDR_actions + 1459;
        goto execFuncs;
    f110:
        _acts = _JDR_actions + 1467;
        goto execFuncs;
    f188:
        _acts = _JDR_actions + 1475;
        goto execFuncs;
    f193:
        _acts = _JDR_actions + 1483;
        goto execFuncs;
    f400:
        _acts = _JDR_actions + 1491;
        goto execFuncs;
    f405:
        _acts = _JDR_actions + 1499;
        goto execFuncs;
    f456:
        _acts = _JDR_actions + 1507;
        goto execFuncs;
    f461:
        _acts = _JDR_actions + 1515;
        goto execFuncs;
    f315:
        _acts = _JDR_actions + 1523;
        goto execFuncs;
    f320:
        _acts = _JDR_actions + 1531;
        goto execFuncs;
    f206:
        _acts = _JDR_actions + 1539;
        goto execFuncs;
    f211:
        _acts = _JDR_actions + 1547;
        goto execFuncs;
    f261:
        _acts = _JDR_actions + 1555;
        goto execFuncs;
    f266:
        _acts = _JDR_actions + 1563;
        goto execFuncs;
    f32:
        _acts = _JDR_actions + 1571;
        goto execFuncs;
    f167:
        _acts = _JDR_actions + 1579;
        goto execFuncs;
    f176:
        _acts = _JDR_actions + 1588;
        goto execFuncs;
    f179:
        _acts = _JDR_actions + 1597;
        goto execFuncs;
    f173:
        _acts = _JDR_actions + 1606;
        goto execFuncs;
    f168:
        _acts = _JDR_actions + 1615;
        goto execFuncs;
    f177:
        _acts = _JDR_actions + 1624;
        goto execFuncs;
    f180:
        _acts = _JDR_actions + 1633;
        goto execFuncs;
    f174:
        _acts = _JDR_actions + 1642;
        goto execFuncs;
    f169:
        _acts = _JDR_actions + 1651;
        goto execFuncs;
    f172:
        _acts = _JDR_actions + 1660;
        goto execFuncs;
    f166:
        _acts = _JDR_actions + 1669;
        goto execFuncs;
    f178:
        _acts = _JDR_actions + 1678;
        goto execFuncs;
    f115:
        _acts = _JDR_actions + 1687;
        goto execFuncs;
    f125:
        _acts = _JDR_actions + 1696;
        goto execFuncs;
    f128:
        _acts = _JDR_actions + 1705;
        goto execFuncs;
    f121:
        _acts = _JDR_actions + 1714;
        goto execFuncs;
    f116:
        _acts = _JDR_actions + 1723;
        goto execFuncs;
    f126:
        _acts = _JDR_actions + 1732;
        goto execFuncs;
    f129:
        _acts = _JDR_actions + 1741;
        goto execFuncs;
    f122:
        _acts = _JDR_actions + 1750;
        goto execFuncs;
    f117:
        _acts = _JDR_actions + 1759;
        goto execFuncs;
    f120:
        _acts = _JDR_actions + 1768;
        goto execFuncs;
    f114:
        _acts = _JDR_actions + 1777;
        goto execFuncs;
    f127:
        _acts = _JDR_actions + 1786;
        goto execFuncs;
    f221:
        _acts = _JDR_actions + 1795;
        goto execFuncs;
    f231:
        _acts = _JDR_actions + 1804;
        goto execFuncs;
    f234:
        _acts = _JDR_actions + 1813;
        goto execFuncs;
    f227:
        _acts = _JDR_actions + 1822;
        goto execFuncs;
    f222:
        _acts = _JDR_actions + 1831;
        goto execFuncs;
    f232:
        _acts = _JDR_actions + 1840;
        goto execFuncs;
    f235:
        _acts = _JDR_actions + 1849;
        goto execFuncs;
    f228:
        _acts = _JDR_actions + 1858;
        goto execFuncs;
    f223:
        _acts = _JDR_actions + 1867;
        goto execFuncs;
    f226:
        _acts = _JDR_actions + 1876;
        goto execFuncs;
    f220:
        _acts = _JDR_actions + 1885;
        goto execFuncs;
    f233:
        _acts = _JDR_actions + 1894;
        goto execFuncs;
    f240:
        _acts = _JDR_actions + 1903;
        goto execFuncs;
    f249:
        _acts = _JDR_actions + 1912;
        goto execFuncs;
    f252:
        _acts = _JDR_actions + 1921;
        goto execFuncs;
    f245:
        _acts = _JDR_actions + 1930;
        goto execFuncs;
    f241:
        _acts = _JDR_actions + 1939;
        goto execFuncs;
    f250:
        _acts = _JDR_actions + 1948;
        goto execFuncs;
    f253:
        _acts = _JDR_actions + 1957;
        goto execFuncs;
    f246:
        _acts = _JDR_actions + 1966;
        goto execFuncs;
    f242:
        _acts = _JDR_actions + 1975;
        goto execFuncs;
    f244:
        _acts = _JDR_actions + 1984;
        goto execFuncs;
    f239:
        _acts = _JDR_actions + 1993;
        goto execFuncs;
    f251:
        _acts = _JDR_actions + 2002;
        goto execFuncs;
    f330:
        _acts = _JDR_actions + 2011;
        goto execFuncs;
    f337:
        _acts = _JDR_actions + 2020;
        goto execFuncs;
    f340:
        _acts = _JDR_actions + 2029;
        goto execFuncs;
    f334:
        _acts = _JDR_actions + 2038;
        goto execFuncs;
    f331:
        _acts = _JDR_actions + 2047;
        goto execFuncs;
    f338:
        _acts = _JDR_actions + 2056;
        goto execFuncs;
    f341:
        _acts = _JDR_actions + 2065;
        goto execFuncs;
    f335:
        _acts = _JDR_actions + 2074;
        goto execFuncs;
    f332:
        _acts = _JDR_actions + 2083;
        goto execFuncs;
    f333:
        _acts = _JDR_actions + 2092;
        goto execFuncs;
    f329:
        _acts = _JDR_actions + 2101;
        goto execFuncs;
    f339:
        _acts = _JDR_actions + 2110;
        goto execFuncs;
    f276:
        _acts = _JDR_actions + 2119;
        goto execFuncs;
    f284:
        _acts = _JDR_actions + 2128;
        goto execFuncs;
    f287:
        _acts = _JDR_actions + 2137;
        goto execFuncs;
    f281:
        _acts = _JDR_actions + 2146;
        goto execFuncs;
    f277:
        _acts = _JDR_actions + 2155;
        goto execFuncs;
    f285:
        _acts = _JDR_actions + 2164;
        goto execFuncs;
    f288:
        _acts = _JDR_actions + 2173;
        goto execFuncs;
    f282:
        _acts = _JDR_actions + 2182;
        goto execFuncs;
    f278:
        _acts = _JDR_actions + 2191;
        goto execFuncs;
    f280:
        _acts = _JDR_actions + 2200;
        goto execFuncs;
    f275:
        _acts = _JDR_actions + 2209;
        goto execFuncs;
    f286:
        _acts = _JDR_actions + 2218;
        goto execFuncs;
    f81:
        _acts = _JDR_actions + 2227;
        goto execFuncs;
    f91:
        _acts = _JDR_actions + 2236;
        goto execFuncs;
    f94:
        _acts = _JDR_actions + 2245;
        goto execFuncs;
    f87:
        _acts = _JDR_actions + 2254;
        goto execFuncs;
    f82:
        _acts = _JDR_actions + 2263;
        goto execFuncs;
    f92:
        _acts = _JDR_actions + 2272;
        goto execFuncs;
    f95:
        _acts = _JDR_actions + 2281;
        goto execFuncs;
    f88:
        _acts = _JDR_actions + 2290;
        goto execFuncs;
    f83:
        _acts = _JDR_actions + 2299;
        goto execFuncs;
    f86:
        _acts = _JDR_actions + 2308;
        goto execFuncs;
    f80:
        _acts = _JDR_actions + 2317;
        goto execFuncs;
    f93:
        _acts = _JDR_actions + 2326;
        goto execFuncs;
    f415:
        _acts = _JDR_actions + 2335;
        goto execFuncs;
    f425:
        _acts = _JDR_actions + 2344;
        goto execFuncs;
    f428:
        _acts = _JDR_actions + 2353;
        goto execFuncs;
    f421:
        _acts = _JDR_actions + 2362;
        goto execFuncs;
    f416:
        _acts = _JDR_actions + 2371;
        goto execFuncs;
    f426:
        _acts = _JDR_actions + 2380;
        goto execFuncs;
    f429:
        _acts = _JDR_actions + 2389;
        goto execFuncs;
    f422:
        _acts = _JDR_actions + 2398;
        goto execFuncs;
    f417:
        _acts = _JDR_actions + 2407;
        goto execFuncs;
    f420:
        _acts = _JDR_actions + 2416;
        goto execFuncs;
    f414:
        _acts = _JDR_actions + 2425;
        goto execFuncs;
    f427:
        _acts = _JDR_actions + 2434;
        goto execFuncs;
    f154:
        _acts = _JDR_actions + 2443;
        goto execFuncs;
    f159:
        _acts = _JDR_actions + 2452;
        goto execFuncs;
    f101:
        _acts = _JDR_actions + 2461;
        goto execFuncs;
    f107:
        _acts = _JDR_actions + 2470;
        goto execFuncs;
    f134:
        _acts = _JDR_actions + 2479;
        goto execFuncs;
    f144:
        _acts = _JDR_actions + 2488;
        goto execFuncs;
    f147:
        _acts = _JDR_actions + 2497;
        goto execFuncs;
    f140:
        _acts = _JDR_actions + 2506;
        goto execFuncs;
    f135:
        _acts = _JDR_actions + 2515;
        goto execFuncs;
    f145:
        _acts = _JDR_actions + 2524;
        goto execFuncs;
    f148:
        _acts = _JDR_actions + 2533;
        goto execFuncs;
    f141:
        _acts = _JDR_actions + 2542;
        goto execFuncs;
    f136:
        _acts = _JDR_actions + 2551;
        goto execFuncs;
    f139:
        _acts = _JDR_actions + 2560;
        goto execFuncs;
    f133:
        _acts = _JDR_actions + 2569;
        goto execFuncs;
    f146:
        _acts = _JDR_actions + 2578;
        goto execFuncs;
    f362:
        _acts = _JDR_actions + 2587;
        goto execFuncs;
    f372:
        _acts = _JDR_actions + 2596;
        goto execFuncs;
    f375:
        _acts = _JDR_actions + 2605;
        goto execFuncs;
    f368:
        _acts = _JDR_actions + 2614;
        goto execFuncs;
    f363:
        _acts = _JDR_actions + 2623;
        goto execFuncs;
    f373:
        _acts = _JDR_actions + 2632;
        goto execFuncs;
    f376:
        _acts = _JDR_actions + 2641;
        goto execFuncs;
    f369:
        _acts = _JDR_actions + 2650;
        goto execFuncs;
    f364:
        _acts = _JDR_actions + 2659;
        goto execFuncs;
    f367:
        _acts = _JDR_actions + 2668;
        goto execFuncs;
    f361:
        _acts = _JDR_actions + 2677;
        goto execFuncs;
    f374:
        _acts = _JDR_actions + 2686;
        goto execFuncs;
    f434:
        _acts = _JDR_actions + 2695;
        goto execFuncs;
    f444:
        _acts = _JDR_actions + 2704;
        goto execFuncs;
    f447:
        _acts = _JDR_actions + 2713;
        goto execFuncs;
    f440:
        _acts = _JDR_actions + 2722;
        goto execFuncs;
    f435:
        _acts = _JDR_actions + 2731;
        goto execFuncs;
    f445:
        _acts = _JDR_actions + 2740;
        goto execFuncs;
    f448:
        _acts = _JDR_actions + 2749;
        goto execFuncs;
    f441:
        _acts = _JDR_actions + 2758;
        goto execFuncs;
    f436:
        _acts = _JDR_actions + 2767;
        goto execFuncs;
    f439:
        _acts = _JDR_actions + 2776;
        goto execFuncs;
    f433:
        _acts = _JDR_actions + 2785;
        goto execFuncs;
    f446:
        _acts = _JDR_actions + 2794;
        goto execFuncs;
    f293:
        _acts = _JDR_actions + 2803;
        goto execFuncs;
    f303:
        _acts = _JDR_actions + 2812;
        goto execFuncs;
    f306:
        _acts = _JDR_actions + 2821;
        goto execFuncs;
    f299:
        _acts = _JDR_actions + 2830;
        goto execFuncs;
    f294:
        _acts = _JDR_actions + 2839;
        goto execFuncs;
    f304:
        _acts = _JDR_actions + 2848;
        goto execFuncs;
    f307:
        _acts = _JDR_actions + 2857;
        goto execFuncs;
    f300:
        _acts = _JDR_actions + 2866;
        goto execFuncs;
    f295:
        _acts = _JDR_actions + 2875;
        goto execFuncs;
    f298:
        _acts = _JDR_actions + 2884;
        goto execFuncs;
    f292:
        _acts = _JDR_actions + 2893;
        goto execFuncs;
    f305:
        _acts = _JDR_actions + 2902;
        goto execFuncs;
    f170:
        _acts = _JDR_actions + 2911;
        goto execFuncs;
    f175:
        _acts = _JDR_actions + 2921;
        goto execFuncs;
    f118:
        _acts = _JDR_actions + 2931;
        goto execFuncs;
    f124:
        _acts = _JDR_actions + 2941;
        goto execFuncs;
    f224:
        _acts = _JDR_actions + 2951;
        goto execFuncs;
    f230:
        _acts = _JDR_actions + 2961;
        goto execFuncs;
    f243:
        _acts = _JDR_actions + 2971;
        goto execFuncs;
    f248:
        _acts = _JDR_actions + 2981;
        goto execFuncs;
    f279:
        _acts = _JDR_actions + 2991;
        goto execFuncs;
    f84:
        _acts = _JDR_actions + 3001;
        goto execFuncs;
    f90:
        _acts = _JDR_actions + 3011;
        goto execFuncs;
    f418:
        _acts = _JDR_actions + 3021;
        goto execFuncs;
    f424:
        _acts = _JDR_actions + 3031;
        goto execFuncs;
    f137:
        _acts = _JDR_actions + 3041;
        goto execFuncs;
    f143:
        _acts = _JDR_actions + 3051;
        goto execFuncs;
    f189:
        _acts = _JDR_actions + 3061;
        goto execFuncs;
    f365:
        _acts = _JDR_actions + 3071;
        goto execFuncs;
    f371:
        _acts = _JDR_actions + 3081;
        goto execFuncs;
    f401:
        _acts = _JDR_actions + 3091;
        goto execFuncs;
    f437:
        _acts = _JDR_actions + 3101;
        goto execFuncs;
    f443:
        _acts = _JDR_actions + 3111;
        goto execFuncs;
    f457:
        _acts = _JDR_actions + 3121;
        goto execFuncs;
    f296:
        _acts = _JDR_actions + 3131;
        goto execFuncs;
    f302:
        _acts = _JDR_actions + 3141;
        goto execFuncs;
    f316:
        _acts = _JDR_actions + 3151;
        goto execFuncs;
    f207:
        _acts = _JDR_actions + 3161;
        goto execFuncs;
    f262:
        _acts = _JDR_actions + 3171;
        goto execFuncs;
    f155:
        _acts = _JDR_actions + 3181;
        goto execFuncs;
    f102:
        _acts = _JDR_actions + 3192;
        goto execFuncs;
    f171:
        _acts = _JDR_actions + 3203;
        goto execFuncs;
    f119:
        _acts = _JDR_actions + 3215;
        goto execFuncs;
    f225:
        _acts = _JDR_actions + 3227;
        goto execFuncs;
    f85:
        _acts = _JDR_actions + 3239;
        goto execFuncs;
    f419:
        _acts = _JDR_actions + 3251;
        goto execFuncs;
    f138:
        _acts = _JDR_actions + 3263;
        goto execFuncs;
    f366:
        _acts = _JDR_actions + 3275;
        goto execFuncs;
    f438:
        _acts = _JDR_actions + 3287;
        goto execFuncs;
    f297:
        _acts = _JDR_actions + 3299;
        goto execFuncs;

    execFuncs:
        _nacts = *_acts++;
        while (_nacts-- > 0) {
            switch (*_acts++) {
                case 0:
#line 79 "JDR.c.rl"
                {
                    mark0[JDRNL] = p - data[0];
                } break;
                case 1:
#line 80 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRNL];
                    tok[1] = p;
                    o = JDRonNL(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 2:
#line 88 "JDR.c.rl"
                {
                    mark0[JDRUtf8cp1] = p - data[0];
                } break;
                case 3:
#line 89 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRUtf8cp1];
                    tok[1] = p;
                    o = JDRonUtf8cp1(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 4:
#line 97 "JDR.c.rl"
                {
                    mark0[JDRUtf8cp2] = p - data[0];
                } break;
                case 5:
#line 98 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRUtf8cp2];
                    tok[1] = p;
                    o = JDRonUtf8cp2(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 6:
#line 106 "JDR.c.rl"
                {
                    mark0[JDRUtf8cp3] = p - data[0];
                } break;
                case 7:
#line 107 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRUtf8cp3];
                    tok[1] = p;
                    o = JDRonUtf8cp3(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 8:
#line 115 "JDR.c.rl"
                {
                    mark0[JDRUtf8cp4] = p - data[0];
                } break;
                case 9:
#line 116 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRUtf8cp4];
                    tok[1] = p;
                    o = JDRonUtf8cp4(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 10:
#line 124 "JDR.c.rl"
                {
                    mark0[JDRInt] = p - data[0];
                } break;
                case 11:
#line 125 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRInt];
                    tok[1] = p;
                    o = JDRonInt(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 12:
#line 133 "JDR.c.rl"
                {
                    mark0[JDRFloat] = p - data[0];
                } break;
                case 13:
#line 134 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRFloat];
                    tok[1] = p;
                    o = JDRonFloat(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 14:
#line 142 "JDR.c.rl"
                {
                    mark0[JDRTerm] = p - data[0];
                } break;
                case 15:
#line 143 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRTerm];
                    tok[1] = p;
                    o = JDRonTerm(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 16:
#line 151 "JDR.c.rl"
                {
                    mark0[JDRRef] = p - data[0];
                } break;
                case 17:
#line 152 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRRef];
                    tok[1] = p;
                    o = JDRonRef(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 18:
#line 160 "JDR.c.rl"
                {
                    mark0[JDRString] = p - data[0];
                } break;
                case 19:
#line 161 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRString];
                    tok[1] = p;
                    o = JDRonString(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 20:
#line 169 "JDR.c.rl"
                {
                    mark0[JDRMLString] = p - data[0];
                } break;
                case 21:
#line 170 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRMLString];
                    tok[1] = p;
                    o = JDRonMLString(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 22:
#line 178 "JDR.c.rl"
                {
                    mark0[JDRStamp] = p - data[0];
                } break;
                case 23:
#line 179 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRStamp];
                    tok[1] = p;
                    o = JDRonStamp(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 24:
#line 187 "JDR.c.rl"
                {
                    mark0[JDRNoStamp] = p - data[0];
                } break;
                case 25:
#line 188 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRNoStamp];
                    tok[1] = p;
                    o = JDRonNoStamp(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 26:
#line 196 "JDR.c.rl"
                {
                    mark0[JDROpenP] = p - data[0];
                } break;
                case 27:
#line 197 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDROpenP];
                    tok[1] = p;
                    o = JDRonOpenP(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 28:
#line 205 "JDR.c.rl"
                {
                    mark0[JDRCloseP] = p - data[0];
                } break;
                case 29:
#line 206 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRCloseP];
                    tok[1] = p;
                    o = JDRonCloseP(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 30:
#line 214 "JDR.c.rl"
                {
                    mark0[JDROpenL] = p - data[0];
                } break;
                case 31:
#line 215 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDROpenL];
                    tok[1] = p;
                    o = JDRonOpenL(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 32:
#line 223 "JDR.c.rl"
                {
                    mark0[JDRCloseL] = p - data[0];
                } break;
                case 33:
#line 224 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRCloseL];
                    tok[1] = p;
                    o = JDRonCloseL(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 34:
#line 232 "JDR.c.rl"
                {
                    mark0[JDROpenE] = p - data[0];
                } break;
                case 35:
#line 233 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDROpenE];
                    tok[1] = p;
                    o = JDRonOpenE(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 36:
#line 241 "JDR.c.rl"
                {
                    mark0[JDRCloseE] = p - data[0];
                } break;
                case 37:
#line 242 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRCloseE];
                    tok[1] = p;
                    o = JDRonCloseE(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 38:
#line 250 "JDR.c.rl"
                {
                    mark0[JDROpenX] = p - data[0];
                } break;
                case 39:
#line 251 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDROpenX];
                    tok[1] = p;
                    o = JDRonOpenX(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 40:
#line 259 "JDR.c.rl"
                {
                    mark0[JDRCloseX] = p - data[0];
                } break;
                case 41:
#line 260 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRCloseX];
                    tok[1] = p;
                    o = JDRonCloseX(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 42:
#line 268 "JDR.c.rl"
                {
                    mark0[JDRComma] = p - data[0];
                } break;
                case 43:
#line 269 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRComma];
                    tok[1] = p;
                    o = JDRonComma(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 44:
#line 277 "JDR.c.rl"
                {
                    mark0[JDRColon] = p - data[0];
                } break;
                case 45:
#line 278 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRColon];
                    tok[1] = p;
                    o = JDRonColon(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 46:
#line 286 "JDR.c.rl"
                {
                    mark0[JDROpen] = p - data[0];
                } break;
                case 47:
#line 287 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDROpen];
                    tok[1] = p;
                    o = JDRonOpen(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 48:
#line 295 "JDR.c.rl"
                {
                    mark0[JDRClose] = p - data[0];
                } break;
                case 49:
#line 296 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRClose];
                    tok[1] = p;
                    o = JDRonClose(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 50:
#line 304 "JDR.c.rl"
                {
                    mark0[JDRInter] = p - data[0];
                } break;
                case 51:
#line 305 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRInter];
                    tok[1] = p;
                    o = JDRonInter(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 52:
#line 313 "JDR.c.rl"
                {
                    mark0[JDRFIRST] = p - data[0];
                } break;
                case 53:
#line 314 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRFIRST];
                    tok[1] = p;
                    o = JDRonFIRST(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 54:
#line 322 "JDR.c.rl"
                {
                    mark0[JDRToken] = p - data[0];
                } break;
                case 55:
#line 323 "JDR.c.rl"
                {
                    tok[0] = data[0] + mark0[JDRToken];
                    tok[1] = p;
                    o = JDRonToken(tok, state);
                    if (o != OK) {
                        {
                            p++;
                            goto _out;
                        }
                    }
                } break;
                case 56:
#line 331 "JDR.c.rl"
                {
                    mark0[JDRRoot] = p - data[0];
                } break;
#line 3385 "JDR.rl.c"
            }
        }
        goto _again;

    _again:
        if (cs == 0) goto _out;
        if (++p != pe) goto _resume;
    _test_eof: {}
        if (p == eof) {
            const char* __acts = _JDR_actions + _JDR_eof_actions[cs];
            unsigned int __nacts = (unsigned int)*__acts++;
            while (__nacts-- > 0) {
                switch (*__acts++) {
                    case 1:
#line 80 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRNL];
                        tok[1] = p;
                        o = JDRonNL(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 11:
#line 125 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRInt];
                        tok[1] = p;
                        o = JDRonInt(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 13:
#line 134 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRFloat];
                        tok[1] = p;
                        o = JDRonFloat(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 15:
#line 143 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRTerm];
                        tok[1] = p;
                        o = JDRonTerm(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 17:
#line 152 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRRef];
                        tok[1] = p;
                        o = JDRonRef(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 19:
#line 161 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRString];
                        tok[1] = p;
                        o = JDRonString(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 21:
#line 170 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRMLString];
                        tok[1] = p;
                        o = JDRonMLString(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 23:
#line 179 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRStamp];
                        tok[1] = p;
                        o = JDRonStamp(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 24:
#line 187 "JDR.c.rl"
                    {
                        mark0[JDRNoStamp] = p - data[0];
                    } break;
                    case 25:
#line 188 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRNoStamp];
                        tok[1] = p;
                        o = JDRonNoStamp(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 27:
#line 197 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDROpenP];
                        tok[1] = p;
                        o = JDRonOpenP(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 29:
#line 206 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRCloseP];
                        tok[1] = p;
                        o = JDRonCloseP(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 31:
#line 215 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDROpenL];
                        tok[1] = p;
                        o = JDRonOpenL(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 33:
#line 224 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRCloseL];
                        tok[1] = p;
                        o = JDRonCloseL(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 35:
#line 233 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDROpenE];
                        tok[1] = p;
                        o = JDRonOpenE(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 37:
#line 242 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRCloseE];
                        tok[1] = p;
                        o = JDRonCloseE(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 39:
#line 251 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDROpenX];
                        tok[1] = p;
                        o = JDRonOpenX(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 41:
#line 260 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRCloseX];
                        tok[1] = p;
                        o = JDRonCloseX(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 43:
#line 269 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRComma];
                        tok[1] = p;
                        o = JDRonComma(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 45:
#line 278 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRColon];
                        tok[1] = p;
                        o = JDRonColon(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 47:
#line 287 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDROpen];
                        tok[1] = p;
                        o = JDRonOpen(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 49:
#line 296 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRClose];
                        tok[1] = p;
                        o = JDRonClose(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 51:
#line 305 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRInter];
                        tok[1] = p;
                        o = JDRonInter(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 53:
#line 314 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRFIRST];
                        tok[1] = p;
                        o = JDRonFIRST(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 55:
#line 323 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRToken];
                        tok[1] = p;
                        o = JDRonToken(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
                    case 56:
#line 331 "JDR.c.rl"
                    {
                        mark0[JDRRoot] = p - data[0];
                    } break;
                    case 57:
#line 332 "JDR.c.rl"
                    {
                        tok[0] = data[0] + mark0[JDRRoot];
                        tok[1] = p;
                        o = JDRonRoot(tok, state);
                        if (o != OK) {
                            {
                                p++;
                                goto _out;
                            }
                        }
                    } break;
#line 3657 "JDR.rl.c"
                }
            }
        }

    _out: {}
    }

#line 410 "JDR.c.rl"

    state->data[0] = (u8*)p;
    if (p != data[1] || cs < JDR_first_final || o != OK) {
        return JDRbad;
    }
    return o;
}
