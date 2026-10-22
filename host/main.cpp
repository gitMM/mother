#include <cinttypes>
#include <cstdio>
#include <string>
#include <vector>

#include "runner/headless.h"
#include "runner/options.h"
#include "runner/test_pattern.h"
#include "runner/windowed.h"

int main(int argc, char** argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    const mother::ParseResult parsed = mother::parse_options(args);
    if (!parsed.ok) {
        std::fprintf(stderr, "mother: %s\n%s", parsed.error.c_str(), mother::kUsage);
        return 2;
    }
    const mother::Options& opts = parsed.opts;
    if (!opts.cart.empty()) {
        std::fprintf(stderr, "mother: cart loading is not implemented yet (M1)\n");
        return 1;
    }

    mother::TestPattern pattern;
    if (opts.headless) {
        const mother::RunResult r = mother::run_headless(pattern, uint64_t(opts.ticks));
        std::printf("result=%s ticks=%" PRIu64 "\n", r.result, r.ticks);
        return 0;
    }
    return mother::run_windowed(pattern, opts.ticks);
}
