#include "runner/commands.h"

#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>

#include "audio/wav.h"
#include "runner/headless.h"
#include "runner/test_pattern.h"
#include "runner/windowed.h"
#include "vm/cart_vm.h"

namespace mother {

namespace {

namespace fs = std::filesystem;

std::unique_ptr<CartVM> load_cart(const std::string& dir, uint64_t seed) {
    CartSource source;
    std::string error;
    if (!load_cart_source(dir, source, error)) {
        std::fprintf(stderr, "mother: %s\n", error.c_str());
        return nullptr;
    }
    auto cart = std::make_unique<CartVM>();
    if (!cart->open(std::move(source), seed)) {
        std::fprintf(stderr, "mother: cart error: %s\n", cart->status().message.c_str());
        return nullptr;
    }
    return cart;
}

bool load_trace(const std::string& path, Trace& trace) {
    std::ifstream in(fs::path(path), std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "mother: could not read %s\n", path.c_str());
        return false;
    }
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::string error;
    if (!parse_trace(text, path, trace, error)) {
        std::fprintf(stderr, "mother: %s\n", error.c_str());
        return false;
    }
    return true;
}

// The seed when none is given: 0 headless, so runs repeat; the clock in a window, so games vary.
uint64_t run_seed(const Options& opts) {
    if (opts.has_seed) return opts.seed;
    if (opts.headless) return 0;
    return uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
}

std::string cart_name(const std::string& dir) {
    const fs::path p = fs::path(dir).lexically_normal();
    return (p.has_filename() ? p.filename() : p.parent_path().filename()).string();
}

// Runs prog headless or windowed; ticks < 0 means no limit (windowed only).
RunResult execute(Program& prog, const Options& opts, int64_t ticks, InputSource* input, const TickObserver& observe,
                  bool& io_ok) {
    if (!opts.headless) return run_windowed(prog, {ticks, input, observe});
    HeadlessRun run{uint64_t(ticks), input, nullptr, observe};
    WavWriter wav;
    if (!opts.wav.empty()) {
        if (!wav.open(opts.wav)) {
            std::fprintf(stderr, "mother: could not write %s\n", opts.wav.c_str());
            io_ok = false;
            return {"error", 0};
        }
        run.audio = [&wav](const int16_t* samples, size_t count) { wav.write(samples, count); };
    }
    const RunResult result = run_headless(prog, run);
    if (!opts.wav.empty() && !wav.close()) {
        std::fprintf(stderr, "mother: could not write %s\n", opts.wav.c_str());
        io_ok = false;
    }
    return result;
}

void print_result(const RunResult& r, uint64_t seed, const Options& opts, const Program& prog, const char* extra) {
    std::printf("result=%s ticks=%" PRIu64 " seed=%" PRIu64 " budget=%d hash=%016" PRIx64, r.result, r.ticks, seed,
                r.budget_errors, r.state_hash);
    if (opts.headless) std::printf(" audio=%016" PRIx64, r.audio_hash);
    std::printf("%s\n", extra);
    if (prog.status().outcome == Outcome::Error) {
        std::fprintf(stderr, "mother: cart error: %s\n", prog.status().message.c_str());
    }
}

int exit_code(const RunResult& r, bool io_ok) {
    return std::strcmp(r.result, "error") == 0 || !io_ok ? 1 : 0;
}

}  // namespace

int run_command(const Options& opts) {
    Trace trace;
    std::unique_ptr<InputSource> input;
    uint64_t seed = run_seed(opts);
    int64_t ticks = opts.ticks;
    if (opts.input == InputKind::Trace) {
        if (!load_trace(opts.input_trace, trace)) return 1;
        seed = trace.seed;
        input = std::make_unique<TraceSource>(trace);
        if (ticks < 0) ticks = int64_t(trace.ticks.size());
    } else if (opts.input == InputKind::Fuzz) {
        input = std::make_unique<FuzzSource>(opts.fuzz_seed);
    }

    std::unique_ptr<Program> prog;
    if (opts.cart.empty()) {
        prog = std::make_unique<TestPattern>();
    } else if (!(prog = load_cart(opts.cart, seed))) {
        return 1;
    }
    bool io_ok = true;
    const RunResult r = execute(*prog, opts, ticks, input.get(), nullptr, io_ok);
    print_result(r, seed, opts, *prog, "");
    return exit_code(r, io_ok);
}

int record_command(const Options& opts) {
    const uint64_t seed = run_seed(opts);
    std::unique_ptr<InputSource> input;
    if (opts.input == InputKind::Fuzz) input = std::make_unique<FuzzSource>(opts.fuzz_seed);
    std::unique_ptr<CartVM> cart = load_cart(opts.cart, seed);
    if (!cart) return 1;

    Trace trace;
    trace.cart = cart_name(opts.cart);
    trace.seed = seed;
    const TickObserver observe = [&trace](uint64_t, const PadState& pad, uint64_t hash) {
        trace.ticks.push_back({pad, hash});
    };
    bool io_ok = true;
    const RunResult r = execute(*cart, opts, opts.ticks, input.get(), observe, io_ok);

    const std::string text = write_trace(trace);
    std::ofstream out(fs::path(opts.trace), std::ios::binary | std::ios::trunc);
    if (!out.write(text.data(), std::streamsize(text.size()))) {
        std::fprintf(stderr, "mother: could not write %s\n", opts.trace.c_str());
        io_ok = false;
    }
    print_result(r, seed, opts, *cart, "");
    return exit_code(r, io_ok);
}

int replay_command(const Options& opts) {
    Trace trace;
    if (!load_trace(opts.trace, trace)) return 1;
    std::unique_ptr<CartVM> cart = load_cart(opts.cart, trace.seed);
    if (!cart) return 1;

    TraceSource input(trace);
    uint64_t mismatches = 0, first = 0;
    const TickObserver observe = [&](uint64_t tick, const PadState&, uint64_t hash) {
        if (opts.print_hashes) std::printf("%" PRIu64 " %016" PRIx64 "\n", tick, hash);
        const uint64_t expected = trace.ticks[size_t(tick - 1)].hash;
        if (hash == expected) return;
        if (++mismatches == 1) {
            first = tick;
            std::fprintf(stderr, "mother: replay: tick %" PRIu64 " hashed %016" PRIx64 ", the trace has %016" PRIx64 "\n",
                         tick, hash, expected);
        }
    };
    bool io_ok = true;
    const RunResult r = execute(*cart, opts, int64_t(trace.ticks.size()), &input, observe, io_ok);
    if (r.ticks < trace.ticks.size()) {
        std::fprintf(stderr, "mother: replay: the cart stopped after %" PRIu64 " of %zu ticks\n", r.ticks,
                     trace.ticks.size());
        if (first == 0) first = r.ticks + 1;
        mismatches += trace.ticks.size() - r.ticks;
    }
    char extra[96] = " replay=ok";
    if (mismatches > 0) {
        std::snprintf(extra, sizeof extra, " replay=mismatch first=%" PRIu64 " count=%" PRIu64, first, mismatches);
    }
    print_result(r, trace.seed, opts, *cart, extra);
    return mismatches > 0 ? 1 : exit_code(r, io_ok);
}

}  // namespace mother
