#include "runner/headless.h"

#include <array>

#include "audio/synth.h"
#include "runtime/hash.h"

namespace mother {

RunResult run_headless(Program& prog, const HeadlessRun& run) {
    Framebuffer fb;
    PadState pad;
    Synth synth(prog.audio_setup());
    TickAudio commands;
    std::array<int16_t, kTickFrames * 2> samples{};
    Fnv64 audio_hash;
    uint64_t done = 0;
    while (done < run.ticks && prog.status().outcome == Outcome::Running) {
        pad = run.input ? run.input->next(pad) : build_pad(RawInput{}, pad);
        prog.update(pad, done);
        ++done;
        if (prog.status().outcome == Outcome::Error) break;
        if (run.observe) run.observe(done, pad, prog.state_hash());
        prog.take_audio(commands);
        synth.render_tick(commands, samples.data());
        for (int16_t s : samples) audio_hash.u16(uint16_t(s));
        if (run.audio) run.audio(samples.data(), samples.size());
        prog.draw(fb);
    }
    return {result_name(prog.status().outcome), done, audio_hash.value(), prog.state_hash(), prog.budget_errors()};
}

}  // namespace mother
