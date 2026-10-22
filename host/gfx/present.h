#pragma once

#include <cstdint>

#include "gfx/framebuffer.h"

namespace mother {

// Where the framebuffer lands in the output: the largest integer scale that fits (at least 1x),
// centred, with the rest letterboxed in black. x and y go negative when the output is smaller
// than 320x180.
struct Viewport {
    int x, y, w, h, scale;
};

Viewport fit_viewport(int out_w, int out_h);

// Converts indexed pixels to 0xAARRGGBB through the palette. out holds kScreenW * kScreenH.
void to_argb(const Framebuffer& fb, uint32_t* out);

}  // namespace mother
