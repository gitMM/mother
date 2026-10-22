#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include "runner/program.h"

namespace mother {

// Receives each tick's rendered audio: kTickFrames interleaved stereo frames (count samples).
using AudioSink = std::function<void(const int16_t* samples, size_t count)>;

// Runs prog for up to `ticks` ticks with no window, no audio device and no wall clock, stopping
// early when it finishes or fails. Each tick updates, renders the tick's audio with a synth of
// its own, and draws into a framebuffer that is never presented. The result carries a hash of
// every sample; sink, when given, gets the samples too. Input is neutral until M4.
RunResult run_headless(Program& prog, uint64_t ticks, const AudioSink& sink = nullptr);

}  // namespace mother
