execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${SRC}
    OUTPUT_VARIABLE H OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(
    COMMAND git describe --tags --abbrev=0
    WORKING_DIRECTORY ${SRC}
    OUTPUT_VARIABLE T OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT H)
    set(H "unknown")
endif()
if(NOT T)
    set(T "unknown")
endif()
set(NEW "#define SPOT_COMMIT_HASH \"${H}\"\n#define SPOT_GIT_TAG \"${T}\"\n")
if(EXISTS ${OUT})
    file(READ ${OUT} OLD)
    if("${OLD}" STREQUAL "${NEW}")
        return()
    endif()
endif()
file(WRITE ${OUT} "${NEW}")
