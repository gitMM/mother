#pragma once

#include <cstdint>
#include <string>

#include "gfx/framebuffer.h"
#include "input/pad.h"

namespace mother {

enum class Outcome : uint8_t { Running, Win, Lose, Error };

struct Status {
    Outcome outcome = Outcome::Running;
    std::string message;  // what went wrong, for Outcome::Error
};

// The result word the runners print; a run that ends while still running is a "timeout".
inline const char* result_name(Outcome outcome) {
    switch (outcome) {
    case Outcome::Win: return "win";
    case Outcome::Lose: return "lose";
    case Outcome::Error: return "error";
    case Outcome::Running: break;
    }
    return "timeout";
}

struct RunResult {
    const char* result;  // result_name(), or "error" when the runner itself failed
    uint64_t ticks;      // ticks run
};

// What the host loop drives: a cart VM, or the built-in test pattern when no cart is given.
struct Program {
    virtual ~Program() = default;
    virtual void update(const PadState& pad, uint64_t tick) = 0;  // one fixed 60 Hz tick
    virtual void draw(Framebuffer& fb) = 0;                         // one rendered frame
    virtual const Status& status() const {
        static const Status running;
        return running;
    }
};

}  // namespace mother
