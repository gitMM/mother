#pragma once

#include <cstdint>

#include <SDL3/SDL_gamepad.h>

#include "input/pad.h"

namespace mother {

// Reads the keyboard (brief section 3 key map, by physical key position) and an optional
// gamepad into RawInput. keys is SDL_GetKeyboardState(); gamepad may be null.
RawInput read_raw_input(const bool* keys, SDL_Gamepad* gamepad);

// Applies the stick deadzone and scales an SDL axis value to -127..127.
int8_t quantise_axis(int16_t value);

}  // namespace mother
