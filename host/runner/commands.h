#pragma once

#include "runner/options.h"

namespace mother {

// The mother subcommands (brief section 2). Each prints one result line,
//   result=<win|lose|timeout|error> ticks=N seed=S budget=B hash=H [audio=A] [replay=...]
// with audio only for headless runs, and returns the process exit code: 1 for errors and
// replay mismatches, 0 otherwise.
int run_command(const Options& opts);     // also import: run with opts.imports; exit 3 if refused
int record_command(const Options& opts);  // also writes opts.trace
int replay_command(const Options& opts);  // --hash also prints "TICK HASH" for every tick
int matrix_command(const Options& opts);  // runner/matrix.cpp

}  // namespace mother
