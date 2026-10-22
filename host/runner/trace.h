#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "input/pad.h"

namespace mother {

// A recorded run: the seed and, for every tick, the pad state and the state hash after it.
// Replaying feeds the same pads from the same seed and must reproduce every hash.
//
// Text format, one item per line:
//   mother-trace 1
//   cart NAME            the cart folder's name (informational)
//   seed N               decimal
//   ticks N              number of tick lines that follow
//   HELD SX SY PTR HASH  per tick: held buttons as 3 hex digits (bit per Button), stick x and y
//                        (-127..127), pointer as - or x,y,down (down 0/1), state hash as 16 hex
struct TraceTick {
    PadState pad;  // `pressed` is not stored; replay derives it from the previous tick
    uint64_t hash = 0;
};

struct Trace {
    std::string cart;
    uint64_t seed = 0;
    std::vector<TraceTick> ticks;
};

std::string write_trace(const Trace& trace);
bool parse_trace(std::string_view text, const std::string& filename, Trace& out, std::string& error);

}  // namespace mother
