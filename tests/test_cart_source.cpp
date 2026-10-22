#include <filesystem>
#include <string>

#include "temp_cart.h"
#include "test.h"
#include "vm/cart_source.h"

using namespace mother;

namespace {

bool load(const std::filesystem::path& dir, CartSource& out, std::string& error) {
    return load_cart_source(dir, out, error);
}

}  // namespace

TEST(cartsource_reads_every_module) {
    CartSource src;
    std::string error;
    CHECK(load(std::filesystem::path(MOTHER_TEST_CARTS) / "hello", src, error));
    CHECK_EQ(src.modules.size(), size_t(2));
    CHECK(src.modules.count("main") == 1 && src.modules.count("box") == 1);
    CHECK(src.modules["box"].find("return box") != std::string::npos);
}

TEST(cartsource_needs_cart_toml_and_main) {
    CartSource src;
    std::string error;
    TempCart no_toml("no_toml", {{"main.luau", "-- hi"}});
    CHECK(!load(no_toml.dir(), src, error));
    CHECK(error.find("cart.toml") != std::string::npos);
    TempCart no_main("no_main", {{"cart.toml", kTestToml}, {"other.luau", "-- hi"}});
    CHECK(!load(no_main.dir(), src, error));
    CHECK(error.find("main.luau") != std::string::npos);
    CHECK(!load(no_main.dir() / "nope", src, error));
}

TEST(cartsource_limits_luau_to_64kb) {
    CartSource src;
    std::string error;
    const std::string half(32 * 1024, '-');
    TempCart exact("exact", {{"cart.toml", kTestToml}, {"main.luau", half}, {"more.luau", half}});
    CHECK(load(exact.dir(), src, error));
    TempCart over("over", {{"cart.toml", kTestToml}, {"main.luau", half}, {"more.luau", half + "-"}});
    CHECK(!load(over.dir(), src, error));
    CHECK(error.find("65536") != std::string::npos);
}

TEST(cartsource_rejects_bad_module_file_names) {
    CartSource src;
    std::string error;
    TempCart bad("bad_name", {{"cart.toml", kTestToml}, {"main.luau", ""}, {"my-mod.luau", ""}});
    CHECK(!load(bad.dir(), src, error));
    CHECK(error.find("my-mod.luau") != std::string::npos);
}

TEST(cartsource_ignores_other_files) {
    CartSource src;
    std::string error;
    TempCart cart("other_files", {{"cart.toml", kTestToml}, {"main.luau", ""}, {"notes.txt", "x"},
                                  {"audio.trk", "x"}, {"main.lua", "x"}});
    CHECK(load(cart.dir(), src, error));
    CHECK_EQ(src.modules.size(), size_t(1));
}

TEST(cartsource_loads_sprites_and_map) {
    CartSource src;
    std::string error;
    TempCart cart("assets", {{"cart.toml", kTestToml},
                             {"main.luau", ""},
                             {"sprites.spr", "ramp 1 4 5\n"},
                             {"map.map", "layer 1 2 1\n0003 0004\n" + flags_block(3, "01")}});
    CHECK(load(cart.dir(), src, error));
    CHECK(src.art.ramps[1] == std::vector<uint8_t>({4, 5}));
    CHECK_EQ(src.map.get(1, 1, 0), 4);
    CHECK_EQ(src.map.flags[3], 1);
}

TEST(cartsource_asset_errors_name_file_and_line) {
    CartSource src;
    std::string error;
    TempCart bad_spr("bad_spr", {{"cart.toml", kTestToml}, {"main.luau", ""}, {"sprites.spr", "sheet 0\nAAAA\n"}});
    CHECK(!load(bad_spr.dir(), src, error));
    CHECK(error.find("sprites.spr:2:") != std::string::npos);
    TempCart bad_map("bad_map", {{"cart.toml", kTestToml}, {"main.luau", ""}, {"map.map", "layer 0 1 1\n0001\n"}});
    CHECK(!load(bad_map.dir(), src, error));
    CHECK(error.find("map.map: missing the flags section") != std::string::npos);
}

TEST(cartsource_module_names) {
    CHECK(valid_module_name("main"));
    CHECK(valid_module_name("Enemy_2"));
    CHECK(!valid_module_name(""));
    CHECK(!valid_module_name("../x"));
    CHECK(!valid_module_name("a/b"));
    CHECK(!valid_module_name("a\\b"));
    CHECK(!valid_module_name("C:x"));
    CHECK(!valid_module_name("a.b"));
    CHECK(!valid_module_name("a b"));
}
