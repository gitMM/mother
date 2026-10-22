#include "gfx/framebuffer.h"
#include "test.h"

using namespace mother;

namespace {

int count(const Framebuffer& fb, uint8_t c) {
    int n = 0;
    for (int i = 0; i < kScreenW * kScreenH; ++i) n += fb.data()[i] == c ? 1 : 0;
    return n;
}

}  // namespace

TEST(framebuffer_clear_and_pset) {
    Framebuffer fb;
    fb.clear(5);
    CHECK_EQ(fb.pget(0, 0), 5);
    CHECK_EQ(fb.pget(319, 179), 5);
    fb.pset(10, 20, 7);
    CHECK_EQ(fb.pget(10, 20), 7);
    CHECK_EQ(count(fb, 7), 1);
}

TEST(framebuffer_ignores_out_of_bounds) {
    Framebuffer fb;
    fb.clear(1);
    fb.pset(-1, 0, 9);
    fb.pset(320, 0, 9);
    fb.pset(0, -1, 9);
    fb.pset(0, 180, 9);
    CHECK_EQ(count(fb, 9), 0);
    CHECK_EQ(fb.pget(-1, 0), 0);
    CHECK_EQ(fb.pget(320, 179), 0);
}

TEST(framebuffer_masks_colours_to_palette) {
    Framebuffer fb;
    fb.pset(0, 0, 67);
    CHECK_EQ(fb.pget(0, 0), 3);
    fb.clear(255);
    CHECK_EQ(fb.pget(5, 5), 63);
}

TEST(framebuffer_rectfill_clips) {
    Framebuffer fb;
    fb.clear(0);
    fb.rectfill(-5, -5, 10, 10, 2);
    CHECK_EQ(count(fb, 2), 25);
    fb.rectfill(315, 175, 10, 10, 4);
    CHECK_EQ(count(fb, 4), 25);
    fb.rectfill(0, 0, 0, 5, 6);
    fb.rectfill(400, 0, 5, 5, 6);
    CHECK_EQ(count(fb, 6), 0);
}

TEST(framebuffer_rect_draws_outline) {
    Framebuffer fb;
    fb.clear(0);
    fb.rect(10, 10, 4, 3, 8);
    CHECK_EQ(count(fb, 8), 10);
    CHECK_EQ(fb.pget(10, 10), 8);
    CHECK_EQ(fb.pget(13, 12), 8);
    CHECK_EQ(fb.pget(11, 11), 0);
}
