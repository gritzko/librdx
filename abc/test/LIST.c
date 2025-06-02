//
// Created by gritzko on 5/11/24.
//
#include "LIST.h"

#include <assert.h>
#include <stdio.h>

#include "INT.h"
#include "PRO.h"
#include "TEST.h"

typedef struct {
    ok64 value;
    list64 _list;
} entry128;

fun z32 entry128z(entry128 const *a, entry128 const *b) {
    return u64z(&a->value, &b->value);
}

#define X(M, name) M##entry128##name
#include "LISTx.h"
#undef X

ok64 LISTtest1() {
    sane(1);
    aBpad(entry128, list, 1024);
    entry128 codes[3] = {
        {.value = LISTnoroom}, {.value = LISTnodata}, {.value = LISTbadndx}};
    call(LISTentry128insert, list, codes + 0, 0);
    call(LISTentry128insert, list, codes + 2, 0);
    call(LISTentry128insert, list, codes + 1, 0);
    u32 i = 0;
    testeq(LISTentry128atp(list, i)->value, codes[0].value);
    i = LISTentry128next(list, i);
    testeq(LISTentry128atp(list, i)->value, codes[1].value);
    i = LISTentry128next(list, i);
    testeq(LISTentry128atp(list, i)->value, codes[2].value);
    done;
}

ok64 LISTtest() {
    sane(1);
    call(LISTtest1);
    done;
}

TEST(LISTtest);
