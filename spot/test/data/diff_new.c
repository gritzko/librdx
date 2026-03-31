void process(void) {
    // New comment about the processing step
    if (flag != OK) {
        if (use_pager) {
            pager_error(code);
            pager_cleanup();
        } else {
            fprintf(stdout, "error %d\n", code);
            cleanup_old();
        }
        return;
    }
    init_data();
    if (use_pager) {
        int sz = (int)data_len;
        hunk_t *hk = &hunks[nhunks];
        memset(hk, 0, sizeof(*hk));
        char hdr[512];
        int tlen = snprintf(hdr, sizeof(hdr), "+++ %s ---", name);
        if (tlen > 0) {
            char *tp = arena_write(hdr, (size_t)tlen);
            if (tp) { hk->title = tp; }
        }
        nhunks++;
        pager_run(hunks, nhunks);
        arena_cleanup();
    } else {
        fprintf(stdout, "output %d\n", result);
        int total = count();
        for (int i = 0; i < total; i++)
            emit_old(i, flag);
        fputc('\n', stdout);
    }
    done_old();
    if (map) unmap(map);
    done;
}
