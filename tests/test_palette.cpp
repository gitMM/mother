#include "gfx/palette.h"
#include "test.h"

using namespace mother;

TEST(palette_has_64_opaque_colours) {
    CHECK_EQ(kPalette.size(), size_t(64));
    for (uint32_t c : kPalette) CHECK_EQ(c >> 24, 0xFFu);
}

TEST(palette_matches_aap64_placeholder) {
    CHECK_EQ(kPalette[0], 0xFF060608u);
    CHECK_EQ(kPalette[9], 0xFFFFFC40u);
    CHECK_EQ(kPalette[22], 0xFFFFFFFFu);
    CHECK_EQ(kPalette[40], 0xFF6D758Du);
    CHECK_EQ(kPalette[63], 0xFF423934u);
}

TEST(palette_colours_are_distinct) {
    for (size_t i = 0; i < kPalette.size(); ++i) {
        for (size_t j = i + 1; j < kPalette.size(); ++j) CHECK(kPalette[i] != kPalette[j]);
    }
}

TEST(palette_reserved_indices) {
    CHECK_EQ(kTransparent, 0);
    CHECK_EQ(kMotherGold, 63);
}
