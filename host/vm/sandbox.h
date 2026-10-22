#pragma once

#include <cstddef>
#include <string>

struct lua_State;

namespace mother {

constexpr size_t kHeapLimit = 16u * 1024 * 1024;  // Luau heap per cart

struct Heap {
    size_t used = 0;
    size_t limit = kHeapLimit;
};

// A new Luau state holding only what a cart may use: the base, coroutine, table, string, math,
// utf8, bit32 and buffer libraries, minus print, gcinfo and math's libm and RNG functions (the
// cart API replaces them). os, debug, vector, io and require are never opened. Allocations
// beyond heap.limit fail with "not enough memory". The caller adds the cart API and freezes it.
lua_State* new_sandbox_state(Heap& heap);

// Compiles a module and pushes it as a function onto L. The module gets its own global table,
// which reads through to the frozen API table referenced by api_ref. Returns false with the
// compile error ("name.luau:line: message") on failure, pushing nothing.
bool load_module(lua_State* L, int api_ref, const std::string& name, const std::string& source,
                 std::string& error);

}  // namespace mother
