add_library(mother_host STATIC
    gfx/framebuffer.cpp
    gfx/palette.cpp
    gfx/present.cpp
    input/keymap.cpp
    input/pad.cpp
    runner/clock.cpp
    runner/headless.cpp
    runner/options.cpp
    runner/test_pattern.cpp
    runner/windowed.cpp
)
target_include_directories(mother_host PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mother_host PUBLIC SDL3::SDL3-static)
target_compile_options(mother_host PRIVATE ${MOTHER_WARNINGS})

add_executable(mother main.cpp)
target_link_libraries(mother PRIVATE mother_host)
target_compile_options(mother PRIVATE ${MOTHER_WARNINGS})
