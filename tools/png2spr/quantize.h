#pragma once

#include <cstdint>

namespace mother {

struct OkLab {
    double L, a, b;
};

// sRGB (0-255 per channel) to OKLab (Björn Ottosson's matrices).
OkLab to_oklab(uint8_t r, uint8_t g, uint8_t b);

// The palette index 1-63 nearest to an sRGB colour in OKLab (squared Euclidean distance; the
// lower index wins a tie). Index 0 is never returned: it is transparent in sprites.
uint8_t nearest_index(uint8_t r, uint8_t g, uint8_t b);

}  // namespace mother
