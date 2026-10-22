# Hardware data and asset text formats: no SDL or Luau, shared by the host and the tools.
add_library(mother_assets STATIC
    assets/manifest.cpp
    assets/map_format.cpp
    assets/spr_format.cpp
    assets/tilemap.cpp
    assets/toml.cpp
    assets/trk_cell.cpp
    assets/trk_format.cpp
    gfx/palette.cpp
)
target_include_directories(mother_assets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(mother_assets PRIVATE ${MOTHER_WARNINGS})

add_library(mother_host STATIC
    audio/audio_out.cpp
    audio/sequencer.cpp
    audio/synth.cpp
    audio/voice.cpp
    audio/wav.cpp
    gfx/draw.cpp
    gfx/draw_sprite.cpp
    gfx/font.cpp
    gfx/framebuffer.cpp
    gfx/present.cpp
    input/fuzz.cpp
    input/keymap.cpp
    input/pad.cpp
    runner/clock.cpp
    runner/commands.cpp
    runner/headless.cpp
    runner/matrix.cpp
    runner/options.cpp
    runner/test_pattern.cpp
    runner/trace.cpp
    runner/windowed.cpp
    runtime/fixed_math.cpp
    runtime/rng.cpp
    vm/api_args.cpp
    vm/api_audio.cpp
    vm/api_gfx.cpp
    vm/api_host.cpp
    vm/api_input.cpp
    vm/api_map.cpp
    vm/api_math.cpp
    vm/api_sprite.cpp
    vm/cart_source.cpp
    vm/cart_vm.cpp
    vm/imports.cpp
    vm/motherlib.cpp
    vm/sandbox.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lib_sources.cpp
)
target_include_directories(mother_host PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# MOTHERLIB (lib/) is compiled into the binary as byte arrays.
file(GLOB_RECURSE motherlib_files CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/lib/*.luau)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lib_sources.cpp
    COMMAND ${CMAKE_COMMAND} -DLIB_DIR=${PROJECT_SOURCE_DIR}/lib -DOUT=${CMAKE_CURRENT_BINARY_DIR}/lib_sources.cpp
            -P ${PROJECT_SOURCE_DIR}/cmake/embed_lib.cmake
    DEPENDS ${motherlib_files} ${PROJECT_SOURCE_DIR}/cmake/embed_lib.cmake
    COMMENT "Embedding MOTHERLIB"
    VERBATIM)
target_link_libraries(mother_host PUBLIC mother_assets SDL3::SDL3-static Luau.VM Luau.Compiler)
target_compile_options(mother_host PRIVATE ${MOTHER_WARNINGS})

add_executable(mother main.cpp)
target_link_libraries(mother PRIVATE mother_host)
target_compile_options(mother PRIVATE ${MOTHER_WARNINGS})
