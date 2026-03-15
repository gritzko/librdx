#include "VER.h"

#include "abc/PRO.h"

ok64 VERParse(ron120p into, u8cs text) {
    sane(into != NULL && $ok(text) && !$empty(text));
    // Scan for operator separator: '-', '+', '='
    u8cp sep = text[0];
    while (sep < text[1] && *sep != '-' && *sep != '+' && *sep != '=') sep++;
    if (sep < text[1] && sep > text[0]) {
        // time<op>origin
        u8cs ts = {text[0], sep};
        u8cs orig = {sep + 1, text[1]};
        u8 op = VER_ANY;
        if (*sep == '-') op = VER_LE;
        else if (*sep == '+') op = VER_GT;
        else if (*sep == '=') op = VER_EQ;
        ron60 time_val = 0;
        ron60 orig_val = 0;
        call(RONutf8sDrain, &time_val, ts);
        call(RONutf8sDrain, &orig_val, orig);
        *into = VERMake(time_val, orig_val, op);
    } else {
        // Just origin, VER_ANY
        ron60 orig_val = 0;
        call(RONutf8sDrain, &orig_val, text);
        *into = VERMake(0, orig_val, VER_ANY);
    }
    done;
}

ok64 VERFormParse(ron120s into, u8cs query) {
    sane(into != NULL);
    if (!$ok(query) || $empty(query)) done;
    ron120p start_pos = into[0];
    u8cp p = query[0];
    u8cp end = query[1];
    while (p < end && into[0] < into[1]) {
        u8cp start = p;
        while (p < end && *p != '&') p++;
        if (p > start) {
            u8cs entry = {start, p};
            ron120 ver = {};
            ok64 o = VERParse(&ver, entry);
            if (o == OK) u128sFeed1(into, ver);
        }
        if (p < end) p++;  // skip '&'
    }
    // Append base entry (0,0) if any entry is ANY or LE
    for (ron120cp e = start_pos; e < into[0]; e++) {
        u8 op = VEROp(e);
        if (op == VER_ANY || op == VER_LE) {
            u128sFeed1(into, VERMake(0, 0, VER_ANY));
            break;
        }
    }
    done;
}

ok64 VERFormFromBranches(ron120s into, int branchc, ron120cp branches) {
    sane(into != NULL);
    for (int i = 0; i < branchc; i++) {
        call(u128sFeed1, into, branches[i]);
    }
    // Always includes base
    if (branchc > 0) {
        call(u128sFeed1, into, VERMake(0, 0, VER_ANY));
    }
    done;
}

b8 VERFormMatch(ron120cs form, ron60 time, ron60 origin) {
    for (ron120cp p = form[0]; p < form[1]; p++) {
        if (VEROrigin(p) != origin) continue;
        u8 op = VEROp(p);
        if (op == VER_ANY) return YES;
        ron60 e_time = VERTime(p);
        if (op == VER_LE && time <= e_time) return YES;
        if (op == VER_GT && time > e_time) return YES;
        if (op == VER_EQ && time == e_time) return YES;
    }
    return NO;
}

ok64 VERutf8Feed(u8s into, ron120cp v) {
    sane($ok(into) && v != NULL);
    ron60 time = VERTime(v);
    ron60 origin = VEROrigin(v);
    u8 op = VEROp(v);
    if (time != 0) {
        call(RONutf8sFeed, into, time);
        u8 sep = 0;
        if (op == VER_LE) sep = '-';
        else if (op == VER_GT) sep = '+';
        else if (op == VER_EQ) sep = '=';
        else sep = '-';  // default separator for points
        u8sFeed1(into, sep);
    }
    call(RONutf8sFeed, into, origin);
    done;
}
