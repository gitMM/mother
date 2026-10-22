#include "gfx/draw.h"

#include <algorithm>
#include <cstdlib>

#include "gfx/font.h"

namespace mother {

Painter::Painter() {
    pal_reset();
    palt_reset();
}

void Painter::camera(int x, int y) {
    cam_x_ = x;
    cam_y_ = y;
}

void Painter::clip(int x, int y, int w, int h) {
    clip_x0_ = std::clamp(x, 0, kScreenW);
    clip_y0_ = std::clamp(y, 0, kScreenH);
    clip_x1_ = std::clamp(x + w, clip_x0_, kScreenW);
    clip_y1_ = std::clamp(y + h, clip_y0_, kScreenH);
}

void Painter::clip_reset() {
    clip(0, 0, kScreenW, kScreenH);
}

void Painter::pal(uint8_t src, uint8_t dst) {
    pal_[src & kColourMask] = uint8_t(dst & kColourMask);
}

void Painter::pal_reset() {
    for (int i = 0; i < kPaletteSize; ++i) pal_[size_t(i)] = uint8_t(i);
}

void Painter::cls(uint8_t c) {
    fb_->clear(c);
}

void Painter::pset(int x, int y, uint8_t c) {
    plot(x - cam_x_, y - cam_y_, c);
}

uint8_t Painter::pget(int x, int y) const {
    return fb_->pget(x - cam_x_, y - cam_y_);
}

void Painter::line(int x0, int y0, int x1, int y1, uint8_t c) {
    // Bresenham, both endpoints included.
    x0 -= cam_x_, x1 -= cam_x_, y0 -= cam_y_, y1 -= cam_y_;
    const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        plot(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) err += dy, x0 += sx;
        if (e2 <= dx) err += dx, y0 += sy;
    }
}

void Painter::rect(int x, int y, int w, int h, uint8_t c) {
    if (w <= 0 || h <= 0) return;
    const int sx = x - cam_x_, sy = y - cam_y_;
    span(sx, sx + w - 1, sy, c);
    span(sx, sx + w - 1, sy + h - 1, c);
    const int top = std::max(sy + 1, clip_y0_), bottom = std::min(sy + h - 1, clip_y1_);
    for (int row = top; row < bottom; ++row) {
        plot(sx, row, c);
        plot(sx + w - 1, row, c);
    }
}

void Painter::rectfill(int x, int y, int w, int h, uint8_t c) {
    if (w <= 0 || h <= 0) return;
    const int sx = x - cam_x_, sy = y - cam_y_;
    const int top = std::max(sy, clip_y0_), bottom = std::min(sy + h, clip_y1_);
    for (int row = top; row < bottom; ++row) span(sx, sx + w - 1, row, c);
}

void Painter::circ(int x, int y, int r, uint8_t c) {
    // Midpoint circle, one octant mirrored eight ways.
    if (r < 0) return;
    const int cx = x - cam_x_, cy = y - cam_y_;
    int dx = r, dy = 0, err = 1 - r;
    while (dx >= dy) {
        plot(cx + dx, cy + dy, c), plot(cx - dx, cy + dy, c);
        plot(cx + dx, cy - dy, c), plot(cx - dx, cy - dy, c);
        plot(cx + dy, cy + dx, c), plot(cx - dy, cy + dx, c);
        plot(cx + dy, cy - dx, c), plot(cx - dy, cy - dx, c);
        ++dy;
        if (err < 0) {
            err += 2 * dy + 1;
        } else {
            --dx;
            err += 2 * (dy - dx) + 1;
        }
    }
}

void Painter::circfill(int x, int y, int r, uint8_t c) {
    if (r < 0) return;
    const int cx = x - cam_x_, cy = y - cam_y_;
    int dx = r, dy = 0, err = 1 - r;
    while (dx >= dy) {
        span(cx - dx, cx + dx, cy + dy, c), span(cx - dx, cx + dx, cy - dy, c);
        span(cx - dy, cx + dy, cy + dx, c), span(cx - dy, cx + dy, cy - dx, c);
        ++dy;
        if (err < 0) {
            err += 2 * dy + 1;
        } else {
            --dx;
            err += 2 * (dy - dx) + 1;
        }
    }
}

void Painter::print(std::string_view text, int x, int y, uint8_t c) {
    const int left = x - cam_x_;
    int cx = left, cy = y - cam_y_;
    for (const char ch : text) {
        if (ch == '\n') {
            cx = left;
            cy += kGlyphH;
            continue;
        }
        const uint16_t bits = glyph_bits(static_cast<unsigned char>(ch));
        for (int bit = 0; bit < 15; ++bit) {
            if (bits & (1u << bit)) plot(cx + bit % 3, cy + bit / 3, c);
        }
        cx += kGlyphW;
    }
}

void Painter::plot(int sx, int sy, uint8_t c) {
    if (sx >= clip_x0_ && sx < clip_x1_ && sy >= clip_y0_ && sy < clip_y1_) {
        fb_->pset(sx, sy, pal_[c & kColourMask]);
    }
}

void Painter::span(int sx0, int sx1, int sy, uint8_t c) {
    if (sy < clip_y0_ || sy >= clip_y1_) return;
    const int a = std::max(sx0, clip_x0_), b = std::min(sx1, clip_x1_ - 1);
    if (a <= b) fb_->rectfill(a, sy, b - a + 1, 1, pal_[c & kColourMask]);
}

}  // namespace mother
