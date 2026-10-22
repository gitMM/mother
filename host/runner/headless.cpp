#include "runner/headless.h"

namespace mother {

RunResult run_headless(Program& prog, uint64_t ticks) {
    Framebuffer fb;
    PadState pad;
    const RawInput neutral;
    uint64_t done = 0;
    while (done < ticks && prog.status().outcome == Outcome::Running) {
        pad = build_pad(neutral, pad);
        prog.update(pad, done);
        ++done;
        if (prog.status().outcome == Outcome::Error) break;
        prog.draw(fb);
    }
    return {result_name(prog.status().outcome), done};
}

}  // namespace mother
