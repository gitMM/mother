set(MOTHER_TEST_MODULES
    palette framebuffer present pad keymap clock options pattern headless
    draw cart_source cart_vm
    spr_format map_format draw_sprite png2spr
    trk_format synth sequencer
    rng fixed_math fuzz trace
    toml manifest world collision damage spawn bounds objective
    knightfall breakout blockfall starfighter jumper rally carts import axes
)

set(test_sources test_main.cpp)
foreach(module ${MOTHER_TEST_MODULES})
    list(APPEND test_sources test_${module}.cpp)
endforeach()

add_executable(mother_tests ${test_sources})
target_link_libraries(mother_tests PRIVATE mother_host png2spr_lib)
target_compile_options(mother_tests PRIVATE ${MOTHER_WARNINGS})
target_compile_definitions(mother_tests PRIVATE MOTHER_TEST_CARTS="${CMAKE_CURRENT_SOURCE_DIR}/carts"
                                                MOTHER_CARTS="${PROJECT_SOURCE_DIR}/carts"
                                                MOTHER_BOTS="${CMAKE_CURRENT_SOURCE_DIR}/bots")

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
foreach(cart dice hello song roundtrip sandbox world)
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

# M5: the MOTHERLIB smoke cart runs 10 minutes under fuzz input, twice alike, with no errors or
# budget overruns.
add_test(NAME cli.cart_world_fuzz
         COMMAND ${CMAKE_COMMAND} -DMOTHER=$<TARGET_FILE:mother> -DRUNS=2 -DTICKS=36000
                 -DCART=${carts}/world "-DARGS=--input;fuzz:2;--seed;2"
                 "-DEXPECT=^result=(win|lose|timeout) ticks=[0-9]+ seed=2 budget=0 hash=[0-9a-f]+ audio=[0-9a-f]+$"
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/headless_repeat.cmake)

# M6 and M7 acceptance (with unit.carts, where bots win and lose them): all six carts run
# 10 minutes under fuzz input with no errors or budget overruns, alike every time, and replay
# their stored traces.
set(games ${PROJECT_SOURCE_DIR}/carts)
foreach(game knightfall breakout blockfall starfighter jumper rally)
    foreach(seed 1 2 3)
        add_test(NAME cli.${game}_fuzz_${seed}
                 COMMAND ${CMAKE_COMMAND} -DMOTHER=$<TARGET_FILE:mother> -DRUNS=2 -DTICKS=36000
                         -DCART=${games}/${game} "-DARGS=--input;fuzz:${seed};--seed;${seed}"
                         "-DEXPECT=^result=(win|lose|timeout) ticks=[0-9]+ seed=${seed} budget=0 hash=[0-9a-f]+ audio=[0-9a-f]+$"
                         -P ${CMAKE_CURRENT_SOURCE_DIR}/headless_repeat.cmake)
    endforeach()
    add_test(NAME cli.replay_${game}
             COMMAND mother replay ${games}/${game} ${CMAKE_CURRENT_SOURCE_DIR}/traces/${game}.trace)
    set_tests_properties(cli.replay_${game} PROPERTIES PASS_REGULAR_EXPRESSION "replay=ok")
endforeach()

add_test(NAME cli.record_then_replay
         COMMAND ${CMAKE_COMMAND} -DMOTHER=$<TARGET_FILE:mother> -DCART=${carts}/dice
                 -DTRACE=${CMAKE_CURRENT_BINARY_DIR}/dice_fresh.trace
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/record_replay.cmake)

# M8 acceptance (with unit.import and unit.axes): mother import runs a cart with another's export,
# and refuses an incompatible one saying which axis failed; mother matrix computes the brief's
# expected table and every compatible pair passes 3 fuzz seeds of 10 minutes (Debug builds run
# 1 minute each: a Debug matrix would take most of an hour).
add_test(NAME cli.import_runs
         COMMAND mother import ${games}/knightfall breakout:ball --headless --ticks 600 --input fuzz:1 --seed 1)
set_tests_properties(cli.import_runs PROPERTIES PASS_REGULAR_EXPRESSION "result=(win|lose|timeout) ticks=[0-9]+ seed=1 budget=0")

add_test(NAME cli.import_refused COMMAND mother import ${games}/breakout jumper:walker --headless --ticks 60)
set_tests_properties(cli.import_refused PROPERTIES
                     PASS_REGULAR_EXPRESSION "can't import jumper:walker: needs surface=ground, host has none")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(matrix_ticks 3600)
else()
    set(matrix_ticks 36000)
endif()
add_test(NAME cli.matrix
         COMMAND ${CMAKE_COMMAND} -DMOTHER=$<TARGET_FILE:mother> -DCARTS=${games} -DTICKS=${matrix_ticks}
                 -DOUT=${CMAKE_CURRENT_BINARY_DIR}/matrix.md -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/matrix_expected.md
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/matrix_check.cmake)
set_tests_properties(cli.matrix PROPERTIES TIMEOUT 1500)
