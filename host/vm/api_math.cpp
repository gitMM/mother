#include <cmath>

#include "lua.h"
#include "lualib.h"

#include "runtime/fixed_math.h"
#include "vm/api.h"
#include "vm/cart_vm.h"

// The m table (fixed-algorithm maths) and the seeded RNG, rnd and rndi (brief section 4).

namespace mother {

namespace {

int l_sin(lua_State* L) {
    lua_pushnumber(L, fixed_sin(luaL_checknumber(L, 1)));
    return 1;
}

int l_cos(lua_State* L) {
    lua_pushnumber(L, fixed_cos(luaL_checknumber(L, 1)));
    return 1;
}

int l_atan2(lua_State* L) {
    const double y = luaL_checknumber(L, 1);
    lua_pushnumber(L, fixed_atan2(y, luaL_checknumber(L, 2)));
    return 1;
}

int l_exp(lua_State* L) {
    lua_pushnumber(L, fixed_exp(luaL_checknumber(L, 1)));
    return 1;
}

int l_sqrt(lua_State* L) {
    lua_pushnumber(L, std::sqrt(luaL_checknumber(L, 1)));
    return 1;
}

int l_floor(lua_State* L) {
    lua_pushnumber(L, std::floor(luaL_checknumber(L, 1)));
    return 1;
}

int l_clamp(lua_State* L) {
    const double x = luaL_checknumber(L, 1), lo = luaL_checknumber(L, 2), hi = luaL_checknumber(L, 3);
    const double r = x < lo ? lo : x;
    lua_pushnumber(L, r > hi ? hi : r);
    return 1;
}

int l_lerp(lua_State* L) {
    const double a = luaL_checknumber(L, 1), b = luaL_checknumber(L, 2), t = luaL_checknumber(L, 3);
    lua_pushnumber(L, t == 1.0 ? b : a + (b - a) * t);
    return 1;
}

int l_rnd(lua_State* L) {
    not_in_draw(L, "rnd");
    const double x = luaL_optnumber(L, 1, 1.0);
    lua_pushnumber(L, CartVM::of(L).rng().unit() * x);
    return 1;
}

int64_t arg_int32(lua_State* L, int idx, const char* fn) {
    const double v = luaL_checknumber(L, idx);
    if (!(v == std::floor(v)) || v < -2147483648.0 || v > 2147483647.0) {
        luaL_error(L, "%s: argument %d must be a whole number in the 32-bit range", fn, idx);
    }
    return int64_t(v);
}

int l_rndi(lua_State* L) {
    not_in_draw(L, "rndi");
    const int64_t a = arg_int32(L, 1, "rndi"), b = arg_int32(L, 2, "rndi");
    if (a > b) luaL_error(L, "rndi(%d, %d): the first argument must not be larger than the second", int(a), int(b));
    lua_pushnumber(L, double(CartVM::of(L).rng().range(a, b)));
    return 1;
}

constexpr luaL_Reg kMath[] = {
    {"sin", l_sin},     {"cos", l_cos},     {"atan2", l_atan2}, {"exp", l_exp},
    {"sqrt", l_sqrt},   {"floor", l_floor}, {"clamp", l_clamp}, {"lerp", l_lerp},
};

}  // namespace

void open_math_api(lua_State* L) {
    lua_createtable(L, 0, int(std::size(kMath)));
    for (const luaL_Reg& f : kMath) {
        lua_pushcfunction(L, f.func, f.name);
        lua_setfield(L, -2, f.name);
    }
    lua_setglobal(L, "m");
    lua_pushcfunction(L, l_rnd, "rnd");
    lua_setglobal(L, "rnd");
    lua_pushcfunction(L, l_rndi, "rndi");
    lua_setglobal(L, "rndi");
}

}  // namespace mother
