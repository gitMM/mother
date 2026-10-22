#include "gfx/present.h"

#include <algorithm>

namespace mother {

Viewport fit_viewport(int out_w, int out_h) {
    const int scale = std::max(1, std::min(out_w / kScreenW, out_h / kScreenH));
    const int w = kScreenW * scale, h = kScreenH * scale;
    return {(out_w - w) / 2, (out_h - h) / 2, w, h, scale};
}

void to_argb(const Framebuffer& fb, uint32_t* out) {
    const uint8_t* px = fb.data();
    for (int i = 0; i < kScreenW * kScreenH; ++i) out[i] = kPalette[px[i]];
}

}  // namespace mother
