#include "assets/tilemap.h"

#include "runtime/hash.h"

namespace mother {

namespace {

uint64_t cell_key(int layer, int x, int y, uint16_t tile) {
    return mix64((uint64_t(layer) << 48) | (uint64_t(x) << 32) | (uint64_t(y) << 16) | tile);
}

uint64_t flag_key(int tile, uint8_t value) {
    return mix64((uint64_t(0xF1A6) << 48) | (uint64_t(tile) << 8) | value);
}

}  // namespace

void TileMap::set(int layer, int x, int y, uint16_t tile) {
    if (layer < 0 || layer >= kMapLayers) return;
    TileLayer& l = layers[size_t(layer)];
    if (x < 0 || y < 0 || x >= l.w || y >= l.h) return;
    uint16_t& cell = l.cells[size_t(y) * size_t(l.w) + size_t(x)];
    hash ^= cell_key(layer, x, y, cell) ^ cell_key(layer, x, y, tile);
    cell = tile;
}

void TileMap::set_flags(int tile, uint8_t value) {
    uint8_t& f = flags[size_t(tile)];
    hash ^= flag_key(tile, f) ^ flag_key(tile, value);
    f = value;
}

void TileMap::rehash() {
    hash = 0;
    for (int n = 0; n < kMapLayers; ++n) {
        const TileLayer& l = layers[size_t(n)];
        for (int y = 0; y < l.h; ++y) {
            for (int x = 0; x < l.w; ++x) hash ^= cell_key(n, x, y, l.cells[size_t(y) * size_t(l.w) + size_t(x)]);
        }
    }
    for (int t = 0; t < kSpriteCount; ++t) hash ^= flag_key(t, flags[size_t(t)]);
}

}  // namespace mother
