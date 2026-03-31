void process(void) {
    // Old comment line about processing
    if (flag != OK) {
        fprintf(stdout, "error %d\n", code);
        cleanup_old();
        return;
    }
    init_data();
    fprintf(stdout, "output %d\n", result);
    int total = count();
    for (int i = 0; i < total; i++)
        emit_old(i, flag);
    fputc('\n', stdout);
    done_old();
    if (map) unmap(map);
    done;
}
