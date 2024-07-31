
#include "$.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "TEST.h"
#include "INT.h"

pro($test1) {
    aBpad(i32, pad, 4);
    i32 a1 = 1;
    i32 a2 = 0;
    i32$ into = Bi32idle(pad);
    i32c$ data = Bi32cdata(pad);

    call($i32feed1, into, a2);
    call($i32feedp, into, &a1);
    sane($len(data) == 2);
    call($i32feed, into, data);
    sane($len(data) == 4);
    sane($at(data, 0) == 0);
    sane($at(data, 1) == 1);
    sane($at(data, 2) == 0);
    sane($at(data, 3) == 1);
    sane($noroom == $i32feed1(into, a1));

    aBpad(i32, padi2, 4);
    i32$ into2 = Bi32idle(padi2);
    $i32copy(into2, data);  // FIXME
    sane($eq(into2, data));

    i32swap(&a1, &a2);
    sane(a1 == 0 && a2 == 1);
    i32mv(&a1, &a2);
    sane(a1 == a2 && a1 == 1);
    done;
}

pro($test2) {
    aBpad(i32, pad, 8);
    i32$ into = Bi32idle(pad);
    i32$ data = Bi32data(pad);
    call($i32feed1, into, 4);
    call($i32feed1, into, 7);
    call($i32feed1, into, 2);
    call($i32feed1, into, 5);
    call($i32feed1, into, 0);
    call($i32feed1, into, 1);
    call($i32feed1, into, 3);
    call($i32feed1, into, 6);

    $i32sort(data);

    a$dup(i32, d, data);
    int j = 0;
    $eat(d) sane(**d == j++);

    for (i32 i = 0; i < $len(data); i++) {
        sane($at(data, i) == i);
        i32* p = $i32bsearch(&i, data);
        sane(p - *data == i);
    }

    done;
}

pro($test) {
    call($test1);
    call($test2);
    done;
}

TEST($test);
