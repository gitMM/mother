#include "input/pad.h"

namespace mother {

namespace {

int8_t axis(bool neg, bool pos, int8_t analog) {
    if (neg != pos) return int8_t(neg ? -kAxisMax : kAxisMax);
    return analog < -kAxisMax ? int8_t(-kAxisMax) : analog;
}

}  // namespace

PadState build_pad(const RawInput& raw, const PadState& prev) {
    PadState pad;
    pad.stick_x = axis(raw.left, raw.right, raw.axis_x);
    pad.stick_y = axis(raw.up, raw.down, raw.axis_y);

    uint16_t held = 0;
    auto set = [&held](Button b, bool on) {
        if (on) held |= button_bit(b);
    };
    set(Button::Left, pad.stick_x <= -kDirThreshold);
    set(Button::Right, pad.stick_x >= kDirThreshold);
    set(Button::Up, pad.stick_y <= -kDirThreshold);
    set(Button::Down, pad.stick_y >= kDirThreshold);
    for (int i = 0; i < kFaceButtonCount; ++i) set(Button(int(Button::A) + i), raw.buttons[size_t(i)]);

    pad.held = held;
    pad.pressed = uint16_t(held & ~prev.held);
    return pad;
}

}  // namespace mother
