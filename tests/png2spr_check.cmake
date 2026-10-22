# Converts PNG with png2spr into OUT and fails unless OUT is byte-identical to EXPECTED.
# Usage: cmake -DPNG2SPR=<exe> -DPNG=<in.png> -DOUT=<out.spr> -DEXPECTED=<spr> -P png2spr_check.cmake

execute_process(COMMAND ${PNG2SPR} ${PNG} ${OUT} RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "png2spr exited with ${rc}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${OUT} ${EXPECTED} RESULT_VARIABLE differs)
if(NOT differs EQUAL 0)
    message(FATAL_ERROR "${OUT} differs from ${EXPECTED}")
endif()
message(STATUS "png2spr output matches ${EXPECTED}")
