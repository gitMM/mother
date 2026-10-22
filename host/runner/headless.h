#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include "runner/input_source.h"
#include "runner/program.h"

namespace mother {

// Receives each tick's rendered audio: kTickFrames interleaved stereo frames (count samples).
using AudioSink = std::function<void(const int16_t* samples, size_t count)>;

struct HeadlessRun {
    uint64_t ticks = 0;
    InputSource* input = nullptr;  // null: a neutral pad
    AudioSink audio;               // optional
    TickObserver observe;          // optional
};

// Runs prog for up to run.ticks ticks with no window, no audio device and no wall clock,
// stopping early when it finishes or fails. Each tick updates, renders the tick's audio with a
// synth of its own and draws into a framebuffer that is never presented. The result carries the
// last state hash and a hash of every audio sample.
RunResult run_headless(Program& prog, const HeadlessRun& run);

}  // namespace mother
