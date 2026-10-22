#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mother {

extern const char* const kUsage;

struct Options {
    std::string command;  // "run"
    std::string cart;     // empty: the built-in palette test pattern
    bool headless = false;
    int64_t ticks = -1;   // stop after this many ticks; -1 = until the window closes
};

struct ParseResult {
    bool ok = false;
    std::string error;
    Options opts;
};

// Parses the command line without argv[0].
ParseResult parse_options(const std::vector<std::string>& args);

}  // namespace mother
