#include "abc/01.h"
#include "abc/NUM.h"
#include "abc/SLOG.h"
#include "abc/TEST.h"
#include "abc/TLV.h"
#include "rdx/RDX.h"
#include "rdx/test/NUMP.h"

// Test: simple FIRST values in EULER container
ok64 SLIK0() {
    sane(1);
    a_pad(u8, pad, PAGESIZE * 4);
    a_pad0(u64, tabs, PAGESIZE);

    // Write: EULER { INT(0), INT(42), INT(100) }
    rdx e;
    rdxWriteInitSLIK(&e, pad, tabs);
    e.type = RDX_TYPE_EULER;
    //fprintf(stderr, "  write EULER\n");
    call(rdxWriteNextSLIK, &e);

    rdx c = {};
    //fprintf(stderr, "  write into\n");
    call(rdxWriteIntoSLIK, &c, &e);
    i64 vals[] = {0, 42, 100};
    for (size_t i = 0; i < 3; i++) {
        c.type = RDX_TYPE_INT;
        c.i = vals[i];
        c.id.seq = i;
        //fprintf(stderr, "  write INT %ld\n", vals[i]);
        call(rdxWriteNextSLIK, &c);
    }
    //fprintf(stderr, "  write outo\n");
    call(rdxWriteOutoSLIK, &c, &e);
    //fprintf(stderr, "  write finish\n");
    call(rdxWriteFinishSLIK, &e);
    //fprintf(stderr, "  written %zu bytes\n", u8bDataLen(pad));
    //fprintf(stderr, "  hex: ");
    for (size_t i = 0; i < u8bDataLen(pad); i++) {
        //fprintf(stderr, "%02x ", pad[1][i]);
    }
    //fprintf(stderr, "\n");

    // Read back
    a_pad0(u64, readstack, 256);
    rdx r;
    //fprintf(stderr, "  init read\n");
    rdxInitSLIK(&r, pad, readstack);
    //fprintf(stderr, "  after init: stack data=%zu past=%zu\n",
    // u64bDataLen(readstack), u64bPastLen(readstack));
    for (size_t k = 0; k < u64bDataLen(readstack); k++) {
        //fprintf(stderr, "    [%zu] = %lu\n", k, readstack[1][k]);
    }
    //fprintf(stderr, "  read next\n");
    call(rdxNextSLIK, &r);
    //fprintf(stderr, "  got type %d\n", r.type);
    //fprintf(stderr, "  after next: stack data=%zu\n", u64bDataLen(readstack));
    for (size_t k = 0; k < u64bDataLen(readstack); k++) {
        //fprintf(stderr, "    [%zu] = %lu\n", k, readstack[1][k]);
    }
    test(r.type == RDX_TYPE_EULER, RDXBAD);

    rdx rc = {};
    //fprintf(stderr, "  read into\n");
    //fprintf(stderr, "  plexc: %zu bytes\n", (size_t)(r.plexc[1] - r.plexc[0]));
    call(rdxIntoSLIK, &rc, &r);
    //fprintf(stderr, "  after into: stack data=%zu past=%zu\n",
    //        u64bDataLen(readstack), u64bPastLen(readstack));
    for (size_t k = 0; k < u64bDataLen(readstack); k++) {
        //fprintf(stderr, "    [%zu] = %lu\n", k, readstack[1][k]);
    }
    //fprintf(stderr, "  rc.next offset=%zu\n", (size_t)(rc.next - pad[1]));
    for (size_t i = 0; i < 3; i++) {
        //fprintf(stderr, "  read next child %zu\n", i);
        ok64 nxt = rdxNextSLIK(&rc);
        //fprintf(stderr, "  result=%lx\n", nxt);
        if (nxt != OK) fail(nxt);
        test(rc.type == RDX_TYPE_INT, RDXBAD);
        testeq(rc.i, vals[i]);
    }
    test(rdxNextSLIK(&rc) == END, RDXBAD);

    done;
}

// Test: STRING values
ok64 SLIK1() {
    sane(1);
    a_pad(u8, pad, PAGESIZE * 4);
    a_pad0(u64, tabs, PAGESIZE);

    // Write: LINEAR { "hello", "world" }
    rdx e;
    rdxWriteInitSLIK(&e, pad, tabs);
    e.type = RDX_TYPE_LINEAR;
    call(rdxWriteNextSLIK, &e);

    rdx c = {};
    call(rdxWriteIntoSLIK, &c, &e);
    const char* strs[] = {"hello", "world"};
    for (size_t i = 0; i < 2; i++) {
        c.type = RDX_TYPE_STRING;
        c.s[0] = (u8c*)strs[i];
        c.s[1] = c.s[0] + strlen(strs[i]);
        c.flags = RDX_UTF_ENC_UTF8;
        c.id.seq = i;
        call(rdxWriteNextSLIK, &c);
    }
    call(rdxWriteOutoSLIK, &c, &e);
    call(rdxWriteFinishSLIK, &e);

    // Read back
    a_pad0(u64, readstack, 256);
    rdx r;
    rdxInitSLIK(&r, pad, readstack);
    call(rdxNextSLIK, &r);
    test(r.type == RDX_TYPE_LINEAR, RDXBAD);

    rdx rc = {};
    call(rdxIntoSLIK, &rc, &r);
    for (size_t i = 0; i < 2; i++) {
        call(rdxNextSLIK, &rc);
        test(rc.type == RDX_TYPE_STRING, RDXBAD);
        testeq($len(rc.s), strlen(strs[i]));
    }
    test(rdxNextSLIK(&rc) == END, RDXBAD);

    done;
}

// Test: nested containers (EULER of TUPLEs)
ok64 SLIK2() {
    sane(1);
    a_pad(u8, pad, PAGESIZE * 4);
    a_pad0(u64, tabs, PAGESIZE);

    //fprintf(stderr, "SLIK2: Write EULER with 3 TUPLEs\n");
    // Write: EULER { TUPLE{0,"a"}, TUPLE{1,"b"}, TUPLE{2,"c"} }
    rdx e;
    rdxWriteInitSLIK(&e, pad, tabs);
    e.type = RDX_TYPE_EULER;
    call(rdxWriteNextSLIK, &e);

    rdx t = {};
    call(rdxWriteIntoSLIK, &t, &e);
    for (int j = 0; j < 3; j++) {
        t.type = RDX_TYPE_TUPLE;
        t.id.seq = j;
        call(rdxWriteNextSLIK, &t);

        rdx inner = {};
        call(rdxWriteIntoSLIK, &inner, &t);

        // key: INT
        inner.type = RDX_TYPE_INT;
        inner.i = j;
        inner.id.seq = 0;
        call(rdxWriteNextSLIK, &inner);

        // value: STRING
        inner.type = RDX_TYPE_STRING;
        char buf[4] = {'a' + j, 0};
        inner.s[0] = (u8c*)buf;
        inner.s[1] = inner.s[0] + 1;
        inner.flags = RDX_UTF_ENC_UTF8;
        inner.id.seq = 1;
        call(rdxWriteNextSLIK, &inner);

        call(rdxWriteOutoSLIK, &inner, &t);
    }
    call(rdxWriteOutoSLIK, &t, &e);
    call(rdxWriteFinishSLIK, &e);

    //fprintf(stderr, "SLIK2: written %zu bytes\n", u8bDataLen(pad));

    // Read back
    a_pad0(u64, readstack, 256);
    rdx r;
    rdxInitSLIK(&r, pad, readstack);
    //fprintf(stderr, "SLIK2: init done, stack len=%zu\n", u64bDataLen(readstack));
    call(rdxNextSLIK, &r);
    //fprintf(stderr, "SLIK2: got EULER type=%d\n", r.type);
    test(r.type == RDX_TYPE_EULER, RDXBAD);

    //fprintf(stderr, "SLIK2: before into EULER, stack: data=%zu past=%zu\n",
            //u64bDataLen(readstack), u64bPastLen(readstack));
    //for (size_t k = 0; k < u64bDataLen(readstack); k++) {
        //fprintf(stderr, "  [%zu] = %lu\n", k, readstack[1][k]);
    //}
    rdx rc = {};
    call(rdxIntoSLIK, &rc, &r);
    //fprintf(stderr, "SLIK2: after into EULER, stack: data=%zu past=%zu\n",
    //        u64bDataLen(readstack), u64bPastLen(readstack));
    for (size_t k = 0; k < u64bDataLen(readstack); k++) {
        //fprintf(stderr, "  [%zu] = %lu\n", k, readstack[1][k]);
    }
    for (int j = 0; j < 3; j++) {
        //fprintf(stderr, "SLIK2: reading TUPLE %d\n", j);
        call(rdxNextSLIK, &rc);
        //fprintf(stderr, "SLIK2: got type=%d, stack: data=%zu\n", rc.type, u64bDataLen(readstack));
        for (size_t k = 0; k < u64bDataLen(readstack); k++) {
            //fprintf(stderr, "  [%zu] = %lu\n", k, readstack[1][k]);
        }
        test(rc.type == RDX_TYPE_TUPLE, RDXBAD);

        rdx inner = {};
        //fprintf(stderr, "SLIK2: into TUPLE %d\n", j);
        call(rdxIntoSLIK, &inner, &rc);
        //fprintf(stderr, "SLIK2: into done, stack: data=%zu past=%zu\n",
        //        u64bDataLen(readstack), u64bPastLen(readstack));
        for (size_t k = 0; k < u64bDataLen(readstack); k++) {
            //fprintf(stderr, "  [%zu] = %lu\n", k, readstack[1][k]);
        }

        //fprintf(stderr, "SLIK2: reading INT\n");
        call(rdxNextSLIK, &inner);
        //fprintf(stderr, "SLIK2: got type=%d i=%ld\n", inner.type, inner.i);
        test(inner.type == RDX_TYPE_INT, RDXBAD);
        testeq(inner.i, j);

        //fprintf(stderr, "SLIK2: reading STRING\n");
        call(rdxNextSLIK, &inner);
        //fprintf(stderr, "SLIK2: got STRING type=%d\n", inner.type);
        test(inner.type == RDX_TYPE_STRING, RDXBAD);

        //fprintf(stderr, "SLIK2: checking END\n");
        ok64 end_o = rdxNextSLIK(&inner);
        //fprintf(stderr, "SLIK2: got %lx (END=%lx)\n", end_o, (u64)END);
        test(end_o == END, RDXBAD);
        //fprintf(stderr, "SLIK2: outo TUPLE %d\n", j);
        call(rdxOutoSLIK, &inner, &rc);
        //fprintf(stderr, "SLIK2: outo done, stack: data=%zu past=%zu\n",
        //        u64bDataLen(readstack), u64bPastLen(readstack));
        for (size_t k = 0; k < u64bDataLen(readstack); k++) {
            //fprintf(stderr, "  [%zu] = %lu\n", k, readstack[1][k]);
        }
    }
    //fprintf(stderr, "SLIK2: checking EULER END\n");
    test(rdxNextSLIK(&rc) == END, RDXBAD);

    done;
}

// Test: all PLEX types
ok64 SLIK3() {
    sane(1);
    u8 plex_types[] = {RDX_TYPE_TUPLE, RDX_TYPE_LINEAR, RDX_TYPE_EULER, RDX_TYPE_MULTIX};

    for (size_t p = 0; p < 4; p++) {
        a_pad(u8, pad, PAGESIZE * 4);
        a_pad0(u64, tabs, PAGESIZE);

        rdx e;
        rdxWriteInitSLIK(&e, pad, tabs);
        e.type = plex_types[p];
        e.id.seq = p;
        call(rdxWriteNextSLIK, &e);

        rdx c = {};
        call(rdxWriteIntoSLIK, &c, &e);
        for (int i = 0; i < 5; i++) {
            c.type = RDX_TYPE_INT;
            c.i = i * 10;
            c.id.seq = i;
            call(rdxWriteNextSLIK, &c);
        }
        call(rdxWriteOutoSLIK, &c, &e);
        call(rdxWriteFinishSLIK, &e);

        // Read back
        a_pad0(u64, readstack, 256);
        rdx r;
        rdxInitSLIK(&r, pad, readstack);
        call(rdxNextSLIK, &r);
        test(r.type == plex_types[p], RDXBAD);
        testeq(r.id.seq, p);

        rdx rc = {};
        call(rdxIntoSLIK, &rc, &r);
        for (int i = 0; i < 5; i++) {
            call(rdxNextSLIK, &rc);
            testeq(rc.i, i * 10);
        }
        test(rdxNextSLIK(&rc) == END, RDXBAD);
    }

    done;
}

// Test: seek 1M INT records in EULER
ok64 SLIK4() {
    sane(1);
    const int N = 100000;
    static u8 padbuf[MB * 16];
    static u64 tabsbuf[PAGESIZE];
    static u64 readstackbuf[PAGESIZE];
    u8p pad[4] = {padbuf, padbuf, padbuf, padbuf + sizeof(padbuf)};
    u64p tabs[4] = {tabsbuf, tabsbuf, tabsbuf, tabsbuf + PAGESIZE};

    // Write: EULER { INT(0), INT(1), ..., INT(N-1) }
    rdx e;
    rdxWriteInitSLIK(&e, pad, tabs);
    e.type = RDX_TYPE_EULER;
    call(rdxWriteNextSLIK, &e);

    rdx c = {};
    call(rdxWriteIntoSLIK, &c, &e);
    for (i64 i = 0; i < N; i++) {
        c.type = RDX_TYPE_INT;
        c.i = i;
        c.id.seq = i;
        call(rdxWriteNextSLIK, &c);
    }
    call(rdxWriteOutoSLIK, &c, &e);
    //fprintf(stderr, "SLIK4: before finish, stack len=%zu\n", u64bDataLen(tabs));
    call(rdxWriteFinishSLIK, &e);
    //fprintf(stderr, "SLIK4: wrote %d records, K written=%lu, data len=%zu\n",
    //        N, SLOG_STATS[SLOG_STAT_KWRITE], u8bDataLen(pad));

    // Seek for each value
    u64p readstack[4] = {readstackbuf, readstackbuf, readstackbuf, readstackbuf + PAGESIZE};
    SLOGStatsReset();
    for (i64 i = 0; i < N; i++) {
        if (i == 0) {
            // Show skip list for first seek
            u64bReset(readstack);
            rdx r0;
            rdxInitSLIK(&r0, pad, readstack);
            rdxNextSLIK(&r0);
            //fprintf(stderr, "SLIK4: after init, root stack len=%zu\n", u64bDataLen(readstack));
            // Enter EULER to load child skip list
            rdx rc0 = {};
            rdxIntoSLIK(&rc0, &r0);
            //fprintf(stderr, "SLIK4: child skip list len=%zu\n", u64bDataLen(readstack));
            //for (size_t k = 0; k < u64bDataLen(readstack) && k < 20; k++) {
                //fprintf(stderr, "  [%zu] = %lu\n", k, readstack[1][k]);
            //}
        }
        if (i % 1000 == 0) {
            //fprintf(stderr, "seek %li: pops=%lu cmp=%lu kexp=%lu kskip=%lu\n",
        //            i, SLOG_STATS[SLOG_STAT_POPS], SLOG_STATS[SLOG_STAT_CMP],
        //            SLOG_STATS[SLOG_STAT_KEXP], SLOG_STATS[SLOG_STAT_KSKIP]);
            SLOGStatsReset();
        }
        u64bReset(readstack);
        rdx r;
        rdxInitSLIK(&r, pad, readstack);
        call(rdxNextSLIK, &r);
        test(r.type == RDX_TYPE_EULER, RDXBAD);

        // Seek by setting c.type and c.i before rdxIntoSLIK
        rdx rc = {};
        rc.type = RDX_TYPE_INT;
        rc.i = i;

        ok64 o = rdxIntoSLIK(&rc, &r);
        if (o != OK) fail(o);
        test(rc.type == RDX_TYPE_INT, RDXBAD);
        testeq(rc.i, i);
    }

    done;
}

// Test: nested structure { a:{1,2}, b:{3}, c:{4,5} }
// EULER of TUPLEs, each TUPLE has TERM key and EULER value with INTs
// Verifies: write, full iteration, seek by key
ok64 SLIK5() {
    sane(1);
    a_pad(u8, pad, PAGESIZE * 8);
    a_pad0(u64, tabs, PAGESIZE);

    const char* keys[] = {"a", "b", "c"};
    i64 vals_a[] = {1, 2};
    i64 vals_b[] = {3};
    i64 vals_c[] = {4, 5};
    i64* all_vals[] = {vals_a, vals_b, vals_c};
    size_t val_counts[] = {2, 1, 2};

    //fprintf(stderr, "SLIK5: Write EULER { a:{1,2}, b:{3}, c:{4,5} }\n");

    // Write outer EULER
    rdx e;
    rdxWriteInitSLIK(&e, pad, tabs);
    e.type = RDX_TYPE_EULER;
    call(rdxWriteNextSLIK, &e);

    rdx tup = {};
    call(rdxWriteIntoSLIK, &tup, &e);

    for (size_t k = 0; k < 3; k++) {
        // Write TUPLE
        tup.type = RDX_TYPE_TUPLE;
        tup.id.seq = k;
        call(rdxWriteNextSLIK, &tup);

        rdx inner = {};
        call(rdxWriteIntoSLIK, &inner, &tup);

        // Write TERM key
        inner.type = RDX_TYPE_TERM;
        inner.t[0] = (u8c*)keys[k];
        inner.t[1] = inner.t[0] + strlen(keys[k]);
        inner.id.seq = 0;
        call(rdxWriteNextSLIK, &inner);

        // Write EULER value
        inner.type = RDX_TYPE_EULER;
        inner.id.seq = 1;
        call(rdxWriteNextSLIK, &inner);

        rdx eu = {};
        call(rdxWriteIntoSLIK, &eu, &inner);

        // Write INTs into EULER
        for (size_t v = 0; v < val_counts[k]; v++) {
            eu.type = RDX_TYPE_INT;
            eu.i = all_vals[k][v];
            eu.id.seq = v;
            call(rdxWriteNextSLIK, &eu);
        }

        call(rdxWriteOutoSLIK, &eu, &inner);
        call(rdxWriteOutoSLIK, &inner, &tup);
    }

    call(rdxWriteOutoSLIK, &tup, &e);
    call(rdxWriteFinishSLIK, &e);
    //fprintf(stderr, "SLIK5: written %zu bytes\n", u8bDataLen(pad));

    // === Part 1: Full iteration ===
    //fprintf(stderr, "SLIK5: Part 1 - Full iteration\n");
    a_pad0(u64, readstack, 512);
    rdx r;
    rdxInitSLIK(&r, pad, readstack);
    call(rdxNextSLIK, &r);
    test(r.type == RDX_TYPE_EULER, RDXBAD);

    rdx rtup = {};
    call(rdxIntoSLIK, &rtup, &r);

    size_t total_ints = 0;
    for (size_t k = 0; k < 3; k++) {
        call(rdxNextSLIK, &rtup);
        test(rtup.type == RDX_TYPE_TUPLE, RDXBAD);

        rdx rinner = {};
        call(rdxIntoSLIK, &rinner, &rtup);

        // Read TERM key
        call(rdxNextSLIK, &rinner);
        test(rinner.type == RDX_TYPE_TERM, RDXBAD);
        //fprintf(stderr, "SLIK5:   key='%.*s'\n", (int)$len(rinner.t), rinner.t[0]);

        // Read EULER value
        call(rdxNextSLIK, &rinner);
        test(rinner.type == RDX_TYPE_EULER, RDXBAD);

        rdx reu = {};
        call(rdxIntoSLIK, &reu, &rinner);

        for (size_t v = 0; v < val_counts[k]; v++) {
            call(rdxNextSLIK, &reu);
            test(reu.type == RDX_TYPE_INT, RDXBAD);
            testeq(reu.i, all_vals[k][v]);
            //fprintf(stderr, "SLIK5:     int=%ld\n", reu.i);
            total_ints++;
        }
        test(rdxNextSLIK(&reu) == END, RDXBAD);
        call(rdxOutoSLIK, &reu, &rinner);

        test(rdxNextSLIK(&rinner) == END, RDXBAD);
        call(rdxOutoSLIK, &rinner, &rtup);
    }
    test(rdxNextSLIK(&rtup) == END, RDXBAD);
    testeq(total_ints, 5);
    //fprintf(stderr, "SLIK5: Part 1 OK - %zu ints\n", total_ints);

    // === Part 2: Seek by TERM key ===
    //fprintf(stderr, "SLIK5: Part 2 - Seek by key\n");
    for (size_t k = 0; k < 3; k++) {
        u64bReset(readstack);
        rdxInitSLIK(&r, pad, readstack);
        call(rdxNextSLIK, &r);

        rdx seek = {};
        seek.type = RDX_TYPE_TERM;
        seek.t[0] = (u8c*)keys[k];
        seek.t[1] = seek.t[0] + strlen(keys[k]);

        ok64 o = rdxIntoSLIK(&seek, &r);
        //fprintf(stderr, "SLIK5:   seek '%s' -> %s type=%d\n",
        //        keys[k], ok64str(o), seek.type);

        if (o == OK && seek.type == RDX_TYPE_TUPLE) {
            rdx rinner = {};
            call(rdxIntoSLIK, &rinner, &seek);
            call(rdxNextSLIK, &rinner);
            test(rinner.type == RDX_TYPE_TERM, RDXBAD);
            test(memcmp(rinner.t[0], keys[k], strlen(keys[k])) == 0, RDXBAD);
        }
    }

    // === Part 3: Seek INT in inner EULER ===
    //fprintf(stderr, "SLIK5: Part 3 - Seek INT in inner EULER\n");
    u64bReset(readstack);
    rdxInitSLIK(&r, pad, readstack);
    call(rdxNextSLIK, &r);

    rtup = (rdx){};
    call(rdxIntoSLIK, &rtup, &r);
    call(rdxNextSLIK, &rtup);  // first TUPLE (a)

    rdx rinner = {};
    call(rdxIntoSLIK, &rinner, &rtup);
    call(rdxNextSLIK, &rinner);  // TERM
    call(rdxNextSLIK, &rinner);  // EULER
    test(rinner.type == RDX_TYPE_EULER, RDXBAD);

    // Seek for INT value 2 in EULER {1,2}
    rdx seekint = {};
    seekint.type = RDX_TYPE_INT;
    seekint.i = 2;

    ok64 o = rdxIntoSLIK(&seekint, &rinner);
    //fprintf(stderr, "SLIK5:   seek INT 2 -> %s type=%d i=%ld\n",
    //        ok64str(o), seekint.type, seekint.i);
    if (o == OK && seekint.type == RDX_TYPE_INT) {
        testeq(seekint.i, 2);
    }

    //fprintf(stderr, "SLIK5: All parts OK\n");
    done;
}

// Table-driven EULER seek test
fun ok64 SLIK6case(i64p keys, size_t nkeys) {
    sane(keys && nkeys > 0);
    a_pad(u8, pad, PAGESIZE * 16);
    a_pad0(u64, tabs, PAGESIZE);
    a_pad0(u64, readstack, PAGESIZE);

    // Write EULER { TUPLE(key, "val"), ... }
    rdx e;
    rdxWriteInitSLIK(&e, pad, tabs);
    e.type = RDX_TYPE_EULER;
    call(rdxWriteNextSLIK, &e);

    rdx child = {};
    call(rdxWriteIntoSLIK, &child, &e);
    for (size_t i = 0; i < nkeys; i++) {
        child.type = RDX_TYPE_TUPLE;
        child.id.seq = keys[i];
        call(rdxWriteNextSLIK, &child);

        rdx inner = {};
        call(rdxWriteIntoSLIK, &inner, &child);

        inner.type = RDX_TYPE_INT;
        inner.i = keys[i];
        call(rdxWriteNextSLIK, &inner);

        inner.type = RDX_TYPE_STRING;
        inner.s[0] = (u8c*)"val";
        inner.s[1] = (u8c*)"val" + 3;
        call(rdxWriteNextSLIK, &inner);

        call(rdxWriteOutoSLIK, &inner, &child);
    }
    call(rdxWriteOutoSLIK, &child, &e);
    call(rdxWriteFinishSLIK, &e);

    // Seek each key
    for (size_t i = 0; i < nkeys; i++) {
        u64bReset(readstack);
        rdx r;
        rdxInitSLIK(&r, pad, readstack);
        call(rdxNextSLIK, &r);
        test(r.type == RDX_TYPE_EULER, RDXBAD);

        rdx seek = {};
        seek.type = RDX_TYPE_INT;
        seek.i = keys[i];

        ok64 o = rdxIntoSLIK(&seek, &r);
        if (o != OK) fail(o);
        test(seek.type == RDX_TYPE_TUPLE, RDXBAD);
    }

    done;
}

ok64 SLIK6() {
    sane(1);

    // Test case table
    i64 case0[] = {-4294967296LL, 0LL, 281384799174655LL};
    i64 case1[] = {0LL, 208260575694684282LL};
    i64 case2[] = {-8319119876378817396LL, 65535LL, 4278190080LL};  // SLIK2fuzz crash

    struct {
        i64p keys;
        size_t n;
    } cases[] = {
        {case0, 3},
        {case1, 2},
        {case2, 3},
    };

    for (size_t c = 0; c < sizeof(cases) / sizeof(cases[0]); c++) {
        call(SLIK6case, cases[c].keys, cases[c].n);
    }

    done;
}

// Test: SLIK2fuzz repro - same structure as fuzz test
// EULER of buckets, each bucket = TUPLE { idx, EULER { pairs } }
// Input broken into ascending runs, each run becomes a bucket
fun u64 SLIK7RunStart(i64cs data, u64 i) {
    u64 start = 0;
    for (u64 j = 1; j <= i; j++) {
        if (data[0][j] <= data[0][j - 1]) start = j;
    }
    return start;
}

fun u64 SLIK7RunEnd(i64cs data, u64 i) {
    u64 len = $len(data);
    for (u64 j = i + 1; j < len; j++) {
        if (data[0][j] <= data[0][j - 1]) return j;
    }
    return len;
}

fun ok64 SLIK7case(i64p vals, size_t nvals) {
    sane(vals && nvals > 0);

    i64cs data = {vals, vals + nvals};

    a_pad(u8, pad, PAGESIZE * 16);
    a_pad0(u64, tabs, PAGESIZE * 4);

    // Write: EULER of buckets (same as SLIK2fuzz)
    rdx root = {};
    rdxWriteInitSLIK(&root, pad, tabs);
    root.type = RDX_TYPE_EULER;
    call(rdxWriteNextSLIK, &root);

    rdx bucket = {};
    call(rdxWriteIntoSLIK, &bucket, &root);

    // Break into ascending runs, each run becomes a bucket
    u64 i = 0;
    while (i < nvals) {
        u64 runstart = i;
        u64 runend = SLIK7RunEnd(data, i);
        i64cs run = {data[0] + runstart, data[0] + runend};
        call(NUMPFeedBucket, &bucket, (i64)runstart, run);
        i = runend;
    }

    call(rdxWriteOutoSLIK, &bucket, &root);
    call(rdxWriteFinishSLIK, &root);

    // Read and seek (same as SLIK2fuzz)
    a_pad0(u64, readstack, 256);
    rdx r;
    rdxInitSLIK(&r, pad, readstack);
    call(rdxNextSLIK, &r);
    test(r.type == RDX_TYPE_EULER, RDXBAD);

    // Seek each index
    for (u64 idx = 0; idx < nvals; idx++) {
        u64 runstart = SLIK7RunStart(data, idx);
        u64 runend = SLIK7RunEnd(data, idx);

        // Seek bucket by runstart index
        rdx rbucket = {};
        rbucket.type = RDX_TYPE_INT;
        rbucket.i = (i64)runstart;
        ok64 o = rdxIntoSLIK(&rbucket, &r);
        test(o == OK && rbucket.type == RDX_TYPE_TUPLE, RDXBAD);
        testeq((i64)rbucket.id.seq, (i64)runstart);

        // Enter bucket: skip buckndx, enter EULER
        rdx inner = {};
        call(rdxIntoSLIK, &inner, &rbucket);
        call(rdxNextSLIK, &inner);
        test(inner.type == RDX_TYPE_INT, RDXBAD);
        call(rdxNextSLIK, &inner);
        test(inner.type == RDX_TYPE_EULER, RDXBAD);

        // Seek pair by value
        rdx pair = {};
        call(NUMPSeekPair, &pair, &inner, data[0][idx]);
        call(NUMPVerifyPair, &pair, data[0][idx]);

        // Drain remaining pairs after idx
        for (u64 n = idx + 1; n < runend; n++) {
            call(NUMPDrainPair, &pair, data[0][n]);
        }
        test(rdxNextSLIK(&pair) == END, RDXBAD);

        call(rdxOutoSLIK, &pair, &inner);
        call(rdxOutoSLIK, &inner, &rbucket);
        call(rdxOutoSLIK, &rbucket, &r);
    }

    done;
}

ok64 SLIK7() {
    sane(1);

    // Test case table - SLIK2fuzz crash repros
    i64 case0[] = {13229323905400887LL, 4785074604096000LL, 16384000LL};  // crash-00e7348

    struct {
        i64p vals;
        size_t n;
    } cases[] = {
        {case0, 3},
    };

    for (size_t c = 0; c < sizeof(cases) / sizeof(cases[0]); c++) {
        call(SLIK7case, cases[c].vals, cases[c].n);
    }

    done;
}

ok64 SLIKtest() {
    sane(1);
    call(SLIK0);
    call(SLIK1);
    call(SLIK2);
    call(SLIK3);
    call(SLIK4);
    call(SLIK5);
    call(SLIK6);
    call(SLIK7);
    done;
}

TEST(SLIKtest);
