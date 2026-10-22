#include "gfx/framebuffer.h"

#include <algorithm>

namespace mother {

namespace {

bool on_screen(int x, int y) {
    return x >= 0 && y >= 0 && x < kScreenW && y < kScreenH;
}

}  // namespace

void Framebuffer::clear(uint8_t c) {
    px_.fill(uint8_t(c & kColourMask));
}

void Framebuffer::pset(int x, int y, uint8_t c) {
    if (on_screen(x, y)) px_[size_t(y * kScreenW + x)] = uint8_t(c & kColourMask);
}

uint8_t Framebuffer::pget(int x, int y) const {
    return on_screen(x, y) ? px_[size_t(y * kScreenW + x)] : 0;
}

void Framebuffer::rectfill(int x, int y, int w, int h, uint8_t c) {
    const int x0 = std::max(x, 0), y0 = std::max(y, 0);
    const int x1 = std::min(x + w, kScreenW), y1 = std::min(y + h, kScreenH);
    if (x0 >= x1 || y0 >= y1) return;
    const uint8_t v = uint8_t(c & kColourMask);
    for (int row = y0; row < y1; ++row) {
        auto line = px_.begin() + row * kScreenW;
        std::fill(line + x0, line + x1, v);
    }
}

void Framebuffer::rect(int x, int y, int w, int h, uint8_t c) {
    if (w <= 0 || h <= 0) return;
    rectfill(x, y, w, 1, c);
    rectfill(x, y + h - 1, w, 1, c);
    rectfill(x, y, 1, h, c);
    rectfill(x + w - 1, y, 1, h, c);
}

}  // namespace mother
