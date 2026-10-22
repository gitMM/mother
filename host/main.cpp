#include <cstdio>
#include <string>
#include <vector>

#include "runner/commands.h"
#include "runner/options.h"

int main(int argc, char** argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    const mother::ParseResult parsed = mother::parse_options(args);
    if (!parsed.ok) {
        std::fprintf(stderr, "mother: %s\n%s", parsed.error.c_str(), mother::kUsage);
        return 2;
    }
    const mother::Options& opts = parsed.opts;
    if (opts.command == "record") return mother::record_command(opts);
    if (opts.command == "replay") return mother::replay_command(opts);
    if (opts.command == "matrix") return mother::matrix_command(opts);
    return mother::run_command(opts);
}
