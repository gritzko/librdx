#include "ZINT.h"

#include "01.h"

ok64 ZINTu8sFeedBlocked(u8s into, u64cs ints) {
    if (!into || !ints) return BADARG;
    ok64 o = OK;
    u32 l = 0;
    u8p bh = 0;
    while (o == OK && !u64csEmpty(ints)) {
        if (l == 0) {
            bh = *into;
            if (!u8sLen(into)) return NOROOM;
            ++*into;
            *bh = 0;
        }
        if (**ints <= 0xff) {
            o = u8sFeed8(into, (u8*)*ints);
            *bh |= (0 << l);
        } else if (**ints <= 0xffff) {
            o = u8sFeed16(into, (u16*)*ints);
            *bh |= (1 << l);
        } else if (**ints <= 0xffffffff) {
            o = u8sFeed32(into, (u32*)*ints);
            *bh |= (2 << l);
        } else {
            o = u8sFeed64(into, (u64*)*ints);
            *bh |= (3 << l);
        }
        ++*ints;
        l = (l + 2) & 7;
    }
    return o;
}

ok64 ZINTu8sDrainBlocked(u8cs from, u64s ints) {
    if (!from || !ints) return BADARG;
    ok64 o = OK;
    u32 l = 0;
    u8 bh = 0;
    while (o == OK && !u8csEmpty(from)) {
        if ((l & 3) == 0) {
            bh = **from;
            ++*from;
        }
        u64 u = 0;
        switch (bh & 3) {
            case 0:
                o = u8sDrain8(from, (u8*)&u);
                break;
            case 1:
                o = u8sDrain16(from, (u16*)&u);
                break;
            case 2:
                o = u8sDrain32(from, (u32*)&u);
                break;
            case 3:
                o = u8sDrain64(from, (u64*)&u);
                break;
        }
        if (o == OK) o = u64sFeed1(ints, u);
        bh >>= 2;
        ++l;
    }
    return o;
}

ok64 ZINTu64sDelta(u64s ints, u64 start) {
    u64 c = start;
    $for(u64, i, ints) {
        if (*i < c) return ZINTBAD;
        u64 p = *i;
        *i -= c;
        c = p;
    }
    return OK;
}

ok64 ZINTu64sUndelta(u64s ints, u64 start) {
    u64 c = start;
    $for(u64, i, ints) {
        u64 s = *i + c;
        *i = s;
        c = s;
    }
    return OK;
}

ok64 ZINTu8sFeed128(u8s into, u64 big, u64 lil) {
    if (lil <= B1) {
        if (big <= B1) {
            if (big != 0 || lil != 0) u8sFeed8(into, (u8*)&big);
        } else if (big <= B2) {
            u8sFeed16(into, (u16*)&big);
        } else if (big <= B4) {
            u8sFeed32(into, (u32*)&big);
        } else {
            u8sFeed64(into, &big);
        }
        if (lil != 0 || big > B1) u8sFeed8(into, (u8*)&lil);
    } else if (lil <= B2) {
        if (big <= B2) {
            u8sFeed16(into, (u16*)&big);
        } else if (big <= B4) {
            u8sFeed32(into, (u32*)&big);
        } else {
            u8sFeed64(into, &big);
        }
        u8sFeed16(into, (u16*)&lil);
    } else if (lil <= B4) {
        if (big <= B4) {
            u8sFeed32(into, (u32*)&big);
        } else {
            u8sFeed64(into, &big);
        }
        u8sFeed32(into, (u32*)&lil);
    } else {
        u8sFeed64(into, &big);
        u8sFeed64(into, &lil);
    }
    return OK;
}
ok64 ZINTu8sDrain128(u8cs from, u64* big, u64* lil) {
    u32 len = $len(from);
    *big = *lil = 0;
    switch (len) {
        case 0:
            break;
        case 1:
            u8sDrain8(from, (u8*)big);
            break;
        case 2:
            u8sDrain8(from, (u8*)big);
            u8sDrain8(from, (u8*)lil);
            break;
        case 3:
            u8sDrain16(from, (u16*)big);
            u8sDrain8(from, (u8*)lil);
            break;
        case 4:
            u8sDrain16(from, (u16*)big);
            u8sDrain16(from, (u16*)lil);
            break;
        case 5:
            u8sDrain32(from, (u32*)big);
            u8sDrain8(from, (u8*)lil);
            break;
        case 6:
            u8sDrain32(from, (u32*)big);
            u8sDrain16(from, (u16*)lil);
            break;
        case 7:
            return ZINTBAD;
        case 8:
            u8sDrain32(from, (u32*)big);
            u8sDrain32(from, (u32*)lil);
            break;
        case 9:
            u8sDrain64(from, (u64*)big);
            u8sDrain8(from, (u8*)lil);
            break;
        case 10:
            u8sDrain64(from, (u64*)big);
            u8sDrain16(from, (u16*)lil);
            break;
        case 11:
            return ZINTBAD;
        case 12:
            u8sDrain64(from, (u64*)big);
            u8sDrain32(from, (u32*)lil);
            break;
        case 13:
            return ZINTBAD;
        case 14:
            return ZINTBAD;
        case 15:
            return ZINTBAD;
        case 16:
            u8sDrain64(from, (u64*)big);
            u8sDrain64(from, (u64*)lil);
            break;
        default:
            return ZINTBAD;
    }
    // Verify canonical encoding (no overlong)
    if (ZINT128len(*big, *lil) != len) return ZINTBAD;
    return OK;
}
