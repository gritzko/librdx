// First function: will change MSTOpen -> HITOpen
ok64 ProcessData(u64css heap, u32g out) {
    sane(1);
    MSTOpen(heap, &count);
    u32gFeed1(out, value);
    done;
}

// Eight unchanged lines to force a gap between hunks
// line 2 of gap
// line 3 of gap
// line 4 of gap
// line 5 of gap
// line 6 of gap
// line 7 of gap
// line 8 of gap

// Second function: will change MSTSeek -> HITSeek
ok64 CollectPaths(u64css iter, u64 prefix, u32g hashes) {
    ok64 o = MSTSeek(iter, prefix);
    if (o != OK) return OK;
    while (!$empty(iter)) {
        u64 entry = MSTHead(iter);
        if (TriOf(entry) != prefix) break;
        u32gFeed1(hashes, (u32)entry);
        MSTStep(iter);
    }
    return OK;
}
