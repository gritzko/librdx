
#include <stdint.h>
#include <unistd.h>

#include "FILE.h"
#include "LINE.h"
#include "TEST.h"

pro(LINEtest1) {
#define LC_1 3
  line lines[LC_1] = {
      {.id = {0, 0}, .body = $u8str("Hello world"), .type = 'A'},
      {.id = {1, 2}, .body = $u8str("Nice to meet you"), .type = 'B'},
      {.id = {UINT64_MAX, UINT64_MAX},
       .body = $u8str("Enough for this test"),
       .type = 'C'},
  };

  for (int i = 0; i < LC_1; i++) {
    aBpad(u8, pad, 1024);
    line k = {.rest = Bu8cidle(pad)};
    call(LINEfeed, &k, lines + i);
    line l = {.rest = Bu8cdata(pad)};
    call(LINEdrain, &l);
    sane(0 == linecmp(lines + i, &l));
  }

  done;
}

pro(LINEtest2) {
#define LC_2 4
  line lines[LC_2] = {
      {.id = {0, 0}, .body = $u8str("Hello world"), .type = 'A'},
      {.id = {1, 2}, .body = $u8str("Nice to meet you"), .type = 'B'},
      {.id = {UINT64_MAX - 1, UINT64_MAX},
       .body = $u8str("Yet another line"),
       .type = 'C'},
      {.id = {UINT64_MAX, UINT64_MAX},
       .body = $u8str("Enough for this test"),
       .type = 'D'},
  };

  aBpad(u8, pad, 4096);
  aLINE(sit, Bu8cidle(pad));

  for (int i = 0; i < LC_2; i++) {
    call(LINEfeed, &sit, lines + i);
  }

  aLINE(cit, Bu8cdata(pad));

  for (int i = 0; i < LC_2; i++) {
    line *correct = lines + i;
    call(LINEdrain, &cit);
    sane(0 == linecmp(correct, &cit));
  }

  done;
}

// FIXME hi?
int srccmp(linec *a, linec *b) { return u64cmp(&a->id._64[1], &b->id._64[1]); }
fun ok64 srcmerge(line *into, $cline from) {
  line *max = from[0];
  for (line *p = from[0] + 1; p < from[1]; p++) {
    if (p->id._64[0] > max->id._64[0])
      max = p;
  }
  return LINEfeed(into, max);
}

pro(LINEtest3) {
#define LC_31 4
  line lines1[LC_31] = {
      {.id = {1, 0}, .body = $u8str("Hello world!"), .type = 'A'},
      {.id = {1, 1}, .body = $u8str("Nice to meet you"), .type = 'B'},
      {.id = {3, 2}, .body = $u8str("Yet another line!"), .type = 'C'},
      {.id = {UINT64_MAX - 1, 3},
       .body = $u8str("Enough for this test"),
       .type = 'D'},
  };
#define LC_32 4
  line lines2[LC_32] = {
      {.id = {0, 0}, .body = $u8str("Hello world"), .type = 'A'},
      {.id = {2, 1}, .body = $u8str("Nice to meet you!"), .type = 'B'},
      {.id = {2, 2}, .body = $u8str("Yet another line"), .type = 'C'},
      {.id = {UINT64_MAX, 3},
       .body = $u8str("Enough for this test!"),
       .type = 'D'},
  };

  aBpad(u8, arg1, 4096);
  aLINE(sit1, Bu8cidle(arg1));
  for (int i = 0; i < LC_31; i++) {
    call(LINEfeed, &sit1, lines1 + i);
  }
  aBpad(u8, arg2, 4096);
  aLINE(sit2, Bu8cidle(arg2));
  for (int i = 0; i < LC_32; i++) {
    call(LINEfeed, &sit2, lines2 + i);
  }

  aBpad(u8, pad, 4096);
  aLINE(sit, Bu8cidle(pad));
  aBpad(line, fpad, 2);
  call(LINEpush, fpad, Bu8cdata(arg1), srccmp);
  call(LINEpush, fpad, Bu8cdata(arg2), srccmp);
  line$ from = Blinedata(fpad);

  while (!$empty(from)) {
    call(LINEmerge, &sit, from, srccmp, srcmerge);
  }
  $println(Bu8cdata(pad));

  int excl = 0;
  a$dup(u8c, dup, Bu8cdata(pad));
  $eat(dup) if (**dup == '!') excl++;
  sane(4 == excl);

  done;
}

pro(LINEtest) {
  call(LINEtest1);
  call(LINEtest2);
  call(LINEtest3);
  done;
}

TEST(LINEtest);
