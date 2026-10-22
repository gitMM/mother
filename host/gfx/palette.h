#pragma once

#include <array>
#include <cstdint>

namespace mother {

constexpr int kPaletteSize = 64;
constexpr uint8_t kTransparent = 0;  // transparent in sprites and maps
constexpr uint8_t kMotherGold = 63;  // reserved: player avatar, host import outlines

// The fixed palette as 0xAARRGGBB. AAP-64 is a placeholder until the final palette is supplied.
extern const std::array<uint32_t, kPaletteSize> kPalette;

}  // namespace mother
