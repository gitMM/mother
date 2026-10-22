#pragma once

#include <array>
#include <cstdint>

#include "input/pad.h"
#include "runtime/rng.h"

namespace mother {

// Seeded, plausible pad input for headless runs (brief section 9): one direction (or none) held
// for 10-90 ticks at a time, and each button pressed in bursts, mostly taps, sometimes holds.
// Start comes rarely; X and Y are pressed too, for the import system. No pointer.
class FuzzInput {
public:
    explicit FuzzInput(uint64_t seed);
    RawInput next();  // this tick's input

private:
    struct ButtonState {
        bool down = false;
        int left = 0;  // ticks until it changes
    };

    int duration(int lo, int hi) { return int(rng_.range(lo, hi)); }

    Rng rng_;
    int dir_ = 0;  // 0 none, 1-8 clockwise from up
    int dir_left_ = 0;
    std::array<ButtonState, kFaceButtonCount> buttons_{};
};

}  // namespace mother
