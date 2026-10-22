#include "vm/sandbox.h"

#include <cstdlib>

#include "lua.h"
#include "luacode.h"
#include "lualib.h"

namespace mother {

namespace {

constexpr const char* kRemovedGlobals[] = {"print", "gcinfo"};

// Everything in math that calls libm transcendentals or the RNG. The rest is exact IEEE
// arithmetic (abs, floor, sqrt, fmod, clamp, ...) and stays.
constexpr const char* kRemovedMath[] = {
    "acos", "asin", "atan", "atan2", "cos", "cosh", "exp", "log", "log10", "pow",
    "random", "randomseed", "sin", "sinh", "tan", "tanh",
};

// The compiler turns calls like math.sin(x) into built-in fast paths that run even when the
// library member has been removed, so every removed builtin is disabled at compile time too.
const char* const kDisabledBuiltins[] = {
    "math.acos", "math.asin", "math.atan", "math.atan2", "math.cos", "math.cosh", "math.exp",
    "math.log", "math.log10", "math.pow", "math.sin", "math.sinh", "math.tan", "math.tanh",
    "vector.create", "vector.magnitude", "vector.normalize", "vector.cross", "vector.dot",
    "vector.floor", "vector.ceil", "vector.abs", "vector.sign", "vector.clamp", "vector.min",
    "vector.max", "vector.lerp",
    nullptr,
};

// The cart table's fields change at run time, so the compiler must not cache them at load.
const char* const kMutableGlobals[] = {"cart", nullptr};

void* heap_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
    Heap& heap = *static_cast<Heap*>(ud);
    if (nsize == 0) {
        std::free(ptr);
        heap.used -= osize;
        return nullptr;
    }
    if (nsize > osize && heap.used - osize + nsize > heap.limit) return nullptr;
    void* block = std::realloc(ptr, nsize);
    if (block) heap.used = heap.used - osize + nsize;
    return block;
}

void open_lib(lua_State* L, lua_CFunction open, const char* name) {
    lua_pushcfunction(L, open, nullptr);
    lua_pushstring(L, name);
    lua_call(L, 1, 0);
}

}  // namespace

lua_State* new_sandbox_state(Heap& heap) {
    lua_State* L = lua_newstate(heap_alloc, &heap);
    if (!L) return nullptr;

    open_lib(L, luaopen_base, "");
    open_lib(L, luaopen_coroutine, LUA_COLIBNAME);
    open_lib(L, luaopen_table, LUA_TABLIBNAME);
    open_lib(L, luaopen_string, LUA_STRLIBNAME);
    open_lib(L, luaopen_math, LUA_MATHLIBNAME);
    open_lib(L, luaopen_utf8, LUA_UTF8LIBNAME);
    open_lib(L, luaopen_bit32, LUA_BITLIBNAME);
    open_lib(L, luaopen_buffer, LUA_BUFFERLIBNAME);

    for (const char* name : kRemovedGlobals) {
        lua_pushnil(L);
        lua_setglobal(L, name);
    }
    lua_getglobal(L, LUA_MATHLIBNAME);
    for (const char* name : kRemovedMath) {
        lua_pushnil(L);
        lua_setfield(L, -2, name);
    }
    lua_pop(L, 1);
    return L;
}

bool load_module(lua_State* L, int api_ref, const std::string& name, const std::string& source,
                 std::string& error) {
    lua_CompileOptions options = {};
    options.optimizationLevel = 1;
    options.debugLevel = 1;
    options.mutableGlobals = kMutableGlobals;
    options.disabledBuiltins = kDisabledBuiltins;
    size_t size = 0;
    char* bytecode = luau_compile(source.data(), source.size(), &options, &size);

    // A thread whose globals are a fresh table reading through to the frozen API.
    lua_State* T = lua_newthread(L);
    lua_newtable(T);
    lua_newtable(T);
    lua_getref(T, api_ref);
    lua_setfield(T, -2, "__index");
    lua_setreadonly(T, -1, true);
    lua_setmetatable(T, -2);
    lua_replace(T, LUA_GLOBALSINDEX);
    lua_setsafeenv(T, LUA_GLOBALSINDEX, true);

    const std::string chunk = "@" + name + ".luau";
    const int status = luau_load(T, chunk.c_str(), bytecode, size, 0);
    std::free(bytecode);
    if (status != 0) {
        const char* message = lua_tostring(T, -1);
        error = message ? message : "compile error in " + name + ".luau";
        lua_pop(L, 1);  // the thread
        return false;
    }
    lua_xmove(T, L, 1);  // the module function, onto L
    lua_remove(L, -2);   // the thread; the function keeps its globals table alive
    return true;
}

}  // namespace mother
