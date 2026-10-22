#include "lua.h"
#include "lualib.h"

#include "vm/api.h"
#include "vm/cart_vm.h"

namespace mother {

namespace {

Painter& painter(lua_State* L, const char* fn) {
    return CartVM::of(L).painter_in_draw(L, fn);
}

int l_cls(lua_State* L) {
    painter(L, "cls").cls(opt_colour(L, 1, 0));
    return 0;
}

int l_pset(lua_State* L) {
    painter(L, "pset").pset(arg_coord(L, 1), arg_coord(L, 2), arg_colour(L, 3));
    return 0;
}

int l_pget(lua_State* L) {
    lua_pushinteger(L, painter(L, "pget").pget(arg_coord(L, 1), arg_coord(L, 2)));
    return 1;
}

int l_line(lua_State* L) {
    painter(L, "line").line(arg_coord(L, 1), arg_coord(L, 2), arg_coord(L, 3), arg_coord(L, 4),
                            arg_colour(L, 5));
    return 0;
}

int l_rect(lua_State* L) {
    painter(L, "rect").rect(arg_coord(L, 1), arg_coord(L, 2), arg_coord(L, 3), arg_coord(L, 4),
                            arg_colour(L, 5));
    return 0;
}

int l_rectfill(lua_State* L) {
    painter(L, "rectfill").rectfill(arg_coord(L, 1), arg_coord(L, 2), arg_coord(L, 3), arg_coord(L, 4),
                                    arg_colour(L, 5));
    return 0;
}

int l_circ(lua_State* L) {
    painter(L, "circ").circ(arg_coord(L, 1), arg_coord(L, 2), arg_coord(L, 3), arg_colour(L, 4));
    return 0;
}

int l_circfill(lua_State* L) {
    painter(L, "circfill").circfill(arg_coord(L, 1), arg_coord(L, 2), arg_coord(L, 3), arg_colour(L, 4));
    return 0;
}

int l_pal(lua_State* L) {
    Painter& p = painter(L, "pal");
    if (lua_gettop(L) == 0) {
        p.pal_reset();
    } else {
        p.pal(arg_colour(L, 1), arg_colour(L, 2));
    }
    return 0;
}

int l_camera(lua_State* L) {
    painter(L, "camera").camera(opt_coord(L, 1, 0), opt_coord(L, 2, 0));
    return 0;
}

int l_clip(lua_State* L) {
    Painter& p = painter(L, "clip");
    if (lua_gettop(L) == 0) {
        p.clip_reset();
    } else {
        p.clip(arg_coord(L, 1), arg_coord(L, 2), arg_coord(L, 3), arg_coord(L, 4));
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
    {"cls", l_cls},       {"pset", l_pset},     {"pget", l_pget},         {"line", l_line},
    {"rect", l_rect},     {"rectfill", l_rectfill}, {"circ", l_circ},     {"circfill", l_circfill},
    {"pal", l_pal},       {"camera", l_camera}, {"clip", l_clip},         {"tri", l_tri},
    {"mesh", l_mesh},     {"cam3d", l_cam3d},
};

}  // namespace

void open_gfx_api(lua_State* L) {
    for (const luaL_Reg& f : kFunctions) {
        lua_pushcfunction(L, f.func, f.name);
        lua_setglobal(L, f.name);
    }
}

}  // namespace mother
