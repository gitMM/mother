#include <vector>

#include "gfx/present.h"
#include "runner/test_pattern.h"
#include "test.h"

using namespace mother;

namespace {

using TP = TestPattern;

Framebuffer render(const PadState& pad, uint64_t tick) {
    TestPattern tp;
    tp.update(pad, tick);
    Framebuffer fb;
    tp.draw(fb);
    return fb;
}

PadState with_a(bool pressed_this_tick) {
    PadState p;
    p.held = button_bit(Button::A);
    p.pressed = pressed_this_tick ? button_bit(Button::A) : 0;
    return p;
}

}  // namespace

TEST(pattern_swatches_show_every_palette_index) {
    const Framebuffer fb = render(PadState{}, 0);
    std::vector<uint32_t> argb(size_t(kScreenW * kScreenH));
    to_argb(fb, argb.data());
    for (int i = 0; i < kPaletteSize; ++i) {
        const int x0 = (i % TP::kSwatchCols) * TP::kSwatchW, y0 = (i / TP::kSwatchCols) * TP::kSwatchH;
        int wrong = 0;
        for (int y = y0; y < y0 + TP::kSwatchH; ++y) {
            for (int x = x0; x < x0 + TP::kSwatchW; ++x) wrong += fb.pget(x, y) == i ? 0 : 1;
        }
        CHECK_EQ(wrong, 0);
        const int cx = x0 + TP::kSwatchW / 2, cy = y0 + TP::kSwatchH / 2;
        CHECK_EQ(argb[size_t(cy * kScreenW + cx)], kPalette[size_t(i)]);
    }
}

TEST(pattern_swatches_tile_the_top_160_rows) {
    CHECK_EQ(TP::kSwatchCols * TP::kSwatchW, kScreenW);
    CHECK_EQ((kPaletteSize / TP::kSwatchCols) * TP::kSwatchH, 160);
}

TEST(pattern_shows_button_states) {
    const int cx = TP::kButtonX + TP::kButtonSize / 2, cy = TP::kButtonY + TP::kButtonSize / 2;
    CHECK_EQ(render(PadState{}, 0).pget(cx, cy), 0);    // up: outline only
    CHECK_EQ(render(with_a(true), 0).pget(cx, cy), 9);  // pressed this tick: yellow
    CHECK_EQ(render(with_a(false), 0).pget(cx, cy), 22);  // held: white
    // B's box stays an outline while A is held.
    CHECK_EQ(render(with_a(false), 0).pget(cx + TP::kButtonStep, cy), 0);
}

TEST(pattern_tick_marker_moves_one_pixel_per_tick) {
    CHECK_EQ(render(PadState{}, 5).pget(5, kScreenH - 1), 22);
    CHECK_EQ(render(PadState{}, 5).pget(6, kScreenH - 1), 0);
    CHECK_EQ(render(PadState{}, 325).pget(5, kScreenH - 1), 22);
}
