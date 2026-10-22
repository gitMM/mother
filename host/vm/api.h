#pragma once

#include <cstdint>

struct lua_State;

namespace mother {

class Painter;

// Register the cart API (brief section 5) as globals on a new cart state, before it is frozen.
void open_gfx_api(lua_State* L);     // cls pset pget line rect rectfill circ circfill print pal
                                     // camera clip, plus the 3D stubs tri mesh cam3d
void open_sprite_api(lua_State* L);  // spr sspr map palt ramp
void open_map_api(lua_State* L);     // mget mset fget fset
void open_input_api(lua_State* L);   // btn btnp stick pointer
void open_audio_api(lua_State* L);   // sfx music stop beat song_get song_set
void open_math_api(lua_State* L);    // the m table, rnd rndi
void open_host_api(lua_State* L);    // mother.finish mother.log tick import

// The cart's painter for a drawing function; raises a Luau error outside cart.draw().
Painter& draw_painter(lua_State* L, const char* fn);
// Raises a Luau error inside cart.draw(), for calls that change simulation state.
void not_in_draw(lua_State* L, const char* fn);

// Argument helpers for the bindings. Coordinates are floored and clamped to -32768..32767,
// with NaN read as 0, so no primitive can loop for long; colours are masked into 0-63.
// Bindings read their arguments in order into locals, so the first bad argument is the one
// reported on every compiler.
int to_coord(double v);
int arg_coord(lua_State* L, int idx);
int opt_coord(lua_State* L, int idx, int def);
uint8_t arg_colour(lua_State* L, int idx);
uint8_t opt_colour(lua_State* L, int idx, uint8_t def);
int arg_layer(lua_State* L, int idx, const char* fn);  // optional map layer, 0 (default) or 1

}  // namespace mother
