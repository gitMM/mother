#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "assets/sprites.h"

namespace mother {

constexpr int kMapLayers = 2;
constexpr int kMaxMapSize = 256;  // tiles per side

struct TileLayer {
    int w = 0, h = 0;
    std::vector<uint16_t> cells;  // row-major tile indices, 0-3071
};

// A cart's tilemap: two layers of up to 256x256 tiles, plus 8 flag bits per tile index. It keeps
// a Zobrist hash of its contents (an XOR of one key per cell and per flag byte) that set() and
// set_flags() update in O(1), so the per-tick state hash can include the tilemap cheaply.
struct TileMap {
    std::array<TileLayer, kMapLayers> layers;
    std::array<uint8_t, kCartTiles> flags{};
    uint64_t hash = 0;  // valid after rehash(); kept current by set() and set_flags()

    // Tile at (x, y); 0 outside the layer or for a layer number other than 0 and 1.
    uint16_t get(int layer, int x, int y) const {
        if (layer < 0 || layer >= kMapLayers) return 0;
        const TileLayer& l = layers[size_t(layer)];
        if (x < 0 || y < 0 || x >= l.w || y >= l.h) return 0;
        return l.cells[size_t(y) * size_t(l.w) + size_t(x)];
    }

    // Sets the tile at (x, y); does nothing outside the layer.
    void set(int layer, int x, int y, uint16_t tile);
    void set_flags(int tile, uint8_t value);  // tile 0-3071
    // Recomputes the hash from scratch, after the layers or flags were filled in directly.
    void rehash();
};

}  // namespace mother
