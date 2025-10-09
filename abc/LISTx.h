#include "Bx.h"

#define T X(, )

fun T* X(LIST, atp)(X(B, ) list, u32 ndx) { return X(B, atp)(list, ndx); }

fun u32 X(LIST, next)(X(B, ) list, u32 ndx) {
    return X(B, atp)(list, ndx)->_list.next;
}

fun ok64 X(LIST, insert)(X(B, ) list, T const* entry, u32 prev) {
    T** data = X(B, data)(list);
    size_t len = $len(data);
    if (len < prev) return LISTbadndx;
    ok64 o = X(, BFeedP)(list, entry);
    if (o != OK) return o;
    u32 next = X(B, atp)(list, prev)->_list.next;
    X(B, atp)(list, len)->_list.next = next;
    X(B, atp)(list, len)->_list.prev = prev;
    X(B, atp)(list, prev)->_list.next = len;
    X(B, atp)(list, next)->_list.prev = len;
    return OK;
}
