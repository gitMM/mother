#include "runner/headless.h"

namespace mother {

RunResult run_headless(Program& prog, uint64_t ticks) {
    Framebuffer fb;
    PadState pad;
    const RawInput neutral;
    for (uint64_t tick = 0; tick < ticks; ++tick) {
        pad = build_pad(neutral, pad);
        prog.update(pad, tick);
        prog.draw(fb);
    }
    return {"timeout", ticks};
}

}  // namespace mother
