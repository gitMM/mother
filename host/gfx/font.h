#pragma once

#include <cstdint>

namespace mother {

constexpr int kGlyphW = 4;  // character cell; glyphs are 3x5 in its top-left corner
constexpr int kGlyphH = 6;

// The built-in font's 3x5 glyph for a character as 15 bits, bit (row * 3 + column) set where a
// pixel is lit. Lowercase letters use the uppercase glyphs; characters outside ASCII 32-126
// have no glyph (0).
uint16_t glyph_bits(unsigned char c);

}  // namespace mother
