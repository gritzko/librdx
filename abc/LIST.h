//
// (c) Victor Grishchenko, 2020-2023
//

#ifndef ABC_LIST_H
#define ABC_LIST_H

#include "S.h"

con ok64 LISTNOROOM = 0x55271d5d86d8616;
con ok64 LISTNODATA = 0x55271d5d834a74a;
con ok64 LISTEND = 0x1549c74e5cd;
con ok64 LISTBADNDX = 0x55271d2ca357361;

typedef struct {
    u32 prev, next;
} list64;

#define LISTat Batp

#define LISTfor(list, i) for (; i != 0 && _ == OK; i = Batp(list, i).next)

#define LISTnext(list, ndx) Batp(list, ndx)->_list.next

#endif
