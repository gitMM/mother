#include <string>

#include "assets/map_format.h"
#include "temp_cart.h"
#include "test.h"

using namespace mother;

namespace {

bool parse(const std::string& text, TileMap& out, std::string& error) {
    return parse_map(text, "x.map", out, error);
}

bool fails_with(const std::string& text, const char* part) {
    TileMap map;
    std::string error;
    if (parse(text, map, error)) {
        std::printf("  parsed, expected an error containing \"%s\"\n", part);
        return false;
    }
    if (error.find(part) != std::string::npos) return true;
    std::printf("  \"%s\" not found in: %s\n", part, error.c_str());
    return false;
}

}  // namespace

TEST(mapformat_parses_layers_and_flags) {
    std::string flags = flags_block(0, "0A");
    flags.replace(flags.rfind("00"), 2, "ff");  // tile 3071
    const std::string text = "layer 0 3 2\n0001 0002 3071\n0000 0000 0005\n\nlayer 1 1 1\n0007\n" + flags;
    TileMap map;
    std::string error;
    CHECK(parse(text, map, error));
    CHECK_EQ(map.layers[0].w, 3);
    CHECK_EQ(map.layers[0].h, 2);
    CHECK_EQ(map.get(0, 0, 0), 1);
    CHECK_EQ(map.get(0, 2, 0), 3071);
    CHECK_EQ(map.get(0, 2, 1), 5);
    CHECK_EQ(map.get(1, 0, 0), 7);
    CHECK_EQ(map.get(0, 3, 0), 0);
    CHECK_EQ(map.get(0, -1, 0), 0);
    CHECK_EQ(map.get(2, 0, 0), 0);
    CHECK_EQ(map.flags[0], 10);
    CHECK_EQ(map.flags[1], 0);
    CHECK_EQ(map.flags[3071], 255);
}

TEST(mapformat_layers_are_optional) {
    TileMap map;
    std::string error;
    CHECK(parse(flags_block(), map, error));
    CHECK_EQ(map.layers[0].w, 0);
    CHECK_EQ(map.get(0, 0, 0), 0);
}

TEST(mapformat_errors_name_the_line) {
    const std::string flags = flags_block();
    CHECK(fails_with("layer 0 2 1\n0001 001\n" + flags, "x.map:2: tile '001' is not 4 digits"));
    CHECK(fails_with("layer 0 1 1\n3072\n" + flags, "x.map:2: tile '3072'"));
    CHECK(fails_with("layer 0 2 1\n0001\n" + flags, "x.map:2: expected 2 tiles, found 1"));
    CHECK(fails_with("layer 0 1 3\n0001\n", "has 1 rows; it needs 3"));
    CHECK(fails_with("layer 2 1 1\n0001\n" + flags, "x.map:1: expected `layer N W H`"));
    CHECK(fails_with("layer 0 0 5\n" + flags, "x.map:1:"));
    CHECK(fails_with("layer 0 257 1\n" + flags, "x.map:1:"));
    CHECK(fails_with("layer 0 1 1\n0001\nlayer 0 1 1\n0001\n" + flags, "x.map:3: layer 0 is defined twice"));
    CHECK(fails_with("layer 0 1 1\n0001\n", "x.map: missing the flags section"));
    CHECK(fails_with(flags_block(5, "0g"), "x.map:7: expected two hex digits for tile 5"));
    CHECK(fails_with("flags\n00\n", "flags has 1 lines; it needs 3072"));
    CHECK(fails_with(flags + flags, "flags are defined twice"));
    CHECK(fails_with("tiles\n", "x.map:1: expected `layer N W H` or `flags`"));
}

TEST(mapformat_set_ignores_cells_outside_the_layer) {
    TileMap map;
    std::string error;
    CHECK(parse("layer 0 2 2\n0000 0000\n0000 0000\n" + flags_block(), map, error));
    map.set(0, 1, 1, 9);
    CHECK_EQ(map.get(0, 1, 1), 9);
    map.set(0, 5, 5, 9);
    map.set(0, -1, 0, 9);
    map.set(1, 0, 0, 9);  // layer 1 is empty
    CHECK_EQ(map.get(1, 0, 0), 0);
    CHECK_EQ(map.get(0, 0, 0), 0);
}
