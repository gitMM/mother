#include <algorithm>
#include <cstdint>

#include "gfx/draw.h"

namespace mother {

namespace {

int floor_div(int a, int b) {
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}

}  // namespace

void Painter::palt(uint8_t c, bool transparent) {
    transparent_[c & kColourMask] = transparent;
}

void Painter::palt_reset() {
    transparent_.fill(false);
    transparent_[0] = true;
}

void Painter::spr(int n, int x, int y, int w, int h, bool flip_x, bool flip_y) {
    if (n < 0 || n >= kSpriteCount) return;
    blit((n % kTilesPerRow) * kTileSize, (n / kTilesPerRow) * kTileSize, w * kTileSize, h * kTileSize,
         x - cam_x_, y - cam_y_, flip_x, flip_y);
}

void Painter::sspr(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh) {
    if (!sheets_ || sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) return;
    dx -= cam_x_;
    dy -= cam_y_;
    const int i0 = std::max(0, clip_x0_ - dx), i1 = std::min(dw, clip_x1_ - dx);
    const int j0 = std::max(0, clip_y0_ - dy), j1 = std::min(dh, clip_y1_ - dy);
    for (int j = j0; j < j1; ++j) {
        const int src_y = sy + int(int64_t(j) * sh / dh);
        for (int i = i0; i < i1; ++i) {
            const int src_x = sx + int(int64_t(i) * sw / dw);
            if (!SpriteSheets::contains(src_x, src_y)) continue;
            const uint8_t c = sheets_->get(src_x, src_y);
            if (!transparent_[c]) fb_->pset(dx + i, dy + j, pal_[c]);
        }
    }
}

void Painter::map(int cx, int cy, int sx, int sy, int cw, int ch, int layer) {
    if (!map_ || cw <= 0 || ch <= 0) return;
    // Only the cells that can touch the clip rectangle are visited.
    const int x0 = sx - cam_x_, y0 = sy - cam_y_;
    const int i0 = std::max(0, floor_div(clip_x0_ - x0, kTileSize));
    const int i1 = std::min(cw, floor_div(clip_x1_ - 1 - x0, kTileSize) + 1);
    const int j0 = std::max(0, floor_div(clip_y0_ - y0, kTileSize));
    const int j1 = std::min(ch, floor_div(clip_y1_ - 1 - y0, kTileSize) + 1);
    for (int j = j0; j < j1; ++j) {
        for (int i = i0; i < i1; ++i) {
            const int tile = map_->get(layer, cx + i, cy + j);
            if (tile == 0) continue;
            blit((tile % kTilesPerRow) * kTileSize, (tile / kTilesPerRow) * kTileSize, kTileSize, kTileSize,
                 x0 + i * kTileSize, y0 + j * kTileSize, false, false);
        }
    }
}

void Painter::blit(int ax, int ay, int w, int h, int sx, int sy, bool flip_x, bool flip_y) {
    if (!sheets_ || w <= 0 || h <= 0) return;
    const int i0 = std::max(0, clip_x0_ - sx), i1 = std::min(w, clip_x1_ - sx);
    const int j0 = std::max(0, clip_y0_ - sy), j1 = std::min(h, clip_y1_ - sy);
    for (int j = j0; j < j1; ++j) {
        const int src_y = ay + (flip_y ? h - 1 - j : j);
        for (int i = i0; i < i1; ++i) {
            const int src_x = ax + (flip_x ? w - 1 - i : i);
            if (!SpriteSheets::contains(src_x, src_y)) continue;
            const uint8_t c = sheets_->get(src_x, src_y);
            if (!transparent_[c]) fb_->pset(sx + i, sy + j, pal_[c]);
        }
    }
}

}  // namespace mother
