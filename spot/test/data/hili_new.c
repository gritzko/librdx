void example(int *gts, int ti) {
    u32 tlo = (ti > 0) ? tok32Offset(gts[0][ti-1]) : 0;
    u32 thi = tok32Offset(gts[0][ti]);
    process(tlo, thi);
}
