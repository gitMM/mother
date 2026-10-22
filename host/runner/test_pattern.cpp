#include "runner/test_pattern.h"

namespace mother {

namespace {

// AAP-64 indices used by the pattern.
constexpr uint8_t kBlack = 0;
constexpr uint8_t kYellow = 9;
constexpr uint8_t kWhite = 22;
constexpr uint8_t kGrey = 40;

constexpr int kBoxY = 162, kBoxSize = 16;
constexpr int kStickBoxX = 4, kDirBoxX = 24;

}  // namespace

void TestPattern::update(const PadState& pad, uint64_t tick) {
    pad_ = pad;
    tick_ = tick;
}

void TestPattern::draw(Framebuffer& fb) {
    fb.clear(kBlack);
    for (int i = 0; i < kPaletteSize; ++i) {
        const int x = (i % kSwatchCols) * kSwatchW, y = (i / kSwatchCols) * kSwatchH;
        fb.rectfill(x, y, kSwatchW, kSwatchH, uint8_t(i));
    }
    draw_pad(fb);
    fb.pset(int(tick_ % kScreenW), kScreenH - 1, kWhite);
}

void TestPattern::draw_pad(Framebuffer& fb) const {
    // Analog stick: a 2 x 2 dot that travels up to 6 px from the box centre.
    fb.rect(kStickBoxX, kBoxY, kBoxSize, kBoxSize, kGrey);
    const int sx = kStickBoxX + 7 + pad_.stick_x * 6 / kAxisMax;
    const int sy = kBoxY + 7 + pad_.stick_y * 6 / kAxisMax;
    fb.rectfill(sx, sy, 2, 2, kYellow);

    // 8-way direction: one lit cell of a 3 x 3 grid.
    fb.rect(kDirBoxX, kBoxY, kBoxSize, kBoxSize, kGrey);
    const int dx = pad_.btn(Button::Left) ? -1 : pad_.btn(Button::Right) ? 1 : 0;
    const int dy = pad_.btn(Button::Up) ? -1 : pad_.btn(Button::Down) ? 1 : 0;
    fb.rectfill(kDirBoxX + 6 + dx * 5, kBoxY + 6 + dy * 5, 4, 4, kWhite);

    // Buttons: outline when up, white when held, yellow on the tick they go down.
    for (int i = 0; i < kFaceButtonCount; ++i) {
        const Button b = Button(int(Button::A) + i);
        const int x = kButtonX + i * kButtonStep;
        if (pad_.btnp(b)) {
            fb.rectfill(x, kButtonY, kButtonSize, kButtonSize, kYellow);
        } else if (pad_.btn(b)) {
            fb.rectfill(x, kButtonY, kButtonSize, kButtonSize, kWhite);
        } else {
            fb.rect(x, kButtonY, kButtonSize, kButtonSize, kGrey);
        }
    }
}

}  // namespace mother
