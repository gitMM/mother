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
                                  {"sprites.spr", "x"}, {"main.lua", "x"}});
    CHECK(load(cart.dir(), src, error));
    CHECK_EQ(src.modules.size(), size_t(1));
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
