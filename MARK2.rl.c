
#line 1 "MARK2.rl"
#include "MARK2.rl.h"



#line 107 "MARK2.rl"



#line 7 "MARK2.rl.c"
static const char _MARK2_actions[] = {
	0, 1, 15, 2, 1, 15, 2, 3, 
	15, 2, 5, 15, 2, 6, 2, 2, 
	7, 15, 2, 9, 15, 2, 11, 15, 
	2, 13, 15, 2, 14, 15, 3, 1, 
	6, 2, 3, 3, 6, 2, 3, 4, 
	10, 0, 3, 5, 6, 2, 3, 5, 
	7, 15, 3, 5, 13, 15, 3, 6, 
	7, 2, 3, 6, 12, 2, 3, 7, 
	6, 2, 3, 9, 6, 2, 3, 11, 
	6, 2, 3, 13, 1, 15, 3, 13, 
	6, 2, 3, 14, 6, 2, 4, 1, 
	4, 10, 0, 4, 1, 6, 12, 2, 
	4, 3, 4, 10, 0, 4, 3, 6, 
	12, 2, 4, 4, 6, 12, 2, 4, 
	4, 7, 10, 0, 4, 5, 4, 10, 
	0, 4, 5, 6, 12, 2, 4, 5, 
	7, 6, 2, 4, 5, 13, 6, 2, 
	4, 6, 7, 12, 2, 4, 6, 8, 
	12, 2, 4, 7, 4, 10, 0, 4, 
	7, 6, 12, 2, 4, 9, 4, 10, 
	0, 4, 9, 6, 12, 2, 4, 11, 
	4, 10, 0, 4, 11, 6, 12, 2, 
	4, 13, 1, 6, 2, 4, 13, 4, 
	10, 0, 4, 13, 6, 12, 2, 4, 
	14, 4, 10, 0, 4, 14, 6, 12, 
	2, 5, 1, 4, 6, 12, 2, 5, 
	1, 6, 8, 12, 2, 5, 3, 4, 
	6, 12, 2, 5, 3, 6, 8, 12, 
	2, 5, 4, 6, 7, 12, 2, 5, 
	5, 4, 6, 12, 2, 5, 5, 6, 
	8, 12, 2, 5, 5, 7, 4, 10, 
	0, 5, 5, 7, 6, 12, 2, 5, 
	5, 13, 4, 10, 0, 5, 5, 13, 
	6, 12, 2, 5, 6, 7, 8, 12, 
	2, 5, 7, 4, 6, 12, 2, 5, 
	9, 4, 6, 12, 2, 5, 9, 6, 
	8, 12, 2, 5, 11, 4, 6, 12, 
	2, 5, 11, 6, 8, 12, 2, 5, 
	13, 1, 4, 10, 0, 5, 13, 1, 
	6, 12, 2, 5, 13, 4, 6, 12, 
	2, 5, 13, 6, 8, 12, 2, 5, 
	14, 4, 6, 12, 2, 5, 14, 6, 
	8, 12, 2, 6, 5, 7, 4, 6, 
	12, 2, 6, 5, 13, 4, 6, 12, 
	2, 6, 13, 1, 4, 6, 12, 2, 
	6, 13, 1, 6, 8, 12, 2
};

static const short _MARK2_key_offsets[] = {
	0, 15, 31, 47, 63, 79, 95, 112, 
	134, 150, 167, 184, 201, 218, 235, 252, 
	269, 286, 303, 321, 344, 361, 379, 394, 
	411, 428, 445, 462, 479, 496, 513, 531, 
	548, 565, 582, 599, 617, 634, 651, 668, 
	686, 702, 718, 734, 750, 767, 784, 801, 
	817, 834, 851, 867, 883, 899, 915, 932, 
	949
};

static const unsigned char _MARK2_trans_keys[] = {
	13u, 32u, 42u, 63u, 95u, 9u, 10u, 33u, 
	34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u, 
	32u, 42u, 63u, 93u, 95u, 9u, 10u, 33u, 
	34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u, 
	32u, 42u, 63u, 91u, 95u, 9u, 10u, 33u, 
	34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u, 
	32u, 42u, 63u, 93u, 95u, 9u, 10u, 33u, 
	34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u, 
	32u, 42u, 63u, 93u, 95u, 9u, 10u, 33u, 
	34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u, 
	32u, 42u, 63u, 93u, 95u, 9u, 10u, 33u, 
	34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u, 
	32u, 42u, 63u, 91u, 93u, 95u, 9u, 10u, 
	33u, 34u, 39u, 41u, 44u, 46u, 58u, 59u, 
	13u, 32u, 42u, 63u, 93u, 95u, 9u, 10u, 
	33u, 34u, 39u, 41u, 44u, 46u, 48u, 57u, 
	58u, 59u, 65u, 90u, 97u, 122u, 13u, 32u, 
	42u, 63u, 93u, 95u, 9u, 10u, 33u, 34u, 
	39u, 41u, 44u, 46u, 58u, 59u, 13u, 32u, 
	42u, 63u, 91u, 93u, 95u, 9u, 10u, 33u, 
	34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u, 
	32u, 42u, 63u, 92u, 93u, 95u, 9u, 10u, 
	33u, 34u, 39u, 41u, 44u, 46u, 58u, 59u, 
	13u, 32u, 42u, 63u, 92u, 93u, 95u, 9u, 
	10u, 33u, 34u, 39u, 41u, 44u, 46u, 58u, 
	59u, 13u, 32u, 42u, 63u, 91u, 92u, 95u, 
	9u, 10u, 33u, 34u, 39u, 41u, 44u, 46u, 
	58u, 59u, 13u, 32u, 42u, 63u, 92u, 93u, 
	95u, 9u, 10u, 33u, 34u, 39u, 41u, 44u, 
	46u, 58u, 59u, 13u, 32u, 42u, 63u, 92u, 
	93u, 95u, 9u, 10u, 33u, 34u, 39u, 41u, 
	44u, 46u, 58u, 59u, 13u, 32u, 42u, 63u, 
	92u, 93u, 95u, 9u, 10u, 33u, 34u, 39u, 
	41u, 44u, 46u, 58u, 59u, 13u, 32u, 42u, 
	63u, 92u, 93u, 95u, 9u, 10u, 33u, 34u, 
	39u, 41u, 44u, 46u, 58u, 59u, 13u, 32u, 
	42u, 63u, 92u, 93u, 95u, 9u, 10u, 33u, 
	34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u, 
	32u, 42u, 63u, 91u, 92u, 93u, 95u, 9u, 
	10u, 33u, 34u, 39u, 41u, 44u, 46u, 58u, 
	59u, 13u, 32u, 42u, 63u, 92u, 93u, 95u, 
	9u, 10u, 33u, 34u, 39u, 41u, 44u, 46u, 
	48u, 57u, 58u, 59u, 65u, 90u, 97u, 122u, 
	13u, 32u, 42u, 63u, 92u, 93u, 95u, 9u, 
	10u, 33u, 34u, 39u, 41u, 44u, 46u, 58u, 
	59u, 13u, 32u, 42u, 63u, 91u, 92u, 93u, 
	95u, 9u, 10u, 33u, 34u, 39u, 41u, 44u, 
	46u, 58u, 59u, 13u, 32u, 42u, 63u, 95u, 
	9u, 10u, 33u, 34u, 39u, 41u, 44u, 46u, 
	58u, 59u, 13u, 32u, 42u, 63u, 92u, 93u, 
	95u, 9u, 10u, 33u, 34u, 39u, 41u, 44u, 
	46u, 58u, 59u, 13u, 32u, 42u, 63u, 92u, 
	93u, 95u, 9u, 10u, 33u, 34u, 39u, 41u, 
	44u, 46u, 58u, 59u, 13u, 32u, 42u, 63u, 
	91u, 92u, 95u, 9u, 10u, 33u, 34u, 39u, 
	41u, 44u, 46u, 58u, 59u, 13u, 32u, 42u, 
	63u, 92u, 93u, 95u, 9u, 10u, 33u, 34u, 
	39u, 41u, 44u, 46u, 58u, 59u, 13u, 32u, 
	42u, 63u, 92u, 93u, 95u, 9u, 10u, 33u, 
	34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u, 
	32u, 42u, 63u, 92u, 93u, 95u, 9u, 10u, 
	33u, 34u, 39u, 41u, 44u, 46u, 58u, 59u, 
	13u, 32u, 42u, 63u, 92u, 93u, 95u, 9u, 
	10u, 33u, 34u, 39u, 41u, 44u, 46u, 58u, 
	59u, 13u, 32u, 42u, 63u, 91u, 92u, 93u, 
	95u, 9u, 10u, 33u, 34u, 39u, 41u, 44u, 
	46u, 58u, 59u, 13u, 32u, 42u, 63u, 92u, 
	93u, 95u, 9u, 10u, 33u, 34u, 39u, 41u, 
	44u, 46u, 58u, 59u, 13u, 32u, 42u, 63u, 
	92u, 93u, 95u, 9u, 10u, 33u, 34u, 39u, 
	41u, 44u, 46u, 58u, 59u, 13u, 32u, 42u, 
	63u, 92u, 93u, 95u, 9u, 10u, 33u, 34u, 
	39u, 41u, 44u, 46u, 58u, 59u, 13u, 32u, 
	42u, 63u, 92u, 93u, 95u, 9u, 10u, 33u, 
	34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u, 
	32u, 42u, 63u, 91u, 92u, 93u, 95u, 9u, 
	10u, 33u, 34u, 39u, 41u, 44u, 46u, 58u, 
	59u, 13u, 32u, 42u, 63u, 92u, 93u, 95u, 
	9u, 10u, 33u, 34u, 39u, 41u, 44u, 46u, 
	58u, 59u, 13u, 32u, 42u, 63u, 92u, 93u, 
	95u, 9u, 10u, 33u, 34u, 39u, 41u, 44u, 
	46u, 58u, 59u, 13u, 32u, 42u, 63u, 92u, 
	93u, 95u, 9u, 10u, 33u, 34u, 39u, 41u, 
	44u, 46u, 58u, 59u, 13u, 32u, 42u, 63u, 
	91u, 92u, 93u, 95u, 9u, 10u, 33u, 34u, 
	39u, 41u, 44u, 46u, 58u, 59u, 13u, 32u, 
	42u, 63u, 91u, 95u, 9u, 10u, 33u, 34u, 
	39u, 41u, 44u, 46u, 58u, 59u, 13u, 32u, 
	42u, 63u, 93u, 95u, 9u, 10u, 33u, 34u, 
	39u, 41u, 44u, 46u, 58u, 59u, 13u, 32u, 
	42u, 63u, 93u, 95u, 9u, 10u, 33u, 34u, 
	39u, 41u, 44u, 46u, 58u, 59u, 13u, 32u, 
	42u, 63u, 93u, 95u, 9u, 10u, 33u, 34u, 
	39u, 41u, 44u, 46u, 58u, 59u, 13u, 32u, 
	42u, 63u, 92u, 93u, 95u, 9u, 10u, 33u, 
	34u, 39u, 41u, 44u, 46u, 58u, 59u, 13u, 
	32u, 42u, 63u, 92u, 93u, 95u, 9u, 10u, 
	33u, 34u, 39u, 41u, 44u, 46u, 58u, 59u, 
	13u, 32u, 42u, 63u, 92u, 93u, 95u, 9u, 
	10u, 33u, 34u, 39u, 41u, 44u, 46u, 58u, 
	59u, 13u, 32u, 42u, 63u, 93u, 95u, 9u, 
	10u, 33u, 34u, 39u, 41u, 44u, 46u, 58u, 
	59u, 13u, 32u, 42u, 63u, 91u, 93u, 95u, 
	9u, 10u, 33u, 34u, 39u, 41u, 44u, 46u, 
	58u, 59u, 13u, 32u, 42u, 63u, 92u, 93u, 
	95u, 9u, 10u, 33u, 34u, 39u, 41u, 44u, 
	46u, 58u, 59u, 13u, 32u, 42u, 63u, 93u, 
	95u, 9u, 10u, 33u, 34u, 39u, 41u, 44u, 
	46u, 58u, 59u, 13u, 32u, 42u, 63u, 93u, 
	95u, 9u, 10u, 33u, 34u, 39u, 41u, 44u, 
	46u, 58u, 59u, 13u, 32u, 42u, 63u, 93u, 
	95u, 9u, 10u, 33u, 34u, 39u, 41u, 44u, 
	46u, 58u, 59u, 13u, 32u, 42u, 63u, 93u, 
	95u, 9u, 10u, 33u, 34u, 39u, 41u, 44u, 
	46u, 58u, 59u, 13u, 32u, 42u, 63u, 91u, 
	93u, 95u, 9u, 10u, 33u, 34u, 39u, 41u, 
	44u, 46u, 58u, 59u, 13u, 32u, 42u, 63u, 
	92u, 93u, 95u, 9u, 10u, 33u, 34u, 39u, 
	41u, 44u, 46u, 58u, 59u, 13u, 32u, 42u, 
	63u, 92u, 93u, 95u, 9u, 10u, 33u, 34u, 
	39u, 41u, 44u, 46u, 58u, 59u, 0
};

static const char _MARK2_single_lengths[] = {
	5, 6, 6, 6, 6, 6, 7, 6, 
	6, 7, 7, 7, 7, 7, 7, 7, 
	7, 7, 8, 7, 7, 8, 5, 7, 
	7, 7, 7, 7, 7, 7, 8, 7, 
	7, 7, 7, 8, 7, 7, 7, 8, 
	6, 6, 6, 6, 7, 7, 7, 6, 
	7, 7, 6, 6, 6, 6, 7, 7, 
	7
};

static const char _MARK2_range_lengths[] = {
	5, 5, 5, 5, 5, 5, 5, 8, 
	5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 8, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 
	5
};

static const short _MARK2_index_offsets[] = {
	0, 11, 23, 35, 47, 59, 71, 84, 
	99, 111, 124, 137, 150, 163, 176, 189, 
	202, 215, 228, 242, 258, 271, 285, 296, 
	309, 322, 335, 348, 361, 374, 387, 401, 
	414, 427, 440, 453, 467, 480, 493, 506, 
	520, 532, 544, 556, 568, 581, 594, 607, 
	619, 632, 645, 657, 669, 681, 693, 706, 
	719
};

static const unsigned char _MARK2_indicies[] = {
	1, 1, 3, 2, 4, 1, 2, 2, 
	2, 2, 0, 6, 6, 8, 7, 9, 
	10, 6, 7, 7, 7, 7, 5, 6, 
	6, 11, 7, 12, 13, 6, 7, 7, 
	7, 7, 5, 6, 6, 8, 7, 9, 
	14, 6, 7, 7, 7, 7, 5, 16, 
	16, 18, 17, 19, 20, 16, 17, 17, 
	17, 17, 15, 6, 6, 21, 7, 9, 
	10, 6, 7, 7, 7, 7, 5, 6, 
	6, 8, 7, 22, 9, 10, 6, 7, 
	7, 7, 7, 5, 6, 6, 8, 7, 
	9, 10, 6, 7, 7, 7, 23, 7, 
	23, 23, 5, 6, 6, 8, 7, 24, 
	10, 6, 7, 7, 7, 7, 5, 26, 
	26, 28, 27, 29, 30, 31, 26, 27, 
	27, 27, 27, 25, 33, 33, 35, 34, 
	36, 37, 38, 33, 34, 34, 34, 34, 
	32, 39, 39, 35, 40, 36, 37, 41, 
	39, 40, 40, 40, 40, 32, 39, 39, 
	42, 40, 43, 36, 44, 39, 40, 40, 
	40, 40, 32, 39, 39, 47, 46, 48, 
	49, 41, 39, 46, 46, 46, 46, 45, 
	51, 51, 53, 52, 54, 55, 41, 51, 
	52, 52, 52, 52, 50, 57, 57, 59, 
	58, 60, 61, 41, 57, 58, 58, 58, 
	58, 56, 39, 39, 62, 40, 36, 37, 
	41, 39, 40, 40, 40, 40, 32, 39, 
	39, 35, 40, 36, 37, 38, 39, 40, 
	40, 40, 40, 32, 39, 39, 35, 40, 
	63, 36, 37, 41, 39, 40, 40, 40, 
	40, 32, 39, 39, 35, 40, 36, 37, 
	41, 39, 40, 40, 40, 64, 40, 64, 
	64, 32, 39, 39, 35, 40, 36, 65, 
	41, 39, 40, 40, 40, 40, 32, 67, 
	67, 69, 68, 70, 71, 72, 41, 67, 
	68, 68, 68, 68, 66, 74, 74, 76, 
	75, 77, 74, 75, 75, 75, 75, 73, 
	6, 6, 35, 40, 36, 37, 38, 6, 
	40, 40, 40, 40, 32, 78, 78, 35, 
	34, 36, 37, 41, 78, 34, 34, 34, 
	34, 32, 80, 80, 82, 81, 83, 84, 
	85, 80, 81, 81, 81, 81, 79, 39, 
	39, 88, 87, 89, 90, 41, 39, 87, 
	87, 87, 87, 86, 92, 92, 94, 93, 
	95, 96, 41, 92, 93, 93, 93, 93, 
	91, 98, 98, 100, 99, 101, 102, 41, 
	98, 99, 99, 99, 99, 97, 92, 92, 
	94, 93, 95, 96, 103, 92, 93, 93, 
	93, 93, 91, 92, 92, 94, 93, 104, 
	95, 96, 41, 92, 93, 93, 93, 93, 
	91, 39, 39, 107, 106, 108, 109, 41, 
	39, 106, 106, 106, 106, 105, 111, 111, 
	113, 112, 114, 115, 41, 111, 112, 112, 
	112, 112, 110, 117, 117, 119, 118, 120, 
	121, 41, 117, 118, 118, 118, 118, 116, 
	111, 111, 113, 112, 114, 115, 122, 111, 
	112, 112, 112, 112, 110, 111, 111, 113, 
	112, 123, 114, 115, 41, 111, 112, 112, 
	112, 112, 110, 125, 125, 127, 126, 128, 
	129, 41, 125, 126, 126, 126, 126, 124, 
	51, 51, 130, 52, 54, 55, 41, 51, 
	52, 52, 52, 52, 50, 51, 51, 53, 
	52, 54, 55, 131, 51, 52, 52, 52, 
	52, 50, 51, 51, 53, 52, 132, 54, 
	55, 41, 51, 52, 52, 52, 52, 50, 
	134, 134, 136, 135, 137, 138, 134, 135, 
	135, 135, 135, 133, 6, 6, 141, 140, 
	142, 143, 6, 140, 140, 140, 140, 139, 
	145, 145, 147, 146, 148, 149, 145, 146, 
	146, 146, 146, 144, 145, 145, 147, 146, 
	148, 150, 145, 146, 146, 146, 146, 144, 
	33, 33, 107, 151, 108, 109, 152, 33, 
	151, 151, 151, 151, 105, 154, 154, 156, 
	155, 157, 158, 41, 154, 155, 155, 155, 
	155, 153, 159, 159, 113, 160, 114, 115, 
	41, 159, 160, 160, 160, 160, 110, 145, 
	145, 161, 146, 148, 149, 145, 146, 146, 
	146, 146, 144, 145, 145, 147, 146, 162, 
	148, 149, 145, 146, 146, 146, 146, 144, 
	163, 163, 53, 164, 54, 55, 131, 163, 
	164, 164, 164, 164, 50, 6, 6, 167, 
	166, 168, 169, 6, 166, 166, 166, 166, 
	165, 171, 171, 173, 172, 174, 175, 171, 
	172, 172, 172, 172, 170, 171, 171, 173, 
	172, 174, 176, 171, 172, 172, 172, 172, 
	170, 178, 178, 180, 179, 181, 182, 178, 
	179, 179, 179, 179, 177, 171, 171, 173, 
	172, 183, 174, 175, 171, 172, 172, 172, 
	172, 170, 184, 184, 94, 185, 95, 96, 
	103, 184, 185, 185, 185, 185, 91, 6, 
	6, 107, 106, 108, 109, 152, 6, 106, 
	106, 106, 106, 105, 0
};

static const char _MARK2_trans_targs[] = {
	1, 2, 3, 5, 23, 1, 2, 3, 
	4, 6, 10, 41, 50, 56, 44, 1, 
	2, 3, 5, 6, 10, 5, 7, 8, 
	9, 1, 2, 3, 4, 7, 6, 10, 
	11, 40, 36, 15, 17, 18, 24, 12, 
	11, 22, 13, 26, 31, 14, 14, 37, 
	38, 39, 11, 12, 11, 15, 17, 18, 
	11, 12, 11, 16, 17, 18, 16, 19, 
	20, 21, 11, 12, 11, 15, 19, 17, 
	18, 1, 2, 3, 5, 23, 25, 11, 
	12, 11, 13, 26, 17, 31, 27, 27, 
	28, 29, 30, 11, 12, 11, 15, 17, 
	18, 11, 12, 11, 16, 17, 18, 24, 
	19, 32, 32, 33, 34, 35, 11, 12, 
	11, 15, 17, 18, 11, 12, 11, 16, 
	17, 18, 24, 19, 11, 12, 11, 15, 
	17, 18, 16, 24, 19, 1, 2, 3, 
	41, 50, 56, 42, 43, 47, 48, 49, 
	1, 2, 3, 4, 6, 10, 44, 45, 
	46, 11, 12, 11, 15, 17, 18, 25, 
	36, 5, 7, 40, 36, 51, 52, 53, 
	54, 55, 1, 2, 3, 4, 6, 10, 
	44, 1, 2, 3, 5, 6, 10, 7, 
	40, 36
};

static const short _MARK2_trans_actions[] = {
	196, 191, 327, 82, 333, 58, 38, 106, 
	12, 58, 141, 12, 58, 141, 141, 186, 
	181, 315, 78, 186, 321, 12, 58, 58, 
	58, 101, 96, 213, 34, 101, 101, 219, 
	58, 38, 106, 12, 58, 58, 141, 38, 
	106, 0, 12, 58, 141, 58, 106, 12, 
	58, 58, 171, 166, 291, 70, 171, 171, 
	186, 181, 315, 78, 186, 186, 12, 58, 
	58, 58, 101, 96, 213, 34, 101, 101, 
	101, 161, 156, 279, 66, 285, 38, 136, 
	111, 225, 54, 136, 136, 267, 58, 106, 
	12, 58, 58, 91, 86, 201, 30, 91, 
	91, 309, 303, 353, 176, 309, 309, 207, 
	91, 58, 106, 12, 58, 58, 121, 116, 
	231, 42, 121, 121, 261, 255, 346, 131, 
	261, 261, 237, 121, 151, 146, 273, 62, 
	151, 151, 70, 297, 171, 136, 111, 225, 
	54, 136, 267, 58, 106, 12, 58, 141, 
	171, 166, 291, 70, 171, 297, 297, 106, 
	141, 249, 243, 339, 126, 249, 249, 116, 
	231, 70, 171, 166, 291, 58, 106, 12, 
	58, 141, 91, 86, 201, 30, 91, 207, 
	207, 309, 303, 353, 176, 309, 360, 91, 
	86, 201
};

static const short _MARK2_eof_actions[] = {
	27, 1, 1, 1, 24, 1, 1, 1, 
	1, 6, 1, 1, 1, 1, 21, 24, 
	1, 1, 1, 1, 1, 6, 18, 1, 
	1, 15, 1, 3, 74, 3, 3, 1, 
	9, 50, 9, 9, 15, 21, 21, 21, 
	15, 1, 21, 21, 1, 46, 9, 21, 
	21, 21, 1, 3, 3, 74, 3, 3, 
	1
};

static const int MARK2_start = 0;
static const int MARK2_first_final = 0;
static const int MARK2_error = -1;

static const int MARK2_en_main = 0;


#line 110 "MARK2.rl"

pro(MARK2lexer, MARK2state* state) {
    a$dup(u8c, text, state->text);
    sane($ok(text));

    int cs = state->cs;
    int res = 0;
    u8c *p = (u8c*) text[0];
    u8c *pe = (u8c*) text[1];
    u8c *eof = state->tbc ? NULL : pe;
    u8c *pb = p;

    u32 sp = 2;
    $u8c tok = {p, p};

    
#line 405 "MARK2.rl.c"
	{
	cs = MARK2_start;
	}

#line 126 "MARK2.rl"
    
#line 408 "MARK2.rl.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const unsigned char *_keys;

	if ( p == pe )
		goto _test_eof;
_resume:
	_keys = _MARK2_trans_keys + _MARK2_key_offsets[cs];
	_trans = _MARK2_index_offsets[cs];

	_klen = _MARK2_single_lengths[cs];
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

	_klen = _MARK2_range_lengths[cs];
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
	_trans = _MARK2_indicies[_trans];
	cs = _MARK2_trans_targs[_trans];

	if ( _MARK2_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _MARK2_actions + _MARK2_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 10 "MARK2.rl"
	{ state->mark0[MARK2Ref0] = p - state->doc[0]; }
	break;
	case 1:
#line 11 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Ref0];
    tok[1] = p;
    call(MARK2onRef0, tok, state); 
}
	break;
	case 2:
#line 16 "MARK2.rl"
	{ state->mark0[MARK2Ref1] = p - state->doc[0]; }
	break;
	case 3:
#line 17 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Ref1];
    tok[1] = p;
    call(MARK2onRef1, tok, state); 
}
	break;
	case 4:
#line 22 "MARK2.rl"
	{ state->mark0[MARK2Em0] = p - state->doc[0]; }
	break;
	case 5:
#line 23 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Em0];
    tok[1] = p;
    call(MARK2onEm0, tok, state); 
}
	break;
	case 6:
#line 28 "MARK2.rl"
	{ state->mark0[MARK2Em1] = p - state->doc[0]; }
	break;
	case 7:
#line 29 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Em1];
    tok[1] = p;
    call(MARK2onEm1, tok, state); 
}
	break;
	case 8:
#line 34 "MARK2.rl"
	{ state->mark0[MARK2Em] = p - state->doc[0]; }
	break;
	case 9:
#line 35 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Em];
    tok[1] = p;
    call(MARK2onEm, tok, state); 
}
	break;
	case 10:
#line 40 "MARK2.rl"
	{ state->mark0[MARK2St0] = p - state->doc[0]; }
	break;
	case 11:
#line 41 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2St0];
    tok[1] = p;
    call(MARK2onSt0, tok, state); 
}
	break;
	case 12:
#line 46 "MARK2.rl"
	{ state->mark0[MARK2St1] = p - state->doc[0]; }
	break;
	case 13:
#line 47 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2St1];
    tok[1] = p;
    call(MARK2onSt1, tok, state); 
}
	break;
	case 14:
#line 58 "MARK2.rl"
	{ state->mark0[MARK2Root] = p - state->doc[0]; }
	break;
#line 552 "MARK2.rl.c"
		}
	}

_again:
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _MARK2_actions + _MARK2_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 1:
#line 11 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Ref0];
    tok[1] = p;
    call(MARK2onRef0, tok, state); 
}
	break;
	case 3:
#line 17 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Ref1];
    tok[1] = p;
    call(MARK2onRef1, tok, state); 
}
	break;
	case 5:
#line 23 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Em0];
    tok[1] = p;
    call(MARK2onEm0, tok, state); 
}
	break;
	case 7:
#line 29 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Em1];
    tok[1] = p;
    call(MARK2onEm1, tok, state); 
}
	break;
	case 9:
#line 35 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Em];
    tok[1] = p;
    call(MARK2onEm, tok, state); 
}
	break;
	case 11:
#line 41 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2St0];
    tok[1] = p;
    call(MARK2onSt0, tok, state); 
}
	break;
	case 13:
#line 47 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2St1];
    tok[1] = p;
    call(MARK2onSt1, tok, state); 
}
	break;
	case 14:
#line 58 "MARK2.rl"
	{ state->mark0[MARK2Root] = p - state->doc[0]; }
	break;
	case 15:
#line 59 "MARK2.rl"
	{
    tok[0] = state->doc[0]+state->mark0[MARK2Root];
    tok[1] = p;
    call(MARK2onRoot, tok, state); 
}
	break;
#line 624 "MARK2.rl.c"
		}
	}
	}

	}

#line 127 "MARK2.rl"

    test(p==text[1], MARK2fail);

    if (state->tbc) {
        test(cs != MARK2_error, MARK2fail);
        state->cs = cs;
    } else {
        test(cs >= MARK2_first_final, MARK2fail);
    }

    nedo(
        state->text[0] = p;
    );
}
