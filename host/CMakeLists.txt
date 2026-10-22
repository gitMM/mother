# Hardware data and asset text formats: no SDL or Luau, shared by the host and the tools.
add_library(mother_assets STATIC
    assets/map_format.cpp
    assets/spr_format.cpp
    gfx/palette.cpp
)
target_include_directories(mother_assets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(mother_assets PRIVATE ${MOTHER_WARNINGS})

add_library(mother_host STATIC
    gfx/draw.cpp
    gfx/draw_sprite.cpp
    gfx/font.cpp
    gfx/framebuffer.cpp
    gfx/present.cpp
    input/keymap.cpp
    input/pad.cpp
    runner/clock.cpp
    runner/headless.cpp
    runner/options.cpp
    runner/test_pattern.cpp
    runner/windowed.cpp
    vm/api_args.cpp
    vm/api_gfx.cpp
    vm/api_host.cpp
    vm/api_input.cpp
    vm/api_map.cpp
    vm/api_sprite.cpp
    vm/cart_source.cpp
    vm/cart_vm.cpp
    vm/sandbox.cpp
)
target_include_directories(mother_host PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mother_host PUBLIC mother_assets SDL3::SDL3-static Luau.VM Luau.Compiler)
target_compile_options(mother_host PRIVATE ${MOTHER_WARNINGS})

add_executable(mother main.cpp)
target_link_libraries(mother PRIVATE mother_host)
target_compile_options(mother PRIVATE ${MOTHER_WARNINGS})
