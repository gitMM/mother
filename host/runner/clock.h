#pragma once

#include <cstdint>

namespace mother {

// Fixed 60 Hz tick accumulator. Time is counted in ns x 60, so one tick is exactly 1e9 units
// and no rounding ever builds up. When more than 4 ticks are owed in one frame, it runs 4,
// keeps the sub-tick remainder and drops the rest.
class TickClock {
public:
    static constexpr int kMaxTicksPerFrame = 4;

    int advance(uint64_t elapsed_ns);  // ticks to run this frame, 0..4

private:
    uint64_t accum_ = 0;
};

}  // namespace mother
