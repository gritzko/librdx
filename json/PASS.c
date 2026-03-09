#include "PASS.h"

#include "abc/PRO.h"

ok64 BASONPass(BASONcurp inputs, u32 k, BASONpassf cb, voidp arg) {
    sane(inputs != NULL && cb != NULL && k <= PASS_MAX);

    // Initial drain on each input
    for (u32 i = 0; i < k; i++) {
        ok64 o = BASONDrain(inputs[i].stk, inputs[i].data,
                            &inputs[i].type, inputs[i].key,
                            inputs[i].val);
        if (o != OK) inputs[i].type = 0;
    }

    for (;;) {
        // Find min key across active inputs
        u32 min = UINT32_MAX;
        for (u32 i = 0; i < k; i++) {
            if (inputs[i].type == 0) continue;
            if (min == UINT32_MAX || $cmp(inputs[i].key, inputs[min].key) < 0)
                min = i;
        }
        if (min == UINT32_MAX) break;  // all exhausted

        // Set PASS_KEY on inputs with min key; find winner (last such)
        u32 winner = min;
        for (u32 i = 0; i < k; i++) {
            inputs[i].bits = 0;
            if (inputs[i].type == 0) continue;
            if ($cmp(inputs[i].key, inputs[min].key) == 0) {
                inputs[i].bits |= PASS_KEY;
                winner = i;
            }
        }

        // Set PASS_VAL on inputs whose value equals winner's
        for (u32 i = 0; i < k; i++) {
            if (!(inputs[i].bits & PASS_KEY)) continue;
            if (inputs[i].type == inputs[winner].type &&
                $len(inputs[i].val) == $len(inputs[winner].val) &&
                ($len(inputs[i].val) == 0 ||
                 memcmp(inputs[i].val[0], inputs[winner].val[0],
                        $len(inputs[i].val)) == 0))
                inputs[i].bits |= PASS_VAL;
        }

        // Callback
        call(cb, arg, inputs, k);

        // Advance all inputs that had PASS_KEY
        for (u32 i = 0; i < k; i++) {
            if (!(inputs[i].bits & PASS_KEY)) continue;
            ok64 o = BASONDrain(inputs[i].stk, inputs[i].data,
                                &inputs[i].type, inputs[i].key,
                                inputs[i].val);
            if (o != OK) inputs[i].type = 0;
        }
    }

    done;
}
