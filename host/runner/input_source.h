#pragma once

#include <cstdint>
#include <functional>

#include "input/fuzz.h"
#include "input/pad.h"
#include "runner/trace.h"

namespace mother {

// Where a run's pad input comes from, one tick at a time. Without a source, windowed runs read
// the keyboard and gamepad and headless runs get a neutral pad.
class InputSource {
public:
    virtual ~InputSource() = default;
    virtual PadState next(const PadState& prev) = 0;
};

class FuzzSource : public InputSource {
public:
    explicit FuzzSource(uint64_t seed) : fuzz_(seed) {}
    PadState next(const PadState& prev) override { return build_pad(fuzz_.next(), prev); }

private:
    FuzzInput fuzz_;
};

// Plays back a trace's pads; after its last tick the pad is neutral.
class TraceSource : public InputSource {
public:
    explicit TraceSource(const Trace& trace) : trace_(trace) {}
    PadState next(const PadState& prev) override {
        if (at_ >= trace_.ticks.size()) return build_pad(RawInput{}, prev);
        PadState pad = trace_.ticks[at_++].pad;
        pad.pressed = uint16_t(pad.held & ~prev.held);
        return pad;
    }

private:
    const Trace& trace_;
    size_t at_ = 0;
};

// Called after every completed tick (1-based) with that tick's pad and state hash.
using TickObserver = std::function<void(uint64_t tick, const PadState& pad, uint64_t state_hash)>;

}  // namespace mother
