#pragma once

#include <array>
#include <cstdint>

namespace mother {

// Virtual pad buttons. Directions come first so btn()/btnp() treat them like buttons.
// X and Y are reserved for the import system; carts use A, B, L, R.
enum class Button : uint8_t { Left, Right, Up, Down, A, B, X, Y, L, R, Start, Count };

constexpr int kFaceButtonCount = int(Button::Count) - int(Button::A);
constexpr int kAxisMax = 127;          // stick axes are quantised to -127..127
constexpr int kDirThreshold = 64;      // |axis| >= 64 (about 0.5) sets the 8-way direction

constexpr int face_index(Button b) { return int(b) - int(Button::A); }
constexpr uint16_t button_bit(Button b) { return uint16_t(1u << int(b)); }

// Host input for one tick before mapping: keyboard and gamepad already merged.
struct RawInput {
    int8_t axis_x = 0, axis_y = 0;  // analog stick, y positive = down
    bool left = false, right = false, up = false, down = false;  // digital (keys, d-pad)
    std::array<bool, kFaceButtonCount> buttons{};                 // A, B, X, Y, L, R, Start
};

// The virtual pad as the simulation sees it on one tick.
struct PadState {
    int8_t stick_x = 0, stick_y = 0;  // -127..127, y positive = down
    uint16_t held = 0;                // one bit per Button
    uint16_t pressed = 0;             // bits that went down this tick

    bool btn(Button b) const { return (held & button_bit(b)) != 0; }
    bool btnp(Button b) const { return (pressed & button_bit(b)) != 0; }
};

// A digital direction overrides its analog axis when exactly one of the pair is held.
PadState build_pad(const RawInput& raw, const PadState& prev);

}  // namespace mother
