set(MOTHER_TEST_MODULES
    palette framebuffer present pad keymap clock options pattern headless
    draw cart_source cart_vm
    spr_format map_format draw_sprite png2spr
    trk_format synth sequencer
    rng fixed_math fuzz trace
)

set(test_sources test_main.cpp)
foreach(module ${MOTHER_TEST_MODULES})
    list(APPEND test_sources test_${module}.cpp)
endforeach()

add_executable(mother_tests ${test_sources})
target_link_libraries(mother_tests PRIVATE mother_host png2spr_lib)
target_compile_options(mother_tests PRIVATE ${MOTHER_WARNINGS})
target_compile_definitions(mother_tests PRIVATE MOTHER_TEST_CARTS="${CMAKE_CURRENT_SOURCE_DIR}/carts")

# Host unit tests: one CTest entry per module, selected by test-name prefix
# (test_cart_vm.cpp's tests start with "cartvm_", and so on).
foreach(module ${MOTHER_TEST_MODULES})
    string(REPLACE "_" "" prefix ${module})
    add_test(NAME unit.${module} COMMAND mother_tests ${prefix}_)
endforeach()

# Command-line tests against the real binary.
set(carts ${CMAKE_CURRENT_SOURCE_DIR}/carts)

add_test(NAME cli.headless_3600 COMMAND mother run --headless --ticks 3600)
set_tests_properties(cli.headless_3600 PROPERTIES PASS_REGULAR_EXPRESSION "^result=timeout ticks=3600")

add_test(NAME cli.headless_repeatable
         COMMAND ${CMAKE_COMMAND} -DMOTHER=$<TARGET_FILE:mother> -DRUNS=3 -DTICKS=3600
                 "-DEXPECT=^result=timeout ticks=3600 seed=0 budget=0 hash=[0-9a-f]+ audio=[0-9a-f]+$"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/headless_repeat.cmake)

add_test(NAME cli.headless_needs_ticks COMMAND mother run --headless)
set_tests_properties(cli.headless_needs_ticks PROPERTIES WILL_FAIL TRUE)

# M1 acceptance: the hello cart runs headless; the sandbox cart proves io, os, math.random and
# the rest are out of reach.
add_test(NAME cli.cart_hello
         COMMAND ${CMAKE_COMMAND} -DMOTHER=$<TARGET_FILE:mother> -DRUNS=2 -DTICKS=600
                 -DCART=${carts}/hello "-DEXPECT=^result=timeout ticks=600 seed=0 budget=0 hash=[0-9a-f]+ audio=[0-9a-f]+$"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/headless_repeat.cmake)

add_test(NAME cli.cart_sandbox COMMAND mother run ${carts}/sandbox --headless --ticks 10)
set_tests_properties(cli.cart_sandbox PROPERTIES PASS_REGULAR_EXPRESSION "result=win ticks=1")

add_test(NAME cli.cart_missing COMMAND mother run ${carts}/no_such_cart --headless --ticks 10)
set_tests_properties(cli.cart_missing PROPERTIES WILL_FAIL TRUE)

# M2 acceptance: png2spr turns art.png into exactly the committed sprites.spr (unit.png2spr then
# checks that the cart draws it pixel-exact), and the round-trip cart runs.
add_test(NAME cli.png2spr_roundtrip
         COMMAND ${CMAKE_COMMAND} -DPNG2SPR=$<TARGET_FILE:png2spr> -DPNG=${carts}/roundtrip/art.png
                 -DOUT=${CMAKE_CURRENT_BINARY_DIR}/roundtrip.spr -DEXPECTED=${carts}/roundtrip/sprites.spr
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/png2spr_check.cmake)

add_test(NAME cli.png2spr_usage COMMAND png2spr)
set_tests_properties(cli.png2spr_usage PROPERTIES WILL_FAIL TRUE)

add_test(NAME cli.cart_roundtrip COMMAND mother run ${carts}/roundtrip --headless --ticks 60)
set_tests_properties(cli.cart_roundtrip PROPERTIES PASS_REGULAR_EXPRESSION "result=timeout ticks=60")

# M3 acceptance: the test song renders headless to identical samples on every run. The hash is
# pinned, so Debug and Release builds and every CI platform must render the very same samples.
add_test(NAME cli.cart_song
         COMMAND ${CMAKE_COMMAND} -DMOTHER=$<TARGET_FILE:mother> -DRUNS=3 -DTICKS=1200
                 -DCART=${carts}/song "-DEXPECT=^result=timeout ticks=1200 seed=0 budget=0 hash=[0-9a-f]+ audio=889c5b9269c72083$"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/headless_repeat.cmake)

add_test(NAME cli.cart_song_wav
         COMMAND mother run ${carts}/song --headless --ticks 600 --wav ${CMAKE_CURRENT_BINARY_DIR}/song.wav)
set_tests_properties(cli.cart_song_wav PROPERTIES PASS_REGULAR_EXPRESSION "result=timeout ticks=600")

# M4 acceptance: one stored trace per cart, recorded once from fuzz input. Replaying it must
# reproduce every tick's state hash, on every build and every CI platform.
foreach(cart dice hello song roundtrip sandbox)
    add_test(NAME cli.replay_${cart}
             COMMAND mother replay ${carts}/${cart} ${CMAKE_CURRENT_SOURCE_DIR}/traces/${cart}.trace)
    set_tests_properties(cli.replay_${cart} PROPERTIES PASS_REGULAR_EXPRESSION "replay=ok")
endforeach()

# Fuzzed headless runs repeat exactly; record then replay agrees within one build.
add_test(NAME cli.fuzz_repeatable
         COMMAND ${CMAKE_COMMAND} -DMOTHER=$<TARGET_FILE:mother> -DRUNS=2 -DTICKS=3000
                 -DCART=${carts}/dice "-DARGS=--input;fuzz:11;--seed;12"
                 "-DEXPECT=^result=timeout ticks=3000 seed=12 budget=0 hash=[0-9a-f]+ audio=[0-9a-f]+$"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/headless_repeat.cmake)

add_test(NAME cli.record_then_replay
         COMMAND ${CMAKE_COMMAND} -DMOTHER=$<TARGET_FILE:mother> -DCART=${carts}/dice
                 -DTRACE=${CMAKE_CURRENT_BINARY_DIR}/dice_fresh.trace
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/record_replay.cmake)
