//
// (c) Victor Grishchenko, 2020-2023
//

#ifndef ABC_LIST_H
#define ABC_LIST_H

#include "S.h"

con ok64 LISTnoroom = 0x55271dcb3db3cf1;
con ok64 LISTNODATA = 0x55271d5d834a74a;
con ok64 LISTend = 0x1549c769ca8;
con ok64 LISTbadndx = 0x55271d9a5a32a3c;

typedef struct {
    u32 prev, next;
} list64;

#define LISTat Batp

#define LISTfor(list, i) for (; i != 0 && _ == OK; i = Batp(list, i).next)

#define LISTnext(list, ndx) Batp(list, ndx)->_list.next

#endif
