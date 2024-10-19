
#line 1 "RDXJ.rl"
#include "RDXJ.rl.h"



#line 178 "RDXJ.rl"



#line 7 "RDXJ.rl.c"
static const char _RDXJ_actions[] = {
	0, 1, 1, 1, 3, 1, 5, 1, 
	7, 1, 9, 1, 10, 1, 11, 1, 
	12, 1, 13, 1, 14, 1, 15, 1, 
	16, 1, 17, 1, 18, 1, 19, 1, 
	20, 1, 21, 1, 22, 1, 23, 1, 
	24, 1, 25, 1, 26, 1, 27, 1, 
	30, 1, 31, 2, 1, 22, 2, 3, 
	22, 2, 5, 22, 2, 7, 22, 2, 
	9, 22, 2, 11, 10, 2, 11, 12, 
	2, 11, 14, 2, 11, 16, 2, 11, 
	18, 2, 11, 20, 2, 11, 24, 2, 
	11, 26, 2, 11, 31, 2, 13, 10, 
	2, 13, 12, 2, 13, 14, 2, 13, 
	16, 2, 13, 18, 2, 13, 20, 2, 
	13, 24, 2, 13, 26, 2, 13, 31, 
	2, 15, 10, 2, 15, 12, 2, 15, 
	14, 2, 15, 16, 2, 15, 18, 2, 
	15, 20, 2, 15, 24, 2, 15, 26, 
	2, 15, 31, 2, 17, 10, 2, 17, 
	12, 2, 17, 14, 2, 17, 16, 2, 
	17, 18, 2, 17, 20, 2, 17, 24, 
	2, 17, 26, 2, 17, 31, 2, 19, 
	10, 2, 19, 12, 2, 19, 14, 2, 
	19, 16, 2, 19, 18, 2, 19, 20, 
	2, 19, 24, 2, 19, 26, 2, 19, 
	31, 2, 21, 10, 2, 21, 12, 2, 
	21, 14, 2, 21, 16, 2, 21, 18, 
	2, 21, 20, 2, 21, 24, 2, 21, 
	26, 2, 21, 31, 2, 25, 10, 2, 
	25, 12, 2, 25, 14, 2, 25, 16, 
	2, 25, 18, 2, 25, 20, 2, 25, 
	24, 2, 25, 26, 2, 25, 31, 2, 
	27, 10, 2, 27, 12, 2, 27, 14, 
	2, 27, 16, 2, 27, 18, 2, 27, 
	20, 2, 27, 24, 2, 27, 26, 2, 
	27, 31, 2, 28, 6, 2, 28, 8, 
	2, 29, 10, 2, 29, 12, 2, 29, 
	14, 2, 29, 16, 2, 29, 18, 2, 
	29, 20, 2, 29, 24, 2, 29, 26, 
	2, 29, 31, 2, 30, 10, 2, 30, 
	12, 2, 30, 14, 2, 30, 16, 2, 
	30, 18, 2, 30, 20, 2, 30, 24, 
	2, 30, 26, 2, 30, 31, 3, 1, 
	29, 10, 3, 1, 29, 12, 3, 1, 
	29, 14, 3, 1, 29, 16, 3, 1, 
	29, 18, 3, 1, 29, 20, 3, 1, 
	29, 24, 3, 1, 29, 26, 3, 1, 
	29, 31, 3, 3, 29, 10, 3, 3, 
	29, 12, 3, 3, 29, 14, 3, 3, 
	29, 16, 3, 3, 29, 18, 3, 3, 
	29, 20, 3, 3, 29, 24, 3, 3, 
	29, 26, 3, 3, 29, 31, 3, 5, 
	29, 10, 3, 5, 29, 12, 3, 5, 
	29, 14, 3, 5, 29, 16, 3, 5, 
	29, 18, 3, 5, 29, 20, 3, 5, 
	29, 24, 3, 5, 29, 26, 3, 5, 
	29, 31, 3, 7, 29, 10, 3, 7, 
	29, 12, 3, 7, 29, 14, 3, 7, 
	29, 16, 3, 7, 29, 18, 3, 7, 
	29, 20, 3, 7, 29, 24, 3, 7, 
	29, 26, 3, 7, 29, 31, 3, 9, 
	29, 10, 3, 9, 29, 12, 3, 9, 
	29, 14, 3, 9, 29, 16, 3, 9, 
	29, 18, 3, 9, 29, 20, 3, 9, 
	29, 24, 3, 9, 29, 26, 3, 9, 
	29, 31, 3, 11, 28, 6, 3, 11, 
	28, 8, 3, 13, 28, 6, 3, 13, 
	28, 8, 3, 15, 28, 6, 3, 15, 
	28, 8, 3, 17, 28, 6, 3, 17, 
	28, 8, 3, 19, 28, 6, 3, 19, 
	28, 8, 3, 21, 28, 6, 3, 21, 
	28, 8, 3, 23, 29, 10, 3, 23, 
	29, 12, 3, 23, 29, 14, 3, 23, 
	29, 16, 3, 23, 29, 18, 3, 23, 
	29, 20, 3, 23, 29, 24, 3, 23, 
	29, 26, 3, 23, 29, 31, 3, 25, 
	28, 6, 3, 25, 28, 8, 3, 27, 
	28, 6, 3, 27, 28, 8, 3, 28, 
	2, 0, 3, 28, 4, 8, 3, 30, 
	28, 6, 3, 30, 28, 8, 4, 11, 
	28, 2, 0, 4, 11, 28, 4, 8, 
	4, 13, 28, 2, 0, 4, 13, 28, 
	4, 8, 4, 15, 28, 2, 0, 4, 
	15, 28, 4, 8, 4, 17, 28, 2, 
	0, 4, 17, 28, 4, 8, 4, 19, 
	28, 2, 0, 4, 19, 28, 4, 8, 
	4, 21, 28, 2, 0, 4, 21, 28, 
	4, 8, 4, 25, 28, 2, 0, 4, 
	25, 28, 4, 8, 4, 27, 28, 2, 
	0, 4, 27, 28, 4, 8, 4, 28, 
	2, 0, 4, 4, 30, 28, 2, 0, 
	4, 30, 28, 4, 8, 5, 11, 28, 
	2, 0, 4, 5, 13, 28, 2, 0, 
	4, 5, 15, 28, 2, 0, 4, 5, 
	17, 28, 2, 0, 4, 5, 19, 28, 
	2, 0, 4, 5, 21, 28, 2, 0, 
	4, 5, 25, 28, 2, 0, 4, 5, 
	27, 28, 2, 0, 4, 5, 30, 28, 
	2, 0, 4
};

static const short _RDXJ_key_offsets[] = {
	0, 0, 4, 7, 9, 15, 21, 28, 
	34, 41, 49, 51, 57, 61, 70, 76, 
	82, 88, 94, 119, 144, 157, 170, 195, 
	220, 245, 261, 278, 303, 326, 345, 363, 
	375, 400, 423, 438, 463, 488, 508, 533, 
	558, 577, 597
};

static const unsigned char _RDXJ_trans_keys[] = {
	34u, 92u, 0u, 31u, 48u, 49u, 57u, 48u, 
	57u, 48u, 57u, 65u, 70u, 97u, 102u, 48u, 
	57u, 65u, 70u, 97u, 102u, 45u, 48u, 57u, 
	65u, 70u, 97u, 102u, 48u, 57u, 65u, 70u, 
	97u, 102u, 45u, 48u, 57u, 65u, 70u, 97u, 
	102u, 43u, 45u, 48u, 57u, 65u, 70u, 97u, 
	102u, 48u, 57u, 48u, 57u, 65u, 70u, 97u, 
	102u, 43u, 45u, 48u, 57u, 34u, 47u, 92u, 
	98u, 102u, 110u, 114u, 116u, 117u, 48u, 57u, 
	65u, 70u, 97u, 102u, 48u, 57u, 65u, 70u, 
	97u, 102u, 48u, 57u, 65u, 70u, 97u, 102u, 
	48u, 57u, 65u, 70u, 97u, 102u, 13u, 32u, 
	34u, 40u, 41u, 44u, 45u, 48u, 58u, 91u, 
	93u, 123u, 125u, 9u, 10u, 49u, 57u, 65u, 
	70u, 71u, 90u, 97u, 102u, 103u, 122u, 13u, 
	32u, 34u, 40u, 41u, 44u, 45u, 48u, 58u, 
	91u, 93u, 123u, 125u, 9u, 10u, 49u, 57u, 
	65u, 70u, 71u, 90u, 97u, 102u, 103u, 122u, 
	13u, 32u, 40u, 41u, 44u, 58u, 64u, 91u, 
	93u, 123u, 125u, 9u, 10u, 13u, 32u, 40u, 
	41u, 44u, 58u, 64u, 91u, 93u, 123u, 125u, 
	9u, 10u, 13u, 32u, 34u, 40u, 41u, 44u, 
	45u, 48u, 58u, 91u, 93u, 123u, 125u, 9u, 
	10u, 49u, 57u, 65u, 70u, 71u, 90u, 97u, 
	102u, 103u, 122u, 13u, 32u, 34u, 40u, 41u, 
	44u, 45u, 48u, 58u, 91u, 93u, 123u, 125u, 
	9u, 10u, 49u, 57u, 65u, 70u, 71u, 90u, 
	97u, 102u, 103u, 122u, 13u, 32u, 34u, 40u, 
	41u, 44u, 45u, 48u, 58u, 91u, 93u, 123u, 
	125u, 9u, 10u, 49u, 57u, 65u, 70u, 71u, 
	90u, 97u, 102u, 103u, 122u, 13u, 32u, 40u, 
	41u, 44u, 46u, 58u, 64u, 69u, 91u, 93u, 
	101u, 123u, 125u, 9u, 10u, 13u, 32u, 40u, 
	41u, 44u, 58u, 64u, 69u, 91u, 93u, 101u, 
	123u, 125u, 9u, 10u, 48u, 57u, 13u, 32u, 
	34u, 40u, 41u, 44u, 45u, 48u, 58u, 91u, 
	93u, 123u, 125u, 9u, 10u, 49u, 57u, 65u, 
	70u, 71u, 90u, 97u, 102u, 103u, 122u, 13u, 
	32u, 40u, 41u, 44u, 45u, 46u, 58u, 64u, 
	69u, 91u, 93u, 101u, 123u, 125u, 9u, 10u, 
	48u, 57u, 65u, 70u, 97u, 102u, 13u, 32u, 
	40u, 41u, 44u, 58u, 64u, 91u, 93u, 123u, 
	125u, 9u, 10u, 48u, 57u, 65u, 70u, 97u, 
	102u, 13u, 32u, 40u, 41u, 44u, 58u, 91u, 
	93u, 123u, 125u, 9u, 10u, 48u, 57u, 65u, 
	70u, 97u, 102u, 13u, 32u, 40u, 41u, 44u, 
	58u, 91u, 93u, 123u, 125u, 9u, 10u, 13u, 
	32u, 34u, 40u, 41u, 44u, 45u, 48u, 58u, 
	91u, 93u, 123u, 125u, 9u, 10u, 49u, 57u, 
	65u, 70u, 71u, 90u, 97u, 102u, 103u, 122u, 
	13u, 32u, 40u, 41u, 44u, 45u, 46u, 58u, 
	64u, 69u, 91u, 93u, 101u, 123u, 125u, 9u, 
	10u, 48u, 57u, 65u, 70u, 97u, 102u, 13u, 
	32u, 40u, 41u, 44u, 58u, 64u, 91u, 93u, 
	123u, 125u, 9u, 10u, 48u, 57u, 13u, 32u, 
	34u, 40u, 41u, 44u, 45u, 48u, 58u, 91u, 
	93u, 123u, 125u, 9u, 10u, 49u, 57u, 65u, 
	70u, 71u, 90u, 97u, 102u, 103u, 122u, 13u, 
	32u, 40u, 41u, 44u, 45u, 58u, 64u, 91u, 
	93u, 95u, 123u, 125u, 9u, 10u, 48u, 57u, 
	65u, 70u, 71u, 90u, 97u, 102u, 103u, 122u, 
	13u, 32u, 40u, 41u, 44u, 58u, 64u, 91u, 
	93u, 95u, 123u, 125u, 9u, 10u, 48u, 57u, 
	65u, 90u, 97u, 122u, 13u, 32u, 34u, 40u, 
	41u, 44u, 45u, 48u, 58u, 91u, 93u, 123u, 
	125u, 9u, 10u, 49u, 57u, 65u, 70u, 71u, 
	90u, 97u, 102u, 103u, 122u, 13u, 32u, 34u, 
	40u, 41u, 44u, 45u, 48u, 58u, 91u, 93u, 
	123u, 125u, 9u, 10u, 49u, 57u, 65u, 70u, 
	71u, 90u, 97u, 102u, 103u, 122u, 13u, 32u, 
	40u, 41u, 44u, 58u, 64u, 91u, 93u, 123u, 
	125u, 9u, 10u, 48u, 57u, 65u, 70u, 97u, 
	102u, 13u, 32u, 40u, 41u, 44u, 45u, 58u, 
	64u, 91u, 93u, 123u, 125u, 9u, 10u, 48u, 
	57u, 65u, 70u, 97u, 102u, 13u, 32u, 40u, 
	41u, 44u, 46u, 58u, 64u, 69u, 91u, 93u, 
	101u, 123u, 125u, 9u, 10u, 48u, 57u, 0
};

static const char _RDXJ_single_lengths[] = {
	0, 2, 1, 0, 0, 0, 1, 0, 
	1, 2, 0, 0, 2, 9, 0, 0, 
	0, 0, 13, 13, 11, 11, 13, 13, 
	13, 14, 13, 13, 15, 11, 10, 10, 
	13, 15, 11, 13, 13, 12, 13, 13, 
	11, 12, 14
};

static const char _RDXJ_range_lengths[] = {
	0, 1, 1, 1, 3, 3, 3, 3, 
	3, 3, 1, 3, 1, 0, 3, 3, 
	3, 3, 6, 6, 1, 1, 6, 6, 
	6, 1, 2, 6, 4, 4, 4, 1, 
	6, 4, 2, 6, 6, 4, 6, 6, 
	4, 4, 2
};

static const short _RDXJ_index_offsets[] = {
	0, 0, 4, 7, 9, 13, 17, 22, 
	26, 31, 37, 39, 43, 47, 57, 61, 
	65, 69, 73, 93, 113, 126, 139, 159, 
	179, 199, 215, 231, 251, 271, 287, 302, 
	314, 334, 354, 368, 388, 408, 425, 445, 
	465, 481, 498
};

static const unsigned char _RDXJ_indicies[] = {
	2, 3, 1, 0, 4, 5, 1, 6, 
	1, 7, 7, 7, 1, 8, 8, 8, 
	1, 9, 8, 8, 8, 1, 10, 10, 
	10, 1, 11, 12, 12, 12, 1, 13, 
	14, 15, 12, 12, 1, 16, 1, 17, 
	7, 7, 1, 13, 13, 16, 1, 0, 
	0, 0, 0, 0, 0, 0, 0, 18, 
	1, 19, 19, 19, 1, 20, 20, 20, 
	1, 21, 21, 21, 1, 0, 0, 0, 
	1, 22, 22, 23, 24, 25, 26, 27, 
	28, 30, 33, 34, 35, 36, 22, 29, 
	31, 32, 31, 32, 1, 37, 37, 38, 
	39, 40, 41, 42, 43, 45, 48, 49, 
	50, 51, 37, 44, 46, 47, 46, 47, 
	1, 52, 52, 53, 54, 55, 56, 57, 
	58, 59, 60, 61, 52, 1, 62, 62, 
	63, 64, 65, 66, 67, 68, 69, 70, 
	71, 62, 1, 72, 72, 73, 74, 75, 
	76, 77, 78, 80, 83, 84, 85, 86, 
	72, 79, 81, 82, 81, 82, 1, 87, 
	87, 88, 89, 90, 91, 92, 93, 95, 
	98, 99, 100, 101, 87, 94, 96, 97, 
	96, 97, 1, 102, 102, 103, 104, 105, 
	106, 107, 108, 110, 113, 114, 115, 116, 
	102, 109, 111, 112, 111, 112, 1, 117, 
	117, 118, 119, 120, 121, 122, 123, 124, 
	125, 126, 124, 127, 128, 117, 1, 129, 
	129, 130, 131, 132, 133, 134, 124, 135, 
	136, 124, 137, 138, 129, 6, 1, 139, 
	139, 140, 141, 142, 143, 144, 145, 147, 
	150, 151, 152, 153, 139, 146, 148, 149, 
	148, 149, 1, 117, 117, 118, 119, 120, 
	11, 121, 122, 123, 154, 125, 126, 154, 
	127, 128, 117, 12, 12, 12, 1, 155, 
	155, 156, 157, 158, 159, 160, 161, 162, 
	163, 164, 155, 7, 7, 7, 1, 165, 
	165, 166, 167, 168, 169, 170, 171, 172, 
	173, 165, 10, 10, 10, 1, 174, 174, 
	63, 64, 65, 66, 68, 69, 70, 71, 
	174, 1, 175, 175, 176, 177, 178, 179, 
	180, 181, 183, 186, 187, 188, 189, 175, 
	182, 184, 185, 184, 185, 1, 117, 117, 
	118, 119, 120, 11, 121, 122, 123, 154, 
	125, 126, 154, 127, 128, 117, 190, 12, 
	12, 1, 129, 129, 130, 131, 132, 133, 
	134, 135, 136, 137, 138, 129, 16, 1, 
	191, 191, 192, 193, 194, 195, 196, 197, 
	199, 202, 203, 204, 205, 191, 198, 200, 
	201, 200, 201, 1, 206, 206, 207, 208, 
	209, 11, 211, 212, 214, 215, 213, 216, 
	217, 206, 210, 210, 213, 210, 213, 1, 
	206, 206, 207, 208, 209, 211, 212, 214, 
	215, 213, 216, 217, 206, 213, 213, 213, 
	1, 218, 218, 219, 220, 221, 222, 223, 
	224, 226, 229, 230, 231, 232, 218, 225, 
	227, 228, 227, 228, 1, 233, 233, 234, 
	235, 236, 237, 238, 239, 241, 244, 245, 
	246, 247, 233, 240, 242, 243, 242, 243, 
	1, 129, 129, 130, 131, 132, 133, 134, 
	135, 136, 137, 138, 129, 17, 7, 7, 
	1, 129, 129, 130, 131, 132, 11, 133, 
	134, 135, 136, 137, 138, 129, 15, 12, 
	12, 1, 117, 117, 118, 119, 120, 121, 
	122, 123, 124, 125, 126, 124, 127, 128, 
	117, 5, 1, 0
};

static const char _RDXJ_trans_targs[] = {
	1, 0, 20, 13, 25, 42, 26, 29, 
	6, 7, 30, 4, 8, 10, 11, 41, 
	34, 40, 14, 15, 16, 17, 19, 1, 
	22, 23, 24, 2, 28, 33, 27, 36, 
	37, 32, 35, 38, 39, 19, 1, 22, 
	23, 24, 2, 28, 33, 27, 36, 37, 
	32, 35, 38, 39, 21, 22, 23, 24, 
	27, 5, 32, 35, 38, 39, 21, 22, 
	23, 24, 27, 5, 32, 35, 38, 39, 
	19, 1, 22, 23, 24, 2, 28, 33, 
	27, 36, 37, 32, 35, 38, 39, 19, 
	1, 22, 23, 24, 2, 28, 33, 27, 
	36, 37, 32, 35, 38, 39, 19, 1, 
	22, 23, 24, 2, 28, 33, 27, 36, 
	37, 32, 35, 38, 39, 21, 22, 23, 
	24, 3, 27, 5, 12, 32, 35, 38, 
	39, 21, 22, 23, 24, 27, 5, 32, 
	35, 38, 39, 19, 1, 22, 23, 24, 
	2, 28, 33, 27, 36, 37, 32, 35, 
	38, 39, 9, 21, 22, 23, 24, 27, 
	5, 32, 35, 38, 39, 31, 22, 23, 
	24, 27, 32, 35, 38, 39, 31, 19, 
	1, 22, 23, 24, 2, 28, 33, 27, 
	36, 37, 32, 35, 38, 39, 33, 19, 
	1, 22, 23, 24, 2, 28, 33, 27, 
	36, 37, 32, 35, 38, 39, 21, 22, 
	23, 24, 36, 27, 5, 37, 32, 35, 
	38, 39, 19, 1, 22, 23, 24, 2, 
	28, 33, 27, 36, 37, 32, 35, 38, 
	39, 19, 1, 22, 23, 24, 2, 28, 
	33, 27, 36, 37, 32, 35, 38, 39
};

static const short _RDXJ_trans_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 47, 630, 
	327, 330, 333, 723, 781, 781, 336, 728, 
	634, 321, 324, 315, 318, 0, 282, 27, 
	31, 39, 622, 718, 718, 43, 626, 285, 
	19, 23, 11, 15, 7, 466, 470, 474, 
	478, 60, 458, 462, 450, 454, 0, 300, 
	303, 306, 309, 35, 294, 297, 288, 291, 
	29, 554, 186, 189, 192, 678, 757, 757, 
	195, 683, 558, 180, 183, 174, 177, 33, 
	562, 213, 216, 219, 688, 763, 763, 222, 
	693, 566, 207, 210, 201, 204, 41, 606, 
	240, 243, 246, 698, 769, 769, 249, 703, 
	610, 234, 237, 228, 231, 1, 358, 362, 
	366, 0, 370, 51, 0, 350, 354, 342, 
	346, 3, 394, 398, 402, 406, 54, 386, 
	390, 378, 382, 45, 614, 267, 270, 273, 
	708, 775, 775, 276, 713, 618, 261, 264, 
	255, 258, 0, 5, 430, 434, 438, 442, 
	57, 422, 426, 414, 418, 37, 586, 590, 
	594, 598, 578, 582, 570, 574, 0, 21, 
	538, 132, 135, 138, 658, 745, 745, 141, 
	663, 542, 126, 129, 120, 123, 0, 25, 
	546, 159, 162, 165, 668, 751, 751, 168, 
	673, 550, 153, 156, 147, 150, 9, 502, 
	506, 510, 0, 514, 63, 0, 494, 498, 
	486, 490, 13, 522, 78, 81, 84, 638, 
	733, 733, 87, 643, 526, 72, 75, 66, 
	69, 17, 530, 105, 108, 111, 648, 739, 
	739, 114, 653, 534, 99, 102, 93, 96
};

static const short _RDXJ_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 339, 49, 482, 312, 198, 225, 
	252, 374, 410, 279, 374, 446, 602, 312, 
	144, 374, 410, 171, 518, 518, 90, 117, 
	410, 410, 374
};

static const int RDXJ_start = 18;
static const int RDXJ_first_final = 18;
static const int RDXJ_error = 0;

static const int RDXJ_en_main = 18;


#line 181 "RDXJ.rl"

pro(RDXJlexer, RDXJstate* state) {

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

    
#line 393 "RDXJ.rl.c"
	{
	cs = RDXJ_start;
	}

#line 199 "RDXJ.rl"
    
#line 396 "RDXJ.rl.c"
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
	_keys = _RDXJ_trans_keys + _RDXJ_key_offsets[cs];
	_trans = _RDXJ_index_offsets[cs];

	_klen = _RDXJ_single_lengths[cs];
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

	_klen = _RDXJ_range_lengths[cs];
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
	_trans = _RDXJ_indicies[_trans];
	cs = _RDXJ_trans_targs[_trans];

	if ( _RDXJ_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _RDXJ_actions + _RDXJ_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 10 "RDXJ.rl"
	{ mark0[RDXJInt] = p - text[0]; }
	break;
	case 1:
#line 11 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJInt];
    tok[1] = p;
    call(RDXJonInt, tok, state); 
}
	break;
	case 2:
#line 16 "RDXJ.rl"
	{ mark0[RDXJFloat] = p - text[0]; }
	break;
	case 3:
#line 17 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJFloat];
    tok[1] = p;
    call(RDXJonFloat, tok, state); 
}
	break;
	case 4:
#line 22 "RDXJ.rl"
	{ mark0[RDXJRef] = p - text[0]; }
	break;
	case 5:
#line 23 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJRef];
    tok[1] = p;
    call(RDXJonRef, tok, state); 
}
	break;
	case 6:
#line 28 "RDXJ.rl"
	{ mark0[RDXJString] = p - text[0]; }
	break;
	case 7:
#line 29 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJString];
    tok[1] = p;
    call(RDXJonString, tok, state); 
}
	break;
	case 8:
#line 34 "RDXJ.rl"
	{ mark0[RDXJTerm] = p - text[0]; }
	break;
	case 9:
#line 35 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJTerm];
    tok[1] = p;
    call(RDXJonTerm, tok, state); 
}
	break;
	case 10:
#line 40 "RDXJ.rl"
	{ mark0[RDXJOpenObject] = p - text[0]; }
	break;
	case 11:
#line 41 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJOpenObject];
    tok[1] = p;
    call(RDXJonOpenObject, tok, state); 
}
	break;
	case 12:
#line 46 "RDXJ.rl"
	{ mark0[RDXJCloseObject] = p - text[0]; }
	break;
	case 13:
#line 47 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJCloseObject];
    tok[1] = p;
    call(RDXJonCloseObject, tok, state); 
}
	break;
	case 14:
#line 52 "RDXJ.rl"
	{ mark0[RDXJOpenArray] = p - text[0]; }
	break;
	case 15:
#line 53 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJOpenArray];
    tok[1] = p;
    call(RDXJonOpenArray, tok, state); 
}
	break;
	case 16:
#line 58 "RDXJ.rl"
	{ mark0[RDXJCloseArray] = p - text[0]; }
	break;
	case 17:
#line 59 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJCloseArray];
    tok[1] = p;
    call(RDXJonCloseArray, tok, state); 
}
	break;
	case 18:
#line 64 "RDXJ.rl"
	{ mark0[RDXJOpenVector] = p - text[0]; }
	break;
	case 19:
#line 65 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJOpenVector];
    tok[1] = p;
    call(RDXJonOpenVector, tok, state); 
}
	break;
	case 20:
#line 70 "RDXJ.rl"
	{ mark0[RDXJCloseVector] = p - text[0]; }
	break;
	case 21:
#line 71 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJCloseVector];
    tok[1] = p;
    call(RDXJonCloseVector, tok, state); 
}
	break;
	case 22:
#line 76 "RDXJ.rl"
	{ mark0[RDXJStamp] = p - text[0]; }
	break;
	case 23:
#line 77 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJStamp];
    tok[1] = p;
    call(RDXJonStamp, tok, state); 
}
	break;
	case 24:
#line 82 "RDXJ.rl"
	{ mark0[RDXJComma] = p - text[0]; }
	break;
	case 25:
#line 83 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJComma];
    tok[1] = p;
    call(RDXJonComma, tok, state); 
}
	break;
	case 26:
#line 88 "RDXJ.rl"
	{ mark0[RDXJColon] = p - text[0]; }
	break;
	case 27:
#line 89 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJColon];
    tok[1] = p;
    call(RDXJonColon, tok, state); 
}
	break;
	case 28:
#line 94 "RDXJ.rl"
	{ mark0[RDXJFIRST] = p - text[0]; }
	break;
	case 29:
#line 95 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJFIRST];
    tok[1] = p;
    call(RDXJonFIRST, tok, state); 
}
	break;
	case 30:
#line 100 "RDXJ.rl"
	{ mark0[RDXJRoot] = p - text[0]; }
	break;
#line 622 "RDXJ.rl.c"
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
	const char *__acts = _RDXJ_actions + _RDXJ_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 1:
#line 11 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJInt];
    tok[1] = p;
    call(RDXJonInt, tok, state); 
}
	break;
	case 3:
#line 17 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJFloat];
    tok[1] = p;
    call(RDXJonFloat, tok, state); 
}
	break;
	case 5:
#line 23 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJRef];
    tok[1] = p;
    call(RDXJonRef, tok, state); 
}
	break;
	case 7:
#line 29 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJString];
    tok[1] = p;
    call(RDXJonString, tok, state); 
}
	break;
	case 9:
#line 35 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJTerm];
    tok[1] = p;
    call(RDXJonTerm, tok, state); 
}
	break;
	case 11:
#line 41 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJOpenObject];
    tok[1] = p;
    call(RDXJonOpenObject, tok, state); 
}
	break;
	case 13:
#line 47 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJCloseObject];
    tok[1] = p;
    call(RDXJonCloseObject, tok, state); 
}
	break;
	case 15:
#line 53 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJOpenArray];
    tok[1] = p;
    call(RDXJonOpenArray, tok, state); 
}
	break;
	case 17:
#line 59 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJCloseArray];
    tok[1] = p;
    call(RDXJonCloseArray, tok, state); 
}
	break;
	case 19:
#line 65 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJOpenVector];
    tok[1] = p;
    call(RDXJonOpenVector, tok, state); 
}
	break;
	case 21:
#line 71 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJCloseVector];
    tok[1] = p;
    call(RDXJonCloseVector, tok, state); 
}
	break;
	case 23:
#line 77 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJStamp];
    tok[1] = p;
    call(RDXJonStamp, tok, state); 
}
	break;
	case 25:
#line 83 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJComma];
    tok[1] = p;
    call(RDXJonComma, tok, state); 
}
	break;
	case 27:
#line 89 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJColon];
    tok[1] = p;
    call(RDXJonColon, tok, state); 
}
	break;
	case 29:
#line 95 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJFIRST];
    tok[1] = p;
    call(RDXJonFIRST, tok, state); 
}
	break;
	case 30:
#line 100 "RDXJ.rl"
	{ mark0[RDXJRoot] = p - text[0]; }
	break;
	case 31:
#line 101 "RDXJ.rl"
	{
    tok[0] = text[0] + mark0[RDXJRoot];
    tok[1] = p;
    call(RDXJonRoot, tok, state); 
}
	break;
#line 752 "RDXJ.rl.c"
		}
	}
	}

	_out: {}
	}

#line 200 "RDXJ.rl"

    if (p!=text[1] || cs < RDXJ_first_final) {
        state->text[0] = p;
        fail(RDXJfail);
    }
    done;
}
