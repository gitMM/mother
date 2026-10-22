# png2spr: PNG -> .spr converter. The only place stb_image is compiled (brief section 2).
add_library(png2spr_lib STATIC
    convert.cpp
    quantize.cpp
    stb_image_impl.cpp
)
target_include_directories(png2spr_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(png2spr_lib SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/vendor/stb)
target_link_libraries(png2spr_lib PUBLIC mother_assets)
target_compile_options(png2spr_lib PRIVATE ${MOTHER_WARNINGS})

add_executable(png2spr main.cpp)
target_link_libraries(png2spr PRIVATE png2spr_lib)
target_compile_options(png2spr PRIVATE ${MOTHER_WARNINGS})
