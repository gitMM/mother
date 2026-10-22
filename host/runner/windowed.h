#pragma once

#include <cstdint>

#include "runner/input_source.h"
#include "runner/program.h"

namespace mother {

struct WindowedRun {
    int64_t max_ticks = -1;        // -1: no limit
    InputSource* input = nullptr;  // null: keyboard, gamepad and mouse
    TickObserver observe;          // optional
};

// Opens the window and runs prog at a fixed 60 Hz, rendering once per display frame and playing
// its audio, until the window closes, prog finishes or fails, or max_ticks ticks have run.
RunResult run_windowed(Program& prog, const WindowedRun& run);

}  // namespace mother
