#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "audio/wav.h"
#include "runner/headless.h"
#include "runner/options.h"
#include "runner/test_pattern.h"
#include "runner/windowed.h"
#include "vm/cart_vm.h"

namespace {

mother::RunResult run_headless(mother::Program& prog, const mother::Options& opts, bool& io_ok) {
    if (opts.wav.empty()) return mother::run_headless(prog, uint64_t(opts.ticks));
    mother::WavWriter wav;
    if (!wav.open(opts.wav)) {
        std::fprintf(stderr, "mother: could not write %s\n", opts.wav.c_str());
        io_ok = false;
        return {"error", 0};
    }
    const mother::RunResult r = mother::run_headless(
        prog, uint64_t(opts.ticks), [&wav](const int16_t* samples, size_t count) { wav.write(samples, count); });
    io_ok = wav.close();
    if (!io_ok) std::fprintf(stderr, "mother: could not write %s\n", opts.wav.c_str());
    return r;
}

// Runs prog in the requested mode, prints the result line (with the audio hash when headless)
// and any cart error, and returns the process exit code: 1 for an error, 0 otherwise.
int run(mother::Program& prog, const mother::Options& opts) {
    bool io_ok = true;
    if (opts.headless) {
        const mother::RunResult r = run_headless(prog, opts, io_ok);
        std::printf("result=%s ticks=%" PRIu64 " audio=%016" PRIx64 "\n", r.result, r.ticks, r.audio_hash);
        if (prog.status().outcome == mother::Outcome::Error) {
            std::fprintf(stderr, "mother: cart error: %s\n", prog.status().message.c_str());
        }
        return std::strcmp(r.result, "error") == 0 || !io_ok ? 1 : 0;
    }
    const mother::RunResult r = mother::run_windowed(prog, opts.ticks);
    std::printf("result=%s ticks=%" PRIu64 "\n", r.result, r.ticks);
    if (prog.status().outcome == mother::Outcome::Error) {
        std::fprintf(stderr, "mother: cart error: %s\n", prog.status().message.c_str());
    }
    return std::strcmp(r.result, "error") == 0 ? 1 : 0;
}

}  // namespace

int main(int argc, char** argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    const mother::ParseResult parsed = mother::parse_options(args);
    if (!parsed.ok) {
        std::fprintf(stderr, "mother: %s\n%s", parsed.error.c_str(), mother::kUsage);
        return 2;
    }
    const mother::Options& opts = parsed.opts;

    if (opts.cart.empty()) {
        mother::TestPattern pattern;
        return run(pattern, opts);
    }

    mother::CartSource source;
    std::string error;
    if (!mother::load_cart_source(opts.cart, source, error)) {
        std::fprintf(stderr, "mother: %s\n", error.c_str());
        return 1;
    }
    mother::CartVM cart;
    if (!cart.open(std::move(source))) {
        std::fprintf(stderr, "mother: cart error: %s\n", cart.status().message.c_str());
        return 1;
    }
    return run(cart, opts);
}
