//
// (c) Victor Grishchenko, 2020-2023
//

#ifndef ABC_LIST_H
#define ABC_LIST_H

#include "$.h"

con ok64 LISTnoroom = 0xc73cf6cf275c495;
con ok64 LISTnodata = 0x978968cf275c495;
con ok64 LISTend = 0x28ca975c495;
con ok64 LISTbadndx = 0xf28ca896675c495;

typedef struct {
    u32 prev, next;
} list64;

#define LISTat Bat

#define LISTfor(list, i) for (; i != 0 && _ == OK; i = Bat(list, i).next)

#define LISTnext(list, ndx) Bat(list, ndx)->_list.next

#endif
