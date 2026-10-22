#pragma once

#include <array>
#include <string>
#include <string_view>

#include "assets/sprites.h"

namespace mother {

// The contents of a .spr file (brief section 4). Grammar, one item per line, blank lines
// allowed between items:
//   sheet N          N = 0..2, followed by exactly 256 lines of exactly 256 pixel characters
//   ramp N i i ...   N = 0..7, then 1 to 64 palette indices (0..63) in decimal
// Pixel characters are the base64 alphabet: A-Z = 0-25, a-z = 26-51, 0-9 = 52-61, + = 62, / = 63.
struct SprData {
    SpriteSheets sheets;
    std::array<bool, kCartSheets> defined{};  // which sheets the file had
    Ramps ramps;
};

// Parses .spr text; errors read "<filename>:<line>: <problem>".
bool parse_spr(std::string_view text, const std::string& filename, SprData& out, std::string& error);

// Writes the defined sheets in order, then the ramps. parse_spr(write_spr(d)) gives back d.
std::string write_spr(const SprData& data);

char spr_char(uint8_t index);  // palette index -> pixel character
int spr_index(char c);         // pixel character -> palette index, or -1

}  // namespace mother
