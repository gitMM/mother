#include "gfx/present.h"

#include <algorithm>
#include <cmath>

namespace mother {

Viewport fit_viewport(int out_w, int out_h) {
    const int scale = std::max(1, std::min(out_w / kScreenW, out_h / kScreenH));
    const int w = kScreenW * scale, h = kScreenH * scale;
    return {(out_w - w) / 2, (out_h - h) / 2, w, h, scale};
}

bool output_to_screen(const Viewport& vp, float ox, float oy, int& sx, int& sy) {
    const float fx = std::floor((ox - float(vp.x)) / float(vp.scale));
    const float fy = std::floor((oy - float(vp.y)) / float(vp.scale));
    if (!(fx >= 0.0f && fx < float(kScreenW) && fy >= 0.0f && fy < float(kScreenH))) return false;
    sx = int(fx);
    sy = int(fy);
    return true;
}

void to_argb(const Framebuffer& fb, uint32_t* out) {
    const uint8_t* px = fb.data();
    for (int i = 0; i < kScreenW * kScreenH; ++i) out[i] = kPalette[px[i]];
}

}  // namespace mother
