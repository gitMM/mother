#pragma once

#include "runner/program.h"

namespace mother {

// Built-in palette test pattern, shown by `mother run` with no cart. Top 160 px: the 64
// palette colours as 16 x 4 swatches of 20 x 40 px, index = row * 16 + column. Bottom
// 20 px: the virtual pad (stick, 8-way direction, A B X Y L R Start) and a dot that
// moves one pixel per tick along the last row.
class TestPattern : public Program {
public:
    static constexpr int kSwatchW = 20, kSwatchH = 40, kSwatchCols = 16;
    static constexpr int kButtonX = 48, kButtonY = 165, kButtonSize = 10, kButtonStep = 14;

    void update(const PadState& pad, uint64_t tick) override;
    void draw(Framebuffer& fb) override;

private:
    void draw_pad(Framebuffer& fb) const;

    PadState pad_;
    uint64_t tick_ = 0;
};

}  // namespace mother
