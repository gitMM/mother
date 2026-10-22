cmake_minimum_required(VERSION 3.25)
project(mother VERSION 0.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Vendored dependencies (see vendor/README.md). SDL3 is linked statically into one binary.
# SDL is added before the strict FP flags: it never touches simulation state, and MSVC
# /fp:strict rejects its constant float initializers.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL SYSTEM)

# Determinism (brief section 4): strict IEEE floating point and no FMA contraction for
# Luau and everything of ours, so hashes match on x64 and ARM64.
if(MSVC)
    add_compile_options(/fp:strict)
else()
    add_compile_options(-ffp-contract=off -fno-fast-math)
endif()

set(LUAU_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/luau EXCLUDE_FROM_ALL SYSTEM)

# Warnings for MOTHER's own targets only.
if(MSVC)
    set(MOTHER_WARNINGS /W4 /utf-8)
else()
    set(MOTHER_WARNINGS -Wall -Wextra -Wpedantic)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

enable_testing()
add_subdirectory(host)
add_subdirectory(tools/png2spr)
add_subdirectory(tests)
