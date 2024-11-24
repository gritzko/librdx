#include "SORT.h"

#include <unistd.h>

#include "PRO.h"
#include "abc/B.h"
#include "abc/TEST.h"

#define LEN1 1024

fuzz(u64, SORTfuzz) {
    sane(1);
    if ($len(input) > LEN1) input[1] = input[0] + LEN1;
    aBpad2(u64, ints, LEN1);
    aBpad2(u64, ints2, LEN1);
    aBpad2(u64, ints3, LEN1);
    $u64feedall(intsidle, input);
    $u64feedall(ints3idle, input);
    $sort(ints3data, u64cmp);
    call(SORTu64, ints2idle, intsdata);
    assert($len(ints2data) == $len(ints3data));
    for (u64 i = 0; i < LEN1; ++i) {
        assert($u64at(ints3data, i) == $u64at(ints2data, i));
    }
    done;
}
