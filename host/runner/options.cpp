#include "runner/options.h"

#include <charconv>
#include <utility>

namespace mother {

const char* const kUsage =
    "usage: mother run [cart] [--headless] [--ticks N] [--wav FILE]\n"
    "  no cart      show the built-in palette test pattern\n"
    "  --headless   no window or audio device; requires --ticks; prints a hash of the audio\n"
    "  --ticks N    stop after N ticks\n"
    "  --wav FILE   with --headless, also write the audio to FILE (48 kHz, 16-bit stereo)\n";

namespace {

ParseResult fail(std::string error) {
    ParseResult r;
    r.error = std::move(error);
    return r;
}

bool parse_count(const std::string& s, int64_t& out) {
    const char* end = s.data() + s.size();
    auto [ptr, ec] = std::from_chars(s.data(), end, out);
    return ec == std::errc() && ptr == end && out >= 0;
}

}  // namespace

ParseResult parse_options(const std::vector<std::string>& args) {
    if (args.empty()) return fail("missing command");
    if (args[0] != "run") return fail("unknown command: " + args[0]);

    ParseResult r;
    r.opts.command = args[0];
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& a = args[i];
        if (a == "--headless") {
            r.opts.headless = true;
        } else if (a == "--ticks") {
            if (++i == args.size() || !parse_count(args[i], r.opts.ticks)) {
                return fail("--ticks needs a non-negative integer");
            }
        } else if (a == "--wav") {
            if (++i == args.size() || args[i].empty()) return fail("--wav needs a file name");
            r.opts.wav = args[i];
        } else if (a.starts_with("--")) {
            return fail("unknown option: " + a);
        } else if (r.opts.cart.empty()) {
            r.opts.cart = a;
        } else {
            return fail("unexpected argument: " + a);
        }
    }
    if (r.opts.headless && r.opts.ticks < 0) return fail("--headless needs --ticks N");
    if (!r.opts.wav.empty() && !r.opts.headless) return fail("--wav needs --headless");
    r.ok = true;
    return r;
}

}  // namespace mother
