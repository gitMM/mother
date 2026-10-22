#include <string_view>

#include "lua.h"
#include "lualib.h"

#include "vm/api.h"
#include "vm/cart_vm.h"

namespace mother {

Painter& draw_painter(lua_State* L, const char* fn) {
    return CartVM::of(L).painter_in_draw(L, fn);
}

namespace {

int l_cls(lua_State* L) {
    Painter& p = draw_painter(L, "cls");
    p.cls(opt_colour(L, 1, 0));
    return 0;
}

int l_pset(lua_State* L) {
    Painter& p = draw_painter(L, "pset");
    const int x = arg_coord(L, 1), y = arg_coord(L, 2);
    p.pset(x, y, arg_colour(L, 3));
    return 0;
}

int l_pget(lua_State* L) {
    Painter& p = draw_painter(L, "pget");
    const int x = arg_coord(L, 1), y = arg_coord(L, 2);
    lua_pushinteger(L, p.pget(x, y));
    return 1;
}

int l_line(lua_State* L) {
    Painter& p = draw_painter(L, "line");
    const int x0 = arg_coord(L, 1), y0 = arg_coord(L, 2), x1 = arg_coord(L, 3), y1 = arg_coord(L, 4);
    p.line(x0, y0, x1, y1, arg_colour(L, 5));
    return 0;
}

int l_rect(lua_State* L) {
    Painter& p = draw_painter(L, "rect");
    const int x = arg_coord(L, 1), y = arg_coord(L, 2), w = arg_coord(L, 3), h = arg_coord(L, 4);
    p.rect(x, y, w, h, arg_colour(L, 5));
    return 0;
}

int l_rectfill(lua_State* L) {
    Painter& p = draw_painter(L, "rectfill");
    const int x = arg_coord(L, 1), y = arg_coord(L, 2), w = arg_coord(L, 3), h = arg_coord(L, 4);
    p.rectfill(x, y, w, h, arg_colour(L, 5));
    return 0;
}

int l_circ(lua_State* L) {
    Painter& p = draw_painter(L, "circ");
    const int x = arg_coord(L, 1), y = arg_coord(L, 2), r = arg_coord(L, 3);
    p.circ(x, y, r, arg_colour(L, 4));
    return 0;
}

int l_circfill(lua_State* L) {
    Painter& p = draw_painter(L, "circfill");
    const int x = arg_coord(L, 1), y = arg_coord(L, 2), r = arg_coord(L, 3);
    p.circfill(x, y, r, arg_colour(L, 4));
    return 0;
}

int l_print(lua_State* L) {
    Painter& p = draw_painter(L, "print");
    size_t len = 0;
    const char* text = luaL_tolstring(L, 1, &len);
    const int x = arg_coord(L, 2), y = arg_coord(L, 3);
    p.print(std::string_view(text, len), x, y, arg_colour(L, 4));
    return 0;
}

int l_pal(lua_State* L) {
    Painter& p = draw_painter(L, "pal");
    if (lua_gettop(L) == 0) {
        p.pal_reset();
    } else {
        const uint8_t src = arg_colour(L, 1);
        p.pal(src, arg_colour(L, 2));
    }
    return 0;
}

int l_camera(lua_State* L) {
    Painter& p = draw_painter(L, "camera");
    const int x = opt_coord(L, 1, 0);
    p.camera(x, opt_coord(L, 2, 0));
    return 0;
}

int l_clip(lua_State* L) {
    Painter& p = draw_painter(L, "clip");
    if (lua_gettop(L) == 0) {
        p.clip_reset();
    } else {
        const int x = arg_coord(L, 1), y = arg_coord(L, 2), w = arg_coord(L, 3), h = arg_coord(L, 4);
        p.clip(x, y, w, h);
    }
    return 0;
}

// 3D is out of scope; the names are reserved (brief section 3).
int l_tri(lua_State* L) {
    luaL_error(L, "tri() is not implemented");
}

int l_mesh(lua_State* L) {
    luaL_error(L, "mesh() is not implemented");
}

int l_cam3d(lua_State* L) {
    luaL_error(L, "cam3d() is not implemented");
}

constexpr luaL_Reg kFunctions[] = {
    {"cls", l_cls},           {"pset", l_pset},     {"pget", l_pget}, {"line", l_line},
    {"rect", l_rect},         {"rectfill", l_rectfill}, {"circ", l_circ}, {"circfill", l_circfill},
    {"print", l_print},       {"pal", l_pal},       {"camera", l_camera}, {"clip", l_clip},
    {"tri", l_tri},           {"mesh", l_mesh},     {"cam3d", l_cam3d},
};

}  // namespace

void open_gfx_api(lua_State* L) {
    for (const luaL_Reg& f : kFunctions) {
        lua_pushcfunction(L, f.func, f.name);
        lua_setglobal(L, f.name);
    }
}

}  // namespace mother
