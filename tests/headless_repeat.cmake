# Runs `mother run [CART] --headless --ticks TICKS [ARGS]` RUNS times and fails unless every run
# exits 0, prints the same thing and matches EXPECT (checked against the output without trailing
# whitespace). ARGS is a ;-separated list of extra arguments.
# Usage: cmake -DMOTHER=<exe> -DRUNS=3 -DTICKS=3600 [-DCART=<dir>] [-DARGS=a;b] -DEXPECT=<regex> -P headless_repeat.cmake

foreach(i RANGE 1 ${RUNS})
    execute_process(COMMAND ${MOTHER} run ${CART} --headless --ticks ${TICKS} ${ARGS}
                    OUTPUT_VARIABLE out RESULT_VARIABLE rc)
    if(NOT rc EQUAL 0)
        message(FATAL_ERROR "run ${i} exited with ${rc}")
    endif()
    if(i EQUAL 1)
        set(first "${out}")
    elseif(NOT out STREQUAL first)
        message(FATAL_ERROR "run ${i} printed:\n${out}\nbut run 1 printed:\n${first}")
    endif()
endforeach()

string(STRIP "${first}" stripped)
if(NOT stripped MATCHES "${EXPECT}")
    message(FATAL_ERROR "output does not match ${EXPECT}:\n${first}")
endif()
message(STATUS "${RUNS} runs, identical: ${first}")
