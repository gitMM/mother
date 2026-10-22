#include "lua.h"
#include "lualib.h"

#include "vm/api.h"
#include "vm/cart_vm.h"

namespace mother {

namespace {

int l_spr(lua_State* L) {
    Painter& p = draw_painter(L, "spr");
    const int n = arg_coord(L, 1);
    if (n < 0 || n >= kSpriteCount) luaL_error(L, "spr(%d): sprite numbers are 0-%d", n, kSpriteCount - 1);
    const int x = arg_coord(L, 2), y = arg_coord(L, 3);
    const int w = opt_coord(L, 4, 1), h = opt_coord(L, 5, 1);
    p.spr(n, x, y, w, h, lua_toboolean(L, 6) != 0, lua_toboolean(L, 7) != 0);
    return 0;
}

int l_sspr(lua_State* L) {
    Painter& p = draw_painter(L, "sspr");
    const int sx = arg_coord(L, 1), sy = arg_coord(L, 2), sw = arg_coord(L, 3), sh = arg_coord(L, 4);
    const int dx = arg_coord(L, 5), dy = arg_coord(L, 6);
    const int dw = opt_coord(L, 7, sw), dh = opt_coord(L, 8, sh);
    p.sspr(sx, sy, sw, sh, dx, dy, dw, dh);
    return 0;
}

int l_map(lua_State* L) {
    Painter& p = draw_painter(L, "map");
    const int cx = arg_coord(L, 1), cy = arg_coord(L, 2), sx = arg_coord(L, 3), sy = arg_coord(L, 4);
    const int cw = arg_coord(L, 5), ch = arg_coord(L, 6);
    p.map(cx, cy, sx, sy, cw, ch, arg_layer(L, 7, "map"));
    return 0;
}

int l_palt(lua_State* L) {
    Painter& p = draw_painter(L, "palt");
    if (lua_gettop(L) == 0) {
        p.palt_reset();
    } else {
        const uint8_t c = arg_colour(L, 1);
        p.palt(c, lua_toboolean(L, 2) != 0);
    }
    return 0;
}

int l_ramp(lua_State* L) {
    const int n = arg_coord(L, 1);
    if (n < 0 || n >= kMaxRamps) luaL_error(L, "ramp(%d): ramps are numbered 0-%d", n, kMaxRamps - 1);
    const std::vector<uint8_t>& ramp = CartVM::of(L).ramps()[size_t(n)];
    lua_createtable(L, int(ramp.size()), 0);
    for (size_t i = 0; i < ramp.size(); ++i) {
        lua_pushinteger(L, ramp[i]);
        lua_rawseti(L, -2, int(i + 1));
    }
    return 1;
}

constexpr luaL_Reg kFunctions[] = {
    {"spr", l_spr}, {"sspr", l_sspr}, {"map", l_map}, {"palt", l_palt}, {"ramp", l_ramp},
};

}  // namespace

void open_sprite_api(lua_State* L) {
    for (const luaL_Reg& f : kFunctions) {
        lua_pushcfunction(L, f.func, f.name);
        lua_setglobal(L, f.name);
    }
}

}  // namespace mother
