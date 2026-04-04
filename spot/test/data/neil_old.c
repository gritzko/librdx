ok64 Process(u64css heap, u32g out) {
    sane(1);
    MSTOpen(heap, &count);
    MSTSeek(heap, &index);
    MSTRead(heap, &value);
    u32gFeed1(out, value);
    MSTStep(heap, &more);
    done;
}

ok64 Collect(u64css iter, u64 prefix, u32g hashes) {
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
