#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "assets/sprites.h"
#include "assets/tilemap.h"
#include "gfx/framebuffer.h"

namespace mother {

// The cart's drawing state and primitives (brief section 5). Coordinates are pixels before the
// camera offset; the clip rectangle is in screen pixels; colours go through the draw palette,
// and sprite and map pixels are skipped when their source colour is transparent (palt). The
// state persists across frames. Drawing is only possible between begin() and end().
class Painter {
public:
    Painter();

    void begin(Framebuffer& fb) { fb_ = &fb; }
    void end() { fb_ = nullptr; }
    bool drawing() const { return fb_ != nullptr; }
    void set_assets(const SpriteSheets* sheets, const TileMap* map) { sheets_ = sheets, map_ = map; }

    void camera(int x, int y);
    void clip(int x, int y, int w, int h);
    void clip_reset();
    void pal(uint8_t src, uint8_t dst);
    void pal_reset();
    void palt(uint8_t c, bool transparent);
    void palt_reset();  // only colour 0 transparent

    void cls(uint8_t c);               // the whole screen; ignores camera, clip and palette
    void pset(int x, int y, uint8_t c);
    uint8_t pget(int x, int y) const;  // the stored index; 0 off screen
    void line(int x0, int y0, int x1, int y1, uint8_t c);
    void rect(int x, int y, int w, int h, uint8_t c);
    void rectfill(int x, int y, int w, int h, uint8_t c);
    void circ(int x, int y, int r, uint8_t c);
    void circfill(int x, int y, int r, uint8_t c);
    void print(std::string_view text, int x, int y, uint8_t c);  // 4x6 cells; '\n' starts a line

    // Sprite n (0-4095) and the w x h tiles right and down from it in the atlas.
    void spr(int n, int x, int y, int w, int h, bool flip_x, bool flip_y);
    // The atlas rectangle (sx, sy, sw, sh) stretched to dw x dh with nearest-neighbour sampling.
    void sspr(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh);
    // Map cells (cx, cy) to (cx + cw - 1, cy + ch - 1) of a layer, 8 px apart; tile 0 is empty.
    void map(int cx, int cy, int sx, int sy, int cw, int ch, int layer);

private:
    void plot(int sx, int sy, uint8_t c);            // screen pixel, clipped, palette applied
    void span(int sx0, int sx1, int sy, uint8_t c);  // inclusive screen span, same rules
    // Copies a w x h atlas block at (ax, ay) to the screen at (sx, sy), optionally mirrored.
    void blit(int ax, int ay, int w, int h, int sx, int sy, bool flip_x, bool flip_y);

    Framebuffer* fb_ = nullptr;
    const SpriteSheets* sheets_ = nullptr;
    const TileMap* map_ = nullptr;
    int cam_x_ = 0, cam_y_ = 0;
    int clip_x0_ = 0, clip_y0_ = 0, clip_x1_ = kScreenW, clip_y1_ = kScreenH;  // half-open
    std::array<uint8_t, kPaletteSize> pal_{};
    std::array<bool, kPaletteSize> transparent_{};
};

}  // namespace mother
