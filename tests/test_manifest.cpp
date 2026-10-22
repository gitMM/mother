#include <string>

#include "assets/manifest.h"
#include "temp_cart.h"
#include "test.h"

using namespace mother;

namespace {

// The brief's KNIGHTFALL example (section 6), plus the avatar's start position.
const char* const kExample = R"([cart]
id = "knightfall"
title = "KNIGHTFALL"
family = "action"            # action | platform | driving | shooter | puzzle

[env]
gravity = "none"
surface = "ground"
scroll  = "none"
avatar  = "8dir"
spawn   = "world"
deploy  = true
bounds  = { left="solid", right="solid", top="solid", bottom="solid" }

[objective]
type = "kill_all"

[avatar]
archetype = "knight"
damage = "hp"
hp = 30
x = 160
y = 88

[[spawn_points]]
x = 8
y = 88

[[exports]]
id = "sword"
archetype = "sword"
params = { tier = 3, arc = 90, reach = 12 }
)";

std::string error_of(const std::string& text) {
    TomlValue doc;
    std::string error;
    if (parse_manifest(text, "cart.toml", doc, error)) return "(valid)";
    return error;
}

bool rejects(const std::string& text, const char* part) {
    const std::string error = error_of(text);
    if (error.find(part) != std::string::npos) return true;
    std::printf("  \"%s\" not found in: %s\n", part, error.c_str());
    return false;
}

}  // namespace

TEST(manifest_accepts_the_brief_example) {
    TomlValue doc;
    std::string error;
    CHECK(parse_manifest(kExample, "cart.toml", doc, error));
    CHECK_EQ(error, std::string());
    CHECK_EQ(doc.find("exports")->items[0].find("params")->find("arc")->integer, int64_t(90));
    CHECK_EQ(error_of(kTestToml), std::string("(valid)"));
}

TEST(manifest_checks_each_section) {
    CHECK(rejects("[env]\n", "cart.toml:1: missing [cart]"));
    CHECK(rejects("cart = 1\n", "cart must be a [cart] table"));
    CHECK(rejects(Toml{}.text() + "[extra]\n", "cart.toml:18: unknown table [extra]"));
    CHECK(rejects(Toml{.gravity = "up"}.text(), "cart.toml:7: [env] gravity must be none|down, not \"up\""));
    CHECK(rejects(Toml{.avatar = "side"}.text(), "[env] avatar must be none|up|lr|8dir|heading"));
    CHECK(rejects(Toml{.deploy = "\"yes\""}.text(), "[env] deploy must be a boolean, not a string"));
    CHECK(rejects(Toml{.bounds = "wall"}.text(), "[env] bounds left must be solid|kill|despawn"));
    CHECK(rejects(Toml{.bounds = "{ left = \"kill\" }"}.text(), "[env] bounds: missing right"));
    CHECK(rejects(Toml{.objective = "type = \"win\""}.text(), "[objective] type must be kill_all|escape"));
    CHECK(rejects(Toml{.objective = "type = \"survive\""}.text(), "[objective]: missing ticks"));
    CHECK(rejects(Toml{.objective = "type = \"race\"\nlaps = 0"}.text(), "[objective] laps must be at least 1"));
    CHECK(rejects(Toml{.objective = "type = \"race\"\nlaps = 3"}.text(), "[objective]: missing line"));
    CHECK(rejects(Toml{.objective = "type = \"race\"\nlaps = 3\nline = [[1, 2]]"}.text(),
                  "[objective] line needs at least 2 points"));
    CHECK(rejects(Toml{.objective = "type = \"race\"\nlaps = 3\nline = [[1, 2], [3]]"}.text(),
                  "[objective] line must be a list of [x, y] points"));
    CHECK(error_of(Toml{.objective = "type = \"race\"\nlaps = 3\nline = [[1, 2], [3.5, 4]]"}.text()) == "(valid)");
    CHECK(rejects(Toml{.objective = "type = \"kill_all\"\nticks = 5"}.text(), "[objective]: unknown key ticks"));
    CHECK(rejects(Toml{.objective = "type = \"escape\"\nexit = { x = 1, y = 2, w = 0, h = 3 }"}.text(),
                  "w and h must be positive"));
    CHECK(rejects(Toml{.objective = "type = \"collect\"\narchetype = \"gem-s\""}.text(),
                  "[objective] archetype must be letters, digits and _ only"));
    CHECK(error_of(Toml{.objective = "type = \"escape\"\nexit = { x = 1, y = 2.5, w = 8, h = 8 }"}.text()) ==
          "(valid)");
}

TEST(manifest_ties_avatar_and_spawn_points_to_env) {
    const std::string avatar = "[avatar]\narchetype = \"dummy\"\ndamage = \"hp\"\nhp = 3\nx = 10\ny = 20\n";
    CHECK(rejects(Toml{.extra = avatar}.text(), "[avatar] needs [env] avatar other than none"));
    CHECK(rejects(Toml{.avatar = "lr"}.text(), "[env] avatar needs an [avatar] table"));
    CHECK_EQ(error_of(Toml{.avatar = "lr", .extra = avatar}.text()), std::string("(valid)"));
    CHECK(rejects(Toml{.avatar = "lr", .extra = avatar + "speed = 2\n"}.text(), "[avatar]: unknown key speed"));
    CHECK(rejects(Toml{.avatar = "up", .extra = "[avatar]\narchetype = \"dummy\"\ndamage = \"hp\"\nhp = 0\nx = 1\ny = 1\n"}.text(),
                  "[avatar] hp must be at least 1"));

    const std::string point = "[[spawn_points]]\nx = 8\ny = 9\n";
    CHECK(rejects(Toml{.extra = point}.text(), "[[spawn_points]] need [env] spawn = world or camera"));
    CHECK(rejects(Toml{.spawn = "world"}.text(), "[env] spawn needs at least one [[spawn_points]]"));
    CHECK_EQ(error_of(Toml{.spawn = "camera", .extra = point + point}.text()), std::string("(valid)"));
    CHECK(rejects(Toml{.spawn = "world", .extra = "[[spawn_points]]\nx = 8\n"}.text(), "[[spawn_points]]: missing y"));
    CHECK(rejects(Toml{.spawn = "world", .extra = "[spawn_points]\nx = 1\n"}.text(), "list of [[spawn_points]] tables"));
}

TEST(manifest_checks_exports) {
    const std::string sword = "[[exports]]\nid = \"sword\"\narchetype = \"sword\"\n";
    CHECK_EQ(error_of(Toml{.extra = sword}.text()), std::string("(valid)"));
    CHECK(rejects(Toml{.extra = sword + sword}.text(), "[[exports]] id sword is used twice"));
    CHECK(rejects(Toml{.extra = "[[exports]]\nid = \"a\"\n"}.text(), "[[exports]]: missing archetype"));
    CHECK(rejects(Toml{.extra = sword + "params = 3\n"}.text(), "[[exports]] params must be a table"));
}

TEST(manifest_reports_toml_errors) {
    CHECK(rejects("[cart\n", "cart.toml:1: expected ']'"));
}
