#include "SORT.h"

#include <unistd.h>

#include "PRO.h"
#include "abc/B.h"
#include "abc/TEST.h"

#define LEN1 1024

FUZZ(u64, SORTfuzz) {
    sane(1);
    if ($len(input) > LEN1) input[1] = input[0] + LEN1;
    aBpad2(u64, ints, LEN1);
    aBpad2(u64, ints2, LEN1);
    aBpad2(u64, ints3, LEN1);
    $u64feedall(intsidle, input);
    $u64feedall(ints3idle, input);
    $sort(ints3data, u64cmp);
    call(SORTu64, ints2idle, intsdata);
    must($len(ints2data) == $len(ints3data), "length mismatch");
    size_t len = $len(ints2data);
    for (size_t i = 0; i < len; ++i) {
        must(u64sAt(ints3data, i) == u64sAt(ints2data, i), "element mismatch");
    }
    done;
}
