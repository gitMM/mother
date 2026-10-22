#pragma once

#include <cstdint>

struct lua_State;

namespace mother {

// Register the cart API (brief section 5) as globals on a new cart state, before it is frozen.
void open_gfx_api(lua_State* L);    // cls pset pget line rect rectfill circ circfill pal camera
                                    // clip, plus the 3D stubs tri mesh cam3d
void open_input_api(lua_State* L);  // btn btnp stick pointer
void open_host_api(lua_State* L);   // mother.finish mother.log tick import

// Argument helpers for the bindings. Coordinates are floored and clamped to -32768..32767,
// with NaN read as 0, so no primitive can loop for long; colours are masked into 0-63.
int to_coord(double v);
int arg_coord(lua_State* L, int idx);
int opt_coord(lua_State* L, int idx, int def);
uint8_t arg_colour(lua_State* L, int idx);
uint8_t opt_colour(lua_State* L, int idx, uint8_t def);

}  // namespace mother
