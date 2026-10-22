#include "assets/map_format.h"

#include <utility>

#include "assets/text_lines.h"

namespace mother {

namespace {

int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

}  // namespace

bool parse_map(std::string_view text, const std::string& filename, TileMap& out, std::string& error) {
    const std::vector<std::string_view> lines = split_lines(text);
    auto fail = [&](size_t line, const std::string& problem) {
        error = filename + ":" + std::to_string(line + 1) + ": " + problem;
        return false;
    };

    TileMap map;
    std::array<bool, kMapLayers> has_layer{};
    bool has_flags = false;
    size_t i = 0;
    while (i < lines.size()) {
        const std::vector<std::string_view> words = split_words(lines[i]);
        if (words.empty()) {
            ++i;
            continue;
        }
        if (words[0] == "layer") {
            int n = -1, w = 0, h = 0;
            if (words.size() != 4 || !parse_uint(words[1], n) || n >= kMapLayers || !parse_uint(words[2], w) ||
                !parse_uint(words[3], h) || w < 1 || w > kMaxMapSize || h < 1 || h > kMaxMapSize) {
                return fail(i, "expected `layer N W H` with N 0 or 1 and W, H from 1 to 256");
            }
            if (has_layer[size_t(n)]) return fail(i, "layer " + std::to_string(n) + " is defined twice");
            has_layer[size_t(n)] = true;
            TileLayer& layer = map.layers[size_t(n)];
            layer.w = w;
            layer.h = h;
            layer.cells.assign(size_t(w) * size_t(h), 0);
            for (int y = 0; y < h; ++y) {
                const size_t at = i + 1 + size_t(y);
                if (at >= lines.size()) {
                    return fail(at, "layer " + std::to_string(n) + " has " + std::to_string(y) + " rows; it needs " +
                                        std::to_string(h));
                }
                const std::vector<std::string_view> row = split_words(lines[at]);
                if (row.size() != size_t(w)) {
                    return fail(at, "expected " + std::to_string(w) + " tiles, found " + std::to_string(row.size()));
                }
                for (int x = 0; x < w; ++x) {
                    int tile = -1;
                    if (row[size_t(x)].size() != 4 || !parse_uint(row[size_t(x)], tile) || tile >= kCartTiles) {
                        return fail(at, "tile '" + std::string(row[size_t(x)]) + "' is not 4 digits from 0000 to 3071");
                    }
                    layer.cells[size_t(y) * size_t(w) + size_t(x)] = uint16_t(tile);
                }
            }
            i += 1 + size_t(h);
        } else if (words[0] == "flags" && words.size() == 1) {
            if (has_flags) return fail(i, "flags are defined twice");
            has_flags = true;
            for (int t = 0; t < kCartTiles; ++t) {
                const size_t at = i + 1 + size_t(t);
                if (at >= lines.size()) {
                    return fail(at, "flags has " + std::to_string(t) + " lines; it needs 3072");
                }
                const std::string_view line = lines[at];
                const int hi = line.size() == 2 ? hex_digit(line[0]) : -1;
                const int lo = line.size() == 2 ? hex_digit(line[1]) : -1;
                if (hi < 0 || lo < 0) return fail(at, "expected two hex digits for tile " + std::to_string(t));
                map.flags[size_t(t)] = uint8_t(hi * 16 + lo);
            }
            i += 1 + size_t(kCartTiles);
        } else {
            return fail(i, "expected `layer N W H` or `flags`");
        }
    }
    if (!has_flags) {
        error = filename + ": missing the flags section";
        return false;
    }
    out = std::move(map);
    return true;
}

}  // namespace mother
