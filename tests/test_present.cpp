#include <vector>

#include "gfx/present.h"
#include "test.h"

using namespace mother;

namespace {

void check_viewport(int out_w, int out_h, int scale, int x, int y) {
    const Viewport v = fit_viewport(out_w, out_h);
    CHECK_EQ(v.scale, scale);
    CHECK_EQ(v.x, x);
    CHECK_EQ(v.y, y);
    CHECK_EQ(v.w, kScreenW * scale);
    CHECK_EQ(v.h, kScreenH * scale);
}

}  // namespace

TEST(present_native_size_is_1x) {
    check_viewport(320, 180, 1, 0, 0);
}

TEST(present_picks_largest_integer_scale) {
    check_viewport(960, 540, 3, 0, 0);
    check_viewport(1920, 1080, 6, 0, 0);
    check_viewport(2560, 1440, 8, 0, 0);
    check_viewport(1000, 600, 3, 20, 30);
}

TEST(present_letterboxes_wide_and_tall_outputs) {
    check_viewport(3440, 1440, 8, 440, 0);
    check_viewport(1080, 1920, 3, 60, 690);
}

TEST(present_never_scales_below_1x) {
    check_viewport(200, 100, 1, -60, -40);
}

TEST(present_maps_output_points_to_screen_pixels) {
    const Viewport vp = fit_viewport(1000, 600);  // 3x at (20, 30)
    int sx = -1, sy = -1;
    CHECK(output_to_screen(vp, 20.0f, 30.0f, sx, sy));
    CHECK_EQ(sx, 0);
    CHECK_EQ(sy, 0);
    CHECK(output_to_screen(vp, 979.9f, 569.9f, sx, sy));
    CHECK_EQ(sx, 319);
    CHECK_EQ(sy, 179);
    CHECK(!output_to_screen(vp, 19.9f, 100.0f, sx, sy));  // left letterbox
    CHECK(!output_to_screen(vp, 500.0f, 570.0f, sx, sy));  // bottom letterbox
}

TEST(present_maps_indices_through_palette) {
    Framebuffer fb;
    fb.clear(0);
    fb.pset(0, 0, 22);
    fb.pset(319, 179, 63);
    std::vector<uint32_t> argb(size_t(kScreenW * kScreenH));
    to_argb(fb, argb.data());
    CHECK_EQ(argb[0], kPalette[22]);
    CHECK_EQ(argb[1], kPalette[0]);
    CHECK_EQ(argb.back(), kPalette[63]);
}
