#include "quantize.h"

#include <array>
#include <cmath>

#include "gfx/palette.h"

namespace mother {

namespace {

double linear(uint8_t c) {
    const double v = c / 255.0;
    return v <= 0.04045 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
}

const std::array<OkLab, kPaletteSize>& palette_oklab() {
    static const std::array<OkLab, kPaletteSize> table = [] {
        std::array<OkLab, kPaletteSize> t{};
        for (int i = 0; i < kPaletteSize; ++i) {
            const uint32_t c = kPalette[size_t(i)];
            t[size_t(i)] = to_oklab(uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c));
        }
        return t;
    }();
    return table;
}

}  // namespace

OkLab to_oklab(uint8_t r8, uint8_t g8, uint8_t b8) {
    const double r = linear(r8), g = linear(g8), b = linear(b8);
    const double l = std::cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b);
    const double m = std::cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b);
    const double s = std::cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b);
    return {0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
            1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
            0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s};
}

uint8_t nearest_index(uint8_t r, uint8_t g, uint8_t b) {
    const OkLab c = to_oklab(r, g, b);
    const std::array<OkLab, kPaletteSize>& pal = palette_oklab();
    uint8_t best = 1;
    double best_d = -1.0;
    for (int i = 1; i < kPaletteSize; ++i) {
        const OkLab& p = pal[size_t(i)];
        const double d = (c.L - p.L) * (c.L - p.L) + (c.a - p.a) * (c.a - p.a) + (c.b - p.b) * (c.b - p.b);
        if (best_d < 0.0 || d < best_d) {
            best = uint8_t(i);
            best_d = d;
        }
    }
    return best;
}

}  // namespace mother
