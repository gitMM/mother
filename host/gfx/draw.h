#pragma once

#include <array>
#include <cstdint>

#include "gfx/framebuffer.h"

namespace mother {

// The cart's drawing state and primitives (brief section 5). Coordinates are pixels before the
// camera offset; the clip rectangle is in screen pixels; colours go through the draw palette.
// The state persists across frames. Drawing is only possible between begin() and end().
class Painter {
public:
    Painter();

    void begin(Framebuffer& fb) { fb_ = &fb; }
    void end() { fb_ = nullptr; }
    bool drawing() const { return fb_ != nullptr; }

    void camera(int x, int y);
    void clip(int x, int y, int w, int h);
    void clip_reset();
    void pal(uint8_t src, uint8_t dst);
    void pal_reset();

    void cls(uint8_t c);               // the whole screen; ignores camera, clip and palette
    void pset(int x, int y, uint8_t c);
    uint8_t pget(int x, int y) const;  // the stored index; 0 off screen
    void line(int x0, int y0, int x1, int y1, uint8_t c);
    void rect(int x, int y, int w, int h, uint8_t c);
    void rectfill(int x, int y, int w, int h, uint8_t c);
    void circ(int x, int y, int r, uint8_t c);
    void circfill(int x, int y, int r, uint8_t c);

private:
    void plot(int sx, int sy, uint8_t c);            // screen pixel, clipped, palette applied
    void span(int sx0, int sx1, int sy, uint8_t c);  // inclusive screen span, same rules

    Framebuffer* fb_ = nullptr;
    int cam_x_ = 0, cam_y_ = 0;
    int clip_x0_ = 0, clip_y0_ = 0, clip_x1_ = kScreenW, clip_y1_ = kScreenH;  // half-open
    std::array<uint8_t, kPaletteSize> pal_{};
};

}  // namespace mother
