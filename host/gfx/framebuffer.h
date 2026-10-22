#pragma once

#include <array>
#include <cstdint>

#include "gfx/palette.h"

namespace mother {

constexpr int kScreenW = 320;
constexpr int kScreenH = 180;
constexpr uint8_t kColourMask = kPaletteSize - 1;  // colours are masked into 0-63

// The 320x180 8-bit indexed framebuffer. All drawing clips to the screen.
class Framebuffer {
public:
    void clear(uint8_t c);
    void pset(int x, int y, uint8_t c);
    uint8_t pget(int x, int y) const;  // 0 outside the screen
    void rectfill(int x, int y, int w, int h, uint8_t c);
    void rect(int x, int y, int w, int h, uint8_t c);  // 1 px outline

    const uint8_t* data() const { return px_.data(); }

private:
    std::array<uint8_t, kScreenW * kScreenH> px_{};
};

}  // namespace mother
