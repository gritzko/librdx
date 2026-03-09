#ifndef LIBRDX_PASS_H
#define LIBRDX_PASS_H

#include "BASON.h"

#define PASS_KEY 1
#define PASS_VAL 2
#define PASS_MAX 8

typedef struct {
    u64bp  stk;
    u8csc  data;
    u8     type;   // 0 = exhausted
    u8     bits;   // PASS_KEY | PASS_VAL
    u8cs   key;
    u8cs   val;
} BASONcur;

typedef BASONcur* BASONcurp;
typedef ok64 (*BASONpassf)(voidp arg, BASONcurp inputs, u32 k);

ok64 BASONPass(BASONcurp inputs, u32 k, BASONpassf cb, voidp arg);

#endif
