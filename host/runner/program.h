#pragma once

#include <cstdint>

#include "gfx/framebuffer.h"
#include "input/pad.h"

namespace mother {

// What the host loop drives: the built-in test pattern now, a cart VM from M1.
struct Program {
    virtual ~Program() = default;
    virtual void update(const PadState& pad, uint64_t tick) = 0;  // one fixed 60 Hz tick
    virtual void draw(Framebuffer& fb) = 0;                         // one rendered frame
};

}  // namespace mother
