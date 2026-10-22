#pragma once

#include <cstdint>

#include "runner/program.h"

namespace mother {

// Runs prog for up to `ticks` ticks with no window, no audio device and no wall clock, stopping
// early when it finishes or fails. Each tick updates and then draws into a framebuffer that is
// never presented. Input is neutral until fuzz and trace input arrive (M4).
RunResult run_headless(Program& prog, uint64_t ticks);

}  // namespace mother
