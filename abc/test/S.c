#include "S.h"

#include <assert.h>
#include <unistd.h>

#include "INT.h"
#include "TEST.h"

ok64 $test1() {
    sane(1);
    a_pad(i32, pad, 4);
    i32 a1 = 1;
    i32 a2 = 0;
    i32$ into = i32bIdle(pad);
    i32c$ data = i32bDataC(pad);

    call(i32sFeed1, into, a2);
    call(i32sFeedP, into, &a1);
    want($len(data) == 2);
    call($i32feed, into, data);
    want($len(data) == 4);
    want($at(data, 0) == 0);
    want($at(data, 1) == 1);
    want($at(data, 2) == 0);
    want($at(data, 3) == 1);
    want(SNOROOM == i32sFeed1(into, a1));

    aBpad(i32, padi2, 4);
    i32$ into2 = i32bIdle(padi2);
    i32sCopy(into2, data);  // FIXME
    want($eq(into2, data));

    i32Swap(&a1, &a2);
    want(a1 == 0 && a2 == 1);
    i32mv(&a1, &a2);
    want(a1 == a2 && a1 == 1);
    done;
}

ok64 $test2() {
    sane(1);
    aBpad(i32, pad, 8);
    i32$ into = i32bIdle(pad);
    i32$ data = i32bData(pad);
    call(i32sFeed1, into, 4);
    call(i32sFeed1, into, 7);
    call(i32sFeed1, into, 2);
    call(i32sFeed1, into, 5);
    call(i32sFeed1, into, 0);
    call(i32sFeed1, into, 1);
    call(i32sFeed1, into, 3);
    call(i32sFeed1, into, 6);

    $i32sort(data);

    a_dup(i32, d, data);
    int j = 0;
    $eat(d) want(**d == j++);

    for (i32 i = 0; i < $len(data); i++) {
        want($at(data, i) == i);
        i32* p = $i32bsearch(&i, (i32c$)data);
        want(p - *data == i);
    }

    done;
}

ok64 findtest() {
    sane(1);
    u8 data[] = "hello:world";
    u8cs slice = {data, data + 11};

    // Find existing character
    u8cs s1 = {slice[0], slice[1]};
    want(u8csFind(s1, ':') == OK);
    want(*s1 == data + 5);
    want(**s1 == ':');

    // Find first character
    u8cs s2 = {slice[0], slice[1]};
    want(u8csFind(s2, 'h') == OK);
    want(*s2 == data);

    // Find last character
    u8cs s3 = {slice[0], slice[1]};
    want(u8csFind(s3, 'd') == OK);
    want(*s3 == data + 10);

    // Character not found
    u8cs s4 = {slice[0], slice[1]};
    want(u8csFind(s4, 'z') == NONE);

    // Test sFind (mutable slice)
    u8s mslice = {data, data + 11};
    want(u8sFind(mslice, ':') == OK);
    want(*mslice == data + 5);

    // Test repeated search
    u8cs s5 = {slice[0], slice[1]};
    int count = 0;
    while (u8csFind(s5, 'l') == OK) {
        count++;
        ++*s5;
    }
    want(count == 3);  // 'l' appears 3 times in "hello:world"

    done;
}

ok64 findStest() {
    sane(1);
    u8 data[] = "hello world, hello universe";
    u8cs haystack = {data, data + 27};

    // Find "world"
    u8 needle1[] = "world";
    u8cs n1 = {needle1, needle1 + 5};
    u8cs h1 = {haystack[0], haystack[1]};
    want(u8csFindS(h1, n1) == OK);
    want(*h1 == data + 6);

    // Find "hello" (first occurrence)
    u8 needle2[] = "hello";
    u8cs n2 = {needle2, needle2 + 5};
    u8cs h2 = {haystack[0], haystack[1]};
    want(u8csFindS(h2, n2) == OK);
    want(*h2 == data);

    // Find "universe" (at end)
    u8 needle3[] = "universe";
    u8cs n3 = {needle3, needle3 + 8};
    u8cs h3 = {haystack[0], haystack[1]};
    want(u8csFindS(h3, n3) == OK);
    want(*h3 == data + 19);

    // Not found
    u8 needle4[] = "foo";
    u8cs n4 = {needle4, needle4 + 3};
    u8cs h4 = {haystack[0], haystack[1]};
    want(u8csFindS(h4, n4) == NONE);

    // Needle longer than haystack
    u8 short_data[] = "hi";
    u8cs short_hay = {short_data, short_data + 2};
    u8 long_needle[] = "hello";
    u8cs ln = {long_needle, long_needle + 5};
    u8cs h5 = {short_hay[0], short_hay[1]};
    want(u8csFindS(h5, ln) == NONE);

    // Empty needle
    u8cs empty = {needle1, needle1};
    u8cs h6 = {haystack[0], haystack[1]};
    want(u8csFindS(h6, empty) == NONE);

    // Find with repeated first char: "llo" in "hello world, hello universe"
    u8 needle5[] = "llo";
    u8cs n5 = {needle5, needle5 + 3};
    u8cs h7 = {haystack[0], haystack[1]};
    want(u8csFindS(h7, n5) == OK);
    want(*h7 == data + 2);  // "llo" starts at index 2

    // Test skipping: "ld" in "hello world" - 'l' at 2,3,9 but "ld" at 9
    u8 data2[] = "hello world";
    u8cs hay2 = {data2, data2 + 11};
    u8 needle6[] = "ld";
    u8cs n6 = {needle6, needle6 + 2};
    u8cs h8 = {hay2[0], hay2[1]};
    want(u8csFindS(h8, n6) == OK);
    want(*h8 == data2 + 9);  // "ld" at index 9

    // Test repeated search: find all "hello" occurrences
    u8cs h9 = {haystack[0], haystack[1]};
    int count = 0;
    while (u8csFindS(h9, n2) == OK) {
        count++;
        ++*h9;
    }
    want(count == 2);  // "hello" appears twice

    done;
}

ok64 $test() {
    sane(1);
    call($test1);
    call($test2);
    call(findtest);
    call(findStest);
    done;
}

TEST($test);
