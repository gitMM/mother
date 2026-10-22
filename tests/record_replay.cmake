# Records a fuzzed headless run of CART to TRACE, then replays it and fails unless every tick's
# state hash matches. Usage: cmake -DMOTHER=<exe> -DCART=<dir> -DTRACE=<file> -P record_replay.cmake

execute_process(COMMAND ${MOTHER} record ${CART} ${TRACE} --headless --ticks 900 --input fuzz:21 --seed 22
                RESULT_VARIABLE rc OUTPUT_VARIABLE out)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "record exited with ${rc}: ${out}")
endif()
execute_process(COMMAND ${MOTHER} replay ${CART} ${TRACE} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT out MATCHES "replay=ok")
    message(FATAL_ERROR "replay failed (${rc}):\n${out}${err}")
endif()
message(STATUS "${out}")
