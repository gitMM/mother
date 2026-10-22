set(MOTHER_TEST_MODULES palette framebuffer present pad keymap clock options pattern headless)

set(test_sources test_main.cpp)
foreach(module ${MOTHER_TEST_MODULES})
    list(APPEND test_sources test_${module}.cpp)
endforeach()

add_executable(mother_tests ${test_sources})
target_link_libraries(mother_tests PRIVATE mother_host)
target_compile_options(mother_tests PRIVATE ${MOTHER_WARNINGS})

# Host unit tests: one CTest entry per module, selected by test-name prefix.
foreach(module ${MOTHER_TEST_MODULES})
    add_test(NAME unit.${module} COMMAND mother_tests ${module}_)
endforeach()

# Command-line tests against the real binary.
add_test(NAME cli.headless_3600 COMMAND mother run --headless --ticks 3600)
set_tests_properties(cli.headless_3600 PROPERTIES PASS_REGULAR_EXPRESSION "^result=timeout ticks=3600")

add_test(NAME cli.headless_repeatable
         COMMAND ${CMAKE_COMMAND} -DMOTHER=$<TARGET_FILE:mother> -DRUNS=3
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/headless_repeat.cmake)

add_test(NAME cli.headless_needs_ticks COMMAND mother run --headless)
set_tests_properties(cli.headless_needs_ticks PROPERTIES WILL_FAIL TRUE)
