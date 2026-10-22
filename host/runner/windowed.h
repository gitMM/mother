#pragma once

#include <cstdint>

#include "runner/program.h"

namespace mother {

// Opens the window and runs prog at a fixed 60 Hz, rendering once per display frame, until the
// window closes, prog finishes or fails, or max_ticks ticks have run (-1 = no limit).
RunResult run_windowed(Program& prog, int64_t max_ticks);

}  // namespace mother
