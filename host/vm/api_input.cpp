#include <cstring>

#include "lua.h"
#include "lualib.h"

#include "vm/api.h"
#include "vm/cart_vm.h"

namespace mother {

namespace {

struct ButtonName {
    const char* name;
    Button button;
};

// X and Y are missing on purpose: they belong to the import system (brief section 5).
constexpr ButtonName kButtons[] = {
    {"left", Button::Left}, {"right", Button::Right}, {"up", Button::Up}, {"down", Button::Down},
    {"a", Button::A},       {"b", Button::B},         {"l", Button::L},   {"r", Button::R},
    {"start", Button::Start},
};

// reserved: whether X and Y may be read (MOTHERLIB's import system only).
Button arg_button(lua_State* L, const char* fn, bool reserved = false) {
    const char* name = luaL_checkstring(L, 1);
    for (const ButtonName& b : kButtons) {
        if (std::strcmp(name, b.name) == 0) return b.button;
    }
    const bool x = std::strcmp(name, "x") == 0, y = std::strcmp(name, "y") == 0;
    if (reserved && (x || y)) return x ? Button::X : Button::Y;
    if (x || y) luaL_error(L, "%s(\"%s\"): X and Y are reserved for the import system", fn, name);
    luaL_error(L, "%s(\"%s\"): unknown button; use left, right, up, down, a, b, l, r or start", fn, name);
}

}  // namespace

int lib_btn(lua_State* L) {
    lua_pushboolean(L, CartVM::of(L).pad().btn(arg_button(L, "host.btn", true)));
    return 1;
}

int lib_btnp(lua_State* L) {
    lua_pushboolean(L, CartVM::of(L).pad().btnp(arg_button(L, "host.btnp", true)));
    return 1;
}

namespace {

int l_btn(lua_State* L) {
    lua_pushboolean(L, CartVM::of(L).pad().btn(arg_button(L, "btn")));
    return 1;
}

int l_btnp(lua_State* L) {
    lua_pushboolean(L, CartVM::of(L).pad().btnp(arg_button(L, "btnp")));
    return 1;
}

int l_stick(lua_State* L) {
    const PadState& pad = CartVM::of(L).pad();
    lua_pushnumber(L, pad.stick_x / double(kAxisMax));
    lua_pushnumber(L, pad.stick_y / double(kAxisMax));
    return 2;
}

int l_pointer(lua_State* L) {
    const PadState& pad = CartVM::of(L).pad();
    if (!pad.pointer) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, pad.pointer_x);
    lua_pushinteger(L, pad.pointer_y);
    lua_pushboolean(L, pad.pointer_down);
    return 3;
}

constexpr luaL_Reg kFunctions[] = {
    {"btn", l_btn}, {"btnp", l_btnp}, {"stick", l_stick}, {"pointer", l_pointer},
};

}  // namespace

void open_input_api(lua_State* L) {
    for (const luaL_Reg& f : kFunctions) {
        lua_pushcfunction(L, f.func, f.name);
        lua_setglobal(L, f.name);
    }
}

}  // namespace mother
