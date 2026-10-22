#include "lua.h"
#include "lualib.h"

#include "vm/api.h"
#include "vm/cart_vm.h"

// Tilemap and tile-flag access. Unlike drawing, these work anywhere: the tilemap is simulation
// state (damageable tiles, BLOCKFALL's well). Tiles are 0-4095: sheet 3's are imports' (map.map
// itself holds 0-3071).

namespace mother {

int arg_layer(lua_State* L, int idx, const char* fn) {
    const int layer = opt_coord(L, idx, 0);
    if (layer < 0 || layer >= kMapLayers) luaL_error(L, "%s: layer must be 0 or 1, got %d", fn, layer);
    return layer;
}

namespace {

int arg_tile(lua_State* L, int idx, const char* fn) {
    const int tile = arg_coord(L, idx);
    if (tile < 0 || tile >= kSpriteCount) luaL_error(L, "%s: tile must be 0-%d, got %d", fn, kSpriteCount - 1, tile);
    return tile;
}

int arg_bit(lua_State* L, int idx, const char* fn) {
    const int bit = arg_coord(L, idx);
    if (bit < 0 || bit > 7) luaL_error(L, "%s: flag bit must be 0-7, got %d", fn, bit);
    return bit;
}

int l_mget(lua_State* L) {
    const int x = arg_coord(L, 1), y = arg_coord(L, 2);
    const int layer = arg_layer(L, 3, "mget");
    lua_pushinteger(L, CartVM::of(L).tilemap().get(layer, x, y));
    return 1;
}

int l_mset(lua_State* L) {
    const int x = arg_coord(L, 1), y = arg_coord(L, 2);
    const int tile = arg_tile(L, 3, "mset");
    const int layer = arg_layer(L, 4, "mset");
    CartVM::of(L).tilemap().set(layer, x, y, uint16_t(tile));
    return 0;
}

int l_fget(lua_State* L) {
    const int tile = arg_tile(L, 1, "fget");
    const uint8_t flags = CartVM::of(L).tilemap().flags[size_t(tile)];
    if (lua_isnoneornil(L, 2)) {
        lua_pushinteger(L, flags);
    } else {
        lua_pushboolean(L, (flags >> arg_bit(L, 2, "fget")) & 1);
    }
    return 1;
}

int l_fset(lua_State* L) {
    const int tile = arg_tile(L, 1, "fset");
    const int bit = arg_bit(L, 2, "fset");
    TileMap& map = CartVM::of(L).tilemap();
    const uint8_t flags = map.flags[size_t(tile)];
    map.set_flags(tile, lua_toboolean(L, 3) ? uint8_t(flags | (1u << bit)) : uint8_t(flags & ~(1u << bit)));
    return 0;
}

constexpr luaL_Reg kFunctions[] = {
    {"mget", l_mget}, {"mset", l_mset}, {"fget", l_fget}, {"fset", l_fset},
};

}  // namespace

void open_map_api(lua_State* L) {
    for (const luaL_Reg& f : kFunctions) {
        lua_pushcfunction(L, f.func, f.name);
        lua_setglobal(L, f.name);
    }
}

}  // namespace mother
