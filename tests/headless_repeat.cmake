# Runs `mother run --headless --ticks 3600` RUNS times and fails unless every run exits 0
# and prints the same thing. Usage: cmake -DMOTHER=<exe> -DRUNS=3 -P headless_repeat.cmake

foreach(i RANGE 1 ${RUNS})
    execute_process(COMMAND ${MOTHER} run --headless --ticks 3600
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

if(NOT first MATCHES "^result=timeout ticks=3600\r?\n$")
    message(FATAL_ERROR "unexpected output: ${first}")
endif()
message(STATUS "${RUNS} runs, identical: ${first}")
