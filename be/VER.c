#include "VER.h"

#include "abc/PRO.h"

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
            // Scan for operator separator: '-', '+', '='
            u8cp sep = start;
            while (sep < p && *sep != '-' && *sep != '+' && *sep != '=') sep++;
            if (sep < p && sep > start) {
                // time<op>origin
                u8cs ts = {start, sep};
                u8cs orig = {sep + 1, p};
                u8 op = VER_ANY;
                if (*sep == '-') op = VER_LE;
                else if (*sep == '+') op = VER_GT;
                else if (*sep == '=') op = VER_EQ;
                ron60 time_val = 0;
                ron60 orig_val = 0;
                ok64 o = RONutf8sDrain(&time_val, ts);
                if (o == OK) o = RONutf8sDrain(&orig_val, orig);
                if (o == OK) {
                    *into[0] = VERMake(time_val, orig_val, op);
                    into[0]++;
                }
            } else {
                // Just origin, VER_ANY (match all waypoints)
                u8cs entry = {start, p};
                ron60 orig_val = 0;
                ok64 o = RONutf8sDrain(&orig_val, entry);
                if (o == OK) {
                    *into[0] = VERMake(0, orig_val, VER_ANY);
                    into[0]++;
                }
            }
        }
        if (p < end) p++;  // skip '&'
    }
    // Append base entry (0,0) if any entry is ANY or LE
    if (into[0] < into[1]) {
        for (ron120cp e = start_pos; e < into[0]; e++) {
            u8 op = VEROp(e);
            if (op == VER_ANY || op == VER_LE) {
                *into[0] = VERMake(0, 0, VER_ANY);
                into[0]++;
                break;
            }
        }
    }
    done;
}

ok64 VERFormFromBranches(ron120s into, int branchc, u8cs *branches) {
    sane(into != NULL);
    for (int i = 0; i < branchc && into[0] < into[1]; i++) {
        ron60 orig = 0;
        ok64 o = RONutf8sDrain(&orig, branches[i]);
        if (o == OK) {
            *into[0] = VERMake(0, orig, VER_ANY);
            into[0]++;
        }
    }
    // All-ANY formula always includes base
    if (branchc > 0 && into[0] < into[1]) {
        *into[0] = VERMake(0, 0, VER_ANY);
        into[0]++;
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
