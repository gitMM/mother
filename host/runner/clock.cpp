#include "runner/clock.h"

namespace mother {

namespace {

constexpr uint64_t kTicksPerSecond = 60;
constexpr uint64_t kUnitsPerTick = 1'000'000'000;  // ns x 60 per tick

}  // namespace

int TickClock::advance(uint64_t elapsed_ns) {
    accum_ += elapsed_ns * kTicksPerSecond;
    const uint64_t owed = accum_ / kUnitsPerTick;
    if (owed > uint64_t(kMaxTicksPerFrame)) {
        accum_ %= kUnitsPerTick;
        return kMaxTicksPerFrame;
    }
    accum_ -= owed * kUnitsPerTick;
    return int(owed);
}

}  // namespace mother
