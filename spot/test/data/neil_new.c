ok64 Process(u64css heap, u32g out) {
    sane(1);
    HITOpen(heap, &count);
    HITSeek(heap, &index);
    HITRead(heap, &value);
    u32gFeed1(out, value);
    HITStep(heap, &more);
    done;
}

ok64 Collect(u64css iter, u64 prefix, u32g hashes) {
    ok64 o = HITSeek(iter, prefix);
    if (o != OK) return OK;
    while (!$empty(iter)) {
        u64 entry = HITHead(iter);
        if (TriOf(entry) != prefix) break;
        u32gFeed1(hashes, (u32)entry);
        HITStep(iter);
    }
    return OK;
}
