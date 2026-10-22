#include "runner/headless.h"

#include <array>

#include "audio/synth.h"

namespace mother {

namespace {

constexpr uint64_t kFnvOffset = 14695981039346656037ull;
constexpr uint64_t kFnvPrime = 1099511628211ull;

// FNV-1a over the samples as little-endian bytes, the same on every platform.
void hash_samples(uint64_t& hash, const int16_t* samples, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        const uint16_t s = uint16_t(samples[i]);
        hash = (hash ^ (s & 0xFF)) * kFnvPrime;
        hash = (hash ^ (s >> 8)) * kFnvPrime;
    }
}

}  // namespace

RunResult run_headless(Program& prog, uint64_t ticks, const AudioSink& sink) {
    Framebuffer fb;
    PadState pad;
    const RawInput neutral;
    Synth synth(prog.audio_setup());
    TickAudio commands;
    std::array<int16_t, kTickFrames * 2> samples{};
    uint64_t hash = kFnvOffset;
    uint64_t done = 0;
    while (done < ticks && prog.status().outcome == Outcome::Running) {
        pad = build_pad(neutral, pad);
        prog.update(pad, done);
        ++done;
        if (prog.status().outcome == Outcome::Error) break;
        prog.take_audio(commands);
        synth.render_tick(commands, samples.data());
        hash_samples(hash, samples.data(), samples.size());
        if (sink) sink(samples.data(), samples.size());
        prog.draw(fb);
    }
    return {result_name(prog.status().outcome), done, hash};
}

}  // namespace mother
