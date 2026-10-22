#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mother {

extern const char* const kUsage;

enum class InputKind { Live, Fuzz, Trace };  // Live: keyboard in a window, neutral headless

struct Options {
    std::string command;  // run | record | replay | import | matrix | rogue
    std::string cart;     // run: empty for the built-in palette test pattern; matrix, rogue: the carts folder
    std::vector<std::string> imports;  // import: 1-3 SOURCE:EXPORT
    std::string out = "matrix.md";     // matrix: the table's file
    int seeds = 3;                     // matrix: fuzz seeds per compatible cell
    std::string trace;    // record: the trace to write; replay: the trace to check
    bool headless = false;
    int64_t ticks = -1;   // stop after this many ticks; -1 = no limit
    bool has_seed = false;
    uint64_t seed = 0;
    InputKind input = InputKind::Live;
    uint64_t fuzz_seed = 0;
    std::string input_trace;
    std::string wav;      // headless: also write the rendered audio here
    bool print_hashes = false;  // replay --hash
};

struct ParseResult {
    bool ok = false;
    std::string error;
    Options opts;
};

// Parses the command line without argv[0].
ParseResult parse_options(const std::vector<std::string>& args);

}  // namespace mother
