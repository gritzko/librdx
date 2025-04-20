#include "OK.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "INT.h"

int main(int argn, char** args) {
    for (int i = 1; i < argn; i++) {
        $u8c str = $u8str(args[i]);
        //$reverse(str);
        u64 num = 0;
        OKscan(&num, str);
        printf("static const ok64 %s\t= 0x%lx;\n", args[i], num);
    }
    return 0;
}
