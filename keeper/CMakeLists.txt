cmake_minimum_required(VERSION 3.20)
project(keeper C)
set(CMAKE_C_STANDARD 23)
include(CTest)
include_directories(.. .)

find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(gitcompat STATIC GIT.c PKT.c PACK.c DELT.c ZINF.c SHA1.c IGNO.c BELT.c WALK.c)
target_link_libraries(gitcompat abc-core ZLIB::ZLIB OpenSSL::Crypto)

add_library(keeplib STATIC KEEP.c REFS.c)
target_link_libraries(keeplib gitcompat dog abc-core)

add_executable(keeper-cli KEEP.cli.c)
target_link_libraries(keeper-cli keeplib gitcompat dog abc-core)
set_target_properties(keeper-cli PROPERTIES OUTPUT_NAME keeper)

add_executable(git-dl git-dl.cli.c)
target_link_libraries(git-dl gitcompat abc-core)

add_executable(belt belt.cli.c)
target_link_libraries(belt gitcompat abc-core)

add_subdirectory(test)
