#include <string>

#include "test.h"
#include "world_cart.h"

using namespace mother;

namespace {

const char* const kDeathLog = R"lua(
local deaths = {}
local function logging()
    world:on("death", function(id) table.insert(deaths, `{id}@{tick()}`) end)
end
)lua";

std::string empty_map(int w, int h) {
    return solid_map(w, h, [](int, int) { return 0; });
}

std::string avatar_at(int x, int y, const char* params) {
    return "[avatar]\narchetype = \"dummy\"\ndamage = \"hp\"\nhp = 1\nx = " + std::to_string(x) +
           "\ny = " + std::to_string(y) + "\nparams = " + params + "\n";
}

}  // namespace

TEST(bounds_solid_kill_and_despawn_edges) {
    const std::string toml =
        Toml{.bounds = "{ left = \"solid\", right = \"kill\", top = \"despawn\", bottom = \"solid\" }"}.text();
    CHECK(wins(run_world("edges", toml, std::string(kDeathLog) + R"lua(
run(function()
    logging()
    world:spawn("dummy", { vx = -3 }, 20, 90, "host")
    world:spawn("dummy", { vx = 3 }, 300, 90, "host")
    world:spawn("dummy", { vy = -3 }, 160, 20, "host")
    world:spawn("dummy", { vy = 3 }, 160, 160, "host")
end, {
    [40] = function()
        eq(world:get(1).x, 4, "held at the left edge")
        eq(world:get(1).vx, 0, "stopped there")
        eq(world:get(2), nil, "killed past the right edge")
        eq(world:get(3), nil, "despawned past the top")
        eq(table.concat(deaths, " "), "2@8", "kill edges kill once wholly past; despawn is silent")
        eq(world:get(4).y, 176, "held at the bottom edge")
    end,
})
)lua")));
}

TEST(bounds_apply_once_an_entity_has_entered) {
    CHECK(wins(run_world("enter", Toml{.bounds = "despawn"}.text(), R"lua(
run(function()
    world:spawn("dummy", { vy = 2 }, 160, -20, "host")
    world:spawn("dummy", { vy = -2 }, 100, -20, "host")
    world:spawn("dummy", { vy = 2 }, 40, 170, "host")
end, {
    [30] = function()
        eq(world:get(1).y, 38, "came in from outside")
        eq(world:get(2).y, -78, "never entered, so never removed")
        eq(world:get(3), nil, "left after entering")
    end,
})
)lua")));
}

TEST(bounds_are_the_map_layer) {
    CHECK(wins(run_world("map_rect", kTestToml, R"lua(
run(function()
    world:spawn("dummy", { vx = 4 }, 380, 20, "host")
    world:spawn("dummy", { vy = 4 }, 20, 220, "host")
end, {
    [20] = function()
        eq(world:get(1).x, 396, "right edge of a 50-tile layer")
        eq(world:get(2).y, 236, "bottom edge of a 30-tile layer")
    end,
})
)lua", 600, {{"map.map", empty_map(50, 30)}})));
}

TEST(bounds_follow_camera_tracks_the_avatar) {
    const std::string toml = Toml{.scroll = "follow", .avatar = "lr", .extra = avatar_at(100, 88, "{ vx = 4 }")}.text();
    CHECK(wins(run_world("follow", toml, R"lua(
local function camera_is(x, y, what)
    local cx, cy = world:camera()
    eq(cx, x, what .. " x")
    eq(cy, y, what .. " y")
end
run(function()
    camera_is(0, 0, "at the start")
end, {
    [31] = function()
        camera_is(60, 0, "centred on the avatar; the layer is less than a screen high")
    end,
    [200] = function()
        camera_is(480, 0, "kept inside the layer")
        eq(world:get(1).x, 796, "the avatar at the edge")
    end,
})
)lua", 600, {{"map.map", empty_map(100, 22)}})));
}

TEST(bounds_auto_scroll_moves_the_edges) {
    const std::string toml =
        Toml{.scroll = "auto", .bounds = "{ left = \"solid\", right = \"solid\", top = \"despawn\", bottom = \"kill\" }"}
            .text();
    CHECK(wins(run_world("auto", toml, std::string(kDeathLog) + R"lua(
run(function()
    logging()
    world:spawn("dummy", nil, 160, 170, "host")
end, {
    [11] = function()
        local x, y = world:camera()
        eq(x * 1000 + y, -5, "the camera after 10 steps")
    end,
    [40] = function()
        eq(table.concat(deaths, " "), "1@28", "the bottom edge passed it")
    end,
})
)lua")));
}

TEST(bounds_scrolling_removes_far_entities) {
    const std::string toml = Toml{.scroll = "follow", .avatar = "lr", .objective = "type = \"kill_all\"",
                                  .extra = avatar_at(100, 88, "{}")}
                                 .text();
    CHECK(wins(run_world("far", toml, R"lua(
run(function()
    world:spawn("dummy", nil, 1000, 88, "wild")
    world:spawn("dummy", nil, 1000, 100, "host")
    world:spawn("dummy", { damage = "stun" }, 1000, 120, "host")
    world:spawn("dummy", nil, 600, 88, "wild")
end, {
    [2] = function()
        eq(world:get(2), nil, "more than a screen outside the view")
        check(world:get(3) ~= nil, "objective entities stay")
        eq(world:get(4), nil, "stun bodies aren't kill_all's")
        check(world:get(5) ~= nil, "within a screen of the view")
        check(world:get(1) ~= nil, "the avatar stays")
    end,
})
)lua", 600, {{"map.map", empty_map(200, 22)}})));
}
