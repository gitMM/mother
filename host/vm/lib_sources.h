#pragma once

#include <cstddef>

namespace mother {

// One MOTHERLIB module: lib/<name>.luau, compiled into the binary by cmake/embed_lib.cmake.
struct LibSource {
    const char* name;  // path under lib/ without ".luau", e.g. "core/world"
    const char* text;
    size_t size;
};

extern const LibSource kLibSources[];  // sorted by name
extern const size_t kLibSourceCount;

}  // namespace mother
