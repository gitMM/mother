#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "convert.h"
#include "gfx/palette.h"
#include "quantize.h"
#include "test.h"
#include "vm/cart_vm.h"

using namespace mother;

namespace {

const std::filesystem::path kRoundtrip = std::filesystem::path(MOTHER_TEST_CARTS) / "roundtrip";

uint8_t channel(uint32_t argb, int shift) {
    return uint8_t(argb >> shift);
}

RgbaImage image(int w, int h, std::initializer_list<uint8_t> px) {
    return {w, h, std::vector<uint8_t>(px)};
}

std::string read_text(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

}  // namespace

TEST(png2spr_exact_palette_colours_keep_their_index) {
    for (int i = 1; i < kPaletteSize; ++i) {
        const uint32_t c = kPalette[size_t(i)];
        CHECK_EQ(nearest_index(channel(c, 16), channel(c, 8), channel(c, 0)), i);
    }
}

TEST(png2spr_never_picks_the_transparent_index) {
    const uint32_t c0 = kPalette[0];
    CHECK(nearest_index(channel(c0, 16), channel(c0, 8), channel(c0, 0)) != 0);
    CHECK(nearest_index(0, 0, 0) != 0);
}

TEST(png2spr_picks_the_nearest_colour) {
    CHECK_EQ(nearest_index(250, 250, 250), 22);  // ffffff
    CHECK_EQ(nearest_index(250, 250, 70), 9);    // fffc40
}

TEST(png2spr_alpha_below_128_is_transparent) {
    SprData data;
    std::string error;
    CHECK(image_to_spr(image(2, 1, {255, 255, 255, 127, 255, 255, 255, 128}), data, error));
    CHECK_EQ(data.sheets.get(0, 0), 0);
    CHECK_EQ(data.sheets.get(1, 0), 22);
}

TEST(png2spr_image_size_sets_the_sheets) {
    SprData data;
    std::string error;
    CHECK(!image_to_spr({257, 1, std::vector<uint8_t>(257 * 4, 255)}, data, error));
    CHECK(error.find("257x1") != std::string::npos);
    CHECK(!image_to_spr({1, 769, std::vector<uint8_t>(769 * 4, 255)}, data, error));
    CHECK(image_to_spr({10, 300, std::vector<uint8_t>(10 * 300 * 4, 255)}, data, error));
    CHECK(data.defined[0] && data.defined[1] && !data.defined[2]);
    CHECK_EQ(data.sheets.get(9, 299), 22);
    CHECK_EQ(data.sheets.get(10, 299), 0);
}

TEST(png2spr_output_matches_the_committed_sprites) {
    RgbaImage png;
    SprData data;
    std::string error;
    CHECK(load_png(kRoundtrip / "art.png", png, error));
    CHECK(image_to_spr(png, data, error));
    CHECK(write_spr(data) == read_text(kRoundtrip / "sprites.spr"));
}

// M2 acceptance: the PNG, converted by png2spr and drawn by a cart, matches pixel for pixel.
TEST(png2spr_roundtrip_cart_draws_the_png_pixel_exact) {
    RgbaImage png;
    std::string error;
    CHECK(load_png(kRoundtrip / "art.png", png, error));
    CHECK_EQ(png.w, 128);
    CHECK_EQ(png.h, 96);

    CartSource source;
    CHECK(load_cart_source(kRoundtrip, source, error));
    CartVM vm;
    CHECK(vm.open(std::move(source)));
    Framebuffer fb;
    vm.draw(fb);
    CHECK(vm.status().outcome == Outcome::Running);

    int wrong = 0, opaque = 0;
    for (int y = 0; y < png.h; ++y) {
        for (int x = 0; x < png.w; ++x) {
            const uint8_t* p = &png.px[(size_t(y) * size_t(png.w) + size_t(x)) * 4];
            const uint8_t index = fb.pget(x, y);
            if (p[3] < 128) {
                wrong += index == 0 ? 0 : 1;
            } else {
                ++opaque;
                const uint32_t c = kPalette[index];
                const bool same = channel(c, 16) == p[0] && channel(c, 8) == p[1] && channel(c, 0) == p[2];
                wrong += same && index != 0 ? 0 : 1;
            }
        }
    }
    CHECK_EQ(wrong, 0);
    CHECK(opaque > 6000);
}
