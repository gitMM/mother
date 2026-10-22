#pragma once

#include <charconv>
#include <string_view>
#include <vector>

namespace mother {

// Splits text into lines on '\n', dropping a trailing '\r' so CRLF files read the same.
inline std::vector<std::string_view> split_lines(std::string_view text) {
    std::vector<std::string_view> lines;
    size_t start = 0;
    while (start < text.size()) {
        size_t end = text.find('\n', start);
        if (end == std::string_view::npos) end = text.size();
        std::string_view line = text.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        lines.push_back(line);
        start = end + 1;
    }
    return lines;
}

// Splits a line on runs of spaces.
inline std::vector<std::string_view> split_words(std::string_view line) {
    std::vector<std::string_view> words;
    size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && line[i] == ' ') ++i;
        const size_t start = i;
        while (i < line.size() && line[i] != ' ') ++i;
        if (i > start) words.push_back(line.substr(start, i - start));
    }
    return words;
}

// Parses a non-negative decimal integer that fills the whole word.
inline bool parse_uint(std::string_view word, int& out) {
    if (word.empty() || word.size() > 9) return false;
    for (char c : word) {
        if (c < '0' || c > '9') return false;
    }
    std::from_chars(word.data(), word.data() + word.size(), out);
    return true;
}

}  // namespace mother
