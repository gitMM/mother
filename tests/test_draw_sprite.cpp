#include "gfx/draw.h"
#include "gfx/font.h"
#include "test.h"

using namespace mother;

namespace {

constexpr uint8_t kBg = 2;

// Sprite 0: colour 5 at (1, 1). Sprite 1 (x 8-15, y 0-7): colour 7, transparent at its top-left,
// colour 9 at its bottom-right. Sprite 33 (one row down): colour 3. Sprite 3072 (sheet 3): 11.
SpriteSheets make_sheets() {
    SpriteSheets s;
    s.set(1, 1, 5);
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            s.set(8 + x, y, 7);
            s.set(8 + x, 8 + y, 3);
            s.set(x, 768 + y, 11);
        }
    }
    s.set(8, 0, 0);
    s.set(15, 7, 9);
    return s;
}

struct Rig {
    SpriteSheets sheets = make_sheets();
    TileMap map;
    Framebuffer fb;
    Painter p;
    Rig() {
        map.layers[0] = {3, 1, {0, 1, 33}};
        map.layers[1] = {1, 1, {33}};
        p.set_assets(&sheets, &map);
        p.begin(fb);
        p.cls(kBg);
    }
};

}  // namespace

TEST(drawsprite_spr_skips_transparent_pixels) {
    Rig r;
    r.p.spr(1, 10, 20, 1, 1, false, false);
    CHECK_EQ(r.fb.pget(10, 20), kBg);
    CHECK_EQ(r.fb.pget(11, 20), 7);
    CHECK_EQ(r.fb.pget(17, 27), 9);
}

TEST(drawsprite_spr_flips) {
    Rig r;
    r.p.spr(1, 10, 20, 1, 1, true, false);
    CHECK_EQ(r.fb.pget(10, 27), 9);
    CHECK_EQ(r.fb.pget(17, 20), kBg);
    r.p.cls(kBg);
    r.p.spr(1, 10, 20, 1, 1, false, true);
    CHECK_EQ(r.fb.pget(17, 20), 9);
    CHECK_EQ(r.fb.pget(10, 27), kBg);
}

TEST(drawsprite_spr_takes_blocks_and_sheet_3) {
    Rig r;
    r.p.spr(1, 0, 0, 1, 2, false, false);
    CHECK_EQ(r.fb.pget(1, 0), 7);
    CHECK_EQ(r.fb.pget(1, 8), 3);
    r.p.spr(3072, 100, 100, 1, 1, false, false);
    CHECK_EQ(r.fb.pget(100, 100), 11);
    r.p.spr(1, 50, 50, 0, 1, false, false);
    CHECK_EQ(r.fb.pget(51, 50), kBg);
}

TEST(drawsprite_palt_and_pal_apply_to_sprites) {
    Rig r;
    r.p.palt(7, true);
    r.p.palt(0, false);
    r.p.pal(9, 12);
    r.p.spr(1, 0, 0, 1, 1, false, false);
    CHECK_EQ(r.fb.pget(0, 0), 0);    // colour 0 now drawn
    CHECK_EQ(r.fb.pget(1, 0), kBg);  // colour 7 now transparent
    CHECK_EQ(r.fb.pget(7, 7), 12);   // 9 through the palette
    r.p.palt_reset();
    r.p.pal_reset();
    r.p.spr(1, 20, 0, 1, 1, false, false);
    CHECK_EQ(r.fb.pget(20, 0), kBg);
    CHECK_EQ(r.fb.pget(21, 0), 7);
}

TEST(drawsprite_sspr_scales_nearest_neighbour) {
    Rig r;
    r.p.sspr(8, 0, 8, 8, 0, 0, 16, 16);
    CHECK_EQ(r.fb.pget(0, 0), kBg);
    CHECK_EQ(r.fb.pget(1, 0), kBg);
    CHECK_EQ(r.fb.pget(2, 0), 7);
    CHECK_EQ(r.fb.pget(14, 14), 9);
    CHECK_EQ(r.fb.pget(15, 15), 9);
    CHECK_EQ(r.fb.pget(13, 13), 7);
    CHECK_EQ(r.fb.pget(16, 16), kBg);
}

TEST(drawsprite_sprites_use_camera_and_clip) {
    Rig r;
    r.p.camera(5, 0);
    r.p.clip(0, 0, 8, 180);
    r.p.spr(1, 10, 20, 1, 1, false, false);
    CHECK_EQ(r.fb.pget(6, 20), 7);
    CHECK_EQ(r.fb.pget(8, 20), kBg);
}

TEST(drawsprite_map_draws_cells_and_skips_tile_0) {
    Rig r;
    r.p.map(0, 0, 0, 0, 3, 1, 0);
    CHECK_EQ(r.fb.pget(1, 1), kBg);  // tile 0 is never drawn
    CHECK_EQ(r.fb.pget(9, 0), 7);
    CHECK_EQ(r.fb.pget(16, 0), 3);
    r.p.map(0, 0, 100, 100, 1, 1, 1);
    CHECK_EQ(r.fb.pget(100, 100), 3);
    r.p.map(-5, 0, 0, 50, 10, 1, 0);  // cells left of the layer are empty
    CHECK_EQ(r.fb.pget(41, 50), kBg);
    CHECK_EQ(r.fb.pget(49, 50), 7);
    r.p.map(0, 0, 0, 0, 32767, 32767, 0);  // returns at once: only visible cells are visited
}

TEST(drawsprite_font_covers_printable_ascii) {
    for (int c = 33; c <= 126; ++c) CHECK(glyph_bits(static_cast<unsigned char>(c)) != 0);
    CHECK_EQ(glyph_bits(' '), 0);
    CHECK_EQ(glyph_bits(127), 0);
    CHECK_EQ(glyph_bits(200), 0);
    CHECK_EQ(glyph_bits('q'), glyph_bits('Q'));
}

TEST(drawsprite_print_lays_glyphs_in_4x6_cells) {
    Rig r;
    r.p.print("A", 10, 20, 9);  // ### / #.# / ### / #.# / #.#
    CHECK_EQ(r.fb.pget(10, 20), 9);
    CHECK_EQ(r.fb.pget(12, 20), 9);
    CHECK_EQ(r.fb.pget(11, 21), kBg);
    CHECK_EQ(r.fb.pget(11, 22), 9);
    CHECK_EQ(r.fb.pget(13, 20), kBg);
    r.p.print("A\n\x01" "A", 50, 50, 9);
    CHECK_EQ(r.fb.pget(50, 56), kBg);  // the unknown character draws nothing...
    CHECK_EQ(r.fb.pget(54, 56), 9);    // ...but takes a cell
    r.p.camera(10, 0);
    r.p.pal(9, 4);
    r.p.print("I", 110, 100, 9);
    CHECK_EQ(r.fb.pget(100, 100), 4);
}
