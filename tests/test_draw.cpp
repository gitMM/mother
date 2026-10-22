#include "gfx/draw.h"
#include "test.h"

using namespace mother;

namespace {

int count(const Framebuffer& fb, uint8_t c) {
    int n = 0;
    for (int i = 0; i < kScreenW * kScreenH; ++i) n += fb.data()[i] == c ? 1 : 0;
    return n;
}

}  // namespace

TEST(draw_only_between_begin_and_end) {
    Painter p;
    CHECK(!p.drawing());
    Framebuffer fb;
    p.begin(fb);
    CHECK(p.drawing());
    p.end();
    CHECK(!p.drawing());
}

TEST(draw_pset_uses_camera_clip_and_palette) {
    Framebuffer fb;
    Painter p;
    p.begin(fb);
    p.cls(0);
    p.camera(10, 20);
    p.pset(15, 25, 8);
    CHECK_EQ(fb.pget(5, 5), 8);
    CHECK_EQ(p.pget(15, 25), 8);  // pget goes through the camera too
    p.camera(0, 0);
    p.pal(8, 9);
    p.pset(1, 1, 8);
    CHECK_EQ(fb.pget(1, 1), 9);
    CHECK_EQ(p.pget(1, 1), 9);  // pget returns the stored index
    p.pal_reset();
    p.clip(100, 100, 10, 10);
    p.pset(99, 100, 7);
    p.pset(100, 100, 7);
    p.pset(110, 100, 7);
    CHECK_EQ(count(fb, 7), 1);
    CHECK_EQ(fb.pget(100, 100), 7);
}

TEST(draw_cls_ignores_camera_clip_and_palette) {
    Framebuffer fb;
    Painter p;
    p.begin(fb);
    p.camera(50, 50);
    p.clip(0, 0, 10, 10);
    p.pal(3, 4);
    p.cls(3);
    CHECK_EQ(count(fb, 3), kScreenW * kScreenH);
}

TEST(draw_line_includes_both_endpoints) {
    Framebuffer fb;
    Painter p;
    p.begin(fb);
    p.cls(0);
    p.line(2, 2, 6, 2, 5);
    CHECK_EQ(count(fb, 5), 5);
    p.line(10, 10, 13, 13, 6);
    CHECK_EQ(count(fb, 6), 4);
    CHECK_EQ(fb.pget(13, 13), 6);
    p.line(30, 33, 27, 30, 7);  // reversed direction
    CHECK_EQ(count(fb, 7), 4);
    CHECK_EQ(fb.pget(27, 30), 7);
    p.line(50, 50, 50, 50, 8);
    CHECK_EQ(count(fb, 8), 1);
}

TEST(draw_rect_and_rectfill_take_width_and_height) {
    Framebuffer fb;
    Painter p;
    p.begin(fb);
    p.cls(0);
    p.rect(10, 10, 4, 3, 8);
    CHECK_EQ(count(fb, 8), 10);
    CHECK_EQ(fb.pget(11, 11), 0);
    p.rectfill(20, 20, 4, 3, 9);
    CHECK_EQ(count(fb, 9), 12);
    p.rect(40, 40, 0, 5, 10);
    p.rectfill(40, 40, 5, -1, 10);
    CHECK_EQ(count(fb, 10), 0);
}

TEST(draw_circles) {
    Framebuffer fb;
    Painter p;
    p.begin(fb);
    p.cls(0);
    p.circ(50, 50, 0, 4);
    CHECK_EQ(count(fb, 4), 1);
    p.circ(100, 100, 3, 5);
    CHECK_EQ(fb.pget(103, 100), 5);
    CHECK_EQ(fb.pget(100, 97), 5);
    CHECK_EQ(fb.pget(100, 100), 0);
    p.circfill(200, 100, 3, 6);
    CHECK_EQ(fb.pget(200, 100), 6);
    CHECK_EQ(fb.pget(203, 100), 6);
    CHECK_EQ(fb.pget(203, 103), 0);
    p.circ(10, 10, -1, 7);
    CHECK_EQ(count(fb, 7), 0);
}

TEST(draw_clip_limits_fills) {
    Framebuffer fb;
    Painter p;
    p.begin(fb);
    p.cls(0);
    p.clip(10, 10, 5, 5);
    p.rectfill(0, 0, kScreenW, kScreenH, 2);
    p.circfill(12, 12, 50, 2);
    CHECK_EQ(count(fb, 2), 25);
    p.clip(10, 10, -5, 5);  // empty
    p.rectfill(0, 0, kScreenW, kScreenH, 3);
    CHECK_EQ(count(fb, 3), 0);
    p.clip_reset();
    p.rectfill(0, 0, kScreenW, kScreenH, 3);
    CHECK_EQ(count(fb, 3), kScreenW * kScreenH);
}

TEST(draw_extreme_coordinates_stay_bounded) {
    Framebuffer fb;
    Painter p;
    p.begin(fb);
    p.cls(0);
    p.line(-32768, -32768, 32767, 32767, 1);
    CHECK_EQ(fb.pget(0, 0), 1);
    CHECK_EQ(fb.pget(179, 179), 1);
    p.circfill(160, 90, 32767, 2);
    CHECK_EQ(count(fb, 2), kScreenW * kScreenH);
}
