#include <cmath>

#include "lua.h"
#include "lualib.h"

#include "gfx/framebuffer.h"
#include "vm/api.h"

namespace mother {

int to_coord(double v) {
    if (!(v == v)) return 0;
    const double f = std::floor(v);
    if (f < -32768.0) return -32768;
    if (f > 32767.0) return 32767;
    return int(f);
}

int arg_coord(lua_State* L, int idx) {
    return to_coord(luaL_checknumber(L, idx));
}

int opt_coord(lua_State* L, int idx, int def) {
    return to_coord(luaL_optnumber(L, idx, def));
}

uint8_t arg_colour(lua_State* L, int idx) {
    return uint8_t(arg_coord(L, idx) & kColourMask);
}

uint8_t opt_colour(lua_State* L, int idx, uint8_t def) {
    return uint8_t(opt_coord(L, idx, def) & kColourMask);
}

}  // namespace mother
