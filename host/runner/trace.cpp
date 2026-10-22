#include "runner/trace.h"

#include <charconv>
#include <cinttypes>
#include <cstdio>
#include <utility>

#include "assets/text_lines.h"

namespace mother {

namespace {

constexpr const char* kMagic = "mother-trace 1";

bool parse_hex(std::string_view word, uint64_t& out, size_t digits) {
    if (word.size() != digits) return false;
    const auto [ptr, ec] = std::from_chars(word.data(), word.data() + word.size(), out, 16);
    return ec == std::errc() && ptr == word.data() + word.size();
}

bool parse_u64(std::string_view word, uint64_t& out) {
    if (word.empty()) return false;
    const auto [ptr, ec] = std::from_chars(word.data(), word.data() + word.size(), out);
    return ec == std::errc() && ptr == word.data() + word.size();
}

bool parse_tick(std::string_view line, TraceTick& out) {
    const std::vector<std::string_view> w = split_words(line);
    uint64_t held = 0, hash = 0;
    int sx = 0, sy = 0;
    if (w.size() != 5 || !parse_hex(w[0], held, 3) || held >= (1u << int(Button::Count)) || !parse_int(w[1], sx) ||
        !parse_int(w[2], sy) || sx < -kAxisMax || sx > kAxisMax || sy < -kAxisMax || sy > kAxisMax ||
        !parse_hex(w[4], hash, 16)) {
        return false;
    }
    TraceTick t;
    t.pad.held = uint16_t(held);
    t.pad.stick_x = int8_t(sx);
    t.pad.stick_y = int8_t(sy);
    t.hash = hash;
    if (w[3] != "-") {
        const std::string_view ptr = w[3];
        const size_t c1 = ptr.find(',');
        const size_t c2 = c1 == std::string_view::npos ? c1 : ptr.find(',', c1 + 1);
        if (c2 == std::string_view::npos) return false;
        int x = 0, y = 0, down = 0;
        if (!parse_int(ptr.substr(0, c1), x) || !parse_int(ptr.substr(c1 + 1, c2 - c1 - 1), y) ||
            !parse_int(ptr.substr(c2 + 1), down) || x < 0 || x > 32767 || y < 0 || y > 32767 || down < 0 || down > 1) {
            return false;
        }
        t.pad.pointer = true;
        t.pad.pointer_x = int16_t(x);
        t.pad.pointer_y = int16_t(y);
        t.pad.pointer_down = down == 1;
    }
    out = t;
    return true;
}

}  // namespace

std::string write_trace(const Trace& trace) {
    std::string text = std::string(kMagic) + "\ncart " + trace.cart + "\nseed " + std::to_string(trace.seed) +
                       "\nticks " + std::to_string(trace.ticks.size()) + "\n";
    char line[96];
    for (const TraceTick& t : trace.ticks) {
        char pointer[32] = "-";
        if (t.pad.pointer) {
            std::snprintf(pointer, sizeof pointer, "%d,%d,%d", t.pad.pointer_x, t.pad.pointer_y, t.pad.pointer_down ? 1 : 0);
        }
        std::snprintf(line, sizeof line, "%03x %d %d %s %016" PRIx64 "\n", unsigned(t.pad.held), t.pad.stick_x,
                      t.pad.stick_y, pointer, t.hash);
        text += line;
    }
    return text;
}

bool parse_trace(std::string_view text, const std::string& filename, Trace& out, std::string& error) {
    const std::vector<std::string_view> lines = split_lines(text);
    auto fail = [&](size_t line, const std::string& problem) {
        error = filename + ":" + std::to_string(line + 1) + ": " + problem;
        return false;
    };
    if (lines.empty() || lines[0] != kMagic) return fail(0, "not a MOTHER trace (expected `mother-trace 1`)");
    Trace trace;
    uint64_t count = 0;
    if (lines.size() < 4 || !lines[1].starts_with("cart ")) return fail(1, "expected `cart NAME`");
    trace.cart = std::string(lines[1].substr(5));
    if (!lines[2].starts_with("seed ") || !parse_u64(lines[2].substr(5), trace.seed)) return fail(2, "expected `seed N`");
    if (!lines[3].starts_with("ticks ") || !parse_u64(lines[3].substr(6), count)) return fail(3, "expected `ticks N`");
    if (lines.size() != 4 + count) {
        return fail(lines.size(), "expected " + std::to_string(count) + " tick lines, found " + std::to_string(lines.size() - 4));
    }
    trace.ticks.resize(size_t(count));
    for (size_t i = 0; i < count; ++i) {
        if (!parse_tick(lines[4 + i], trace.ticks[i])) {
            return fail(4 + i, "expected `HELD SX SY PTR HASH`, like `010 127 0 - 00ff00ff00ff00ff`");
        }
    }
    out = std::move(trace);
    return true;
}

}  // namespace mother
