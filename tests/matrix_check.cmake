# Runs `mother matrix` on CARTS (TICKS ticks per run) into OUT, then checks that it passed and
# that its table is exactly EXPECTED (the brief's section 9 table).
execute_process(COMMAND ${MOTHER} matrix ${CARTS} --out ${OUT} --ticks ${TICKS} RESULT_VARIABLE code)
if(NOT code EQUAL 0)
    message(FATAL_ERROR "mother matrix failed (exit ${code})")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${OUT} ${EXPECTED} RESULT_VARIABLE differ)
if(NOT differ EQUAL 0)
    message(FATAL_ERROR "the computed matrix (${OUT}) differs from ${EXPECTED}")
endif()
