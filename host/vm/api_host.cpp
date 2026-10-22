#include <cstring>
#include <string>

#include "lua.h"
#include "lualib.h"

#include "vm/api.h"
#include "vm/cart_vm.h"

namespace mother {

namespace {

int l_finish(lua_State* L) {
    const char* result = luaL_checkstring(L, 1);
    if (std::strcmp(result, "win") == 0) {
        CartVM::of(L).finish(Outcome::Win);
    } else if (std::strcmp(result, "lose") == 0) {
        CartVM::of(L).finish(Outcome::Lose);
    } else {
        luaL_error(L, "mother.finish(\"%s\"): the result must be \"win\" or \"lose\"", result);
    }
    return 0;
}

int l_log(lua_State* L) {
    std::string line;
    const int n = lua_gettop(L);
    for (int i = 1; i <= n; ++i) {
        size_t len = 0;
        const char* s = luaL_tolstring(L, i, &len);
        if (i > 1) line += '\t';
        line.append(s, len);
        lua_pop(L, 1);
    }
    CartVM::of(L).log(line);
    return 0;
}

int l_tick(lua_State* L) {
    lua_pushnumber(L, double(CartVM::of(L).ticks()));
    return 1;
}

int l_import(lua_State* L) {
    return CartVM::of(L).import_module(L, luaL_checkstring(L, 1));
}

}  // namespace

void open_host_api(lua_State* L) {
    lua_newtable(L);
    lua_pushcfunction(L, l_finish, "mother.finish");
    lua_setfield(L, -2, "finish");
    lua_pushcfunction(L, l_log, "mother.log");
    lua_setfield(L, -2, "log");
    lua_setglobal(L, "mother");

    lua_pushcfunction(L, l_tick, "tick");
    lua_setglobal(L, "tick");
    lua_pushcfunction(L, l_import, "import");
    lua_setglobal(L, "import");
}

}  // namespace mother
