add_executable(SNIFFtest SNIFF.c)
target_link_libraries(SNIFFtest snifflib)
add_test(NAME SNIFFtest COMMAND SNIFFtest)
