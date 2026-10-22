#include "assets/spr_format.h"

#include <utility>

#include "assets/text_lines.h"

namespace mother {

namespace {

constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr size_t kMaxRampLength = 64;

}  // namespace

char spr_char(uint8_t index) {
    return kAlphabet[index & (kPaletteSize - 1)];
}

int spr_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

bool parse_spr(std::string_view text, const std::string& filename, SprData& out, std::string& error) {
    const std::vector<std::string_view> lines = split_lines(text);
    auto fail = [&](size_t line, const std::string& problem) {
        error = filename + ":" + std::to_string(line + 1) + ": " + problem;
        return false;
    };

    SprData data;
    std::array<bool, kMaxRamps> has_ramp{};
    size_t i = 0;
    while (i < lines.size()) {
        const std::vector<std::string_view> words = split_words(lines[i]);
        if (words.empty()) {
            ++i;
            continue;
        }
        int n = -1;
        if (words[0] == "sheet") {
            if (words.size() != 2 || !parse_uint(words[1], n) || n >= kCartSheets) {
                return fail(i, "expected `sheet N` with N from 0 to 2");
            }
            if (data.defined[size_t(n)]) return fail(i, "sheet " + std::to_string(n) + " is defined twice");
            data.defined[size_t(n)] = true;
            for (int y = 0; y < kSheetSize; ++y) {
                const size_t at = i + 1 + size_t(y);
                if (at >= lines.size()) {
                    return fail(at, "sheet " + std::to_string(n) + " has " + std::to_string(y) +
                                        " rows; it needs 256");
                }
                const std::string_view row = lines[at];
                if (row.size() != size_t(kSheetSize)) {
                    return fail(at, "expected 256 pixel characters, found " + std::to_string(row.size()));
                }
                for (int x = 0; x < kSheetSize; ++x) {
                    const int c = spr_index(row[size_t(x)]);
                    if (c < 0) {
                        return fail(at, "'" + std::string(1, row[size_t(x)]) + "' at column " +
                                            std::to_string(x + 1) + " is not a pixel character (A-Z a-z 0-9 + /)");
                    }
                    data.sheets.set(x, n * kSheetSize + y, uint8_t(c));
                }
            }
            i += 1 + size_t(kSheetSize);
        } else if (words[0] == "ramp") {
            if (words.size() < 3 || words.size() > 2 + kMaxRampLength || !parse_uint(words[1], n) ||
                n >= kMaxRamps) {
                return fail(i, "expected `ramp N i i ...` with N from 0 to 7 and 1 to 64 indices");
            }
            if (has_ramp[size_t(n)]) return fail(i, "ramp " + std::to_string(n) + " is defined twice");
            has_ramp[size_t(n)] = true;
            for (size_t w = 2; w < words.size(); ++w) {
                int index = -1;
                if (!parse_uint(words[w], index) || index >= kPaletteSize) {
                    return fail(i, "ramp entry '" + std::string(words[w]) + "' is not a palette index 0-63");
                }
                data.ramps[size_t(n)].push_back(uint8_t(index));
            }
            ++i;
        } else {
            return fail(i, "expected `sheet N` or `ramp N i i ...`");
        }
    }
    out = std::move(data);
    return true;
}

std::string write_spr(const SprData& data) {
    std::string text;
    for (int n = 0; n < kCartSheets; ++n) {
        if (!data.defined[size_t(n)]) continue;
        text += "sheet " + std::to_string(n) + "\n";
        for (int y = 0; y < kSheetSize; ++y) {
            for (int x = 0; x < kSheetSize; ++x) text += spr_char(data.sheets.get(x, n * kSheetSize + y));
            text += '\n';
        }
    }
    for (int n = 0; n < kMaxRamps; ++n) {
        const std::vector<uint8_t>& ramp = data.ramps[size_t(n)];
        if (ramp.empty()) continue;
        text += "ramp " + std::to_string(n);
        for (uint8_t index : ramp) text += " " + std::to_string(index);
        text += '\n';
    }
    return text;
}

}  // namespace mother
