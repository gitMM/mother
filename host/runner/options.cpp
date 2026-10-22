#include "runner/options.h"

#include <charconv>
#include <utility>

namespace mother {

const char* const kUsage =
    "usage:\n"
    "  mother run [CART] [--headless --ticks N] [--seed S] [--input fuzz:S|trace:FILE] [--wav FILE]\n"
    "  mother record CART OUT.trace [--headless --ticks N] [--seed S] [--input fuzz:S]\n"
    "  mother replay CART TRACE [--hash]\n"
    "\n"
    "  run with no CART shows the built-in palette test pattern\n"
    "  --headless   no window or audio device; needs --ticks unless the input is a trace\n"
    "  --ticks N    stop after N ticks\n"
    "  --seed S     the cart's RNG seed (default: 0 headless, the clock in a window)\n"
    "  --input      fuzz:S plays seeded random input; trace:FILE plays a trace's input and seed\n"
    "  --wav FILE   with --headless, also write the audio (48 kHz, 16-bit stereo)\n"
    "  --hash       replay: print every tick's state hash\n";

namespace {

ParseResult fail(std::string error) {
    ParseResult r;
    r.error = std::move(error);
    return r;
}

template <typename T>
bool parse_number(const std::string& s, T& out) {
    const char* end = s.data() + s.size();
    const auto [ptr, ec] = std::from_chars(s.data(), end, out);
    return ec == std::errc() && ptr == end;
}

}  // namespace

ParseResult parse_options(const std::vector<std::string>& args) {
    if (args.empty()) return fail("missing command");
    ParseResult r;
    Options& o = r.opts;
    o.command = args[0];
    if (o.command != "run" && o.command != "record" && o.command != "replay") {
        return fail("unknown command: " + o.command);
    }

    std::vector<std::string> positional;
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& a = args[i];
        const bool has_value = i + 1 < args.size();
        if (a == "--headless") {
            o.headless = true;
        } else if (a == "--hash") {
            o.print_hashes = true;
        } else if (a == "--ticks") {
            if (!has_value || !parse_number(args[++i], o.ticks) || o.ticks < 0) {
                return fail("--ticks needs a non-negative integer");
            }
        } else if (a == "--seed") {
            if (!has_value || !parse_number(args[++i], o.seed)) return fail("--seed needs a non-negative integer");
            o.has_seed = true;
        } else if (a == "--input") {
            const std::string spec = has_value ? args[++i] : "";
            if (spec.starts_with("fuzz:") && parse_number(spec.substr(5), o.fuzz_seed)) {
                o.input = InputKind::Fuzz;
            } else if (spec.starts_with("trace:") && spec.size() > 6) {
                o.input = InputKind::Trace;
                o.input_trace = spec.substr(6);
            } else {
                return fail("--input needs fuzz:SEED or trace:FILE");
            }
        } else if (a == "--wav") {
            if (!has_value || args[i + 1].empty()) return fail("--wav needs a file name");
            o.wav = args[++i];
        } else if (a.starts_with("--")) {
            return fail("unknown option: " + a);
        } else {
            positional.push_back(a);
        }
    }

    if (o.command == "run") {
        if (positional.size() > 1) return fail("unexpected argument: " + positional[1]);
        if (!positional.empty()) o.cart = positional[0];
    } else {
        if (positional.size() != 2) return fail(o.command + " needs a cart and a trace file");
        o.cart = positional[0];
        o.trace = positional[1];
    }

    if (o.command == "replay") {
        if (o.ticks >= 0 || o.has_seed || o.input != InputKind::Live || !o.wav.empty()) {
            return fail("replay takes its ticks, seed and input from the trace");
        }
        o.headless = true;
        r.ok = true;
        return r;
    }
    if (o.print_hashes) return fail("--hash only goes with replay");
    if (o.command == "record" && o.input == InputKind::Trace) return fail("record can't take a trace as input");
    if (o.input == InputKind::Trace && o.has_seed) return fail("a trace input brings its own seed; drop --seed");
    if (o.headless && o.ticks < 0 && o.input != InputKind::Trace) return fail("--headless needs --ticks N");
    if (!o.wav.empty() && !o.headless) return fail("--wav needs --headless");
    r.ok = true;
    return r;
}

}  // namespace mother
