#include "input/fuzz.h"

namespace mother {

namespace {

// Directions 1-8 clockwise from up: x and y steps.
constexpr int kDirX[9] = {0, 0, 1, 1, 1, 0, -1, -1, -1};
constexpr int kDirY[9] = {0, -1, -1, 0, 1, 1, 1, 0, -1};

// Idle time between presses, per button A B X Y L R Start.
constexpr int kIdleMin[kFaceButtonCount] = {5, 10, 60, 60, 30, 30, 1200};
constexpr int kIdleMax[kFaceButtonCount] = {60, 120, 600, 600, 300, 300, 3600};

}  // namespace

FuzzInput::FuzzInput(uint64_t seed) : rng_(seed) {
    for (int i = 0; i < kFaceButtonCount; ++i) buttons_[size_t(i)].left = duration(0, kIdleMax[i]);
}

RawInput FuzzInput::next() {
    if (dir_left_ == 0) {
        dir_ = rng_.range(0, 2) == 0 ? 0 : int(rng_.range(1, 8));
        dir_left_ = duration(10, 90);
    }
    --dir_left_;

    RawInput raw;
    raw.left = kDirX[dir_] < 0;
    raw.right = kDirX[dir_] > 0;
    raw.up = kDirY[dir_] < 0;
    raw.down = kDirY[dir_] > 0;

    for (int i = 0; i < kFaceButtonCount; ++i) {
        ButtonState& b = buttons_[size_t(i)];
        if (b.left == 0) {
            b.down = !b.down;
            if (b.down) {
                b.left = rng_.range(0, 9) < 7 ? duration(1, 6) : duration(15, 60);  // tap or hold
            } else {
                b.left = duration(kIdleMin[i], kIdleMax[i]);
            }
        }
        --b.left;
        raw.buttons[size_t(i)] = b.down;
    }
    return raw;
}

}  // namespace mother
