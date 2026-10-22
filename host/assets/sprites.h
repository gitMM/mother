#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "gfx/palette.h"

namespace mother {

constexpr int kTileSize = 8;                                  // px per side
constexpr int kSheetSize = 256;                               // px per side
constexpr int kSheetCount = 4;                                // sheet 3 is reserved for imports
constexpr int kCartSheets = 3;                                // sheets a cart's sprites.spr defines
constexpr int kAtlasW = kSheetSize;
constexpr int kAtlasH = kSheetSize * kSheetCount;
constexpr int kTilesPerRow = kSheetSize / kTileSize;          // 32
constexpr int kTilesPerSheet = kTilesPerRow * kTilesPerRow;   // 1024
constexpr int kSpriteCount = kTilesPerSheet * kSheetCount;    // 4096
constexpr int kCartTiles = kTilesPerSheet * kCartSheets;      // 3072
constexpr int kMaxRamps = 8;

// Sprite memory: the four 256x256 sheets stacked into one 256x1024 atlas, so sheet s starts at
// y = 256 * s and 8x8 sprite n has its top-left pixel at ((n % 32) * 8, (n / 32) * 8).
struct SpriteSheets {
    std::vector<uint8_t> px = std::vector<uint8_t>(size_t(kAtlasW) * kAtlasH, 0);

    static bool contains(int x, int y) { return x >= 0 && y >= 0 && x < kAtlasW && y < kAtlasH; }

    uint8_t get(int x, int y) const { return contains(x, y) ? px[size_t(y) * kAtlasW + size_t(x)] : 0; }

    void set(int x, int y, uint8_t c) {
        if (contains(x, y)) px[size_t(y) * kAtlasW + size_t(x)] = uint8_t(c & (kPaletteSize - 1));
    }
};

// Palette ramps: up to 8 ordered lists of palette indices (fog, fades, dithered lighting).
using Ramps = std::array<std::vector<uint8_t>, kMaxRamps>;

}  // namespace mother
